# -*- coding: utf-8 -*-
# an extended version of autopkgtest check for minieigen
# (C) 2015 Anton Gladky <gladk@debian.org>
# (C) 2019 Janek Kozicki

import unittest, math, sys
import yade.minieigenHP as mne
import yade

import testMathHelper

if(yade.config.highPrecisionMpmath):
	#print('\n\033[92m'+"Using "+str(yade.math.getRealHPPythonDigits10())+" decimal digits in python. Importing mpmath"+'\033[0m\n')
	import mpmath

class ExtendedMinieigenTests(unittest.TestCase):
	def needsMpmathAtN(self,N): return yade.math.needsMpmathAtN(N)
	def hasMpfr(self): return ('MPFR' in yade.config.features)
	def setUp(self):
		mne.RealHPConfig  = yade.math.RealHPConfig
		self.testLevelsHP = mne.RealHPConfig.getSupportedByMinieigen()
		self.baseDigits   = mne.RealHPConfig.getDigits10(1)
		self.skip33       = mne.RealHPConfig.isFloat128Broken        # this is for local testing only. It's here because with older compiler and -O0 the float128 is segfaulting
		self.builtinHP    = { 6 : [6,15,18,24,33] , 15 : [15,33] } # higher precisions are multiplies of baseDigits, see NthLevelRealHP in lib/high-precision/RealHP.hpp

	def getDigitsHP(self,N):
		ret = None
		if (self.baseDigits in self.builtinHP) and (N <= len(self.builtinHP[self.baseDigits])):
			ret = self.builtinHP[self.baseDigits][N-1]
		else:
			ret = self.baseDigits*N
		self.assertEqual(ret,mne.RealHPConfig.getDigits10(N))
		return ret

	def adjustDigs0(self,N,HPn,MPn):
		self.HPnHelper = HPn
		self.digs0     = self.getDigitsHP(N)
		self.digs1     = self.digs0 + mne.RealHPConfig.extraStringDigits10
		MPn.mp.dps     = self.digs1
		# tolerance = 1.001×10⁻ᵈ⁺¹, where ᵈ==self.digs0
		# so basically we store one more decimal digit, and expect one less decimal digit. That amounts to ignoring one (two, if the extra one is counted) least significant digits.
		self.tolerance = (MPn.mpf(10)**(-self.digs0+1))*MPn.mpf("1.001")

	def runCheck(self,N,func):
		nameHP = "HP" + str(N)               # the same as the line 'string    name = "HP" + boost::lexical_cast<std::string>(N);' in function registerInScope in ToFromPythonConverter.hpp
		HPn    = getattr(mne,nameHP);        # the same as the line 'py::scope HPn  = boost::python::class_<ScopeHP<N>>(name.c_str());'   in ToFromPythonConverter.hpp
		if(self.needsMpmathAtN(N)):
			MPn = mpmath
		else:
			MPn = testMathHelper
		self.MPnHelper = MPn
		if(N==1):
			self.adjustDigs0(N,mne,MPn)
			if((self.digs0 == 33) and self.skip33): return
			func(N,mne,"mne.",MPn)       # test global scope functions with RealHP<1>
		self.adjustDigs0(N,HPn,MPn)
		if((self.digs0 == 33) and self.skip33): return
		print('RealHP<'+str(N)+'>', end=' ')
		func(N,HPn,"mne."+nameHP+".",MPn)    # test scopes HP1, HP2, etc

	def checkRelativeError(self,a,b,mult=1.0):
		MPn = self.MPnHelper
		if b!=0:
			self.assertLessEqual(abs( (MPn.mpf(a)-MPn.mpf(b))/MPn.mpf(b) ),self.tolerance*mult)
		else:
			self.assertLessEqual(abs( (MPn.mpf(a)-MPn.mpf(b))/self.tolerance ),self.tolerance*mult)
	def checkRelativeComplexError(self,a,b):
		MPn = self.MPnHelper
		self.assertLessEqual(abs( (MPn.mpc(a)-MPn.mpc(b))/MPn.mpc(b) ),self.tolerance)

	def testMpmath(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestMpmath)

	def HPtestMpmath(self,N,HPn,prefix,MPn):
		self.assertEqual(2    ,float(MPn.mpf(2)))
		self.assertEqual(2    ,float(MPn.mpf("2")))
		self.assertEqual(2**3 ,MPn.mpf("2")**3)
		self.assertEqual(2/4  ,MPn.mpf("2")/4)
		self.assertEqual(3/2,3/MPn.mpf("2"))
		self.assertEqual(3*2,3*MPn.mpf("2"))
		self.assertEqual(2-3  ,MPn.mpf("2")-3)

		self.assertEqual(2    ,complex(MPn.mpc(2)))
		self.assertEqual(2+5j ,complex(MPn.mpc(2+5j)))
		self.assertEqual(2    ,complex(MPn.mpc("2")))
		self.assertEqual(2+5j ,complex(MPn.mpc("2","5")))
		self.assertEqual(2-3  ,MPn.mpc("2")-3)
		self.assertEqual(2/4  ,MPn.mpc("2")/4)
		self.assertEqual(3/2,3/MPn.mpc("2"))
		self.assertEqual(5,abs(MPn.mpc("-3","-4")))


	def testVector2i(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestVector2i)

	def HPtestVector2i(self,N,HPn,prefix,MPn):
		a2i = HPn.Vector2i(2,1)
		b2i = HPn.Vector2i(3,5)
		c2i = a2i + b2i
		#self.assertEqual(c2i.eigenFlags(),352)
		#self.assertEqual(c2i.eigenStorageOrder(),0)

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c2i[0] , MPn.mpf("5") )
		self.checkRelativeError( c2i[1] , MPn.mpf("6") )

		c2i *= 2

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c2i[0] , MPn.mpf("10") )
		self.checkRelativeError( c2i[1] , MPn.mpf("12") )

		self.assertEqual( c2i , eval(prefix+c2i.__str__()) )

	def testVector2(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestVector2)

	def HPtestVector2(self,N,HPn,prefix,MPn):
		a2 = HPn.Vector2(2,1)
		b2 = HPn.Vector2(3,5)
		c2 = a2 + b2
		#self.assertEqual(c2.eigenFlags(),352)
		#self.assertEqual(c2.eigenStorageOrder(),0)

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c2[0] , MPn.mpf("5") )
		self.checkRelativeError( c2[1] , MPn.mpf("6") )

		c2 *= 2

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c2[0] , MPn.mpf("10") )
		self.checkRelativeError( c2[1] , MPn.mpf("12") )

		self.assertEqual( c2 , eval(prefix+c2.__str__()) )

	def testVector2c(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestVector2c)

	def HPtestVector2c(self,N,HPn,prefix,MPn):
		a2c = HPn.Vector2c(2-10j,1)
		b2c = HPn.Vector2c(3,5)
		c2c = a2c + b2c
		#self.assertEqual(c2c.eigenFlags(),352)
		#self.assertEqual(c2c.eigenStorageOrder(),0)

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeComplexError( c2c[0] , MPn.mpc("5","-10") )
		self.checkRelativeComplexError( c2c[1] , MPn.mpf("6") )

		c2c *= 2

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeComplexError( c2c[0] , MPn.mpc("10","-20") )
		self.checkRelativeComplexError( c2c[1] , MPn.mpf("12") )

		# the replace mpc →→ MPn.mpc is to make sure that the complex numbers are accessible in this test
		self.assertEqual( c2c , eval(prefix+c2c.__str__().replace("mpc","MPn.mpc")) )

	def testVector3i(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestVector3i)

	def HPtestVector3i(self,N,HPn,prefix,MPn):
		a3i = HPn.Vector3i(2,1,4)
		b3i = HPn.Vector3i(3,5,5)
		c3i = a3i + b3i
		#self.assertEqual(c3i.eigenFlags(),352)
		#self.assertEqual(c3i.eigenStorageOrder(),0)

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c3i[0] , MPn.mpf("5") )
		self.checkRelativeError( c3i[1] , MPn.mpf("6") )
		self.checkRelativeError( c3i[2] , MPn.mpf("9") )

		c3i *= 3

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c3i[0] , MPn.mpf("15") )
		self.checkRelativeError( c3i[1] , MPn.mpf("18") )
		self.checkRelativeError( c3i[2] , MPn.mpf("27") )

		self.assertEqual( c3i , eval(prefix+c3i.__str__()) )

	def testVector3(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestVector3)

	def HPtestVector3(self,N,HPn,prefix,MPn):
		a3r = HPn.Vector3(2.1,1.1,4.3)
		b3r = HPn.Vector3(3.1,5.1,5.2)
		c3r = a3r + b3r
		#self.assertEqual(c3r.eigenFlags(),352)
		#self.assertEqual(c3r.eigenStorageOrder(),0)

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c3r[0] , MPn.mpf("5.2") )
		self.checkRelativeError( c3r[1] , MPn.mpf("6.2") )
		self.checkRelativeError( c3r[2] , MPn.mpf("9.5") )

		c3r *= 3

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c3r[0] , MPn.mpf("15.6") )
		self.checkRelativeError( c3r[1] , MPn.mpf("18.6") )
		self.checkRelativeError( c3r[2] , MPn.mpf("28.5") )

		self.checkRelativeError( c3r[0] , eval(prefix+c3r.__str__())[0] )
		self.checkRelativeError( c3r[1] , eval(prefix+c3r.__str__())[1] )
		self.checkRelativeError( c3r[2] , eval(prefix+c3r.__str__())[2] )

	def testVector3c(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestVector3c)

	def HPtestVector3c(self,N,HPn,prefix,MPn):
		a3c = HPn.Vector3c(2.1+1j,1.1+2.5j,4.3-1j)
		b3c = HPn.Vector3c(3.1,5.1,5.2)
		c3c = a3c + b3c
		#self.assertEqual(c3c.eigenFlags(),352)
		#self.assertEqual(c3c.eigenStorageOrder(),0)

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeComplexError( c3c[0] , MPn.mpc("5.2","1") )
		self.checkRelativeComplexError( c3c[1] , MPn.mpc("6.2","2.5") )
		self.checkRelativeComplexError( c3c[2] , MPn.mpc("9.5","-1") )

		c3c *= 3

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeComplexError( c3c[0] , MPn.mpc("15.6","3") )
		self.checkRelativeComplexError( c3c[1] , MPn.mpc("18.6","7.5") )
		self.checkRelativeComplexError( c3c[2] , MPn.mpc("28.5","-3") )

		self.checkRelativeComplexError( c3c[0] , eval(prefix+c3c.__str__().replace("mpc","MPn.mpc"))[0] )
		self.checkRelativeComplexError( c3c[1] , eval(prefix+c3c.__str__().replace("mpc","MPn.mpc"))[1] )
		self.checkRelativeComplexError( c3c[2] , eval(prefix+c3c.__str__().replace("mpc","MPn.mpc"))[2] )

	def testVector3na(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestVector3na)

	def HPtestVector3na(self,N,HPn,prefix,MPn):
		if((HPn.vectorize == False) or (not hasattr(HPn, 'Vector3na'))): return
		a3a = HPn.Vector3na(2.1,1.1,4.3)
		b3a = HPn.Vector3na(3.1,5.1,5.2)
		c3a = a3a + b3a
		#self.assertEqual(c3a.eigenFlags(),352)
		#self.assertEqual(c3a.eigenStorageOrder(),0)

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c3a[0] , MPn.mpf("5.2") )
		self.checkRelativeError( c3a[1] , MPn.mpf("6.2") )
		self.checkRelativeError( c3a[2] , MPn.mpf("9.5") )

		c3a *= 3

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c3a[0] , MPn.mpf("15.6") )
		self.checkRelativeError( c3a[1] , MPn.mpf("18.6") )
		self.checkRelativeError( c3a[2] , MPn.mpf("28.5") )

		self.checkRelativeError( c3a[0] , eval(prefix+c3a.__str__())[0] )
		self.checkRelativeError( c3a[1] , eval(prefix+c3a.__str__())[1] )
		self.checkRelativeError( c3a[2] , eval(prefix+c3a.__str__())[2] )

	def testVector4(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestVector4)

	def HPtestVector4(self,N,HPn,prefix,MPn):
		# The Vector4 bug was fixed only recently, don't test if there's nothing to test
		if(not hasattr(HPn, 'Vector4')): return
		a4r = HPn.Vector4(2.1,1.1,4.3,5.5)
		b4r = HPn.Vector4(3.1,5.1,5.2,-5.0)
		c4r = a4r + b4r
		#self.assertEqual(c4r.eigenFlags(),352)
		#self.assertEqual(c4r.eigenStorageOrder(),0)

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c4r[0] , MPn.mpf("5.2") )
		self.checkRelativeError( c4r[1] , MPn.mpf("6.2") )
		self.checkRelativeError( c4r[2] , MPn.mpf("9.5") )
		self.checkRelativeError( c4r[3] , MPn.mpf("0.5") )

		c4r *= 3

		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c4r[0] , MPn.mpf("15.6") )
		self.checkRelativeError( c4r[1] , MPn.mpf("18.6") )
		self.checkRelativeError( c4r[2] , MPn.mpf("28.5") )
		self.checkRelativeError( c4r[3] , MPn.mpf("1.5") )

		self.checkRelativeError( c4r[0] , eval(prefix+c4r.__str__())[0] )
		self.checkRelativeError( c4r[1] , eval(prefix+c4r.__str__())[1] )
		self.checkRelativeError( c4r[2] , eval(prefix+c4r.__str__())[2] )
		self.checkRelativeError( c4r[3] , eval(prefix+c4r.__str__())[3] )

	def testMatrix3Test(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestMatrix3Test)

	def HPtestMatrix3Test(self,N,HPn,prefix,MPn):
		a3m=HPn.Matrix3(1,2,3,
		                4,5,6,
		                7,8,9)
		#self.assertEqual(a3m.eigenFlags(),352)
		#self.assertEqual(a3m.eigenStorageOrder(),0)
		b3m=a3m.transpose()
		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( b3m[0][0] , MPn.mpf("1") )
		self.checkRelativeError( b3m[0][1] , MPn.mpf("4") )
		self.checkRelativeError( b3m[0][2] , MPn.mpf("7") )
		self.checkRelativeError( b3m[1][0] , MPn.mpf("2") )
		self.checkRelativeError( b3m[1][1] , MPn.mpf("5") )
		self.checkRelativeError( b3m[1][2] , MPn.mpf("8") )
		self.checkRelativeError( b3m[2][0] , MPn.mpf("3") )
		self.checkRelativeError( b3m[2][1] , MPn.mpf("6") )
		self.checkRelativeError( b3m[2][2] , MPn.mpf("9") )

		c3m=a3m.diagonal()
		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( c3m[0] , MPn.mpf("1") )
		self.checkRelativeError( c3m[1] , MPn.mpf("5") )
		self.checkRelativeError( c3m[2] , MPn.mpf("9") )

		self.checkRelativeError( a3m.maxAbsCoeff() , MPn.mpf("9") )

		for i in range(3):
			for j in range(3):
				self.checkRelativeError( b3m[i][j] , eval(prefix+b3m.__str__())[i][j] )
		#print(b3m.__str__())

	def testMatrix3cTest(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestMatrix3cTest)

	def HPtestMatrix3cTest(self,N,HPn,prefix,MPn):
		a3m=HPn.Matrix3c(1+1j,2,3,
		                4,5,6,
		                7,8,9-9j)
		#self.assertEqual(a3m.eigenFlags(),352)
		#self.assertEqual(a3m.eigenStorageOrder(),0)
		b3m=a3m.transpose()
		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeComplexError( b3m[0][0] , MPn.mpc("1","1") )
		self.checkRelativeComplexError( b3m[0][1] , MPn.mpc("4","0") )
		self.checkRelativeComplexError( b3m[0][2] , MPn.mpc("7","0") )
		self.checkRelativeComplexError( b3m[1][0] , MPn.mpc("2","0") )
		self.checkRelativeComplexError( b3m[1][1] , MPn.mpc("5","0") )
		self.checkRelativeComplexError( b3m[1][2] , MPn.mpc("8","0") )
		self.checkRelativeComplexError( b3m[2][0] , MPn.mpc("3","0") )
		self.checkRelativeComplexError( b3m[2][1] , MPn.mpc("6","0") )
		self.checkRelativeComplexError( b3m[2][2] , MPn.mpc("9","-9") )

		c3m=a3m.diagonal()
		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeComplexError( c3m[0] , MPn.mpc("1","1") )
		self.checkRelativeComplexError( c3m[1] , MPn.mpc("5","0") )
		self.checkRelativeComplexError( c3m[2] , MPn.mpc("9","-9") )

		self.checkRelativeComplexError( a3m.maxAbsCoeff() , abs(MPn.mpc("9","-9")) )

		for i in range(3):
			for j in range(3):
				self.checkRelativeComplexError( b3m[i][j] , eval(prefix+b3m.__str__().replace("mpc","MPn.mpc"))[i][j] )
		#print(b3m.__str__())


	def testQuaternion(self):
		for N in self.testLevelsHP:
			self.runCheck(N , self.HPtestQuaternion)

	def HPtestQuaternion(self,N,HPn,prefix,MPn):
		q1 = HPn.Quaternion.Identity
		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( q1[3] , MPn.mpf("1") )
		#self.assertEqual(q1.eigenFlags(),32)
		#self.assertEqual(q1.eigenStorageOrder(),0)

		q2 = q1.inverse()
		self.checkRelativeError( q2[3] , MPn.mpf("1") )
		if(self.needsMpmathAtN(N)):
			q3=HPn.Quaternion(axis=HPn.Vector3(1,0,0),angle=MPn.pi/2.0)
			q3a=HPn.Quaternion((HPn.Vector3(1,0,0),MPn.pi/2.0))
			q3b=HPn.Quaternion((MPn.pi/2.0,HPn.Vector3(1,0,0)))
		else:
			q3=HPn.Quaternion(axis=HPn.Vector3(1,0,0),angle=1.570796326794896619231321691639)
			q3a=HPn.Quaternion((HPn.Vector3(1,0,0),1.570796326794896619231321691639))
			q3b=HPn.Quaternion((1.570796326794896619231321691639,HPn.Vector3(1,0,0)))
		m3q=q3.toRotationMatrix()
		self.checkRelativeError( m3q[0][0] , MPn.mpf("1") )
		#print(m3q[1][2].__repr__())
		self.checkRelativeError( m3q[1][2] , MPn.mpf("-1") )
		#print(q3)
		self.assertEqual(MPn.mp.dps , self.digs1 )

		q4 = HPn.Quaternion.Identity
		q4.setFromTwoVectors(HPn.Vector3(1,2,3),HPn.Vector3(2,3,4))
		#print(q4.norm().__repr__())
		self.assertEqual(MPn.mp.dps , self.digs1 )
		self.checkRelativeError( q4.norm() , MPn.mpf("1") )

		for qq in (q3,q3a,q3b): # cpp_bin_float needs larger tolerance here.
			self.checkRelativeError( qq[0] , eval(prefix+q3.__str__())[0] , 1 if self.hasMpfr() else 50) # cpp_bin_float needs higher tolerance.
			self.checkRelativeError( qq[1] , eval(prefix+q3.__str__())[1] , 1 if self.hasMpfr() else 50)
			self.checkRelativeError( qq[2] , eval(prefix+q3.__str__())[2] , 1 if self.hasMpfr() else 50)
			self.checkRelativeError( qq[3] , eval(prefix+q3.__str__())[3] , 1 if self.hasMpfr() else 50)
			#print(q3.__str__())

