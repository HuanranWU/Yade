/*************************************************************************
*  Copyright (C) 2006 by Janek Kozicki                                   *
*  Copyright (C) 2019 by Anton Gladky                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "ThreadWorker.hpp"

namespace yade {

void ThreadWorker::setTerminate(bool b) { m_should_terminate = b; };

bool ThreadWorker::shouldTerminate() const { return m_should_terminate; };

bool ThreadWorker::done() const { return m_done; };

void ThreadWorker::callSingleAction()
{
	m_done = false;
	this->singleAction();
	m_done = true;
};

} // namespace yade
