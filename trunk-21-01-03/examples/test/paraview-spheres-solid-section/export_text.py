maxCorner = (10,10,10)
center = (5,5,5)
normal = (1,1,1)

from yade import pack, export
pred = pack.inAlignedBox(Vector3.Zero,maxCorner)
O.bodies.append(pack.randomDensePack(pred,radius=1.,rRelFuzz=.5,spheresInCell=500))

export.text('/tmp/test.txt')
# text2vtk and text2vtkSection function can be copy-pasted from yade/py/export.py into separate py file to avoid executing yade or to use pure python
export.text2vtk('/tmp/test.txt','/tmp/test.vtk')
export.text2vtkSection('/tmp/test.txt','/tmp/testSection.vtk',point=center,normal=normal)

# now open paraview, click "Tools" menu -> "Python Shell", click "Run Script", choose "pv_section.py" from this directiory
# or just run python pv_section.py
# and enjoy :-)
