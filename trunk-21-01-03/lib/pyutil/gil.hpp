// 2009 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
// XXX never do #include<Python.h>, see https://www.boost.org/doc/libs/1_71_0/libs/python/doc/html/building/include_issues.html
#include <boost/python/detail/wrap_python.hpp>
#include <string>
//! class (scoped lock) managing python's Global Interpreter Lock (gil)
class gilLock {
	PyGILState_STATE state;

public:
	gilLock() { state = PyGILState_Ensure(); }
	~gilLock() { PyGILState_Release(state); }
};
//! run string as python command; locks & unlocks GIL automatically
void pyRunString(const std::string& cmd, bool ignoreErrors = false);
