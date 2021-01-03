#!/usr/bin/python
# -*- coding: utf-8 -*-

from __future__ import print_function
############################################
##### the screenshot parameters       #####
############################################
from testGuiHelper import TestGUIHelper
# FIXME: it should deduce the name automatically, it's the end of the filename. See also testGui.sh
#        if you add a new file, you have to manually add it into scripts/checks-and-tests/gui/testGui.sh
#        or even better finally fix this FIXME, so that it works automatically
#        and just like scripts/checks-and-tests/checks/checkList.py is finding all the files to run.
scr = TestGUIHelper("Empty")

guiIterPeriod=500000

O.engines = O.engines+[PyRunner(iterPeriod=guiIterPeriod,command='scr.screenshotEngine()')]

#O.engines=[
#	ForceResetter()
#	, InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Box_Aabb()])
#	, InteractionLoop(
#		[Ig2_Sphere_Sphere_ScGeom6D(),Ig2_Box_Sphere_ScGeom6D()],	
#		[Ip2_CohFrictMat_CohFrictMat_CohFrictPhys()],
#		[law]
#	)
#	, GlobalStiffnessTimeStepper(active=1,timeStepUpdateInterval=50,timestepSafetyCoefficient=.0001)
#	, NewtonIntegrator(kinSplit=True,gravity=(0,0,-10))
#	, PyRunner(iterPeriod=guiIterPeriod,command='scr.screenshotEngine()')
#]

############################################################################################################################################
############################################################# test GUI #####################################################################
############################################################################################################################################
# here start changes of script simple-scene-energy-tracking.py, maybe later this duplication of code above can be removed.
# The code below, takes screenshot before and after each GUI action. And yade is hopefully not crashing in between.
# The test runs also on debug build, so anyway we should get a useful backtrace from gitlab-CI

O.dt = O.dt*0.0001
O.run(guiIterPeriod*scr.getTestNum()+1)

