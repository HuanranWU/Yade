/*************************************************************************
*  Copyright (C) 2006 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "ThreadRunner.hpp"
#include "ThreadWorker.hpp"
#include <lib/base/Logging.hpp>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>

namespace yade { // Cannot have #include directive inside.

CREATE_LOGGER(ThreadRunner);

void ThreadRunner::run()
{
	// this is the body of execution of separate thread
	const std::lock_guard<std::mutex> lock(m_runmutex);
	try {
		workerThrew = false;
		while (looping()) {
			call();
			if (m_thread_worker->shouldTerminate()) {
				stop();
				return;
			}
		}
	} catch (std::exception& e) {
		LOG_FATAL("Exception occured: " << std::endl << e.what());
		workerException = std::exception(e);
		workerThrew     = true;
		stop();
		return;
	}
}

void ThreadRunner::call()
{
	// this is the body of execution of separate thread
	//
	// FIXME - if several threads are blocked here and waiting, and the
	// destructor is called we get a crash. This happens if some other
	// thread is calling spawnSingleAction in a loop (instead of calling
	// start() and stop() as it normally should). This is currently the
	// case of SimulationController with synchronization turned on.
	//
	// the solution is to use a counter (perhaps recursive_mutex?) which
	// will count the number of threads in the queue, and only after they
	// all finish execution the destructor will be able to finish its work
	//
	const std::lock_guard<std::mutex> lock(m_callmutex);
	m_thread_worker->setTerminate(false);
	m_thread_worker->callSingleAction();
}

void ThreadRunner::spawnSingleAction()
{
	if (m_looping)
		return;
	const std::lock_guard<std::mutex> calllock(m_callmutex);
	boost::function0<void>            call(boost::bind(&ThreadRunner::call, this));
	boost::thread                     th(call);
}

void ThreadRunner::start()
{
	if (!m_looping.exchange(true)) {
		boost::function0<void> run(boost::bind(&ThreadRunner::run, this));
		boost::thread          th(run);
	}
}

void ThreadRunner::stop() { m_looping = false; }

bool ThreadRunner::looping() const { return m_looping; }

ThreadRunner::~ThreadRunner()
{
	stop();
	m_thread_worker->setTerminate(true);
	const std::lock_guard<std::mutex> runlock(m_runmutex);
	const std::lock_guard<std::mutex> calllock(m_callmutex);
}

} // namespace yade
