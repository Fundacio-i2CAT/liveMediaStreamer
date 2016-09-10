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

TaskQueue::TaskQueue(){
    iter = queue.begin();
}

void TaskQueue::pushBack(Runnable *run){
    if (sQueue.find(run) == sQueue.end()){
        sQueue.insert(run);
        queue.push_back(run);
        resetIterator();
    }
}

void TaskQueue::pop(){
    if (!queue.empty()){
        sQueue.erase(queue.front());
        queue.pop_front();
        resetIterator();
    }
}

void TaskQueue::resetIterator(){
    iter = queue.begin();
}

void TaskQueue::clear(){
    queue.clear();
    sQueue.clear();
}

Runnable* TaskQueue::current(){
    if (iter != queue.end()){
        return *iter;
    }
    return NULL;
}

void TaskQueue::next(){
    if (iter != queue.end()){
        iter++;
    }
}


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
                std::vector<int> enabledJobs;
                bool added = false;
                
                while(true) {
                    std::unique_lock<std::mutex> guard(mtx);
                    queue.resetIterator();
                    while (run) {
                        job = queue.current();
                        if (!job){
                            qCheck.wait_for(guard, std::chrono::milliseconds(IDLE));
                        } else if (!job->isRunning() && !job->ready()) {
                            qCheck.wait_until(guard, job->getTime());
                        } else if (!job->isRunning() && job->ready()){
                            queue.pop();
                            break;
                        } else {
                            queue.next();
                            continue;
                        }
                        queue.resetIterator();
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
                            queue.pushBack(runnables[id]);
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
    queue.clear();
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
        queue.pushBack(task);
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
        Runnable *runnable = runnables[id];
        runnables.erase(id);
        guard.unlock();
        qCheck.notify_one();
        while(runnable->isRunning()){
            utils::warningMsg("awaiting task to finish " + std::to_string(id));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return true;
    }
    
    return false;
}
