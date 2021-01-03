##########################################################################
#  2019        Janek Kozicki                                             #
#                                                                        #
#  This program is free software; it is licensed under the terms of the  #
#  GNU General Public License v2 or later. See file LICENSE for details. #
##########################################################################
#
# This module sets following variables:
#
#  MPMATH_FOUND   - python-mpmath found
#  MPMATH_VERSION - python-mpmath version


MESSAGE(STATUS "Will now try to find python-mpmath using ${PYTHON_EXECUTABLE}")
execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
    "import mpmath ; print(mpmath.__version__)"
    RESULT_VARIABLE _MPMATH_COMMAND_SUCCESS
    OUTPUT_VARIABLE _MPMATH_OUTPUT_LIST
    ERROR_VARIABLE  _MPMATH_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

IF(_MPMATH_COMMAND_SUCCESS EQUAL 0)
  IF(_MPMATH_OUTPUT_LIST)
    set(MPMATH_FOUND TRUE)
    list(GET _MPMATH_OUTPUT_LIST 0 _MPMATH_VERSION)
    set(MPMATH_VERSION ${_MPMATH_VERSION})
  ELSE()
    set(MPMATH_FOUND FALSE)
  ENDIF()
ELSE()
  set(MPMATH_FOUND FALSE)
ENDIF()

