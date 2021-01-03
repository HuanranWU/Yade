/*************************************************************************
*  2006-2008 © Václav Šmilauer                                           *
*  2019      © Janek Kozicki, rewritten using boost::log the version     *
*              which was removed in 2008 (git revision 014b11496)        *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// hint: follow changes in d067b0696a8 to add new modules.

#include <lib/base/AliasNamespaces.hpp>
#include <lib/base/Logging.hpp>
#include <lib/base/TimedLogging.hpp>
#include <lib/pyutil/doc_opts.hpp>
#include <core/Omega.hpp>
#include <boost/core/demangle.hpp>
#include <chrono>
#include <string>
#include <thread>

CREATE_CPP_LOCAL_LOGGER("_log.cpp");

namespace yade { // Cannot have #include directive inside.

#ifdef YADE_BOOST_LOG

int getDefaultLogLevel() { return Logging::instance().getDefaultLogLevel(); }

void setDefaultLogLevel(int level) { Logging::instance().setNamedLogLevel("Default", level); }

// accepted streams: "clog", "cerr", "cout", "filename"
// It is possible to set different levels per log file, see notes about that in Logging::setOutputStream(…)
void setOutputStream(std::string streamName, bool reset)
{
	Logging::instance().setOutputStream(streamName, reset);
	if (reset) {
		LOG_INFO("Log output stream has been set to " << streamName << ". Other output streams were removed.");
	} else {
		LOG_INFO("Additional output stream has been set to " << streamName << ".");
	}
}

void resetOutputStream()
{
	Logging::instance().setOutputStream("clog", true);
	LOG_INFO("Log output stream has been reset to std::clog. File sinks are not removed.");
}

void setLevel(std::string className, int level)
{
	Logging::instance().setNamedLogLevel(className, level);
	LOG_INFO("filter log level for " << className << " has been set to " << Logging::instance().getNamedLogLevel(className));
}

void unsetLevel(std::string className)
{
	Logging::instance().unsetNamedLogLevel(className);
	LOG_INFO("filter log level for " << className << " has been unset to " << Logging::instance().getNamedLogLevel(className));
}

py::dict getAllLevels()
{
	py::dict ret {};
	for (const auto& a : Logging::instance().getClassLogLevels()) {
		ret[a.first] = a.second;
	}
	return ret;
}

py::dict getUsedLevels()
{
	py::dict ret {};
	for (const auto& a : Logging::instance().getClassLogLevels()) {
		if (a.second != -1) {
			ret[a.first] = a.second;
		}
	}
	return ret;
}

void setUseColors(bool use) { Logging::instance().setUseColors(use); }

void readConfigFile(std::string fname) { Logging::instance().readConfigFile(fname); }

void saveConfigFile(std::string fname) { Logging::instance().saveConfigFile(fname); }

std::string defaultConfigFileName() { return Logging::instance().defaultConfigFileName(); }

int getMaxLevel() { return Logging::instance().maxLogLevel; }

#else // #ifdef YADE_BOOST_LOG, without boost::log use the simplified logging method:

void printNoBoostLogWarning()
{
	std::cerr
	        << "\nWarning: yade was compiled with cmake option -DENABLE_LOGGER=OFF, any attempts to manipulate log filter levels will not have effect.\n\n";
}

int      getDefaultLogLevel() { return std::min(MAX_LOG_LEVEL, MAX_HARDCODED_LOG_LEVEL); }
void     setDefaultLogLevel(int) { printNoBoostLogWarning(); }
void     setOutputStream(std::string, bool) { printNoBoostLogWarning(); }
void     resetOutputStream() { printNoBoostLogWarning(); }
void     setLevel(std::string, int) { printNoBoostLogWarning(); }
void     unsetLevel(std::string) { printNoBoostLogWarning(); }
py::dict getAllLevels()
{
	printNoBoostLogWarning();
	return {};
}
py::dict getUsedLevels()
{
	printNoBoostLogWarning();
	return {};
}
void        setUseColors(bool) { printNoBoostLogWarning(); }
void        readConfigFile(std::string) { printNoBoostLogWarning(); }
void        saveConfigFile(std::string) { printNoBoostLogWarning(); }
std::string defaultConfigFileName()
{
	printNoBoostLogWarning();
	return "";
}
int getMaxLevel() { return std::min(MAX_LOG_LEVEL, MAX_HARDCODED_LOG_LEVEL); }

#endif // END #ifdef YADE_BOOST_LOG

void testAllLevels()
{
	int                testInt = 0;
	std::string        testStr = "test string";
	Real               testReal(11);
	Vector3r           testVec(1, 2, 3);
	Matrix3r           testMat = (Matrix3r() << 1, 2, 3, 4, 5, 6, 7, 8, 9).finished();
	std::complex<Real> testComplex(-1, 1);

	LOG_0_NOFILTER("Test log level: LOG_0_NOFILTER, test int: " << testInt++);
	LOG_1_FATAL("Test log level: LOG_1_FATAL, test int: " << testInt++);
	LOG_2_ERROR("Test log level: LOG_2_ERROR, test int: " << testInt++);
	LOG_3_WARN("Test log level: LOG_3_WARN, test int: " << testInt++);
	LOG_4_INFO("Test log level: LOG_4_INFO, test int: " << testInt++);
	LOG_5_DEBUG("Test log level: LOG_5_DEBUG, test int: " << testInt++);
	LOG_6_TRACE("Test log level: LOG_6_TRACE, test int: " << testInt++);

	LOG_0_NOFILTER("Below 6 variables are printed at filter level TRACE, then macro TRACE; is used");
	TRVAR1(testInt);
	TRVAR2(testInt, testStr);
	TRVAR3(testInt, testStr, testReal);
	TRVAR4(testInt, testStr, testReal, testVec);
	TRVAR5(testInt, testStr, testReal, testVec, testMat);
	TRVAR6(testInt, testStr, testReal, testVec, testMat, testComplex);
	LOG_TRACE("\n\nTest print of arbitrary number of variables (one argument):");
	TRVARn((testInt));
	LOG_TRACE("\n\nTest print of arbitrary number of variables (8 arguments):");
	TRVARn((testInt)(testStr)(testReal)(testVec)(testMat)(testComplex)(7)(8));

	TRACE;

	LOG_TRACE("\n\nTest print of TIMED_TRVARn macro family (every 2 seconds, one time):\n\n");
	for (int i = 0; i < 11; ++i) {
		TIMED_TRVAR1(1s, testInt);
		TIMED_TRVAR2(1s, testInt, testStr);
		TIMED_TRVAR3(1s, testInt, testStr, testReal);
		TIMED_TRVAR4(1s, testInt, testStr, testReal, testVec);
		TIMED_TRVAR5(1s, testInt, testStr, testReal, testVec, testMat);
		TIMED_TRVAR6(1s, testInt, testStr, testReal, testVec, testMat, testComplex);

		LOG_TIMED_TRACE(1s, "\n\nTest print of arbitrary number of variables (one argument):");

		TIMED_TRVARn(1s, (testInt));

		LOG_TIMED_TRACE(1s, "\n\nTest print of arbitrary number of variables (8 arguments):");

		TIMED_TRVARn(1s, (testInt)(testStr)(testReal)(testVec)(testMat)(testComplex)(7)(8));

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(100ms);
	}
}

void testTimedLevels()
{
	unsigned long long int testInt = 0;
	using namespace std::chrono_literals; // use the following function signature:
	//
	//        constexpr chrono::duration<…,…> operator "" s(unsigned long long);
	//
	// to be able to write 1s to mean 1 second. See:
	// * https://en.cppreference.com/w/cpp/chrono/operator%22%22h
	// * https://en.cppreference.com/w/cpp/thread/sleep_for

	// with `using namespace std::chrono_literals` you can write this:
	constexpr auto wait_half = 500ms;
	constexpr auto wait_1s   = 1s;
	constexpr auto wait_2s   = 2s;
	// without `using namespace std::chrono_literals` you have to write this:
	constexpr std::chrono::duration<int64_t, std::ratio<int64_t(1), int64_t(1000)>> wait_half_without_using(
	        500); // int64_t type supports ±292.5 years in nanoseconds.
	// the constexpr above was used only for the purpose to demonstrate that these are the same:
	static_assert(wait_half == wait_half_without_using, "Half a second should be 500ms.");
	static_assert(std::is_same<decltype(wait_half), decltype(wait_half_without_using)>::value, "And the type is decltype(500ms)");

	TRVAR3(wait_half.count(), wait_1s.count(), wait_2s.count())
	using boost::core::demangle; // to print exact type of std::chrono::duration

	// This auto type declared above for 500ms, 1s, 2s is actually std::chrono::duration<…,…> like this:
	LOG_DEBUG(
	        "\n The types of LOG_TIMED_* first argument are any of std::chrono::duration<…> like for example:\n "
	        << "type of wait_half is " << demangle(typeid(decltype(wait_half)).name()) << "; wait_half.count() == " << wait_half.count() << "\n "
	        << "type of wait_1s   is " << demangle(typeid(decltype(wait_1s)).name()) << "   ; wait_1s.count()   == " << wait_1s.count() << "\n "
	        << "type of wait_2s   is " << demangle(typeid(decltype(wait_2s)).name()) << "   ; wait_2s.count()   == " << wait_2s.count()
	        << "\n----------------\n")

	LOG_NOFILTER("Starting the 2.1 second test. The '2s' logs should be printed once, the '0.5s' should be printed four times.\n\n")
	for (int i = 0; i < 21; ++i) {
		// write 2s  here     ↓ instead of using variable wait_2s, because that's how it is comfortable to be used. The variable wait_2s above was only a demonstration of how the type works.
		LOG_TIMED_0_NOFILTER(2s, "Test 2s timed log level: LOG_0_NOFILTER, test int: " << testInt++);
		LOG_TIMED_1_FATAL(2s, "Test 2s timed log level: LOG_1_FATAL, test int: " << testInt++);
		LOG_TIMED_2_ERROR(2s, "Test 2s timed log level: LOG_2_ERROR, test int: " << testInt++);
		LOG_TIMED_3_WARN(1s, "Test 1s timed log level: LOG_3_WARN, test int: " << testInt++);
		LOG_TIMED_4_INFO(1s, "Test 1s timed log level: LOG_4_INFO, test int: " << testInt++);
		LOG_TIMED_5_DEBUG(500ms, "Test 0.5s timed log level: LOG_5_DEBUG, test int: " << testInt++ << "\n");
		LOG_TIMED_6_TRACE(500ms, "Test 0.5s timed log level: LOG_6_TRACE, test int: " << testInt++ << "\n");
		std::this_thread::sleep_for(100ms);
	}

	LOG_WARN("Upon next call of this function the '2s' will be printed first also, because the timers are 'thread_local static', which means that they "
	         "preserve timers between different calls to this function, but have separate static instances for each different thread.")
}

} // namespace yade

// BOOST_PYTHON_MODULE cannot be inside yade namespace, it has 'extern "C"' keyword, which strips it out of any namespaces.
BOOST_PYTHON_MODULE(_log)
try {
	using namespace yade; // 'using namespace' inside function keeps namespace pollution under control. Alernatively I could add y:: in front of function names below and put 'namespace y  = ::yade;' here.
	namespace py = ::boost::python;
	YADE_SET_DOCSTRING_OPTS;
	// We can use C++ string literal just like """ """ in python to write docstrings (see. https://en.cppreference.com/w/cpp/language/string_literal )
	// The """ is a custom delimeter, we could use    R"RAW( instead, or any other delimeter. This decides what will be the termination delimeter.
	// The docstrings can use syntax :param ……: ……… :return: ……. For details see https://thomas-cokelaer.info/tutorials/sphinx/docstring_python.html

	py::def("testAllLevels", testAllLevels, R"""(
This function prints test messages on all log levels. Can be used to see how filtering works and to what streams the logs are written.
	)""");

	py::def("testTimedLevels", testTimedLevels, R"""(
This function prints timed test messages on all log levels. In this test the log levels ∈ [0…2] are timed to print every 2 seconds, levels ∈ [3,4] every 1 second and levels ∈ [5,6] every 0.5 seconds. The loop lasts for 2.1 seconds. Can be used to see how timed filtering works and to what streams the logs are written.
	)""");

	// default level
	py::def("getDefaultLogLevel", getDefaultLogLevel, R"""(
:return: The current ``Default`` filter log level.
	)""");

	py::def("setDefaultLogLevel", setDefaultLogLevel, R"""(
:param int level: Sets the ``Default`` filter log level, same as calling ``log.setLevel("Default",level)``.
	)""");

	// output streams, files, cout, cerr, clog.
	py::def("setOutputStream", setOutputStream, R"""(
:param str streamName: sets the output stream, special names ``cout``, ``cerr``, ``clog`` use the ``std::cout``, ``std::cerr``, ``std::clog`` counterpart (``std::clog`` the is the default output stream). Every other name means that log will be written to a file with name provided in the argument.
:param bool reset: dictates whether all previously set output streams are to be removed. When set to false: the new output stream is set additionally to the current one.
	)""");

	py::def("resetOutputStream", resetOutputStream, R"""(
Resets log output stream to default state: all logs are printed on ``std::clog`` channel, which usually redirects to ``std::cerr``.
	)""");

	// filter levels
	py::def("setLevel", setLevel, R"""(
Set filter level (constants ``TRACE`` (6), ``DEBUG`` (5), ``INFO`` (4), ``WARN`` (3), ``ERROR`` (2), ``FATAL`` (1), ``NOFILTER`` (0)) for given logger.

:param str className: The logger name for which the filter level is to be set. Use name ``Default`` to change the default filter level.
:param int level: The filter level to be set.
.. warning:: setting ``Default`` log level higher than ``MAX_LOG_LEVEL`` provided during compilation will have no effect. Logs will not be printed because they are removed during compilation.
	)""");

	py::def("unsetLevel", unsetLevel, R"""(
:param str className: The logger name for which the filter level is to be unset, so that a ``Default`` will be used instead. Unsetting the ``Default`` level will change it to max level and print everything.
	)""");

	py::def("getAllLevels", getAllLevels, R"""(
:return: A python dictionary with all known loggers in yade. Those without a debug level set will have value -1 to indicate that ``Default`` filter log level is to be used for them.
	)""");

	py::def("getUsedLevels", getUsedLevels, R"""(
:return: A python dictionary with all used log levels in yade. Those without a debug level (value -1) are omitted.
	)""");

	py::def("getMaxLevel", getMaxLevel, R"""(
:return: the MAX_LOG_LEVEL of the current yade build.
	)""");

	// colors
	py::def("setUseColors", setUseColors, R"""(
Turn on/off colors in log messages. By default is on. If logging to a file then it is better to be turned off.
	)""");

	// config file
	py::def("readConfigFile", readConfigFile, R"""(
Loads the given configuration file.

:param str fname: the config file to be loaded.
	)""");
	py::def("saveConfigFile", saveConfigFile, R"""(
Saves log config to specified file.

:param str fname: the config file to be saved.
	)""");
	py::def("defaultConfigFileName", defaultConfigFileName, R"""(
:return: the default log config file, which is loaded at startup, if it exists.
	)""");

	py::scope().attr("TRACE")    = int(6);
	py::scope().attr("DEBUG")    = int(5);
	py::scope().attr("INFO")     = int(4);
	py::scope().attr("WARN")     = int(3);
	py::scope().attr("ERROR")    = int(2);
	py::scope().attr("FATAL")    = int(1);
	py::scope().attr("NOFILTER") = int(0);

} catch (...) {
	// How to work with python exceptions:
	//     https://www.boost.org/doc/libs/1_71_0/libs/python/doc/html/reference/high_level_components/boost_python_errors_hpp.html#high_level_components.boost_python_errors_hpp.example
	//     https://www.boost.org/doc/libs/1_71_0/libs/python/doc/html/tutorial/tutorial/embedding.html
	//     https://stackoverflow.com/questions/1418015/how-to-get-python-exception-text
	// If we wanted custom yade exceptions thrown to python:
	//     https://www.boost.org/doc/libs/1_71_0/libs/python/doc/html/tutorial/tutorial/exception.html
	//     https://www.boost.org/doc/libs/1_71_0/libs/python/doc/html/reference/high_level_components/boost_python_exception_translato.html
	LOG_FATAL("Importing this module caused an exception and this module is in an inconsistent state now.");
	PyErr_Print();
	PyErr_SetString(PyExc_SystemError, __FILE__);
	boost::python::handle_exception();
	throw;
}
