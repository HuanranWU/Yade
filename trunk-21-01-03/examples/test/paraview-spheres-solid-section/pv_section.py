#!/usr/bin/pvpython
# Script taken from paraview's "python trace", with slighly renaming and deleting unnecessary code
# To be run by pvpython or Paraview, not Yade (!)
# In paraview: Tools -> Python shell, then click Run Script and choose this file

# should correspond to the values in export_text.py
from __future__ import print_function
center = (5,5,5)
normal = (1,1,1)

maxRadius = 1.5

from paraview.simple import *

view = GetActiveViewOrCreate('RenderView')

testvtk = LegacyVTKReader(FileNames=['/tmp/test.vtk'],guiName="test.vtk")

glyph1 = Glyph(Input=testvtk, GlyphType='Sphere', Scalars=['POINTS','radius'])
glyph1.GlyphType.Radius = 1.0
glyph1.GlyphType.ThetaResolution = 16
glyph1.GlyphType.PhiResolution = 16
glyph1.ScaleMode = 'scalar'
glyph1.ScaleFactor = 1.0
glyph1.GlyphMode = 'All Points'

clip = Clip(Input=glyph1)
clip.ClipType.Origin = center
clip.ClipType.Normal = normal

testSectionvtk = LegacyVTKReader(FileNames=['/tmp/testSection.vtk'], guiName="testSection.vtk" )

sections = Glyph(Input=testSectionvtk, GlyphType='Cylinder', Vectors=['POINTS','normal'], Scalars=['POINTS','radius'])
sections.GlyphType.Height = 0.01
sections.GlyphType.Radius = 1.0
sections.GlyphType.Resolution = 16
sections.GlyphTransform.Rotate = [0.0, 0.0, -90.0]
sections.ScaleMode = 'scalar'
sections.ScaleFactor = 1.0
sections.GlyphMode = 'All Points'

clipDisplay = Show(clip, view)
ColorBy(clipDisplay, ('POINTS', 'radius'))
radiusLUT = GetColorTransferFunction('radius')
radiusLUT.RescaleTransferFunction(0.0, maxRadius)

# either this:
#   HideScalarBarIfNotNeeded(radiusLUT, view)
# or this command:
#   clipDisplay.SetScalarBarVisibility(view, False)
# depending on paraview version. Can we chack paraview version inside script? Maybe https://public.kitware.com/pipermail/paraview/2014-May/031180.html

sectionsDisplay = Show(sections, view)
ColorBy(sectionsDisplay, ('POINTS', 'radiusOrig'))
radiusOrigLUT = GetColorTransferFunction('radiusOrig')
radiusOrigLUT.RescaleTransferFunction(0.0, maxRadius)

# either this:
#   HideScalarBarIfNotNeeded(radiusOrigLUT, view)
# or this command:
#   sectionsDisplay.SetScalarBarVisibility(view, False)
# depending on paraview version. Can we chack paraview version inside script? Maybe https://public.kitware.com/pipermail/paraview/2014-May/031180.html

SetActiveSource(sections)

view.ResetCamera()
view.OrientationAxesVisibility = False
view.Background = [1.0, 1.0, 1.0]
view.ViewSize = [600,600]
view.CameraFocalPoint = center
view.CameraPosition = [-10,12,-16]
view.CameraViewUp = [1,0,0]
view.CameraParallelScale = 0

RenderAllViews()
out = "/tmp/test.png"
SaveScreenshot(out)
print("Screenshot saved to {}".format(out))
