# Example of ymport.unv function and also export.VTKExporter.exportFacetsAsMesh
# Reads facets from shell.unv and saves them as one mesh to vtk file

from yade import ymport,export

f,n,ct = ymport.unv('shell.unv',returnConnectivityTable=True)
O.bodies.append(f)
vtk = export.VTKExporter('test')
vtk.exportFacetsAsMesh(connectivityTable=ct)
