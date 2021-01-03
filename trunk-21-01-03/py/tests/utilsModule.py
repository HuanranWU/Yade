
#import unittest
#import random
#from yade.wrapper import *
#from yade import utils

import unittest
import random,math
from yade.wrapper import *
from yade._customConverters import *
from yade import utils
from yade import *
from yade.minieigenHP import *


class TestUtils(unittest.TestCase):
	def setUp(self):
		pass # no setup needed for tests here
	def testUserCreatedInteraction(self):
		O.engines=[
				ForceResetter(),
				InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb(),Bo1_Box_Aabb()],label="collider"),
				InteractionLoop(
					[Ig2_Sphere_Sphere_ScGeom(),Ig2_Facet_Sphere_ScGeom(),Ig2_Box_Sphere_ScGeom()],
					[Ip2_FrictMat_FrictMat_FrictPhys()],	#for linear model only
					[Law2_ScGeom_FrictPhys_CundallStrack(label="law")],	#for linear model only
					label="interactionLoop"
				),
				GlobalStiffnessTimeStepper(timeStepUpdateInterval=10,label="timeStepper"),
				NewtonIntegrator(label="newton")
			]
		O.bodies.append([utils.sphere((0,0,0),0.5),
			utils.sphere((2,0,0),0.5), #(0,1) no overlap , no contacts
			utils.sphere((0.9,0.9,0),0.5), #(0,2) overlapping bounds, no contacts
			utils.sphere((-0.99,0,0),0.5)]) #(0,3) overlaping + contact
		O.dt=0
		O.dynDt=False
		O.step()
		i=utils.createInteraction(0,1)
		self.assert_(i.iterBorn==1 and i.iterMadeReal==1)
		j=utils.createInteraction(0,2)
		self.assert_(j.iterBorn==1 and j.iterMadeReal==1)
		self.assertRaises(RuntimeError,lambda: utils.createInteraction(0,3))
