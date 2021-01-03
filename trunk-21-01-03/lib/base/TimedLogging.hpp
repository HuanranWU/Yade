/*************************************************************************
*  2020      Janek Kozicki                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once
#include <lib/base/Logging.hpp>
#include <lib/base/Timer.hpp>

namespace yade { // Cannot have #include directive inside.

/*
 * This header provides macros LOG_TIMED_* very similar to the regular LOG_* macros, with one extra functionality:
 * the log messages are printed not more often than the first macro argument 'howOften'.
 *
 * They are intended is to make it easier to debug parts of code which print a lot of messages extremely fast.
 * It adds an extra code block that checks time and compares with time of last print. It means a that bit more
 * calculations are done than typical LOG_* macros which only perform an integer comparison to check filter level.
 * Here an extra subtraction is done to check times.
 *
 * Suggested use is only during debugging, because this is not supposed to be fast. Then better remove them afterwards.
 *
 * The *_TRACE family of macros are removed by compiler during release builds. So those are very safe to use, but make sure to compile yade
 * with `cmake -DMAX_LOG_LEVEL=6` option (because default is 5), or you would never see LOG_TRACE and LOG_TIMED_TRACE print anything.
 *
 *
 * In general there are two solutions to make each message check its timing separately:
 *  1. Create a std::map< int uniqueIdentifier , std::chrono::time_point<…> lastCheck > inside class Logging (or class derived from Logging) and check there.
 *  2. Or use a thread_local static variable inside a macro
 *
 * They both would work exactly the same. But std::map<…,…> inside a class has following disadvantages:
 *  - adds an extra lookup in std::map
 *  - requires boost::log, because it requires the Logging class
 * While it would have one advantage over macro solution:
 *  + would not have to check time if the filter level integer comparison says to skip printing.
 *
 * So for now I will implement this with a macro. So this works without boost::log and no std::map<…> lookup is performed,
 * because the variable is sitting right here.
 *
 * This could be later turned into a class derived from Logging and have exactly the same interface.
 * We will see what works best.
 *
 *
 * The usage of this macro is following:
 *
 *   LOG_TIMED_TRACE( howOften , MESSAGE ) // or LOG_TIMED_DEBUG(… , …) or other macros from this family.
 *
 *   → The howOften argument is of std::chrono::duration type https://en.cppreference.com/w/cpp/chrono/duration
 *     See file py/_log.cpp function testTimedLevels(); for example usage.
 *   → The MESSAGE argument is the same message as in regular LOG_* macros.
 *
 * To make this macro "type safe", like regular function calls I put a static_assert inside. It checks that the duration is compatible
 * with std::chrono::high_resolution_clock::now(); Without static_assert it would end up with weird compilation errors.
 *
 * So it will accept arguments such as:
 *
 *   using namespace std::chrono_literals;  // use this namespace to enable 's' and 'ms' postfix literals. Nice example in https://en.cppreference.com/w/cpp/thread/sleep_for
 *                                          // I can't use this namespace inside this header file, because this is a .hpp file, and that would produce namespace pollution problems.
 *
 *   LOG_TIMED_INFO( 2s  , "Print every 2 seconds")
 *   LOG_TIMED_INFO( 20ms, "Print every 20 milliseconds")
 *
 * These 2s or 20ms is a compile-time rational constant from namespace std::chrono_literals, so the compiler optimizes it into a single integer comparison.
 * Only seconds and milliseconds are accepted as arguments, to produce a more readable compiler error message (in case of problems).
 *
 */

namespace units { // C++ standard uses int64_t because ↓ it supports ±292.5 years in nanoseconds.
	using Seconds      = ::std::chrono::duration<int64_t, std::ratio<int64_t(1), int64_t(1)>>;
	using MilliSeconds = ::std::chrono::duration<int64_t, std::ratio<int64_t(1), int64_t(1000)>>;

	template <typename TimeUnit> const constexpr bool isSecond        = std::is_same<TimeUnit, Seconds>::value;
	template <typename TimeUnit> const constexpr bool isMilliSecond   = std::is_same<TimeUnit, MilliSeconds>::value;
	template <typename TimeUnit> const constexpr bool isSecOrMilliSec = isSecond<TimeUnit> or isMilliSecond<TimeUnit>;
}

// quick test, with invocation:  yade -f6
//   import log ; yade.log.testTimedLevels()

// use std::chrono_literals; in this block to properly expand 10s or 500ms from https://en.cppreference.com/w/cpp/header/chrono#Literals, it does not leak out.

#define LOG_TIMED(howOften, MSG)                                                                                                                               \
	{                                                                                                                                                      \
		using namespace std::chrono_literals;                                                                                                          \
		static_assert(units::isSecOrMilliSec<decltype(howOften)>, "LOG_TIMED_* Bad first argument, see testTimedLevels(); in file py/_log.cpp.");      \
		thread_local static auto t = Timer();                                                                                                          \
		if (t.check(howOften)) {                                                                                                                       \
			MSG                                                                                                                                    \
		}                                                                                                                                              \
	}

#define LOG_TIMED_NOFILTER(howOften, msg) LOG_TIMED(howOften, LOG_NOFILTER(msg))
#define LOG_TIMED_FATAL(howOften, msg) LOG_TIMED(howOften, LOG_FATAL(msg))
#define LOG_TIMED_ERROR(howOften, msg) LOG_TIMED(howOften, LOG_ERROR(msg))
#define LOG_TIMED_WARN(howOften, msg) LOG_TIMED(howOften, LOG_WARN(msg))
#define LOG_TIMED_INFO(howOften, msg) LOG_TIMED(howOften, LOG_INFO(msg))
#define LOG_TIMED_DEBUG(howOften, msg) LOG_TIMED(howOften, LOG_DEBUG(msg))
#define LOG_TIMED_TRACE(howOften, msg) LOG_TIMED(howOften, LOG_TRACE(msg))

// Logger aliases:
#define LOG_TIMED_6_TRACE(t, msg) LOG_TIMED_TRACE(t, msg)
#define LOG_TIMED_5_DEBUG(t, msg) LOG_TIMED_DEBUG(t, msg)
#define LOG_TIMED_4_INFO(t, msg) LOG_TIMED_INFO(t, msg)
#define LOG_TIMED_3_WARN(t, msg) LOG_TIMED_WARN(t, msg)
#define LOG_TIMED_2_ERROR(t, msg) LOG_TIMED_ERROR(t, msg)
#define LOG_TIMED_1_FATAL(t, msg) LOG_TIMED_FATAL(t, msg)
#define LOG_TIMED_0_NOFILTER(t, msg) LOG_TIMED_NOFILTER(t, msg)

#define LOG_TRACE_TIMED(t, msg) LOG_TIMED_TRACE(t, msg)
#define LOG_DEBUG_TIMED(t, msg) LOG_TIMED_DEBUG(t, msg)
#define LOG_INFO_TIMED(t, msg) LOG_TIMED_INFO(t, msg)
#define LOG_WARN_TIMED(t, msg) LOG_TIMED_WARN(t, msg)
#define LOG_ERROR_TIMED(t, msg) LOG_TIMED_ERROR(t, msg)
#define LOG_FATAL_TIMED(t, msg) LOG_TIMED_FATAL(t, msg)
#define LOG_NOFILTER_TIMED(t, msg) LOG_TIMED_NOFILTER(t, msg)

#define LOG_TIMED_6(t, msg) LOG_TIMED_TRACE(t, msg)
#define LOG_TIMED_5(t, msg) LOG_TIMED_DEBUG(t, msg)
#define LOG_TIMED_4(t, msg) LOG_TIMED_INFO(t, msg)
#define LOG_TIMED_3(t, msg) LOG_TIMED_WARN(t, msg)
#define LOG_TIMED_2(t, msg) LOG_TIMED_ERROR(t, msg)
#define LOG_TIMED_1(t, msg) LOG_TIMED_FATAL(t, msg)
#define LOG_TIMED_0(t, msg) LOG_TIMED_NOFILTER(t, msg)

// macros for quick debugging without spamming terminal:
#define TIMED_TRVAR1(T, a) LOG_TIMED_TRACE(T, _TRV(a))
#define TIMED_TRVAR2(T, a, b) LOG_TIMED_TRACE(T, _TRV(a) << _TRV(b))
#define TIMED_TRVAR3(T, a, b, c) LOG_TIMED_TRACE(T, _TRV(a) << _TRV(b) << _TRV(c))
#define TIMED_TRVAR4(T, a, b, c, d) LOG_TIMED_TRACE(T, _TRV(a) << _TRV(b) << _TRV(c) << _TRV(d))
#define TIMED_TRVAR5(T, a, b, c, d, e) LOG_TIMED_TRACE(T, _TRV(a) << _TRV(b) << _TRV(c) << _TRV(d) << _TRV(e))
#define TIMED_TRVAR6(T, a, b, c, d, e, f) LOG_TIMED_TRACE(T, _TRV(a) << _TRV(b) << _TRV(c) << _TRV(d) << _TRV(e) << _TRV(f))

#define TIMED_TRVARn_PRINT_ONE(r, WAIT, VARn) TIMED_TRVAR1(WAIT, VARn)

// this one prints arbitrary number of variables, but they must be a boost preprocessor sequence like (var1)(var2)(var3), see py/_log.cpp for example usage.
#define TIMED_TRVARn(WAIT, ALL_VARS) BOOST_PP_SEQ_FOR_EACH(TIMED_TRVARn_PRINT_ONE, WAIT, ALL_VARS)

} // namespace yade
