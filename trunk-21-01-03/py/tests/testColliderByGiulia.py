# -*- coding: utf-8 -*-
# https://gitlab.com/yade-dev/trunk/issues/7
# https://answers.launchpad.net/yade/+question/220785
# https://bugs.launchpad.net/yade/+bug/1112763

import unittest

from yade.utils import *

# https://docs.python.org/3/library/unittest.html

class TestColliderByGiulia(unittest.TestCase):
	def setUp(self):
		self.wontFixIssue7 = [(-1.,True,2,0,0.),(1.,False,2,0,0.),(-1.,True,0,1,0.),(1.,False,0,1,0.),(-1.,True,1,2,0.),(1.,False,1,2,0.) ]
		self.skip_4200_iterations=True # =False # means that test will be more comprehensive and a bit slower.
		self.DbigSphere	= 1.
		self.cellSizes=[self.DbigSphere*1. , self.DbigSphere*3. , self.DbigSphere*3. ]
		self.radius_bigSphere= self.DbigSphere/2
		self.radius_sph	= self.DbigSphere/10
	def tearDown(self):
		# here finalize the tests, maybe close an opened file, or something else
		O.reset()
	def printOK(self,sign,ordering,axis,initialSortAxis,change):
		self.assertTrue(True)
		#axis = (2-axis)%3 # bacause the movement is along Z axis, while axis numbering referes to X. Better to print the axis which causes problems, than axis used in the loop numbering.
		#print("\033[92m OK, sign=",sign," ordering=",ordering," axis=",axis," initialSortAxis=",initialSortAxis,"\033[0m")
	def noteFail(self,sign,ordering,axis,initialSortAxis,change):
		axis = (2-axis)%3 # bacause the movement is along Z axis, while axis numbering referes to X. Better to print the axis which causes problems, than axis used in the loop numbering.
		if( (sign,ordering,axis,initialSortAxis,change) in self.wontFixIssue7):
			#print("\033[92m OK, sign=",sign," ordering=",ordering," axis=",axis," initialSortAxis=",initialSortAxis," change=",change,"\033[0m")
			self.assertTrue(True)
		else:
			print("\033[91m failure, sign=",sign," ordering=",ordering," axis=",axis," initialSortAxis=",initialSortAxis," change=",change,"\033[0m")
			self.assertTrue(False)
		self.assertTrue( (sign,ordering,axis,initialSortAxis,change) in self.wontFixIssue7)
	def testColiderRegressions(self):
		for initialSortAxis in [0,1,2]:
			for axis in [0,1,2]:
				for sign in [-1.0,1.0]:
					for ordering in [True,False]:
						for change in [1e-15,0]:
							O.reset()
							O.periodic  = True
							O.cell.setBox(Vector3(self.cellSizes[(0+axis)%3]+change,self.cellSizes[(1+axis)%3]+change,self.cellSizes[(2+axis)%3]+change))
							# Big blue sphere
							centerPos        =        [self.cellSizes[0]/2.,self.cellSizes[1]/2.,self.cellSizes[2]*(0.7 if sign>0 else 0.3)]
							center_bigSphere = Vector3(centerPos[(0+axis)%3],centerPos[(1+axis)%3],centerPos[(2+axis)%3])
							bigSphere        = sphere(center=center_bigSphere, radius=self.radius_bigSphere, color=(0,0,1))
							# Red small sphere
							centerSmall      =        [centerPos[0],	centerPos[1]+1./5.*self.DbigSphere,	centerPos[2]+sign*(-self.DbigSphere/2.-self.radius_sph)]
							center_sph	 = Vector3(centerSmall[(0+axis)%3],centerSmall[(1+axis)%3],centerSmall[(2+axis)%3])
							sphereSmall	 = sphere(center=center_sph, radius=self.radius_sph, color=(1,0,0))
							forceAxis        = [0,0,-sign*100]

							if(ordering==True):
								O.bodies.append((sphereSmall,bigSphere))
							else:
								O.bodies.append((bigSphere,sphereSmall))

							O.engines=[
								ForceResetter(),
								InsertionSortCollider ([Bo1_Sphere_Aabb()], allowBiggerThanPeriod=True),
								InteractionLoop ([Ig2_Sphere_Sphere_ScGeom()],[Ip2_FrictMat_FrictMat_MindlinPhys(),],[Law2_ScGeom_MindlinPhys_Mindlin()]),
								ForceEngine (force=(forceAxis[(0+axis)%3],forceAxis[(1+axis)%3],forceAxis[(2+axis)%3]),ids=[bigSphere.id]),
								NewtonIntegrator (damping=0.5),
							]
							O.dt=0.2*PWaveTimeStep()
							testedCollider=typedEngine("InsertionSortCollider")
							testedCollider.sortAxis=initialSortAxis

							# at O.iter==2 is appears first time and i.isReal == False, at O.iter==4240, i.isReal==True and Overlap 1.1326899061803175e-05
							O.run(2,True)
							#print("dumpBounds():",testedCollider.dumpBounds()[(2-axis)%3])
							try:
								i=O.interactions[bigSphere.id,sphereSmall.id]
								if((i.isReal == False) and (O.iter==2)):
									if(self.skip_4200_iterations):
										self.printOK(sign,ordering,axis,initialSortAxis,change)
									else:
										O.run(4240-3,True)
										wasReal = i.isReal
										O.run(1,True)
										if((wasReal==False) and (i.isReal == True) and (O.iter==4240) and (abs(O.interactions[bigSphere.id,sphereSmall.id].geom.penetrationDepth - 1.1326899061803175e-05) < 1e-15 )):
											self.printOK(sign,ordering,axis,initialSortAxis,change)
											#print('iter first interaction\tStep',O.iter,'\tOverlap',O.interactions[bigSphere.id,sphereSmall.id].geom.penetrationDepth)
										else:
											self.noteFail(sign,ordering,axis,initialSortAxis,change)
								else:
									self.noteFail(sign,ordering,axis,initialSortAxis,change)
							except Exception as e:
								self.noteFail(sign,ordering,axis,initialSortAxis,change)


