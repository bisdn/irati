//
// Timer facilities
//
//    Bernat Gaston         <bernat.gaston@i2cat.net>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA
//

#include <cerrno>

#define RINA_PREFIX "librina.timer"

#include "librina/logs.h"
#include "librina/timer.h"
#include "librina/common.h"

namespace rina {

// CLASS Time
Time::Time() {
	gettimeofday(&time_, 0);
}
Time::Time(timeval t) {
	time_ = t;
}
int Time::get_current_time_in_ms() const {
	return (int) get_time_seconds() * 1000 + get_only_milliseconds();
}
int Time::get_time_seconds() const {
	return (int) time_.tv_sec;
}
int Time::get_only_milliseconds() const {
	return (int) (time_.tv_usec / 1000);
}
bool Time::operator<(const Time &other) const {
	if ((int) time_.tv_sec != (int) other.get_time_seconds())
		return (int) time_.tv_sec < (int) other.get_time_seconds();
	else
		return (int) (time_.tv_usec / 1000) < (int) other.get_only_milliseconds();
}
void Time::set_timeval(timeval t) {
	time_ = t;
}

// CLASS LockableMap
void* doWorkTask(void *arg) {
	TimerTask *timer_task = (TimerTask*) arg;
	timer_task->run();
	delete timer_task;
	timer_task = 0;
	return (void *) 0;
}

TaskScheduler::TaskScheduler() :
		Lockable() {
}
TaskScheduler::~TaskScheduler() throw () {
  for (std::map<Time, std::list<TimerTask*>* >::iterator iter_map = tasks_.begin();
      iter_map != tasks_.end(); ++iter_map) {
    for (std::list<TimerTask*>::iterator iter_list = iter_map->second->begin();
        iter_list != iter_map->second->end(); ++iter_list){
      delete *iter_list;
    }
    iter_map->second->empty();
    delete iter_map->second;
  }
  tasks_.empty();
}
void TaskScheduler::insert(Time time, TimerTask* timer_task) {
	lock();

	std::map<Time, std::list<TimerTask*>* >::iterator iter = tasks_.find(time);
	if (iter != tasks_.end()) {
		iter->second->push_back(timer_task);
	} else {
		std::list<TimerTask*> *list = new std::list<TimerTask*>;
		list->push_back(timer_task);
		std::pair<Time, std::list<TimerTask*>* > pair(time, list);
		tasks_.insert(pair);
	}

	unlock();
}

void TaskScheduler::runTasks() {
	lock();
	Time now;
	std::map<Time, std::list<TimerTask*>* >::iterator first = tasks_.begin();
	std::map<Time, std::list<TimerTask*>* >::iterator last = tasks_.upper_bound(
			now);
	for (std::map<Time, std::list<TimerTask*>* >::iterator iter_map = first;
			iter_map != last; ++iter_map) {
		for (std::list<TimerTask*>::iterator iter_list = iter_map->second->begin();
				iter_list != iter_map->second->end(); ++iter_list){
			try {
				ThreadAttributes threadAttributes;
				threadAttributes.setDettached();
				Thread *t = new Thread(&doWorkTask,
						       (void *) (*iter_list),
						       &threadAttributes);
				t->start();
				delete t;
				t = 0;
			} catch (Exception &e) {
				LOG_ERR("Problems creating thread: %s", e.what());
			}
		}
		delete iter_map->second;
	}
	tasks_.erase(first, last);
	unlock();
}

void TaskScheduler::cancelTask(TimerTask *task) {
	lock();
	for (std::map<Time, std::list<TimerTask*>* >::iterator iter_map =
			tasks_.begin(); iter_map != tasks_.end(); ++iter_map) {
		for (std::list<TimerTask*>::iterator iter_list = iter_map->second->begin();
				iter_list != iter_map->second->end(); ++iter_list){
			if (*iter_list == task){
				delete *iter_list;
				*iter_list = 0;
				std::list<TimerTask*>::iterator removeIter = iter_list;
				--iter_list;
				iter_map->second->erase(removeIter);
			}
		}
		if (iter_map->second->size() == 0){
			delete iter_map->second;
			tasks_.erase(iter_map);
		}
	}
unlock();
}

// CLASS Timer
void* doWorkTimer(void *arg) {
	Timer *timer = (Timer*) arg;
	Sleep sleep;
	while (timer->is_continue()) {
		sleep.sleepForMili(500);
		timer->get_task_scheduler()->runTasks();
	}
	return (void *) 0;
}

Timer::Timer() {
	ThreadAttributes threadAttributes;
	threadAttributes.setJoinable();
	continue_lock_.lock();
	continue_ = true;
	continue_lock_.unlock();
	task_scheduler = new TaskScheduler();
	thread_ = new Thread(&doWorkTimer, (void *) this, &threadAttributes);
	thread_->start();
	LOG_DBG("Timer with ID %d started", thread_);
}
Timer::~Timer() {
	cancel();

	if (task_scheduler) {
		delete task_scheduler;
		task_scheduler = 0;
	}

	if (thread_) {
		delete thread_;
		thread_ = 0;
	}
}

void Timer::scheduleTask(TimerTask* task, long delay_ms) {
	Time executeTime;
	timeval t;
	int milisecondsNotNorm = executeTime.get_only_milliseconds() + (delay_ms % 1000);
	int miliseconds = milisecondsNotNorm % 1000;
	int seconds = executeTime.get_time_seconds() + (delay_ms / 1000) + (milisecondsNotNorm / 1000);
	t.tv_sec = seconds;
	t.tv_usec = miliseconds * 1000;
	executeTime.set_timeval(t);
	task_scheduler->insert(executeTime, task);
}
void Timer::cancelTask(TimerTask* task) {
	task_scheduler->cancelTask(task);
}
void Timer::cancel() {
	continue_lock_.lock();
	continue_ = false;
	continue_lock_.unlock();
	void *r;
	LOG_DBG("Waiting for the timer %d to join", thread_);
	thread_->join(&r);
	LOG_DBG("Timer with ID %d ended", thread_);
}
TaskScheduler* Timer::get_task_scheduler() const {
	return task_scheduler;
}
bool Timer::is_continue() {
	bool result;
	continue_lock_.lock();
	result = continue_;
	continue_lock_.unlock();
	return result;
}
}

