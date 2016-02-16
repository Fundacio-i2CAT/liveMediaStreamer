/*
 *  WorkersPool.cpp - This Worker Pool contains all processing threads
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors:  David Cassany <david.cassany@i2cat.net>
 *
 *
 */

#include <chrono>

#include "WorkersPool.hh"
#include "Utils.hh"

#define HW_CONC_FACTOR 2

WorkersPool::WorkersPool(size_t threads) : run(true)
{
    if (threads == 0 || 
        threads > std::thread::hardware_concurrency()*HW_CONC_FACTOR){
        threads = std::thread::hardware_concurrency()*HW_CONC_FACTOR;
    }
    
    utils::infoMsg("starting "  + std::to_string(threads) + " threads");
    
    for (unsigned int i = 0; i < threads; i++){
        workers.push_back(
            std::thread([this](unsigned int j){
                Runnable* job = NULL;
                std::set<Runnable*>::iterator iter;
                std::vector<int> enabledJobs;
                bool added = false;
                
                while(true) {
                    std::unique_lock<std::mutex> guard(mtx);
                    iter = jobQueue.begin();
                    while (run) {
                        if (iter == jobQueue.end()){
                            qCheck.wait_for(guard, std::chrono::milliseconds(IDLE));
                        } else if (!(*iter)->isRunning() && !(*iter)->ready()) {
                            qCheck.wait_until(guard, (*iter)->getTime());
                        } else if (!(*iter)->isRunning() && (*iter)->ready()){
                            job = *iter;
                            iter = jobQueue.erase(iter);
                            break;
                        } else {
                            iter++;
                            continue;
                        }
                        iter = jobQueue.begin();
                    }

                    if(!run){
                        break;
                    }
                    
                    added = false;
                    
                    job->setRunning();
                    guard.unlock();
                    
                    qCheck.notify_one();
                    
                    enabledJobs = job->runProcessFrame();
                    
                    guard.lock();
                    job->unsetRunning();
                    
                    if (job->pendingJobs()){
                        enabledJobs.push_back(job->getId());
                    }
                    
                    for(auto id : enabledJobs){
                        if (runnables.count(id) > 0){
                            jobQueue.insert(runnables[id]);
                        }
                        added = true;
                    }
                    
                    guard.unlock();
                    if (added){
                        qCheck.notify_one();
                    }
                }
            }, i)
        );
    }
}


WorkersPool::~WorkersPool()
{
    stop();
}

void WorkersPool::stop()
{
    run = false;
    qCheck.notify_all();
    for (std::thread &worker : workers){
        if (worker.joinable()){
            worker.join();
        }
    }
    jobQueue.clear();
}

bool WorkersPool::addTask(Runnable* const task)
{
    int id = task->getId();
    if (id < 0){
        return false;
    }
    std::unique_lock<std::mutex> guard(mtx);
    if (runnables.count(id) == 0){
        runnables[id] = task;
        jobQueue.insert(task);
        guard.unlock();
        qCheck.notify_one();
        return true;
    }
    return false;
}

bool WorkersPool::removeTask(const int id)
{
    std::unique_lock<std::mutex> guard(mtx);
    if (runnables.count(id) > 0){
        runnables.erase(id);
        guard.unlock();
        qCheck.notify_one();
        return true;
    }
    
    return false;
}
