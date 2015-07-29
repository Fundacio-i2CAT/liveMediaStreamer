/*
 *  WorkersPool.hh - This Worker Pool contains all processing threads
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

#ifndef _WORKERS_POOL_HH
#define _WORKERS_POOL_HH

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>
#include <list>

#include "Runnable.hh"

#define IDLE 10


class WorkersPool
{
public:
    WorkersPool(size_t threads = 0);
    ~WorkersPool();
        
    bool addTask(Runnable* const runnable);
    bool removeTask(const int id);
    
private:
    bool addJob(int id);
    bool addGroupJob(std::vector<int> group);
    bool removeFromQueue(int id);

private:
    std::vector<std::thread>    workers;
    std::mutex                  mtx;
    std::condition_variable     qCheck;
    std::map<int, Runnable*>    runnables;
    std::list<Runnable*>        jobQueue;
    bool                        run;
};

#endif