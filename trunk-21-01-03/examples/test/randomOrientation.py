# viusally testing utils.randomOrientation function
n = 100
O.bodies.append([box((0,0,0),(10,1,.1),orientation=randomOrientation()) for _ in range(n)])
O.step()
yade.qt.View()
