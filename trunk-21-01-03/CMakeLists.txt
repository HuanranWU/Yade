# The YADE has the following parameters to configure:
#  CMAKE_INSTALL_PREFIX: path, where Yade will be installed (/usr/local by default)
#  LIBRARY_OUTPUT_PATH: path to install libraries (lib by default)
#  DEBUG: compile in debug-mode (OFF by default)
#  ENABLE_LOGGER: use boost::log library for logging (ON by default)
#  MAX_LOG_LEVEL: set maximum level for LOG_* macros compiled with ENABLE_LOGGER. For production build use MAX_LOG_LEVEL<=5, to avoid filter integer comparison on each call to LOG_TRACE(…)
#  ENABLE_USEFUL_ERRORS: enable useful compiler errors which help a lot in error-free development.
#  CMAKE_VERBOSE_MAKEFILE: output additional information during compiling (OFF by default)
#  SUFFIX: suffix, added after binary-names (version number by default)
#  NOSUFFIX: do not add a suffix after binary-name (OFF by default)
#  YADE_VERSION: explicitely set version number (is defined from git-directory by default)
#  ENABLE_GUI: enable GUI option (ON by default)
#  ENABLE_CGAL: enable CGAL option (ON by default)
#  ENABLE_VTK: enable VTK-export option (ON by default)
#  ENABLE_OPENMP: enable OpenMP-parallelizing option (ON by default)
#  ENABLE_MPI : Enable MPI communications in Yade. Used for Yade-OpenFOAM coupling.
#  ENABLE_GTS: enable GTS-option (ON by default)
#  ENABLE_GL2PS: enable GL2PS-option (ON by default)
#  ENABLE_LINSOLV: enable LINSOLV-option (ON by default)
#  ENABLE_PFVFLOW: enable PFVFLOW-option, FlowEngine (ON by default)
#  ENABLE_TWOPHASEFLOW: enable TWOPHASEFLOW-option, TwoPhaseFlowEngine (ON by default)
#  ENABLE_LBMFLOW: enable LBMFLOW-option, LBM_ENGINE (ON by default)
#  ENABLE_SPH: enable SPH-option, Smoothed Particle Hydrodynamics (OFF by default)
#  ENABLE_LIQMIGRATION: enable LIQMIGRATION-option, see [Mani2013] for details (OFF by default)
#  ENABLE_MASK_ARBITRARY: enable MASK_ARBITRARY option (OFF by default)
#  ENABLE_PROFILING: enable profiling, e.g. shows some more metrics, which can define bottlenecks of the code (OFF by default)
#  ENABLE_POTENTIAL_BLOCKS: enable potential blocks option (ON by default)
#  ENABLE_POTENTIAL_PARTICLES: enable potential particles option (ON by default)
#  ENABLE_DEFORM: enable constant volume deformation engine (OFF by default)
#  ENABLE_FEMLIKE: enable meshed solids (FEM-like)
#  ENABLE_OAR: generate a script for oar-based task scheduler (OFF by default)
#  ENABLE_ASAN: AddressSanitizer build, please see documentation (OFF by default)
#  ENABLE_MPFR: use https://www.mpfr.org/ library, can be used for higher precision calculations or for CGAL exact predicates (OFF by default)
#  ENABLE_PARTIALSAT: enable the partially saturated clay engine, under construction (OFF by default)
#  REAL_PRECISION_BITS , REAL_DECIMAL_PLACES: specify either of them to use a custom calculation precision. By default double (64 bits, 15 decimal places) precision is used.
#  runtimePREFIX: used for packaging, when install directory is not the same is runtime directory (/usr/local by default)
#  CHUNKSIZE: set >1, if you want several sources to be compiled at once. Increases compilation speed and RAM-consumption during it (1 by default)
#  VECTORIZE: enables vectorization and alignment in Eigen3 library, experimental (OFF by default)
#  USE_QT5: use QT5 for GUI (ON by default)
#  CHOLMOD_GPU link Yade to custom SuiteSparse installation and activate GPU accelerated PFV (OFF by default)
#  SUITESPARSEPATH: define this variable with the path to a custom suitesparse install
#  PYTHON_VERSION: force python version to the given one, set -1 to automatically use the last version on the system. (-1 by default)

project(Yade C CXX)
cmake_minimum_required(VERSION 3.5.0)
IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0.0)
  set (CMAKE_CXX_STANDARD 14)
ELSE()
  set (CMAKE_CXX_STANDARD 17)
ENDIF()
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# https://misc.flogisoft.com/bash/tip_colors_and_formatting
# https://stackoverflow.com/questions/18968979/how-to-get-colorized-output-with-cmake , use ${Esc} for printing colors
string(ASCII 27 Esc)
# To have a cmake option with multiple values I used https://cmake.org/pipermail/cmake/2016-October/064342.html
# set maximum level for LOG_* macros compiled with ENABLE_LOGGER. For production build use MAX_LOG_LEVEL below 5, to avoid filter integer comparison on each call to LOG_TRACE(…)
set(MAX_LOG_LEVEL "5" CACHE STRING "default MAX_LOG_LEVEL")
set_property(CACHE MAX_LOG_LEVEL PROPERTY STRINGS "0" "1" "2" "3" "4" "5" "6")  # define list of values GUI will offer for the variable


#===========================================================
SET(DEFAULT ON CACHE INTERNAL "Default value for enabled by default options")
SET(LINKLIBS "")
SET(CONFIGURED_FEATS "")
SET(DISABLED_FEATS "")

OPTION(ENABLE_LOGGER "use boost::log library for logging (ON by default)" ON)
OPTION(ENABLE_USEFUL_ERRORS "enable useful compiler errors which help a lot in error-free development." ON)
OPTION(ENABLE_VTK "Enable VTK" ${DEFAULT})
OPTION(ENABLE_OPENMP "Enable OpenMP" ${DEFAULT})
OPTION(ENABLE_MPI "Enable MPI environement and communications, required for coupling Yade with OpenFOAM  " ON)
OPTION(ENABLE_GTS "Enable GTS" ${DEFAULT})
OPTION(ENABLE_GUI "Enable GUI" ${DEFAULT})
OPTION(ENABLE_CGAL "Enable CGAL" ${DEFAULT})
OPTION(ENABLE_GL2PS "Enable GL2PS" ${DEFAULT})
OPTION(ENABLE_LINSOLV "Enable direct solver for the flow engines (experimental)" ${DEFAULT})
OPTION(ENABLE_PFVFLOW "Enable one-phase flow engines" ${DEFAULT})
OPTION(ENABLE_TWOPHASEFLOW "Enable two-phase flow engines" ${DEFAULT})
OPTION(ENABLE_SPH "Enable SPH" OFF)
OPTION(ENABLE_LBMFLOW "Enable LBM engine (very experimental)" ON)
OPTION(ENABLE_LIQMIGRATION "Enable liquid control (very experimental), see [Mani2013] for details" OFF)
OPTION(ENABLE_MASK_ARBITRARY "Enable arbitrary precision of bitmask variables (only Body::groupMask yet implemented) (experimental). Use -DMASK_ARBITRARY_SIZE=int to set number of used bits (256 by default)" OFF)
OPTION(ENABLE_PROFILING "Enable profiling, e.g. shows some more metrics, which can define bottlenecks of the code (OFF by default)" OFF)
OPTION(ENABLE_POTENTIAL_PARTICLES "Enable potential particles" ON)
OPTION(ENABLE_OAR "Generate script for oar-based task scheduler" OFF)
OPTION(ENABLE_MPFR "Use mpfr library (higher precision calculations or CGAL exact predicates)" OFF)
OPTION(USE_QT5 "USE Qt5 for GUI" ON)
OPTION(ENABLE_POTENTIAL_BLOCKS "Enable PotentialBlocks" ON)
OPTION(ENABLE_DEFORM "Enable Deformation Engine" OFF)
OPTION(ENABLE_FEMLIKE "Enable deformable solids" ON)
OPTION(CHOLMOD_GPU "Enable GPU acceleration flow engine direct solver" OFF)
OPTION(ENABLE_THERMAL "Enable thermal engine (experimental)" ON)
OPTION(ENABLE_ASAN "Enable AddressSanitizer build, please see documentation" OFF)
OPTION(ENABLE_PARTIALSAT "Enable the partially saturated clay engine" OFF)
#Set -DPYTHON_VERSION numeric option below:
set(PYTHON_VERSION -1 CACHE STRING "Force python version to the given one. Defaults to -DPYTHON_VERSION=-1 which automatically uses the newest available python version on the system.")
#===========================================================



set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cMake")

INCLUDE(GetVersion)
INCLUDE(GNUInstallDirs)

INCLUDE_DIRECTORIES (${CMAKE_SOURCE_DIR})

#===========================================================
# HACK!!! If the version of gcc is 4.8 or greater, we add -ftrack-macro-expansion=0
# and -save-temps into compiler to reduce the memory consumption during compilation.
# See http://bugs.debian.org/726009 for more information
# Can be removed later, if gcc fixes its regression
# Taken from http://stackoverflow.com/questions/4058565/check-gcc-minor-in-cmake

# this line was trying to get the g++ version. But it does not work with ccache.
# But we already have CMAKE_CXX_COMPILER_VERSION, so we don't need this line!
#EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)

IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")  # Check whether we compile with GCC
  IF (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.8 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.8)
    MESSAGE(STATUS "GCC Version >= 4.8. Adding -ftrack-macro-expansion=0")
    ADD_DEFINITIONS("-ftrack-macro-expansion=0")
  ELSE()
    MESSAGE(STATUS "GCC Version < 4.8. will not use -ftrack-macro-expansion=0")
  ENDIF()
ENDIF()

# -save-temps is not supported with ccache
IF ((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.8 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.8) AND ( NOT "${CMAKE_CXX_COMPILER_LAUNCHER}" MATCHES ".*ccache" AND NOT "${CMAKE_CXX_COMPILER}" MATCHES ".*ccache.*" AND NOT DISABLE_SAVE_TEMPS))
  MESSAGE(STATUS "GCC Version >= 4.8. Adding -save-temps")
  ADD_DEFINITIONS("-save-temps")
ELSE()
  IF ( "${CMAKE_CXX_COMPILER_LAUNCHER}" MATCHES ".*ccache" OR "${CMAKE_CXX_COMPILER}" MATCHES ".*ccache.*" )
    MESSAGE(STATUS "ccache detected, will not use -save-temps")
# on ubuntu 18.04 and 18.10 when ccache is used we need to fix some bug, see https://gitlab.kitware.com/cmake/cmake/issues/17275
# but we can't do this on ubuntu 16.04 with cmake version 3.5.1, because that produces error: "list does not recognize sub-command FILTER".
# So we must be above version 3.6, because https://cmake.org/cmake/help/v3.6/release/3.6.html?highlight=filter
# Later maybe on some higher cmake version this will become unnecessary.
    IF(CMAKE_VERSION VERSION_GREATER 3.6 OR CMAKE_VERSION VERSION_EQUAL 3.6)
        string (STRIP "${CMAKE_CXX_COMPILER_ARG1}" __CXX_COMPILER_ARG1)
        set(CMAKE_CXX_COMPILER_PREDEFINES_COMMAND "${CMAKE_CXX_COMPILER}" "${__CXX_COMPILER_ARG1}" "-dM" "-E" "-c" "${CMAKE_ROOT}/Modules/CMakeCXXCompilerABI.cpp")
        list(FILTER CMAKE_CXX_COMPILER_PREDEFINES_COMMAND EXCLUDE REGEX "^$")
        unset(__CXX_COMPILER_ARG1)
    ENDIF()
  ELSE()
    MESSAGE(STATUS "GCC Version < 4.8 or DISABLE_SAVE_TEMPS. will not use -save-temps")
  ENDIF()
ENDIF()

#===========================================================

FIND_PACKAGE(Eigen3 REQUIRED)

#===========================================================
# AddressSanitizer build
IF(ENABLE_ASAN)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -g -fsanitize=address -fno-omit-frame-pointer")
  SET(CMAKE_BUILD_TYPE Debug)
  SET(CMAKE_VERBOSE_MAKEFILE 1)
ENDIF(ENABLE_ASAN)
#===========================================================

IF(DEBUG)
  SET(CMAKE_VERBOSE_MAKEFILE 1)
  SET(CMAKE_BUILD_TYPE Debug)
  ADD_DEFINITIONS("-DYADE_DEBUG")
ELSE (DEBUG)
  IF (NOT(CMAKE_BUILD_TYPE))
    # If no build_type set, we use Release profile
    SET(CMAKE_BUILD_TYPE Release)
  ENDIF (NOT(CMAKE_BUILD_TYPE))
ENDIF (DEBUG)

IF(ENABLE_LOGGER)
  MESSAGE(STATUS "${Esc}[36mLog filtering: enabled${Esc}[0m")
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} BoostLog")
  ADD_DEFINITIONS("-DYADE_BOOST_LOG")
  ADD_DEFINITIONS("-DBOOST_LOG_DYN_LINK")
  IF (DEBUG)
    MESSAGE(STATUS "Enabling boost::log library and DEBUG=ON. Files will be very large with full debug info inside.")
    IF(${MAX_LOG_LEVEL} LESS 6)
      MESSAGE(STATUS "${Esc}[33mWarning: using DEBUG=ON with MAX_LOG_LEVEL=${MAX_LOG_LEVEL} (less than 6, the maximum log level) rarely makes sense, use cmake option MAX_LOG_LEVEL=6 to fix this.${Esc}[0m")
    ENDIF()
  ELSE (DEBUG)
    MESSAGE(STATUS "Enabling boost::log library and DEBUG=OFF. Logging will work nicely, backtraces will not have debug info, files will be small.")
  ENDIF (DEBUG)
ELSE(ENABLE_LOGGER)
  MESSAGE(STATUS "${Esc}[36mLog filtering: disabled${Esc}[0m")
  IF (DEBUG)
    MESSAGE(STATUS "Disabling boost::log library (only rudimentary LOG_* macros will work) and DEBUG=ON. Files will be very large with full debug info inside.")
  ELSE (DEBUG)
    MESSAGE(STATUS "Disabling boost::log library (only rudimentary LOG_* macros will work, and only up to LOG_WARN) and DEBUG=OFF. Backtraces will not have debug info, files will be small.")
  ENDIF (DEBUG)
ENDIF(ENABLE_LOGGER)

ADD_DEFINITIONS("-DMAX_LOG_LEVEL=${MAX_LOG_LEVEL}")

#=====================================================================
#= Detection of library and architecture versions for libVersions ====
#=====================================================================
INCLUDE(FindMissingVersions)
#==================================
#====== *** PYTHON/BOOST *** ======
#==================================
INCLUDE(YadePythonHelpers)

SET(PY3_VERSIONS 3.9 3.8 3.7 3.6 3.5 3.4 3.3 3.2 3.1 3.0) #append newer python versions at the beginning here.
SET(PY2_VERSIONS 2.7 2.6)

IF (DEFINED PYTHON_EXECUTABLE)
	SET(USER_DEFINED_PYTHON_EX ${PYTHON_EXECUTABLE})
ENDIF()
IF(${PYTHON_VERSION} EQUAL -1)
	SET(PY_VERSIONS_CHECK "${PY3_VERSIONS};${PY2_VERSIONS}")
ELSEIF(${PYTHON_VERSION} EQUAL 2)
	SET(PY_VERSIONS_CHECK "${PY2_VERSIONS}")
ELSEIF(${PYTHON_VERSION} EQUAL 3)
	SET(PY_VERSIONS_CHECK "${PY3_VERSIONS}")
ELSE()
	SET(PY_VERSIONS_CHECK "${PYTHON_VERSION}")
ENDIF()

MESSAGE("Loop on the following python versions and check available dependencies:${PY_VERSIONS_CHECK}" )
FOREACH(py_version ${PY_VERSIONS_CHECK})
	#Set desired python version
	set(Python_ADDITIONAL_VERSIONS ${py_version})
	#Find python
	FIND_PACKAGE(PythonInterp)
	IF(NOT PythonInterp_FOUND)
		MESSAGE(FATAL_ERROR "No python version could be found in your system.")
	ENDIF()
	IF (DEFINED USER_DEFINED_PYTHON_EX)
        	MESSAGE("Using user defined PYTHON_EXECUTABLE: ${USER_DEFINED_PYTHON_EX}")
        	SET(PYTHON_EXECUTABLE ${USER_DEFINED_PYTHON_EX} CACHE FILEPATH "user defined python executable" FORCE)
	ENDIF()

	#FindPythonInterp will try to fallback in older Python version, we have to check manually if it found the version we wish
	PYTHON_VERSION_MATCHES(${py_version})
	IF(NOT ${PYTHON_VERSION_MATCH})
	    #Reset variables to be able to use finpythoninterp multiple times
		unset(PYTHON_EXECUTABLE CACHE)
		unset(PYTHONINTERP_FOUND CACHE)
		CONTINUE()
	ENDIF()
	MESSAGE("Python version ${PYTHON_VERSION_STRING} found, try to import dependencies...")
	FIND_PYTHON_PACKAGES()
	IF(ALL_PYTHON_DEPENDENCIES_FOUND)
		MESSAGE("Found all python dependencies with version ${PYTHON_VERSION_STRING}, will compile yade with that.")
		BREAK()
	ELSE()
	    #Reset variables to be able to use finpythoninterp multiple times
		unset(PYTHON_EXECUTABLE CACHE)
		unset(PYTHONINTERP_FOUND CACHE)
	ENDIF()
ENDFOREACH()

#After the loop on all python versions, having ALL_PYTHON_DEPENDENCIES_FOUND==FALSE must trigger an error:
IF(NOT ALL_PYTHON_DEPENDENCIES_FOUND)
	MESSAGE(FATAL_ERROR "No python version with all dependencies was found.")
ENDIF()

#======================================
#====== *** END PYTHON/BOOST *** ======
#======================================

#======================================
#====== **** Compiler flags **** ======
#======================================
if(ENABLE_USEFUL_ERRORS)
  # Enable as many warnings as possible, and treat them as errors (using -Werror flag).
  # Four warnings are impossible to fix, because they are in external libraries:
  #   (1) libeigen: -Wmaybe-uninitialized, -Wcomment, -Wdeprecated-copy ; and only in g++ 7 or 8: -Wint-in-bool-context
  #   (2) open mpi: -Wcast-function-type
  # Three warnings are impossible to fix, because they are in older compiler, or in older library version
  #   (1) libeigen ver. <=3.3.4 : -Wshadow=compatible-local -Wno-error=float-conversion
  #   (2) numpy    ver. < 1.13  : -Wunused-function
  # These impossible to fix warnings are in ${WORKAROUND_LIBRARY_WARNINGS}
  # So depending on compiler version I need to turn these errors off.
  # They will be still printed as warnings. But they will not be an error due to -Werror flag.

  # Notes:
  #  -Wfloat-conversion warning: you have to select the int(…) conversion policy: std::floor(…), std::ceil(…), std::round(…), e.g. with int(std::ceil(11.9999999999999993))

  # g++ ver < 11 ; all workarounds are for g++ < ver.11; When version 11 appears I hope that there will be fewer warnigs to disable. This 'IF' is to trigger the re-evealuation when g++ v.11 appears.
  IF (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0 )
    SET(WORKAROUND_LIBRARY_WARNINGS " -Wno-error=maybe-uninitialized -Wno-comment")
    # g++ ver >= 8
    IF (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 8.0)
      SET(WORKAROUND_LIBRARY_WARNINGS "${WORKAROUND_LIBRARY_WARNINGS} -Wno-cast-function-type")
      # g++ ver >= 9
      IF (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 9.0)
        SET(WORKAROUND_LIBRARY_WARNINGS "${WORKAROUND_LIBRARY_WARNINGS} -Wno-deprecated-copy")
      ENDIF()
    ENDIF()
    # g++ ver >= 7
    IF (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 7.0)
      # g++ ver < 8
      IF (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
        SET(WORKAROUND_LIBRARY_WARNINGS "${WORKAROUND_LIBRARY_WARNINGS} -Wno-int-in-bool-context")
      ENDIF()
      # g++ ver >= 7 and eigen > 3.3.4
      IF(NOT ((${EIGEN3_MAJOR_VERSION} EQUAL 3) AND ( (${EIGEN3_MINOR_VERSION} LESS 4) OR (${EIGEN3_MINOR_VERSION} EQUAL 4))))
        # eigen > 3.3.4, (newer than ubuntu 16.04 xenial or 18.04 bionic)
        SET(WORKAROUND_LIBRARY_WARNINGS "${WORKAROUND_LIBRARY_WARNINGS} -Wshadow=compatible-local")
      ENDIF()
    ENDIF()
  ENDIF()

  IF((${EIGEN3_MAJOR_VERSION} LESS 2) OR (${EIGEN3_MAJOR_VERSION} EQUAL 2))
    # eigen <= 3.2, ubuntu 16.04 xenial
    SET(WORKAROUND_LIBRARY_WARNINGS "${WORKAROUND_LIBRARY_WARNINGS} -Wno-error=float-conversion")
  ENDIF()
  IF((${NUMPY_VERSION_MAJOR} EQUAL 1) AND (${NUMPY_VERSION_MINOR} LESS 13))
    # numpy 1.11.0 has unused-function, 1.13 does not. So we have to print warning only when it appears.
    SET(WORKAROUND_LIBRARY_WARNINGS "${WORKAROUND_LIBRARY_WARNINGS} -Wno-unused-function")
  ENDIF()
  SET(ALL_ENABLED_USEFUL_ERRORS " -Werror -Wformat -Wformat-security -Wformat-nonliteral -Wall -Wextra -Wnarrowing -Wreturn-type -Wuninitialized -Wfloat-conversion -Wcast-align -Wdisabled-optimization -Wtrampolines -Wpointer-arith -Wswitch-bool -Wwrite-strings -Wnon-virtual-dtor -Wreturn-local-addr")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ALL_ENABLED_USEFUL_ERRORS} ${WORKAROUND_LIBRARY_WARNINGS} -Wno-error=cpp -fdce -fstack-protector-strong")

#  -Wcast-qual      is useful, but too many warnings in libeigen
#  -Wlogical-op     (1 unfixable warning in /usr/include/vtk-6.3/vtkVariantInlineOperators.h: logical ‘or’ of equal expressions )
#  -Wswitch-default (1 unfixable warning in /usr/include/coin/CoinHelperFunctions.hpp )
#  -Wswitch-enum    is not useful.
# Also see: https://stackoverflow.com/questions/5088460/flags-to-enable-thorough-and-verbose-g-warnings
# Below is the list of all still not fixed warnings `| sort | uniq`, maybe some of them are not much useful:
# -Weffc++              (4465) -Wsign-conversion  (4176) -Wpedantic    (1744) -Wconversion         ( 755) -Wshadow         ( 527)
# -Wfloat-equal         ( 486) -Wvla              ( 119) -Wswitch-enum (  36) -Wcast-qual          (  34) -Wswitch-default (  16)
# -Wmaybe-uninitialized (  10) -Wcomment          (   3) -Wcpp         (   2) -Wcast-function-type (   2) -Wlogical-op     (   1)

ELSE(ENABLE_USEFUL_ERRORS)
  SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wformat -Wformat-security -Werror=format-security -Wall -fstack-protector-strong")
ENDIF(ENABLE_USEFUL_ERRORS)
#===========================================================
MESSAGE(STATUS   "${Esc}[36mChosen compiler flags are: CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS} ${Esc}[0m")
#======================================
#====== ** END Compiler flags ** ======
#======================================

#======================================
#====== **** Real precision **** ======
#======================================
#
# This section accepts either one of these two input parameters:
#  * REAL_PRECISION_BITS
#  * REAL_DECIMAL_PLACES
# Additionally:
#  * ENABLE_MPFR - optional: if more precision than float128 is required then either MPFR or boost::multiprecision::cpp_bin_float type is used.
#
# A hardware-accelerated numerical type is used when available, otherwise mpfr library (or boost cpp_bin_float.hpp) is used for arbitrary precision. The supported types are following:
#     type                   bits     decimal places
#     float,float32_t        32       6                      hardware accelerated
#     double,float64_t       64       15                     hardware accelerated
#     long double,float80_t  80       18                     hardware accelerated
#     float128_t             128      33                     depending on processor type it can be hardware accelerated
#     mpfr                   Nbit     Nbit/(log(2)/log(10))
#     boost::cpp_bin_float   Nbit     Nbit/(log(2)/log(10))
#
# This section does following things:
#  * set REAL_PRECISION_BITS as one of: 32,64,80,128 or higher
#  * set REAL_DECIMAL_PLACES as one of:  6,15,18, 33 or higher
#  * set REAL_USE_ARBITRARY  as one of: --- "OFF" -- or "MPFR" or "BOOST_BIN_FLOAT"
#  * set REAL_USE_MPMATH     as one of: -OFF-,------ON--------
#  * set REAL_PREC_NAME      as one of: "PrecisionFloat" "PrecisionDouble", "PrecisionLongDouble", "PrecisionFloat128", "PrecisionMpfr" or "PrecisionBoostBinFloat"
#  * set REAL_MULTI_HP       is set to "ON" when the compiler supports it, otherwise to "OFF"
#  * set CONFIGURED_FEATS    ${REAL_PREC_NAME} ${REAL_USE_MPMATH}
#  * #define YADE_REAL_BIT  REAL_PRECISION_BITS
#  * #define YADE_REAL_DEC  REAL_DECIMAL_PLACES
#  * #define YADE_REAL_MPFR    // defined when Real is expressed using mpfr (requires mpfr)
#  * #define YADE_REAL_BBFLOAT // defined when Real is expressed using boost::multiprecision::cpp_bin_float
#  * #define YADE_MPFR         // defined when yade links with optional mpfr library, for example to use with CGAL exact predicates. Unrelated about whether Real uses arbitrary precision.
#
# Some useful links:
# https://www.boost.org/doc/libs/1_71_0/libs/multiprecision/doc/html/boost_multiprecision/tut/floats/float128.html
# https://www.boost.org/doc/libs/1_71_0/libs/multiprecision/doc/html/boost_multiprecision/intro.html
# https://www.boost.org/doc/libs/1_71_0/libs/multiprecision/doc/html/boost_multiprecision/tut/floats/mpfr_float.html
# http://charette.no-ip.com:81/programming/doxygen/boost/classstd_1_1numeric__limits_3_01boost_1_1multiprecision_1_1number_3_01boost_1_1multiprecision_1_1aef0fd61e32c5f58e49aeed0766e135.html
# http://christian-seiler.de/projekte/fpmath/

MESSAGE(STATUS "Determining preferred precision of Real numbers")

SET(REAL_USE_ARBITRARY "OFF") # this line later might need some extra IF() if someone will want to use CGAL exact predicates, see lib/base/AliasCGAL.hpp 'CGAL definitions - does not work with another kernel'

IF( (    REAL_DECIMAL_PLACES) AND (    REAL_PRECISION_BITS))
  MESSAGE(FATAL_ERROR "Cannot specify both REAL_DECIMAL_PLACES and REAL_PRECISION_BITS")
ENDIF()

IF( (NOT REAL_DECIMAL_PLACES) AND (NOT REAL_PRECISION_BITS))
  SET(REAL_PRECISION_BITS "64")
  SET(REAL_DECIMAL_PLACES "15")
  SET(REAL_PREC_NAME "PrecisionDouble")
ENDIF()

function(floatexprToInt expr output)
    # If "%f" was instead of "%i" in this place  ↓↓ then it would return a floating point value
    execute_process(COMMAND awk "BEGIN {printf \"%i\",${expr}}" OUTPUT_VARIABLE __output)
    set(${output} ${__output} PARENT_SCOPE)
endfunction()

IF( (    REAL_DECIMAL_PLACES) AND (NOT REAL_PRECISION_BITS))
  IF(${REAL_DECIMAL_PLACES} LESS 7)
    SET(REAL_PRECISION_BITS "32")
    SET(REAL_DECIMAL_PLACES "6")
    SET(REAL_PREC_NAME "PrecisionFloat")
  ELSEIF(${REAL_DECIMAL_PLACES} LESS 16)
    SET(REAL_PRECISION_BITS "64")
    SET(REAL_DECIMAL_PLACES "15")
    SET(REAL_PREC_NAME "PrecisionDouble")
  ELSEIF(${REAL_DECIMAL_PLACES} LESS 19)
    SET(REAL_PRECISION_BITS "80")
    SET(REAL_DECIMAL_PLACES "18")
    SET(REAL_PREC_NAME "PrecisionLongDouble")
    SET(REAL_USE_MPMATH "ON")
  ELSEIF(${REAL_DECIMAL_PLACES} LESS 34)
    SET(REAL_PRECISION_BITS "128")
    SET(REAL_DECIMAL_PLACES "33")
    SET(REAL_PREC_NAME "PrecisionFloat128")
    SET(REAL_USE_MPMATH "ON")
  ELSEIF(${REAL_DECIMAL_PLACES} LESS 39)
    MESSAGE(STATUS "${Esc}[36mNote: decimal places between 34 and 39 make it unclear if you want mpfr or float128, defaulting to float128. It is because some bits are used by the exponent.${Esc}[0m")
    SET(REAL_PRECISION_BITS "128")
    SET(REAL_DECIMAL_PLACES "33")
    SET(REAL_PREC_NAME "PrecisionFloat128")
    SET(REAL_USE_MPMATH "ON")
  ELSE()
    # calculate Nbits as Ndecimal/((log(2)/log(10)))
    floatexprToInt("(${REAL_DECIMAL_PLACES}/0.30102999566398119521)" REAL_PRECISION_BITS)
    #MATH(EXPR REAL_PRECISION_BITS "(${REAL_DECIMAL_PLACES}/0.30102999566398119521)") # ← doesn't work with floats
    SET(REAL_USE_ARBITRARY "ON") # we know it has to be arbitrary, but we don't know yet if mpfr is available
    SET(REAL_USE_MPMATH "ON")
  ENDIF()
ENDIF()

IF( (NOT REAL_DECIMAL_PLACES) AND (    REAL_PRECISION_BITS))
  IF(${REAL_PRECISION_BITS} LESS 33)
    SET(REAL_PRECISION_BITS "32")
    SET(REAL_DECIMAL_PLACES "6")
    SET(REAL_PREC_NAME "PrecisionFloat")
  ELSEIF(${REAL_PRECISION_BITS} LESS 65)
    SET(REAL_PRECISION_BITS "64")
    SET(REAL_DECIMAL_PLACES "15")
    SET(REAL_PREC_NAME "PrecisionDouble")
  ELSEIF(${REAL_PRECISION_BITS} LESS 81)
    SET(REAL_PRECISION_BITS "80")
    SET(REAL_DECIMAL_PLACES "18")
    SET(REAL_PREC_NAME "PrecisionLongDouble")
    SET(REAL_USE_MPMATH "ON")
  ELSEIF(${REAL_PRECISION_BITS} LESS 129)
    SET(REAL_PRECISION_BITS "128")
    SET(REAL_DECIMAL_PLACES "33")
    SET(REAL_PREC_NAME "PrecisionFloat128")
    SET(REAL_USE_MPMATH "ON")
  ELSE()
    # calculate Ndecimal as Nbits*((log(2)/log(10)))
    floatexprToInt("(${REAL_PRECISION_BITS}*0.30102999566398119521)" REAL_DECIMAL_PLACES)
    SET(REAL_USE_ARBITRARY "ON") # we know it has to be arbitrary, but we don't know yet if mpfr is available
    SET(REAL_USE_MPMATH "ON")
  ENDIF()
ENDIF()

IF(REAL_USE_ARBITRARY)
  IF(NOT ENABLE_MPFR)
    SET(REAL_PREC_NAME "PrecisionBoostBinFloat${REAL_DECIMAL_PLACES}")
    SET(REAL_USE_ARBITRARY "BOOST_BIN_FLOAT")
    ADD_DEFINITIONS("-DYADE_REAL_BBFLOAT")
    MESSAGE(STATUS "To use faster higher precision use -DENABLE_MPFR=ON cmake option. Currently a simple type boost::multiprecision::cpp_bin_float will be used.")
  ELSE()
    SET(REAL_PREC_NAME "PrecisionMpfr${REAL_DECIMAL_PLACES}")
    SET(REAL_USE_ARBITRARY "MPFR")
    ADD_DEFINITIONS("-DYADE_REAL_MPFR")
    MESSAGE(STATUS "Real type will use mpfr precision.")
  ENDIF()
ENDIF()

# Detect if RealHP<N> is supported. Allow option to override this for testing.
FIND_PACKAGE(Mpmath)
IF((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 9.2.1) OR ((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.3.0) AND (ENABLE_MPFR) AND (REAL_PRECISION_BITS GREATER 128)) OR (ALLOW_CRASHES))
 IF(Boost_MAJOR_VERSION EQUAL 1 AND Boost_MINOR_VERSION LESS 67)
    SET(REAL_MULTI_HP "OFF")
    MESSAGE(STATUS "${Esc}[31mDisabling RealHP<N> because boost version < 1.67 is too old.${Esc}[0m")
 ELSE()
  IF(MPMATH_FOUND)
    SET(REAL_MULTI_HP "ON")
    IF(NOT ENABLE_MPFR)
      IF(Boost_MAJOR_VERSION EQUAL 1 AND Boost_MINOR_VERSION LESS 71 AND REAL_PRECISION_BITS EQUAL 80)
        MESSAGE(STATUS "${Esc}[31mCannot enable RealHP<N> with 'long double' type because cpp_bin_float in boost version < 1.71 has interoperability problems, please use cmake option ENABLE_MPFR=ON to fix this or upgrade boost library.${Esc}[0m")
        SET(REAL_MULTI_HP "OFF")
      ELSE()
        MESSAGE(STATUS "${Esc}[31mWarning: RealHP<N> higher precision types will use slow boost cpp_bin_float, consider passing cmake option ENABLE_MPFR=ON to fix this.${Esc}[0m")
      ENDIF()
    ENDIF()
  ELSE()
    SET(REAL_MULTI_HP "OFF")
    MESSAGE(STATUS "${Esc}[31mDisabling RealHP<N>, it can work on this system, but python3-mpmath package is missing.${Esc}[0m")
  ENDIF()
 ENDIF()
ELSE()
  SET(REAL_MULTI_HP "OFF")
  MESSAGE(STATUS "${Esc}[31mDisabling RealHP<N>, this compiler is too old to make this work.${Esc}[0m")
ENDIF()
IF(REAL_MULTI_HP)
  SET(REAL_USE_MPMATH "ON")
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} RealHP")
  MESSAGE(STATUS "${Esc}[32mEnabling RealHP<N>, double, quadruple and more precisions are available: RealHP<N> for N ∈ {1,2,3,4,8,10,20}. See file lib/high-precision/RealHPConfig.hpp${Esc}[0m")
ELSE()
  ADD_DEFINITIONS("-DYADE_DISABLE_REAL_MULTI_HP")
  SET(DISABLED_FEATS "${DISABLED_FEATS} RealHP")
ENDIF()

# if REAL_MULTI_HP is enabled then REAL_USE_MPMATH must be ON
IF(REAL_USE_MPMATH)
  IF(MPMATH_FOUND)
    MESSAGE(STATUS "python-mpmath found, version: ${MPMATH_VERSION}")
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} mpmath")
    SET(MPMATH_IS_USED "True")
  ELSE()
    MESSAGE(FATAL_ERROR "Cannot find python-mpmath or python3-mpmath package http://mpmath.org to use high precision in python, but it is necessary for selected Real precision.")
  ENDIF()
ELSE()
  SET(DISABLED_FEATS "${DISABLED_FEATS} mpmath")
  SET(MPMATH_IS_USED "False")
ENDIF()

IF(NOT ((${REAL_PRECISION_BITS} EQUAL 64)))
# This means that new high-precision capabilities will be used. To make this work we need a patched version of /usr/include/miniegen
  MESSAGE(STATUS "Checking compiler version for ${REAL_PREC_NAME}")
  IF(NOT ALLOW_CRASHES)
    IF(REAL_USE_ARBITRARY AND ENABLE_MPFR)
      IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.3.0 )
        MESSAGE(FATAL_ERROR "MPFR works since g++ version around 8.3.0. For possible supported platforms see https://gitlab.com/cosurgi/minieigen-real/pipelines/104924921 you can use -DALLOW_CRASHES=ON to override.")
      ENDIF()
    ELSE()
      IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.2.1 )
        MESSAGE(FATAL_ERROR "High precision '${REAL_PREC_NAME}' works since g++ version around 9.2.1. For possible supported platforms see https://gitlab.com/cosurgi/minieigen-real/pipelines/104924921 you can use -DALLOW_CRASHES=ON to override.")
      ENDIF()
    ENDIF()
  ELSE()
    MESSAGE(STATUS "${Esc}[31mSkipping this check, ALLOW_CRASHES=ON${Esc}[0m")
  ENDIF()
ENDIF()

ADD_DEFINITIONS("-DYADE_REAL_BIT=${REAL_PRECISION_BITS}")
ADD_DEFINITIONS("-DYADE_REAL_DEC=${REAL_DECIMAL_PLACES}")

SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} ${REAL_PREC_NAME}")

IF(ENABLE_MPFR)
  FIND_PACKAGE(MPFR REQUIRED)
  FIND_PACKAGE(MPFRCPP REQUIRED)
  IF(MPFR_FOUND AND MPFRCPP_FOUND)
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} MPFR")
    MESSAGE(STATUS "MPFR enabled, version: ${MPFR_VERSION}, linking with ${MPFR_LIBRARIES}")
    ADD_DEFINITIONS("-DYADE_MPFR")
  ELSE()
    MESSAGE(FATAL_ERROR "Cannot find MPFR library https://www.mpfr.org, two debian packages libmpfr-dev libmpfrc++-dev")
  ENDIF()
ELSE()
  SET(DISABLED_FEATS "${DISABLED_FEATS} MPFR")
  MESSAGE(STATUS "mpfr disabled")
ENDIF()

IF(REAL_USE_ARBITRARY EQUAL "ON")
  MESSAGE(FATAL_ERROR " there's an error in CMakeLists.txt, REAL_USE_ARBITRARY == ${REAL_USE_ARBITRARY}, but should be OFF,MPFR,BOOST_BIN_FLOAT")
ENDIF()

MESSAGE(STATUS "Will use ${Esc}[93m${REAL_PREC_NAME}${Esc}[0m as Real type with ${Esc}[36m${REAL_DECIMAL_PLACES}${Esc}[0m decimal places precision and use ${Esc}[36m${REAL_PRECISION_BITS}${Esc}[0m bits of storage.")
#======================================
#====== * Unsupported features * ======
#======================================
# non-double Real precision does not work with extarnal library cholmod.
IF(NOT (${REAL_PRECISION_BITS} EQUAL 64))
  IF(ENABLE_TWOPHASEFLOW OR ENABLE_LINSOLV OR ENABLE_PFVFLOW)
     MESSAGE(STATUS "ENABLE_TWOPHASEFLOW OR ENABLE_LINSOLV OR ENABLE_PFVFLOW were disabled automatically because they use double precision external library: cholesky linear solver. It can be replaced with native Eigen solvers, then it will work.")
     SET(ENABLE_TWOPHASEFLOW OFF)
     SET(ENABLE_LINSOLV OFF)
     SET(ENABLE_PFVFLOW OFF)
  ENDIF(ENABLE_TWOPHASEFLOW OR ENABLE_LINSOLV OR ENABLE_PFVFLOW)
  IF(ENABLE_TWOPHASEFLOW OR ENABLE_LINSOLV OR ENABLE_PFVFLOW)
    MESSAGE(FATAL_ERROR "Cannot (yet) use ENABLE_TWOPHASEFLOW OR ENABLE_LINSOLV OR ENABLE_PFVFLOW with high precision.")
  ENDIF(ENABLE_TWOPHASEFLOW OR ENABLE_LINSOLV OR ENABLE_PFVFLOW)
  IF(ENABLE_POTENTIAL_BLOCKS)
     MESSAGE(STATUS "ENABLE_POTENTIAL_BLOCKS was disabled because external library coinor can only work with double type.")
     SET(ENABLE_POTENTIAL_BLOCKS OFF)
  ENDIF(ENABLE_POTENTIAL_BLOCKS)
  IF(ENABLE_POTENTIAL_BLOCKS)
    MESSAGE(FATAL_ERROR "Cannot (yet) use ENABLE_POTENTIAL_BLOCKS with high precision.")
  ENDIF(ENABLE_POTENTIAL_BLOCKS)
  IF(ENABLE_MPI)
     MESSAGE(STATUS "ENABLE_MPI was disabled because it hasn't been tested. It is possible that a pure python-only library will correctly send mpmath.mpf(…) high precision Real type over network, but this was not tested.")
     SET(ENABLE_MPI OFF)
  ENDIF(ENABLE_MPI)
  IF(ENABLE_MPI)
    MESSAGE(FATAL_ERROR "Cannot (yet) use ENABLE_MPI with high precision.")
  ENDIF(ENABLE_MPI)
ENDIF(NOT (${REAL_PRECISION_BITS} EQUAL 64))

# This is for testing compilation with Real as float. Only useful to have wider testing coverage, not for real world use.
IF(${REAL_PRECISION_BITS} EQUAL 32)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=float-conversion -Wno-error=narrowing")
ENDIF()

#======================================
#====== ** END Real precision ** ======
#======================================

#===========================================================

FIND_PACKAGE(BZip2 REQUIRED)
FIND_PACKAGE(ZLIB REQUIRED)
#===========================================================

# Use Eigen3 by default
IF (EIGEN3_FOUND)
  INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})
  MESSAGE(STATUS "Found Eigen3, version: ${EIGEN3_VERSION}")

  # Minimal required version 3.2.1
  IF ((${EIGEN3_MAJOR_VERSION} LESS 2) OR ((${EIGEN3_MAJOR_VERSION} EQUAL 2) AND (${EIGEN3_MINOR_VERSION} LESS 1)))
    MESSAGE(FATAL_ERROR "Minimal required Eigen3 version is 3.2.1, please update Eigen3!")
  ENDIF ((${EIGEN3_MAJOR_VERSION} LESS 2) OR ((${EIGEN3_MAJOR_VERSION} EQUAL 2) AND (${EIGEN3_MINOR_VERSION} LESS 1)))

  IF (NOT VECTORIZE)
    MESSAGE(STATUS "Disable vectorization")
    ADD_DEFINITIONS("-DEIGEN_DONT_VECTORIZE -DEIGEN_DONT_ALIGN -DEIGEN_DISABLE_UNALIGNED_ARRAY_ASSERT")
  ELSE (NOT VECTORIZE)
    IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.2.1 )
      MESSAGE(FATAL_ERROR "SSE vectorization works since g++ version around 9.2.1.")
    ENDIF()
    MESSAGE(STATUS "Enable vectorization")
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} VECTORIZE")
  ENDIF (NOT VECTORIZE)

ENDIF(EIGEN3_FOUND)
#===========================================================
INCLUDE_DIRECTORIES(${BZIP2_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS})
SET(LINKLIBS  "${LINKLIBS};${BZIP2_LIBRARIES};${ZLIB_LIBRARIES};")
#===========================================================
IF((Boost_MAJOR_VERSION EQUAL 1) OR (Boost_MAJOR_VERSION GREATER 1) AND
  ((Boost_MINOR_VERSION EQUAL 53) OR (Boost_MINOR_VERSION GREATER 53)))
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} Odeint")
  ADD_DEFINITIONS("-DYADE_ODEINT")
ELSE((Boost_MAJOR_VERSION EQUAL 1) OR (Boost_MAJOR_VERSION GREATER 1) AND
    ((Boost_MINOR_VERSION EQUAL 53) OR (Boost_MINOR_VERSION GREATER 53)))
  SET(DISABLED_FEATS "${DISABLED_FEATS} Odeint")
  MESSAGE(STATUS "Boost Odeint can be enabled, only if Boost>=1.53 is used")
ENDIF((Boost_MAJOR_VERSION EQUAL 1) OR (Boost_MAJOR_VERSION GREATER 1) AND
     ((Boost_MINOR_VERSION EQUAL 53) OR (Boost_MINOR_VERSION GREATER 53)))
#===========================================================
IF(ENABLE_VTK)
  FIND_PACKAGE(VTK COMPONENTS vtkIOParallelXML vtkCommonCore vtkIOImage vtkIOXML vtkFiltersCore vtkImagingCore vtkRenderingCore vtkImagingGeneral vtkImagingHybrid)
  IF(VTK_FOUND)
    INCLUDE_DIRECTORIES(${VTK_INCLUDE_DIRS})
    LINK_DIRECTORIES( ${VTK_LIBRARY_DIRS} )
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DYADE_VTK")
    MESSAGE(STATUS "Found VTK")
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} VTK")
  ELSE(VTK_FOUND)
    MESSAGE(STATUS "VTK NOT found")
    SET(ENABLE_VTK OFF)
    SET(DISABLED_FEATS "${DISABLED_FEATS} VTK")
  ENDIF(VTK_FOUND)
ELSE(ENABLE_VTK)
  SET(DISABLED_FEATS "${DISABLED_FEATS} VTK")
ENDIF(ENABLE_VTK)
#===========================================================
IF(ENABLE_OPENMP)
  FIND_PACKAGE(OpenMP)
  IF(OPENMP_FOUND)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DYADE_OPENMP ${OpenMP_CXX_FLAGS}")
    MESSAGE(STATUS "Found OpenMP")
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} OpenMP")
  ELSE(OPENMP_FOUND)
    MESSAGE(STATUS "OpenMP NOT found")
    SET(ENABLE_OPENMP OFF)
    SET(DISABLED_FEATS "${DISABLED_FEATS} OPENMP")
  ENDIF(OPENMP_FOUND)
ELSE(ENABLE_OPENMP)
  SET(DISABLED_FEATS "${DISABLED_FEATS} OPENMP")
ENDIF(ENABLE_OPENMP)
#===========================================================
IF(ENABLE_GTS)
  FIND_PACKAGE(GTS)
  FIND_PACKAGE(glib2)
  IF(GTS_FOUND AND GLIB2_FOUND)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DYADE_GTS ${CMAKE_GTS_CXX_FLAGS}")
    SET(CMAKE_LD_FLAGS  "${CMAKE_LD_FLAGS} ${GTS_LIBRARIES}")
    INCLUDE_DIRECTORIES(${GTS_INCLUDE_DIR})
    INCLUDE_DIRECTORIES(${GLIB2_INCLUDE_DIRS})
    MESSAGE(STATUS "Found GTS")
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} GTS")
  ELSE(GTS_FOUND AND GLIB2_FOUND)
    MESSAGE(STATUS "GTS NOT found")
    SET(DISABLED_FEATS "${DISABLED_FEATS} GTS")
    SET(ENABLE_GTS OFF)
  ENDIF(GTS_FOUND AND GLIB2_FOUND)
ELSE(ENABLE_GTS)
  SET(DISABLED_FEATS "${DISABLED_FEATS} GTS")
ENDIF(ENABLE_GTS)
#===========================================================
IF(ENABLE_GUI)
  FIND_PACKAGE(OpenGL)
  FIND_PACKAGE(GLUT)
  FIND_PACKAGE(glib2)

  IF(USE_QT5)
    MESSAGE(STATUS "USE QT5")
    FIND_PACKAGE(QGLVIEWER-qt5 REQUIRED)
    FIND_PACKAGE(Qt5 CONFIG REQUIRED Widgets Xml OpenGL)
    FIND_PACKAGE(Qt5Widgets)
    IF(Qt5Widgets_FOUND AND OPENGL_FOUND AND GLUT_FOUND AND GLIB2_FOUND AND QGLVIEWER_FOUND)
      SET(GUI_LIBS ${GLUT_LIBRARY} ${QGLVIEWER_LIBRARIES})
      SET(GUI_SRC_LIB "lib/opengl/GLUtils.cpp")
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DYADE_OPENGL -DYADE_QT5")
      SET(Used_QT_VERSION_MAJOR ${QT5_VERSION_MAJOR})
      SET(Used_QT_VERSION_MINOR ${QT5_VERSION_MINOR})
      SET(Used_QT_VERSION_PATCH ${QT5_VERSION_PATCH})
      MESSAGE(STATUS "Found GUI-Qt5-LIBS")
      SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} GUI-Qt5")
    ELSE(Qt5Widgets_FOUND AND OPENGL_FOUND AND GLUT_FOUND AND GLIB2_FOUND AND QGLVIEWER_FOUND)
      MESSAGE(STATUS "GUI-Qt5-LIBS NOT found")
      SET(DISABLED_FEATS "${DISABLED_FEATS} GUI-Qt5")
      SET(ENABLE_GUI OFF)
    ENDIF(Qt5Widgets_FOUND AND OPENGL_FOUND AND GLUT_FOUND AND GLIB2_FOUND AND QGLVIEWER_FOUND)
  ELSE(USE_QT5)   # Use Qt4
    MESSAGE(FATAL_ERROR "QT4 is no longer supported")
  ENDIF(USE_QT5)
ELSE(ENABLE_GUI)
  SET(DISABLED_FEATS "${DISABLED_FEATS} GUI")
ENDIF(ENABLE_GUI)
#===========================================================
# This one will cry on attempts to build CGAL-based packages without cgal
IF(NOT ENABLE_CGAL AND (ENABLE_PFVFLOW OR ENABLE_TWOPHASEFLOW OR ENABLE_THERMAL))
  MESSAGE(STATUS "PFVFLOW, TWOPHASEFLOW and THERMAL depends on CGAL, but CGAL is disabled (not found or turned OFF)")
ENDIF(NOT ENABLE_CGAL AND (ENABLE_PFVFLOW OR ENABLE_TWOPHASEFLOW OR ENABLE_THERMAL))
#===========================================================
IF(ENABLE_CGAL)
  INCLUDE(FindGMP)
  FIND_PACKAGE(CGAL)
  FIND_PACKAGE(GMP)
  IF(CGAL_FOUND AND GMP_FOUND AND (NOT("${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1}" MATCHES ".*clang"))) #Check for clang should be removed, when CGAL will be compilable by clang

    FILE(GLOB CGAL_SRC_LIB "lib/triangulation/*.cpp")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CGAL_DEFINITIONS} -DYADE_CGAL")
    IF(NOT (${CGAL_LIBRARIES} STREQUAL "CGAL_LIBRARIES-NOTFOUND"))
      SET(LINKLIBS  "${LINKLIBS};${CGAL_LIBRARIES};${GMP_LIBRARIES};${GMPXX_LIBRARIES};")
    ELSE()
      SET(LINKLIBS  "${LINKLIBS};${GMP_LIBRARIES};${GMPXX_LIBRARIES};")
    ENDIF()
    MESSAGE(STATUS "Found CGAL")
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} CGAL")

    ADD_DEFINITIONS("-DCGAL_DISABLE_ROUNDING_MATH_CHECK -frounding-math")

    IF(ENABLE_PFVFLOW)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFLOW_ENGINE")
      SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} PFVFLOW")
    ELSE(ENABLE_PFVFLOW)
      SET(DISABLED_FEATS "${DISABLED_FEATS} PFVFLOW")
    ENDIF(ENABLE_PFVFLOW)

  ELSE(CGAL_FOUND AND GMP_FOUND AND (NOT("${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1}" MATCHES ".*clang")))
    MESSAGE(STATUS "CGAL NOT found")
    SET(DISABLED_FEATS "${DISABLED_FEATS} CGAL")
    SET(ENABLE_CGAL OFF)
    IF(ENABLE_PFVFLOW)
      SET(DISABLED_FEATS "${DISABLED_FEATS} PFVFLOW TWOPHASEFLOW")
      MESSAGE(STATUS "CGAL NOT found: PFVFLOW and TWOPHASEFLOW disabled")
    ENDIF(ENABLE_PFVFLOW)

  ENDIF(CGAL_FOUND AND GMP_FOUND AND (NOT("${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1}" MATCHES ".*clang")))
ELSE(ENABLE_CGAL)
  SET(DISABLED_FEATS "${DISABLED_FEATS} CGAL")
ENDIF(ENABLE_CGAL)

#===========================================================
IF(ENABLE_PFVFLOW)
	IF(ENABLE_CGAL)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFLOW_ENGINE")
		SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} PFVFLOW")
	ELSE(ENABLE_CGAL)
		SET(DISABLED_FEATS "${DISABLED_FEATS} PFVFLOW")
		SET(ENABLE_PFVFLOW OFF)
	ENDIF(ENABLE_CGAL)
ELSE(ENABLE_PFVFLOW)
	SET(DISABLED_FEATS "${DISABLED_FEATS} PFVFLOW")
ENDIF(ENABLE_PFVFLOW)

#===========================================================
IF(ENABLE_PFVFLOW AND ENABLE_LINSOLV)
  IF(DEFINED SUITESPARSEPATH) 
    set(SUITESPARSE_PREFIX_PATH ${SUITESPARSEPATH})
    MESSAGE(STATUS "Using custom suitsparse path " ${SUITESPARSE_PREFIX_PATH})
  ELSE(DEFINED SUITESPARSEPATH)
    set(SUITESPARSE_PREFIX_PATH /usr)
    MESSAGE(STATUS "Using typical suitesparse path " ${SUITESPARSE_PREFIX_PATH})
  ENDIF(DEFINED SUITESPARSEPATH)
  FIND_PACKAGE(Cholmod)
  FIND_PACKAGE(OpenBlas)
  FIND_PACKAGE(Metis)

  IF(CHOLMOD_FOUND AND OPENBLAS_FOUND AND METIS_FOUND AND CGAL_FOUND)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CGAL_DEFINITIONS} -DLINSOLV")

    SET(LINKLIBS  "${LINKLIBS};${CHOLMOD_LIBRARIES};${AMD_LIBRARY};${CAMD_LIBRARY};${COLAMD_LIBRARY};${CCOLAMD_LIBRARY};${OPENBLAS_LIBRARY};${METIS_LIBRARY};${SUITESPARSE_LIBRARY};${SUITESPARSECONFIG_LIBRARY}")
    INCLUDE_DIRECTORIES(${METIS_INCLUDE_DIR} ${CHOLMOD_INCLUDE_DIR} ${BLAS_INCLUDE_DIR})
    MESSAGE(STATUS "Found Cholmod in " ${CHOLMOD_LIBRARIES})
    MESSAGE(STATUS "Found OpenBlas in " ${OPENBLAS_LIBRARY})
    MESSAGE(STATUS "Found Metis in " ${METIS_LIBRARY})
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} LINSOLV")

    IF(CHOLMOD_GPU)
      FIND_PACKAGE(CuBlas)
      FIND_PACKAGE(Lapack)
      IF(CUBLAS_FOUND AND LAPACK_FOUND)
        ADD_DEFINITIONS("-DPFV_GPU")
        SET(LINKLIBS "${LINKLIBS};${CUBLAS_LIBRARY};${CUDART_LIBRARY};${LAPACK_LIBRARY}")
        MESSAGE(STATUS "Found CuBlas in " ${CUBLAS_LIBRARY})
        MESSAGE(STATUS "Found Lapack in " ${LAPACK_LIBRARY})
        SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} CHOLMOD_GPU")
      ELSE(CUBLAS_FOUND AND LAPACK_FOUND)
        MESSAGE(STATUS "Missing dependency for CHOLMOD_GPU, disabled")
        SET(DISABLED_FEATS "${DISABLED_FEATS} CHOLMOD_GPU")
        SET(CHOLMOD_GPU OFF)
      ENDIF(CUBLAS_FOUND AND LAPACK_FOUND)
    ELSE(CHOLMOD_GPU)
      SET(DISABLED_FEATS "${DISABLED_FEATS} CHOLMOD_GPU")
    ENDIF(CHOLMOD_GPU)

  ELSE(CHOLMOD_FOUND AND OPENBLAS_FOUND AND METIS_FOUND AND CGAL_FOUND)
    MESSAGE(STATUS "Missing dependency for LINSOLV, disabled")
    SET(DISABLED_FEATS "${DISABLED_FEATS} LINSOLV")
    SET(ENABLE_LINSOLV OFF)
  ENDIF(CHOLMOD_FOUND AND OPENBLAS_FOUND AND METIS_FOUND AND CGAL_FOUND)
ELSE(ENABLE_PFVFLOW AND ENABLE_LINSOLV)
  SET(DISABLED_FEATS "${DISABLED_FEATS} LINSOLV")
      IF(ENABLE_TWOPHASEFLOW)
         MESSAGE(STATUS "TWOPHASEFLOW was disabled automatically because LINSOLV is disabled")
         SET(ENABLE_TWOPHASEFLOW OFF)
      ENDIF(ENABLE_TWOPHASEFLOW)
ENDIF(ENABLE_PFVFLOW AND ENABLE_LINSOLV)
#===============================================
IF(ENABLE_MPI)
  IF(${PYTHON_VERSION_STRING} VERSION_GREATER 3)
  FIND_PACKAGE(MPI REQUIRED)
  FIND_PACKAGE(MPI4PY REQUIRED)
  IF (MPI_FOUND AND MPI4PY_FOUND)
    INCLUDE_DIRECTORIES(${MPI_CXX_INCLUDE_PATH} ${MPI4PY_INCLUDE_DIR})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS} -DYADE_MPI")
    string (STRIP "${MPI_CXX_LIBRARIES}" MPI_LIBRARIES)
    string (STRIP "${MPI_CXX_LINK_FLAGS}" MPI_LINK_FLAGS)
    SET (LINKLIBS "${LINKLIBS};${MPI_LIBRARIES};${MPI_LINK_FLAGS}")
    MESSAGE(STATUS "MPI found")
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} MPI")
    # sometimes mpi.h is not found. cmake should find it, but didn't. I'm not sure why. Here I put all directotries where it can be found.
    # it is an OR operation from ubuntu, debian and opensuse of this: https://packages.debian.org/search?suite=sid&arch=any&mode=path&searchon=contents&keywords=include%2Fmpi.h
#     find_path(MPI_H_INCLUDE_DIR mpi.h /usr/include/openmpi /usr/lib/openmpi /usr/lib/openmpi/include /usr/lib/x86_64-linux-gnu /usr/lib64/mpi/gcc/openmpi 
#     /usr/include/aarch64-linux-gnu /usr/include/arm-linux-gnueabihf /usr/include/i386-kfreebsd-gnu /usr/include/i386-linux-gnu
#     /usr/include/mips64el-linux-gnuabi64 /usr/include/openblas /usr/include/powerpc64-linux-gnu /usr/include/powerpc64le-linux-gnu
#     /usr/include/s390x-linux-gnu /usr/include/sparc64-linux-gnu /usr/include/x86_64-kfreebsd-gnu /usr/include/x86_64-linux-gnu
#     /usr/lib/aarch64-linux-gnu /usr/lib/alpha-linux-gnu /usr/lib/arm-linux-gnueabi /usr/lib/arm-linux-gnueabihf
#     /usr/lib/hppa-linux-gnu /usr/lib/i386-gnu /usr/lib/i386-kfreebsd-gnu /usr/lib/i386-linux-gnu
#     /usr/lib/m68k-linux-gnu /usr/lib/mips-linux-gnu /usr/lib/mips64el-linux-gnuabi64 /usr/lib/mipsel-linux-gnu
#     /usr/lib/powerpc-linux-gnu /usr/lib/powerpc-linux-gnuspe /usr/lib/powerpc64-linux-gnu /usr/lib/powerpc64le-linux-gnu
#     /usr/lib/riscv64-linux-gnu /usr/lib/s390x-linux-gnu /usr/lib/sh4-linux-gnu /usr/lib/sparc64-linux-gnu
#     /usr/lib/x86_64-kfreebsd-gnu /usr/lib/x86_64-linux-gnux32
#     )
#     INCLUDE_DIRECTORIES(${MPI_H_INCLUDE_DIR})
  ELSE(MPI_FOUND AND MPI4PY_FOUND)
    MESSAGE(STATUS "MPI NOT FOUND, YADE-OpenFOAM coupling disabled")
    SET (DISABLED_FEATS "${DISABLED_FEATS} MPI")
  ENDIF(MPI_FOUND AND MPI4PY_FOUND)
  ELSE(${PYTHON_VERSION_STRING} VERSION_GREATER 3)
    MESSAGE(STATUS "MPI was disabled automatically because it requires Python 3 and you'll use ${PYTHON_VERSION_STRING}")
    SET(DISABLED_FEATS "${DISABLED_FEATS} MPI")
 ENDIF(${PYTHON_VERSION_STRING} VERSION_GREATER 3)
ELSE(ENABLE_MPI)
    SET (DISABLED_FEATS "${DISABLED_FEATS} MPI")
ENDIF(ENABLE_MPI)
#===============================================
IF(ENABLE_TWOPHASEFLOW)
	IF(ENABLE_PFVFLOW)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTWOPHASEFLOW")
		SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} TWOPHASEFLOW")
	ELSE(ENABLE_PFVFLOW)
		SET(DISABLED_FEATS "${DISABLED_FEATS} TWOPHASEFLOW")
		MESSAGE(STATUS "PFVFLOW disabled, TWOPHASEFLOW will not be compiled")
	ENDIF(ENABLE_PFVFLOW)
ELSE(ENABLE_TWOPHASEFLOW)
	SET(DISABLED_FEATS "${DISABLED_FEATS} TWOPHASEFLOW")
ENDIF(ENABLE_TWOPHASEFLOW)
#===============================================
IF(ENABLE_SPH)
  ADD_DEFINITIONS("-DYADE_SPH")
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} SPH")
ELSE(ENABLE_SPH)
  SET(DISABLED_FEATS "${DISABLED_FEATS} SPH")
ENDIF(ENABLE_SPH)
#===============================================
IF(ENABLE_DEFORM)
  ADD_DEFINITIONS("-DYADE_DEFORM")
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} DEFORM")
ELSE(ENABLE_DEFORM)
  SET(DISABLED_FEATS "${DISABLED_FEATS} DEFORM")
ENDIF(ENABLE_DEFORM)
#===============================================
IF(ENABLE_FEMLIKE)
  ADD_DEFINITIONS("-DYADE_FEM")
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} FEMLIKE")
ELSE(ENABLE_FEMLIKE)
  SET(DISABLED_FEATS "${DISABLED_FEATS} FEMLIKE")
ENDIF(ENABLE_FEMLIKE)
#===============================================
IF(ENABLE_LIQMIGRATION)
  ADD_DEFINITIONS("-DYADE_LIQMIGRATION")
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} LIQMIGRATION")
ELSE(ENABLE_LIQMIGRATION)
  SET(DISABLED_FEATS "${DISABLED_FEATS} LIQMIGRATION")
ENDIF(ENABLE_LIQMIGRATION)
#===============================================
IF(ENABLE_GL2PS)
  FIND_PACKAGE(GL2PS)
  IF(GL2PS_FOUND)
    INCLUDE_DIRECTORIES(${GL2PS_INCLUDE_DIR})
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DYADE_GL2PS")
    SET(LINKLIBS  "${LINKLIBS};${GL2PS_LIBRARIES};")
    MESSAGE(STATUS "Found GL2PS")
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} GL2PS")
  ELSE(GL2PS_FOUND)
    MESSAGE(STATUS "GL2PS NOT found")
    SET(DISABLED_FEATS "${DISABLED_FEATS} GL2PS")
    SET(ENABLE_GL2PS OFF)
  ENDIF(GL2PS_FOUND)
ELSE(ENABLE_GL2PS)
  SET(DISABLED_FEATS "${DISABLED_FEATS} GL2PS")
ENDIF(ENABLE_GL2PS)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

#===========================================================
IF(ENABLE_LBMFLOW)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLBM_ENGINE")
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} LBMFLOW")
  MESSAGE("LBMFLOW is still experimental, building and running LBM engine are at your own risk!")
ELSE(ENABLE_LBMFLOW)
  SET(DISABLED_FEATS "${DISABLED_FEATS} LBMFLOW")
ENDIF(ENABLE_LBMFLOW)
#===============================================
IF(ENABLE_MASK_ARBITRARY)
  ADD_DEFINITIONS("-DYADE_MASK_ARBITRARY")
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} MASK_ARBITRARY")
  IF(NOT MASK_ARBITRARY_SIZE)
    SET(MASK_ARBITRARY_SIZE "256")
  ENDIF(NOT MASK_ARBITRARY_SIZE)
  ADD_DEFINITIONS(-DYADE_MASK_ARBITRARY_SIZE=${MASK_ARBITRARY_SIZE})
  MESSAGE("MASK_ARBITRARY_SIZE = ${MASK_ARBITRARY_SIZE}")
ELSE(ENABLE_MASK_ARBITRARY)
  SET(DISABLED_FEATS "${DISABLED_FEATS} MASK_ARBITRARY")
ENDIF(ENABLE_MASK_ARBITRARY)

#===========================================================

IF(ENABLE_THERMAL AND ENABLE_PFVFLOW)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTHERMAL")
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} THERMAL")
ELSE(ENABLE_THERMAL AND ENABLE_PFVFLOW)
  SET(DISABLED_FEATS "${DISABLED_FEATS} THERMAL")
ENDIF(ENABLE_THERMAL AND ENABLE_PFVFLOW)

IF(ENABLE_PARTIALSAT)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPARTIALSAT")
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} PARTIALSAT")
ELSE(ENABLE_PARTIALSAT)
  SET(DISABLED_FEATS "${DISABLED_FEATS} PARTIALSAT")
ENDIF(ENABLE_PARTIALSAT)

IF(ENABLE_PROFILING)
  SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} PROFILING")
  ADD_DEFINITIONS("-DUSE_TIMING_DELTAS -DISC_TIMING")
ELSE(ENABLE_PROFILING)
  SET(DISABLED_FEATS "${DISABLED_FEATS} PROFILING")
ENDIF(ENABLE_PROFILING)

#===========================================================
IF(ENABLE_POTENTIAL_PARTICLES)
  FIND_PACKAGE(OpenBlas REQUIRED)
  FIND_PACKAGE(LAPACK REQUIRED)
  IF(OPENBLAS_FOUND AND LAPACK_FOUND)
    ADD_DEFINITIONS("-DYADE_POTENTIAL_PARTICLES")
    SET(LINKLIBS  "${LINKLIBS};${OPENBLAS_LIBRARY};${LAPACK_LIBRARY}")
    MESSAGE(STATUS "Found OpenBlas")
    MESSAGE(STATUS "Found Lapack")
    INCLUDE_DIRECTORIES(${BLAS_INCLUDE_DIR})
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} PotentialParticles")
  ELSE(OPENBLAS_FOUND AND LAPACK_FOUND)
    MESSAGE(STATUS "Missing dependency for PotentialParticles, disabled")
    SET(DISABLED_FEATS "${DISABLED_FEATS} PotentialParticles")
    SET(ENABLE_POTENTIAL_PARTICLES OFF)
  ENDIF(OPENBLAS_FOUND AND LAPACK_FOUND)
ELSE(ENABLE_POTENTIAL_PARTICLES)
  SET(DISABLED_FEATS "${DISABLED_FEATS} PotentialParticles")
ENDIF(ENABLE_POTENTIAL_PARTICLES)
#===========================================================


#===========================================================
IF(ENABLE_POTENTIAL_BLOCKS)
  INCLUDE(FindCLP)
  FIND_PACKAGE(OpenBlas REQUIRED)
  FIND_PACKAGE(LAPACK REQUIRED)
  IF(CLP_FOUND AND OPENBLAS_FOUND AND LAPACK_FOUND)
    ADD_DEFINITIONS("-DYADE_POTENTIAL_BLOCKS")
    INCLUDE_DIRECTORIES(${CLP_INCLUDE_DIR} ${CLP2_INCLUDE_DIR} )
    SET(LINKLIBS  "${LINKLIBS};${CLP_LIBRARY};${CLP2_LIBRARY};${CLP3_LIBRARY};${OPENBLAS_LIBRARY};${LAPACK_LIBRARY}")
    MESSAGE(STATUS "Found CLP version: ${CLP_VERSION}, coinutils version: ${COINUTILS_VERSION}")
    INCLUDE_DIRECTORIES(${BLAS_INCLUDE_DIR})
    SET(CONFIGURED_FEATS "${CONFIGURED_FEATS} PotentialBlocks")
  ELSE(CLP_FOUND AND OPENBLAS_FOUND AND LAPACK_FOUND)
    MESSAGE(STATUS "CLP NOT found")
    SET(DISABLED_FEATS "${DISABLED_FEATS} PotentialBlocks")
    SET(ENABLE_POTENTIAL_BLOCKS OFF)
  ENDIF(CLP_FOUND AND OPENBLAS_FOUND AND LAPACK_FOUND)
IF(COINUTILS_VERSION VERSION_LESS 2.11.3)
  set (CMAKE_CXX_STANDARD 14)
  MESSAGE(STATUS "${Esc}[91mWARNING: coinutils version older than 2.11.3. Falling back to C++14.${Esc}[0m")
ENDIF(COINUTILS_VERSION VERSION_LESS 2.11.3)
ELSE(ENABLE_POTENTIAL_BLOCKS)
  SET(DISABLED_FEATS "${DISABLED_FEATS} PotentialBlocks")
ENDIF(ENABLE_POTENTIAL_BLOCKS)
#===========================================================

IF (INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX})
  MESSAGE(WARNING "Use CMAKE_INSTALL_PREFIX option instead of INSTALL_PREFIX! It will be removed soon.")
ENDIF (INSTALL_PREFIX)

IF (CMAKE_INSTALL_PREFIX)
  MESSAGE("Yade will be installed to ${CMAKE_INSTALL_PREFIX}")
ELSE (CMAKE_INSTALL_PREFIX)
  MESSAGE("Yade will be installed to default path ${CMAKE_INSTALL_PREFIX}, if you want to override it use -DCMAKE_INSTALL_PREFIX option.")
ENDIF (CMAKE_INSTALL_PREFIX)

IF (NOT SUFFIX)
  SET (SUFFIX "-${YADE_VERSION}")
ENDIF (NOT SUFFIX)

IF(NOSUFFIX)   #For packaging
  SET (SUFFIX "")
ENDIF(NOSUFFIX)   #For packaging

IF(NOT LIBRARY_OUTPUT_PATH)   #For packaging
  SET (LIBRARY_OUTPUT_PATH ${CMAKE_INSTALL_LIBDIR})
ENDIF(NOT LIBRARY_OUTPUT_PATH)   #For packaging

IF (NOT runtimePREFIX)
  SET (runtimePREFIX ${CMAKE_INSTALL_PREFIX})
ENDIF (NOT runtimePREFIX)

MESSAGE (STATUS "Suffix is set to " ${SUFFIX})
MESSAGE (STATUS "LIBRARY_OUTPUT_PATH is set to " ${LIBRARY_OUTPUT_PATH})
MESSAGE (STATUS "runtimePREFIX is set to " ${runtimePREFIX})
#===========================================================

SET(YADE_LIB_PATH ${CMAKE_INSTALL_PREFIX}/${LIBRARY_OUTPUT_PATH}/yade${SUFFIX})
SET(YADE_EXEC_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
SET(YADE_PY_PATH ${YADE_LIB_PATH}/py)
SET(YADE_DOC_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/doc/yade${SUFFIX})
SET(YADE_MAN_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_MANDIR})


SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH "${YADE_LIB_PATH};${YADE_PY_PATH};${YADE_PY_PATH}/yade/;${YADE_PY_PATH}/yade/qt;${YADE_PY_PATH}/gts")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#===========================================================
IF(ENABLE_GUI)
  ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/gui")
ENDIF(ENABLE_GUI)
ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/py")
#===========================================================

FILE(GLOB SRC_CORE "core/*.cpp")
FILE(GLOB_RECURSE SRC_PKG  "pkg/*.cpp")
FILE(GLOB SRC_LIB  "lib/*.cpp")

SET(SRC_LIB "${SRC_LIB};lib/base/Math.cpp;lib/high-precision/RealHPConfig.cpp;lib/factory/ClassFactory.cpp;lib/factory/DynLibManager.cpp;lib/base/Logging.cpp;lib/compatibility/LapackCompatibility.cpp;lib/compatibility/VTKCompatibility.cpp")
SET(SRC_LIB "${SRC_LIB};lib/serialization/Serializable.cpp;lib/pyutil/gil.cpp;core/main/pyboot.cpp;${GUI_SRC_LIB};${CGAL_SRC_LIB}")
IF(ENABLE_POTENTIAL_PARTICLES OR ENABLE_POTENTIAL_BLOCKS)
  SET(SRC_LIB "${SRC_LIB};lib/computational-geometry/MarchingCube.cpp")
ENDIF(ENABLE_POTENTIAL_PARTICLES OR ENABLE_POTENTIAL_BLOCKS)

#===========================================================

IF (CHUNKSIZE)
  INCLUDE(CombineSources)
  COMBINE_SOURCES(${CMAKE_BINARY_DIR}/core "${SRC_CORE}" ${CHUNKSIZE})
  FILE(GLOB SRC_CORE_COMBINED "${CMAKE_BINARY_DIR}/core.*.cpp")
  COMBINE_SOURCES(${CMAKE_BINARY_DIR}/pkg "${SRC_PKG}" ${CHUNKSIZE})
  FILE(GLOB SRC_PKG_COMBINED "${CMAKE_BINARY_DIR}/pkg.*.cpp")
  COMBINE_SOURCES(${CMAKE_BINARY_DIR}/lib "${SRC_LIB}" ${CHUNKSIZE})
  FILE(GLOB SRC_LIB_COMBINED "${CMAKE_BINARY_DIR}/lib.*.cpp")
  ADD_LIBRARY(yade SHARED ${SRC_LIB_COMBINED} ${SRC_CORE_COMBINED} ${SRC_PKG_COMBINED})
ELSE (CHUNKSIZE)
  ADD_LIBRARY(yade SHARED ${SRC_CORE} ${SRC_PKG} ${SRC_LIB})
ENDIF (CHUNKSIZE)

#===========================================================

ADD_LIBRARY(boot SHARED ${CMAKE_CURRENT_SOURCE_DIR}/core/main/pyboot.cpp)
SET_TARGET_PROPERTIES(boot PROPERTIES PREFIX "" LINK_FLAGS "-Wl,--as-needed" )
TARGET_LINK_LIBRARIES(yade ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${LINKLIBS} -lrt)
SET_TARGET_PROPERTIES(yade  PROPERTIES LINK_FLAGS "-Wl,--as-needed" )
TARGET_LINK_LIBRARIES(boot yade)

IF(ENABLE_MPFR)
  TARGET_LINK_LIBRARIES(yade ${MPFR_LIBRARIES})
ENDIF()

IF(REAL_PRECISION_BITS EQUAL 128 OR ((REAL_MULTI_HP) AND (REAL_PRECISION_BITS LESS 128)))
  # the quadmath library is a part of g++ package, no need to check if it is installed.
  TARGET_LINK_LIBRARIES(yade -lquadmath)
  MESSAGE(STATUS "linking -lquadmath")
ELSE()
  MESSAGE(STATUS "linking -lquadmath skipped")
ENDIF()

IF(ENABLE_VTK)
  IF(${VTK_MAJOR_VERSION} GREATER 5)
    TARGET_LINK_LIBRARIES(yade ${VTK_LIBRARIES})
    ADD_DEFINITIONS("-DYADE_VTK6")
  IF(${VTK_MAJOR_VERSION} GREATER 7)
   ADD_DEFINITIONS("-DYADE_VTK8")
   MESSAGE(STATUS "VTK version >7 is found")
  ELSE(${VTK_MAJOR_VERSION} GREATER 7)
   MESSAGE(STATUS "VTK version >5 and <8 is found")
  ENDIF(${VTK_MAJOR_VERSION} GREATER 7)
   ELSE(${VTK_MAJOR_VERSION} GREATER 5)
    TARGET_LINK_LIBRARIES(yade vtkHybrid)
  ENDIF(${VTK_MAJOR_VERSION} GREATER 5)
ENDIF(ENABLE_VTK)

IF(ENABLE_GUI)
  TARGET_LINK_LIBRARIES(yade _GLViewer ${GUI_LIBS})
ENDIF(ENABLE_GUI)

#====================================
#Back compatibility with scons
SET (realVersion ${YADE_VERSION})
SET (version ${YADE_VERSION})
SET (pyExecutable ${PYTHON_EXECUTABLE})
SET (profile "default")
SET (sourceRoot "${CMAKE_CURRENT_SOURCE_DIR}")
#====================================

CONFIGURE_FILE(core/main/yade-batch.in "${CMAKE_BINARY_DIR}/bins/yade${SUFFIX}-batch")
IF(ENABLE_OAR)
  CONFIGURE_FILE(core/main/yade-oar.in "${CMAKE_BINARY_DIR}/bins/yade${SUFFIX}-oar")
ENDIF(ENABLE_OAR)
CONFIGURE_FILE(core/main/main.py.in "${CMAKE_BINARY_DIR}/bins/yade${SUFFIX}")
CONFIGURE_FILE(py/config.py.in "${CMAKE_BINARY_DIR}/config.py")
CONFIGURE_FILE(py/libVersions.py.in "${CMAKE_BINARY_DIR}/libVersions.py")
CONFIGURE_FILE(py/__init__.py.in "${CMAKE_BINARY_DIR}/__init__.py")

#===========================================================
# Create header files for PFV from FlowEngine.hpp.in-template.
# All @TEMPLATE_FLOW_NAME@ are replacing by a given names

SET (TEMPLATE_FLOW_NAMES DFNFlowEngineT DummyFlowEngineT FlowEngineT FlowEngine_PeriodicInfo SoluteFlowEngineT UnsaturatedEngineT TwoPhaseFlowEngineT PartialSatClayEngineT)
FOREACH(TF ${TEMPLATE_FLOW_NAMES})
  SET (TEMPLATE_FLOW_NAME ${TF})
  CONFIGURE_FILE(pkg/pfv/FlowEngine.hpp.in "${CMAKE_BINARY_DIR}/pkg/pfv/FlowEngine_${TF}.hpp" @ONLY)
  CONFIGURE_FILE(pkg/pfv/FlowEngine.ipp.in "${CMAKE_BINARY_DIR}/pkg/pfv/FlowEngine_${TF}.ipp" @ONLY)
ENDFOREACH(TF)
INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/pkg/pfv/")
#===========================================================

INSTALL(PROGRAMS "${CMAKE_BINARY_DIR}/bins/yade${SUFFIX}-batch" DESTINATION ${YADE_EXEC_PATH}/)
IF(ENABLE_OAR)
  INSTALL(PROGRAMS "${CMAKE_BINARY_DIR}/bins/yade${SUFFIX}-oar" DESTINATION ${YADE_EXEC_PATH}/)
ENDIF(ENABLE_OAR)
INSTALL(PROGRAMS "${CMAKE_BINARY_DIR}/bins/yade${SUFFIX}" DESTINATION ${YADE_EXEC_PATH}/)
INSTALL(CODE "execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink ${YADE_EXEC_PATH}/yade${SUFFIX} ${YADE_EXEC_PATH}/yade${SUFFIX}.py)")
INSTALL(FILES "${CMAKE_BINARY_DIR}/config.py" DESTINATION ${YADE_PY_PATH}/yade/)
INSTALL(FILES "${CMAKE_BINARY_DIR}/libVersions.py" DESTINATION ${YADE_PY_PATH}/yade/)
INSTALL(FILES "${CMAKE_BINARY_DIR}/__init__.py" DESTINATION ${YADE_PY_PATH}/yade/)
FILE(GLOB filesPYChecks "${CMAKE_CURRENT_SOURCE_DIR}/scripts/checks-and-tests/checks/*.py")
INSTALL(FILES ${filesPYChecks} DESTINATION ${YADE_PY_PATH}/yade/tests/checks)
FILE(GLOB filesGUIChecks "${CMAKE_CURRENT_SOURCE_DIR}/scripts/checks-and-tests/gui/*.py" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/checks-and-tests/gui/*.sh")
INSTALL(FILES ${filesGUIChecks} DESTINATION ${YADE_PY_PATH}/yade/tests/gui)
FILE(GLOB filesGUItestHelper "${CMAKE_CURRENT_SOURCE_DIR}/scripts/checks-and-tests/gui/helper/*.py")
INSTALL(FILES ${filesGUItestHelper} DESTINATION ${YADE_PY_PATH}/yade)
INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scripts/checks-and-tests/checks/data/" DESTINATION ${YADE_PY_PATH}/yade/tests/checks/data)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/doc/yade-logo-note.png" DESTINATION "${YADE_DOC_PATH}/img")
FILE(GLOB filesHP "${CMAKE_CURRENT_SOURCE_DIR}/py/high-precision/*.py" "${CMAKE_CURRENT_SOURCE_DIR}/py/tests/testMathHelper.py")
INSTALL(FILES ${filesHP} DESTINATION ${YADE_PY_PATH}/yade)

INSTALL(TARGETS boot DESTINATION "${YADE_PY_PATH}/yade/")
INSTALL(TARGETS yade DESTINATION ${YADE_LIB_PATH})

#===========================================================
MESSAGE(STATUS "===========================================================")
MESSAGE(STATUS "Yade configured with following features:${Esc}[36m${CONFIGURED_FEATS}${Esc}[0m")
MESSAGE(STATUS "Disabled features:${Esc}[33m${DISABLED_FEATS}${Esc}[0m")
IF(ENABLE_LOGGER)
  MESSAGE(STATUS "${Esc}[36mUsing MAX_LOG_LEVEL=${MAX_LOG_LEVEL}, ENABLE_LOGGER=ON${Esc}[0m")
ELSE(ENABLE_LOGGER)
  MESSAGE(STATUS "${Esc}[36mUsing MAX_LOG_LEVEL=${MAX_LOG_LEVEL}, ENABLE_LOGGER=OFF${Esc}[0m")
ENDIF(ENABLE_LOGGER)
IF (DEBUG)
  MESSAGE(STATUS "${Esc}[33mDebug build${Esc}[0m")
  SET (debugbuild " (debug build)")
ELSE (DEBUG)
  IF (ENABLE_ASAN)
    MESSAGE(STATUS "${Esc}[33mAddressSanitizer build! Check documentation how to run${Esc}[0m")
  ELSE (ENABLE_ASAN)
    MESSAGE(STATUS "${Esc}[32mOptimized build${Esc}[0m")
  ENDIF (ENABLE_ASAN)
ENDIF (DEBUG)
MESSAGE(STATUS "C++ standard version: ${CMAKE_CXX_STANDARD}")
IF (CHUNKSIZE)
  MESSAGE(STATUS "CHUNKSIZE is set to " ${CHUNKSIZE})
ENDIF (CHUNKSIZE)

MESSAGE(STATUS "===========================================================")
#===========================================================
#Building doc
ADD_CUSTOM_TARGET(doc)
FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc)
FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc/sphinx)
ADD_CUSTOM_COMMAND(
                  TARGET doc PRE_BUILD
                  COMMAND rm -rf ${CMAKE_BINARY_DIR}/doc/sphinx/_build
                  COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/doc/* ${CMAKE_BINARY_DIR}/doc
                  COMMAND PYTHONPATH=${CMAKE_BINARY_DIR}/doc/sphinx ${YADE_EXEC_PATH}/yade${SUFFIX} -x yadeSphinx.py html
                  COMMAND PYTHONPATH=${CMAKE_BINARY_DIR}/doc/sphinx ${YADE_EXEC_PATH}/yade${SUFFIX} -x yadeSphinx.py latex
                  COMMAND PYTHONPATH=${CMAKE_BINARY_DIR}/doc/sphinx ${YADE_EXEC_PATH}/yade${SUFFIX} -x yadeSphinx.py epub
                  COMMAND PYTHONPATH=${CMAKE_BINARY_DIR}/doc/sphinx ${YADE_EXEC_PATH}/yade${SUFFIX} -x yadeSphinx.py workarounds
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc/sphinx
                  DEPENDS ${YADE_EXEC_PATH}/yade${SUFFIX}
                  )
ADD_CUSTOM_COMMAND(
                  TARGET doc POST_BUILD
                  COMMAND xelatex Yade.tex
                  COMMAND xelatex Yade.tex
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc/sphinx/_build/latex
                  DEPENDS ${YADE_EXEC_PATH}/yade${SUFFIX} ${CMAKE_BINARY_DIR}/doc/sphinx/_build/latex/Yade.tex
                  )
ADD_CUSTOM_COMMAND(
                  TARGET doc POST_BUILD
                  COMMAND rm -rf ${YADE_DOC_PATH}/html
                  COMMAND mv ${CMAKE_BINARY_DIR}/doc/sphinx/_build/html ${YADE_DOC_PATH}/html
                  COMMAND mv ${CMAKE_BINARY_DIR}/doc/sphinx/_build/latex/Yade.pdf ${YADE_DOC_PATH}/
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc/sphinx/_build/latex
                  DEPENDS ${YADE_EXEC_PATH}/yade${SUFFIX} ${CMAKE_BINARY_DIR}/doc/sphinx/_build/latex/Yade.tex
                  )
ADD_CUSTOM_COMMAND(
                  TARGET doc POST_BUILD
                  COMMAND mv ${CMAKE_BINARY_DIR}/doc/sphinx/_build/epub/Yade.epub ${YADE_DOC_PATH}/ || true
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc/sphinx/_build/latex
                  DEPENDS ${YADE_EXEC_PATH}/yade${SUFFIX} ${CMAKE_BINARY_DIR}/doc/sphinx/_build/latex/Yade.tex
                  )
#===========================================================
#Building manpage
ADD_CUSTOM_TARGET(manpage)
ADD_CUSTOM_COMMAND(
                  TARGET manpage POST_BUILD
                  COMMAND help2man ${YADE_EXEC_PATH}/yade${SUFFIX} > yade${SUFFIX}.1
                  COMMAND help2man ${YADE_EXEC_PATH}/yade${SUFFIX}-batch > yade${SUFFIX}-batch.1
                  COMMAND mkdir -p ${YADE_MAN_PATH}
                  COMMAND mv *.1 ${YADE_MAN_PATH}
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                  DEPENDS ${YADE_EXEC_PATH}/yade${SUFFIX} /usr/bin/help2man
                  )
#===========================================================
#Execute standard checks
ADD_CUSTOM_TARGET(check)
ADD_CUSTOM_COMMAND(
                  TARGET check POST_BUILD
                  COMMAND ${YADE_EXEC_PATH}/yade${SUFFIX} --test
                  COMMAND ${YADE_EXEC_PATH}/yade${SUFFIX} --check
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                  DEPENDS ${YADE_EXEC_PATH}/yade${SUFFIX}
                  )
#===========================================================
