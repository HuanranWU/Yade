##########################################################################
#  2019        Janek Kozicki                                             #
#                                                                        #
#  This program is free software; it is licensed under the terms of the  #
#  GNU General Public License v2 or later. See file LICENSE for details. #
##########################################################################
#
# This module sets following variable:
#
#  MPFRCPP_FOUND  - libmpfrc++-dev found


MESSAGE(STATUS "Checking /usr/include/mpreal.h provided by package libmpfrc++-dev")

if(EXISTS "/usr/include/mpreal.h")
    set(MPFRCPP_FOUND TRUE)
else()
    set(MPFRCPP_FOUND FALSE)
endif()

