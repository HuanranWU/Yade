#!/usr/bin/python
# -*- coding: utf-8 -*-

# Read parameters from a table, or assign default values
readParamsFromTable(young=1.e8, friction=0.5, viscosity=10, roughness=7.e-4,numberOfGrains=10000, savedir=".",dv_xy=0,dv_xz=10, g=-50, dRho=1000)

from yade.params import table

from yade import pack,ymport,export,geom,bodiesHandling, plot
import math
import pylab
#from yade import qt
import numpy as np
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
import datetime as DT
import os
import re

# Prepare python dict to save parameters in output file
m_simuParams = {k: yade.params.table.__dict__[k] for k in table.__dict__['__all__']}
m_simuParams['date_start'] = DT.datetime.now().isoformat();

# Prepare output destinations
m_savefile  = table.savedir+'/results/data';

if not os.path.exists(table.savedir+'/results/'):
    os.makedirs(table.savedir+'/results/');

i = 0;
while os.path.exists(m_savefile+'.'+str(i)+'.txt'):
    i=i+1;

m_savefile += '.'+str(i)+'.txt';
open(m_savefile, 'a').close(); # Create the empty file asap

m_vtkDir = table.savedir+'/results/vtk.'+str(i)+'/';

if not os.path.exists(m_vtkDir):
    os.makedirs(m_vtkDir);

sp=pack.SpherePack();
launchDT = DT.datetime.now().isoformat();

# Physics parameters
m_young             = table.young; #Pa
m_friction          = atan(table.friction); #rad
m_viscosity         = table.viscosity; #Pa.s
m_sphRadius         = 1e-2; # m
m_epsilon           = table.roughness;

# Simulation
m_N                 = table.numberOfGrains; # Number of spheres
m_savePeriod        = 1./max(abs(table.dv_xy),max(abs(table.dv_xz),1.));

m_simuParams['a'] = m_sphRadius;

#define material for all bodies:
id_Mat=O.materials.append(FrictMat(young=m_young,poisson=0.3,density=table.dRho,frictionAngle=m_friction))
Mat=O.materials[id_Mat]

# Simulation cell
sq = math.floor((table.numberOfGrains*20)**(1./3.))

width   = sq*m_sphRadius;
length  = sq*m_sphRadius;
height  = 10.*sq*m_sphRadius;

m_simuParams['A'] = width*length;

O.periodic=True
O.cell.hSize=Matrix3(   width, 0, 0,
                        0 ,height, 0,
                        0, 0, length);

# Shearing
O.cell.velGrad = Matrix3(0,table.dv_xy,table.dv_xz,
                         0,0,0,
                         0,0,0);

# Sedimentation time
tss = (-2.*height/table.g)**(1./2.);

# Lower plate (break periodicity)
sphs = [utils.sphere(radius=m_sphRadius,center=m_sphRadius*Vector3(x,random.uniform(-0.5,0.5),z),fixed=True,mask=3) for x in range(0,sq+2) for z in range(0,sq+2)];
O.bodies.append(sphs);

# Generate sphere cloud
No = sp.makeCloud((0,5*m_sphRadius,0),O.cell.size-Vector3(0,5*m_sphRadius,0),m_sphRadius,0.1,m_N,periodic=True,seed=1)
spheres = [utils.sphere(s[0],s[1],mask=2) for s in sp];
O.bodies.append(spheres)

# Simulations engines
O.engines = [ForceResetter(),
    InsertionSortCollider([Bo1_Sphere_Aabb(aabbEnlargeFactor=2., label="aabb")],
                          verletDist=-1,
                          allowBiggerThanPeriod=False,
                          label="collider"),
    InteractionLoop(
        [Ig2_Sphere_Sphere_ScGeom6D(interactionDetectionFactor=2.,label="Ig2")],
        [Ip2_FrictMat_FrictMat_LubricationPhys(eta=m_viscosity,eps=m_epsilon)],
        [Law2_ScGeom_ImplicitLubricationPhys(   activateTangencialLubrication=True,
                                                activateTwistLubrication=True,
                                                activateRollLubrication=True,
                                                resolution = 2,
                                                theta = 1,
                                                SolutionTol = 1.e-8,
                                                MaxIter = 50,
                                                MaxDist = 1.)] # Use lubrication law (suspension)
    ),
    LinearDragEngine(nu=m_viscosity, dead=1,label="drag"),
    NewtonIntegrator(damping=0., gravity=(0,table.g,0),label="newton"),
    GlobalStiffnessTimeStepper(active=1,timeStepUpdateInterval=100,timestepSafetyCoefficient=0.8, defaultDt=1e-6,label="TimeStepper",viscEl=False),
    VTKRecorder(fileName=m_vtkDir,recorders=['spheres', 'velocity','lubrication','mask'], virtPeriod=m_savePeriod,label="VtkRecorder"),
    PyRunner(command="upPlot()",virtPeriod=m_savePeriod),
    PyRunner(command="SavePlot()",realPeriod=600),
    PyRunner(command="CheckFlip()",virtPeriod=1./(10*max(table.dv_xy, max(table.dv_xz,1.)))), # Very important: Check if cell must be flipped
    PyRunner(command="drag.ids=[b.id for b in O.bodies if len(O.interactions.withBody(b.id)) < 2 and b.mask == 2]", iterPeriod=1000), # Maybe important: apply drag for particles with less than 2 interactions: prevent free fall on the interface
    PyRunner(command="drag.dead=0;", nDo=2, virtPeriod=2*tss),
    ];
        
collider.avoidSelfInteractionMask=1; # Do not collide spheres on the bottom plane

O.dt = 1e-5;

# Plot will show height isovalues
plot.plots={'time':('y0','y50','y80','y95','ym')};
plot.plot();

# Enhanced flipping
def CheckFlip():
    
    f_xy = 0;
    f_xz = 0;
    
    if abs(O.cell.hSize[0,2]/O.cell.hSize[0,0]) > 1:
        f_xz = -ceil(abs(O.cell.hSize[0,2]/O.cell.hSize[0,0]))*O.cell.hSize[0,2]/abs(O.cell.hSize[0,2]);
        
    if abs(O.cell.hSize[0,1]/O.cell.hSize[0,0]) > 1:
        f_xy = -ceil(abs(O.cell.hSize[0,1]/O.cell.hSize[0,0]))*O.cell.hSize[0,1]/abs(O.cell.hSize[0,1]);
    
    if f_xy != 0 or f_xz != 0:
        flipCell(Matrix3(0,f_xy,f_xz,
                        0,0,0,
                        0,0,0));

# Update plot
def upPlot():
    yBodies = [b.state.pos[1] for b in O.bodies if b.mask == 2];
    yBodies.sort();
    
    
    plot.addData(iter=O.iter,
                 time = O.time,
                 strain = O.cell.trsf,
                 y0 = yBodies[0],
                 y50 = yBodies[math.floor(0.5*len(yBodies))],
                 y80 = yBodies[math.floor(0.8*len(yBodies))],
                 y95 = yBodies[math.floor(0.95*len(yBodies))],
                 ym = yBodies[-1]);

# Save plot (not too often to preserve performances)
def SavePlot():
    global m_savefile, m_simuParams;
    plot.saveDataTxt(m_savefile,headers=m_simuParams);

# Call this function to quit
def SaveAndQuit():
    print("Quit condition reach.");
    SavePlot();
    O.stopAtIter = O.iter+1;

# Usefull ifbatch
if 'YADE_BATCH' in os.environ:
    O.run();
    waitIfBatch();
                        
