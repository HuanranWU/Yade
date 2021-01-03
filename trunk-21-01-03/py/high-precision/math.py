# encoding: utf-8
##########################################################################
#  2019      Janek Kozicki                                               #
#                                                                        #
#  This program is free software; it is licensed under the terms of the  #
#  GNU General Public License v2 or later. See file LICENSE for details. #
##########################################################################
"""
This python module exposes all C++ math functions for Real and Complex types to python.
In fact it sort of duplicates ``import math``, ``import cmath`` or ``import mpmath``.
Also it facilitates migration of old python scripts to high precision calculations.

This module has following purposes:

1. To reliably :ysrc:`test<py/tests/testMath.py>` all C++ math functions of arbitrary precision Real and Complex types against mpmath.
2. To act as a "migration helper" for python scripts which call python mathematical functions that do not work
   well with mpmath. As an example see :yref:`yade.math.linspace` below and `this merge request <https://gitlab.com/yade-dev/trunk/-/merge_requests/414>`__
3. To allow writing python math code in a way that mirrors C++ math code in Yade. As a bonus it
   will be faster than mpmath because mpmath is a purely python library (which was one of the main
   difficulties when writing :ysrc:`lib/high-precision/ToFromPythonConverter.hpp`)
4. To test Eigen NumTraits
5. To test CGAL NumTraits

If another ``C++`` :ref:`math function<cpp-hp-compatibility>` is needed it should be added to following files:

1. :ysrc:`lib/high-precision/MathFunctions.hpp`
2. :ysrc:`py/high-precision/_math.cpp`
3. :ysrc:`py/tests/testMath.py`
4. :ysrc:`py/tests/testMathHelper.py`

If another ``python`` math function does not work well with ``mpmath`` it should be added below, and original
calls to this function should call this function instead, e.g. ``numpy.linspace(…)`` is replaced with ``yade.math.linspace(…)``.

The ``RealHP<n>`` :ref:`higher precision<higher-hp-precision>` math functions can be accessed in python by using the ``.HPn`` module scope. For example::

	import yade.math as mth
	mth.HP2.sqrt(2) # produces square root of 2 using RealHP<2> precision
	mth.sqrt(2)     # without using HPn module scope it defaults to RealHP<1>

"""

# all C++ functions are accessible now:
from yade._math import *

import yade.config

# import these two functions to yade.math
getDigits10 = yade._math.RealHPConfig.getDigits10
getDigits2  = yade._math.RealHPConfig.getDigits2

def needsMpmathAtN(N):
	"""
	:param N: The ``int`` N value of ``RealHP<N>`` in question. Must be ``N >= 1``.
	:return: ``True`` or ``False`` with information if using ``mpmath`` is necessary to avoid losing precision when working with ``RealHP<N>``.
	"""
	if(N < 1):
		raise ValueError("Incorrect N argument: "+str(N))
	return yade._math.RealHPConfig.getDigits10(N) > 15

def getRealHPCppDigits10():
	"""
	:return: tuple containing amount of decimal digits supported on C++ side by Eigen and CGAL.
	"""
	return tuple(yade._math.RealHPConfig.getDigits10(i) for i in yade._math.RealHPConfig.getSupportedByEigenCgal())

def getRealHPPythonDigits10():
	"""
	:return: tuple containing amount of decimal digits supported on python side by yade.minieigenHP.
	"""
	return tuple(yade._math.RealHPConfig.getDigits10(i) for i in yade._math.RealHPConfig.getSupportedByMinieigen())

def linspace(a,b,num):
	"""
	This function calls ``numpy.linspace(…)`` or ``mpmath.linspace(…)``, because ``numpy.linspace`` function does not work with mpmath.
	"""
	if(needsMpmathAtN(1)):
		import mpmath
		return mpmath.linspace(a,b,num)
	else:
		import numpy
		return numpy.linspace(a,b,num=num)

def toHP1(arg):
	"""
	This function is for compatibility of calls like: ``g = yade.math.toHP1("-9.81")``. If yade is compiled with default ``Real`` precision set as ``double``,
	then python won't accept string arguments as numbers. However when using higher precisions only calls
	``yade.math.toHP1("1.234567890123456789012345678901234567890")`` do not cut to the first 15 decimal places.
	The calls such as ``yade.math.toHP1(1.234567890123456789012345678901234567890)`` will use default ``python`` ↔ ``double`` conversion and will cut
	the number to its first 15 digits.

	If you are debugging a high precision python script, and have difficulty finding places where such cuts have happened you should use ``yade.math.toHP1(string)``
	for declaring all python floating point numbers which are physically important in the simulation.
	This function will throw exception if bad conversion is about to take place.

	Also see example high precision check :ysrc:`checkGravityRungeKuttaCashKarp54.py<scripts/checks-and-tests/checks/checkGravityRungeKuttaCashKarp54.py>`.
	"""
	if(type(arg) == float):
		raise RuntimeError("Error: only first 15 digits would be used for arg = "+str(arg)+", better pass the argument as string or python mpmath high precision type.")
		#print('\033[91m'+"Warning: only 15 digits are used for arg = ",arg,'\033[0m')
	if((type(arg) == str) and (getDigits2(1)==53)): # also: ("PrecisionDouble" in yade.config.features)
		return yade._math.toHP1(float(arg))     # if yade is compiled with `double` then toHP1(…) cannot accept string and python float has enough precision. So here we make sure it works.
	else:
		return yade._math.toHP1(arg)

def radiansHP1(arg):
	"""
	The default python function ``import math ; math.radians(arg)`` only works on 15 digit ``double`` precision. If you want to carry on calculations in higher precision
	it is advisable to use this function ``yade.math.radiansHP1(arg)`` instead. It uses full yade ``Real`` precision numbers.

	NOTE: in the future this function may replace ``radians(…)`` function which is called in yade in many scripts, and which in fact is a call to native python ``math.radians``.
	We only need to find the best backward compatible approach for this. The function ``yade.math.radiansHP1(arg)`` will remain as the function which uses native yade ``Real`` precision.
	"""
	return yade.math.HP1.Pi() * toHP1(arg) / toHP1(180)

