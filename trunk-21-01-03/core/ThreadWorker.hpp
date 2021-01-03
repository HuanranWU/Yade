/*************************************************************************
*  Copyright (C) 2006 by Janek Kozicki                                   *
*  Copyright (C) 2019 by Anton Gladky                                    *
*                                                                        *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include <atomic>

namespace yade { // Cannot have #include directive inside.

class ThreadRunner;

/*!
\brief	ThreadWorker contains information about tasks to be performed when
	the separate thread is executed.
 */

class ThreadWorker //         perhaps simulation steps, or stage? as it is a single stage
                   //         of the simulation, that consists of several steps
                   // Update: it is more general now. simulation stages perhaps will be derived from this class
{
private:
	/// You should check out ThreadRunner, it is used for execution control of this class
	friend class ThreadRunner;
	std::atomic_bool m_should_terminate { false };
	std::atomic_bool m_done { false };
	void             callSingleAction();

protected:
	void setTerminate(bool);
	/// singleAction() can check whether someone asked for termination, and terminate if/when possible
	bool shouldTerminate() const;
	/// derived classes must define this method, that's what is executed in separate thread
	virtual void singleAction() = 0;

public:
	ThreadWorker()          = default;
	virtual ~ThreadWorker() = default;
	/// Check whether execution is finished,
	bool done() const;
};

} // namespace yade
