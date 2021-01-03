#!/usr/bin/python
# -*- coding: utf-8 -*-
##########################################################################
#  2019        Janek Kozicki                                             #
#                                                                        #
#  This program is free software; it is licensed under the terms of the  #
#  GNU General Public License v2 or later. See file LICENSE for details. #
##########################################################################

import subprocess,time,os
import yade
from yade import qt

class TestGUIHelper:
	"""
	This simple class makes screenshots.
	"""
	def __init__(self,name=None):
		self.scrNum=0;
		# FIXME : this number 14 is hardcoded in scripts/checks-and-tests/gui/testGui.sh when testing if screenshots are present.
		self.maxTestNum=14
		if(name != None):
			self.name=name;
		else:
			self.name="";

	def getTestNum(self):
		return self.maxTestNum

	def createEmptyFile(self,path):
		with open(path, 'a'):
			os.utime(path, None)

	def finish(self):
#		sys.exit(0)
		os._exit(0)
#		ip = get_ipython()
#		ip.ask_exit()
#		ip.pt_cli.exit()
#		quit()

	def makeNextScreenshot(self):
		time.sleep(0.25)
		subprocess.run(["/usr/bin/scrot", "-z" , "scr"+"_"+self.name+"_"+str(self.scrNum).zfill(2)+".png"])
		time.sleep(0.25)

	def clickOnScreen(self,x,y,mouseButton=1):
		time.sleep(0.25)
		subprocess.run(["/usr/bin/xdotool", "mousemove" , str(x) , str(y) , "click" , str(mouseButton) , "mousemove" , "restore" ])
		time.sleep(0.25)

	def screenshotEngine(self):
		self.scrNum += 1
		intro = "Test '"+self.name+"', stage:"+str(self.scrNum)+" iter:"+str(O.iter)
		if(self.scrNum == 1):
			self.makeNextScreenshot();
			print(intro+" moving yade.qt.Controller()")
			yade.qt.Controller()
			yade.qt.controller.setWindowTitle('GUI test: '+self.name)
			yade.qt.controller.setGeometry(550,20,500,1100)
		if(self.scrNum == 2):
			self.makeNextScreenshot();
			print(intro+" opening yade.qt.View()")
			yade.qt.View();
			vv=yade.qt.views()[0]
			vv.axes=True
			vv.eyePosition=(2.8717367257539266,-3.2498802823394684,11.986065750108025)
			vv.upVector  =(-0.786154130840315,-0.2651054185084529,0.558283798475441)
			vv.center()
		if(self.scrNum == 3):
			self.makeNextScreenshot();
			print(intro+" opening yade.qt.Inspector() , setting wire=True, setting intrGeom=True")
			self.clickOnScreen(982,60)
			success=False
			for attempt in range(0,20):
				try:
					yade.qt.controller.inspector.setGeometry(1050,20,500,1100)
					success=True
				except:
					print("waiting for Inspector window to open after mouse click.")
					time.sleep(0.25)
			if(not success):
				self.createEmptyFile("screenshots/mouse_click_the_Inspector_open_FAILED_"+self.name)
				print("*** ERROR: self.clickOnScreen failed to open Inspector window ***")
				self.finish();
			yade.qt.controller.inspector.show()
			qt.Renderer().wire=True
			qt.Renderer().intrGeom=True
		if(self.scrNum == 4):
			self.makeNextScreenshot();
			print(intro+" changing tab to bodies, setting intrPhys=True, setting bound=True")
			self.clickOnScreen(1148,26)
			#Previously these were used. They work, but do not always trigger crashes.
			# yade.qt.controller.inspector.close()
			# yade.qt.controller.inspector.setGeometry(1050,20,500,1100)
			# yade.qt.controller.inspector.tabWidget.setCurrentIndex(2)
			# yade.qt.controller.inspector.show()
			qt.Renderer().intrPhys=True
			qt.Renderer().bound=True
		if(self.scrNum == 5):
			self.makeNextScreenshot();
			print(intro+" clicking on interaction, setting wire=False, setting intrWire=True")
			self.clickOnScreen(1494,55)
			qt.Renderer().wire=False
			qt.Renderer().intrWire=True
		if(self.scrNum == 6):
			self.makeNextScreenshot();
			print(intro+" changing tab to interactions, setting wire=False, setting intrWire=True")
			self.clickOnScreen(1234,26)
			qt.Renderer().wire=False
			qt.Renderer().intrWire=True
		if(self.scrNum == 7):
			self.makeNextScreenshot();
			print(intro+" changing tab to cell, setting intrPhys=False")
			self.clickOnScreen(1312,26)
			qt.Renderer().intrPhys=False
		if(self.scrNum == 8):
			self.makeNextScreenshot();
			print(intro+" changing tab to bodies, setting intrWire=False, setting intrGeom=False, setting intrPhys=True")
			self.clickOnScreen(1148,26)
			qt.Renderer().intrWire=False
			qt.Renderer().intrGeom=False
			qt.Renderer().intrPhys=True
		if(self.scrNum == 9):
			self.makeNextScreenshot();
			print(intro+" changing tab to display, setting intrAllWire=True")
			self.clickOnScreen(668,26)
			#yade.qt.controller.setTabActive('display')
			qt.Renderer().intrAllWire=True
		if(self.scrNum == 10):
			self.makeNextScreenshot();
			print(intro+" changing tab to generator, setting intrGeom=True")
			self.clickOnScreen(744,26)
			#yade.qt.controller.setTabActive('generator')
			qt.Renderer().intrGeom=True
		if(self.scrNum == 11):
			self.makeNextScreenshot();
			print(intro+" changing tab to python, setting intrAllWire=False")
			self.clickOnScreen(821,26)
			#yade.qt.controller.setTabActive('python')
			qt.Renderer().intrAllWire=False
		if(self.scrNum == 12):
			self.makeNextScreenshot();
			print(intro+" changing tab to simulation")
			self.clickOnScreen(580,26)
			#yade.qt.controller.setTabActive('simulation')
		if(self.scrNum == 13):
			print(intro+" (testing of matplotlib is skipped for now...)")
			self.makeNextScreenshot();
			# FIXME: I couldn't get matplotlib to draw the plot, while screenshotting is going on.
			# self.makeNextScreenshot();
			# O.pause()
			# print(intro+" opening yade.plot.plot()")
			# plot.liveInterval=0
			# plot.plot(subPlots=False)
			# fig=yade.plot.plot();
			# time.sleep(5)
			# matplotlib.pyplot.draw()
			# time.sleep(5)
			# self.makeNextScreenshot();
			# matplotlib.pyplot.close(fig)
		if(self.scrNum == 14):
			self.makeNextScreenshot();
			print(intro+" exiting\n\n")
			O.pause()
			vv=yade.qt.views()[0]
			vv.close()
			yade.qt.controller.inspector.close()
			yade.qt.controller.close()
			self.finish()

