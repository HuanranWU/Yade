#include <lib/base/Logging.hpp>
#include <lib/pyutil/gil.hpp>

CREATE_CPP_LOCAL_LOGGER("gil.cpp")
void pyRunString(const std::string& cmd, bool ignoreErrors)
{
	// FIXME: when a new function is declared inside ipython session an extra python command must be called: globals().update(locals())
	//        https://stackoverflow.com/questions/43956636/internal-function-call-not-working-in-ipython
	//        calling it always here, before each call to cmd.c_str() is worth discussing.
	//        For now I will just add a note about that in documentation: doc/sphinx/tutorial-data-mining.rst
	gilLock lock;
	auto    result = PyRun_SimpleString(cmd.c_str());
	if (result != 0) {
		if (ignoreErrors) {
			LOG_WARN("Error running command: '" << cmd << "', but has ignoreErrors == true; not throwing exception.");
		} else {
			LOG_ERROR("Error running command: '" << cmd << "'");
			// An exception occurred. Unfortunately the memory barrier between both threads makes it rather difficult to find out
			// what kind of exception it was. So let's just print it and throw a runtime_error.
			// FIXME : find a way to extract exception from python and rethrow the same exception. It will have to be
			//         a "reconstruction" (not even a copy) of the original exception though, because here we are inside
			//         the memory region of a separate thread. But in fact also the same problem occurs on the barrier
			//         between ThreadRunner and ThreadWorker in file core/ThreadRunner.cpp:36 where this exception gets
			//         converted to std::exception. So if someone wants to fix this here, he should also fix it there.
			throw std::runtime_error("PyRunner → pyRunString encountered error while executing command: '" + cmd + "'");
		}
	}
};
