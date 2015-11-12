#include <thread>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include "../src/Utils.hh"

#define MAX_DASHERS 32
#define BASE_CONTROL_PORT 9990

extern char **environ;

struct DashCtl {
    pid_t pid;
    int cport;
    posix_spawn_file_actions_t action;
};

void usage() {
    utils::infoMsg("Usage:\n"
        "-r <input RTSP URI>\n"
        "-f <dash folder>\n"
        "-n <number of dashers>\n"
        "-statsfile <output statistics filename>\n"
        "-timeout <seconds to wait before closing. 0 means forever>\n"
        "-configfile <configuration file. See testdash for contents>\n"
        "\n"
        "profiledash runs <number of dashers> simultaneously for <timeout> seconds and then outputs some\n"
        "stats. Care is taken to remove stabilization time from calculations.\n"
        "The same input is provided to all dashers. Subfolders are created inside <dash folder> to\n"
        "avoid collisions.\n");
}

int main (int argc, char *argv[]) {
    int timeout = 0, numDashers = 0;
    std::string rtspUri, dFolder, statsFilename, configFilename;

    DashCtl dasher[MAX_DASHERS];

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-r")==0) {
            rtspUri = argv[i+1];
            utils::infoMsg("configuring input RTSP URI: " + rtspUri);
        } else if (strcmp(argv[i],"-f")==0) {
            dFolder = argv[i+1];
            utils::infoMsg("configuring dash folder: " + dFolder);
        } else if (strcmp(argv[i],"-statsfile")==0) {
            statsFilename = argv[i+1];
            utils::infoMsg("output stats filename: " + statsFilename);
        } else if (strcmp(argv[i],"-timeout")==0) {
            timeout = std::stoi(argv[i+1]);
            utils::infoMsg("timeout: " + std::to_string(timeout) + "s.");
        } else if (strcmp(argv[i],"-n")==0) {
            numDashers = std::stoi(argv[i+1]);
            utils::infoMsg("num dashers: " + std::to_string(numDashers));
        } else if (strcmp(argv[i],"-configfile")==0) {
            configFilename = argv[i+1];
            utils::infoMsg("configuration file name: " + configFilename);
        }
    }

    if (rtspUri.empty() || dFolder.empty() || numDashers==0 || timeout==0 || configFilename.empty()) {
        usage();
        return 1;
    }

    for (int i=0; i<numDashers; i++) {
        std::string folder = dFolder + "/" + std::to_string(i);
        mkdir(folder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        dasher[i].cport = BASE_CONTROL_PORT + i;

        std::string cportstr = std::to_string(dasher[i].cport);
        std::string timeoutstr = std::to_string(timeout);
        std::string statsfilestr = statsFilename + "." + std::to_string(i);
        const char *argv[] = {"./testdash",
            "-r", rtspUri.c_str(),
            "-f", folder.c_str(),
            "-c", cportstr.c_str(),
            "-timeout", timeoutstr.c_str(),
            "-statsfile", statsfilestr.c_str(),
            "-configfile", configFilename.c_str(),
            NULL};

        int status;
        posix_spawn_file_actions_init(&dasher[i].action);
        posix_spawn_file_actions_addopen (&dasher[i].action, STDOUT_FILENO,
                (folder + std::string("/profiledash.log")).c_str(),
                O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        posix_spawn_file_actions_addopen (&dasher[i].action, STDERR_FILENO,
                (folder + std::string("/profiledash.err")).c_str(),
                O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        std::string cmd;
        const char **c = argv;
        while (*c) {
            cmd += *c;
            cmd += " ";
            c++;
        }

        utils::infoMsg ("Running " + cmd);
        status = posix_spawn(&dasher[i].pid, "./testdash", &dasher[i].action, NULL, (char **)argv, environ);
        if (status != 0) {
            utils::errorMsg(std::string("posix_spawn:") + strerror(status));
            return -1;
        }
        // Wait for dasher to stabilize (receive sdp file and start encoders)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Now wait for stats to be available, collect them and generate totals
    int avgDelay = 0, blockLosses = 0;
    float bitrate = 0, packetLosses = 0, cpu = 0;
    for (int i=0; i<numDashers; i++) {
        std::string statsfilestr = statsFilename + "." + std::to_string(i);
        FILE *f = NULL;
        do {
            f = fopen (statsfilestr.c_str(), "rt");
            if (!f) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } while (!f);
        int i1, i2;
        float f1, f2, f3;
        if (fscanf (f, "%d, %d, %f, %f, %f", &i1, &i2, &f1, &f2, &f3) != 5) {
            utils::errorMsg("Incorrect statsfile for child " + std::to_string(i));
        }
        fclose (f);
        remove (statsfilestr.c_str());
        avgDelay += i1;
        blockLosses += i2;
        bitrate += f1;
        packetLosses += f2;
        cpu += f3;
        utils::infoMsg(std::string("Collected stats from child ") + std::to_string(i));
    }
    avgDelay /= numDashers;

    // Write result
    FILE *f = fopen (statsFilename.c_str(), "a+t");
    if (!f) {
        utils::errorMsg(std::string("Could not open result statsfile: ") + statsFilename);
        return 1;
    }
    fprintf (f, "%s\t%d\t%d\t%d\t%f\t%f\t%f\n", configFilename.c_str(), numDashers,
            avgDelay, blockLosses, bitrate, packetLosses, cpu);
    fclose(f);

    // At this point all children have produced their stats, collect their processes
    // or kill them (because sometimes testdash fails to shutdown...)
    for (int i=0; i<numDashers; i++) {
        int status = 0;
        if (waitpid(dasher[i].pid, &status, WNOHANG) != 0) {
            std::string msg = std::string("Child ") + std::to_string(i) +
                std::string(" exited with status ") + std::to_string(WEXITSTATUS(status));
            utils::infoMsg(msg);
        } else {
            // This will kill children which did not manage to exit in time
            std::string msg = std::string("Child ") + std::to_string(i) +
                std::string(" did not exit. Killing process ") + std::to_string(dasher[i].pid);
            utils::infoMsg(msg);
            if (kill(dasher[i].pid, SIGKILL) != 0) {
                perror ("kill");
            }
        }
        posix_spawn_file_actions_destroy(&dasher[i].action);
    }
}
