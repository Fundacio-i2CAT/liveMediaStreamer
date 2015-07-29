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

WorkersPool::WorkersPool(size_t threads) : run(true)
{
    if (threads == 0 || 
        threads > std::thread::hardware_concurrency()){
        threads = std::thread::hardware_concurrency();
    }
    
    utils::infoMsg("starting "  + std::to_string(threads) + " threads");
    
    for (unsigned int i = 0; i < threads; i++){
        workers.push_back(
            std::thread([this](unsigned int j){
                Runnable* job = NULL;
                std::list<Runnable*>::iterator iter;
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
                    
                    for(auto job : enabledJobs){
                        added |= addJob(job);
                    }
                    
                    if (job->isPeriodic()){
                        jobQueue.push_back(job);
                        addGroupJob(job->getGroupIds());
                        jobQueue.sort(RunnableLess());
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
    run = false;
    qCheck.notify_all();
    for (std::thread &worker : workers){
        if (worker.joinable()){
            worker.join();
        }
    }
}

bool WorkersPool::addTask(Runnable* const task){
    int id = task->getId();
    if (id < 0){
        return false;
    }
    std::unique_lock<std::mutex> guard(mtx);
    if (runnables.count(id) == 0){
        runnables[id] = task;
        jobQueue.push_back(task);
        jobQueue.sort(RunnableLess());
        guard.unlock();
        qCheck.notify_one();
        return true;
    }
    return false;
}

bool WorkersPool::removeTask(const int id){
    Runnable* runnable;
    std::unique_lock<std::mutex> guard(mtx);
    if (runnables.count(id) > 0){
        runnable = runnables[id];
        runnables.erase(id);
        removeFromQueue(id);
        while (runnable->isRunning()){
            qCheck.wait_for(guard, std::chrono::milliseconds(IDLE));
        }
        removeFromQueue(id);
        guard.unlock();
        qCheck.notify_one();
        return true;
    }
    
    return false;
}

bool WorkersPool::removeFromQueue(int id){
    std::list<Runnable*>::iterator iter;
    bool found = false;
    iter = jobQueue.begin();
    while (iter != jobQueue.end()){
        if ((*iter)->getId() == id){
            iter = jobQueue.erase(iter);
            found = true;
        }
        iter++;
    }
    return found;
}

bool WorkersPool::addJob(const int id){
    bool added = false;
    if (runnables.count(id) > 0 && !runnables[id]->isPeriodic()){
        added = addGroupJob(runnables[id]->getGroupIds());
        jobQueue.sort(RunnableLess());
    }
    return added;
}

bool WorkersPool::addGroupJob(std::vector<int> group)
{
    bool added = false;
    for (auto runId : group){
        if (runnables.count(runId) > 0 && !runnables[runId]->isPeriodic()){
            jobQueue.push_back(runnables[runId]);
            added = true;
        }
    }
    return added;
}
