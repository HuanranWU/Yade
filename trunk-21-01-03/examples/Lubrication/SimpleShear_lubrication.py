#!/usr/bin/python
# -*- coding: utf-8 -*-

# This script perform a simple shear experiment with lubrication law.
# It shows the use of
# - Lubrication law
# - PDFEngine
# - VTK Lubrication recorder

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

sp=pack.SpherePack();

m_savefile = "data.txt";
m_vtkDir = "vtk";
if not os.path.exists(m_vtkDir):
    os.makedirs(m_vtkDir);


# Physics parameters
m_young             = 1e9; #Pa
m_friction          = atan(0.5); #rad
m_viscosity         = 100; #Pa.s
m_sphRadius         = 0.1; # m
m_epsilon           = 1e-3;

# Simulation
m_shearRate         = 10;
m_N                 = 500; # Number of spheres
m_targetVolfrac     = 0.5;
m_stopOnStrain      = 100;

# Saving
m_sampling          = 100.; # Number of sampling while 1 deformation
m_vtkSampling       = 10.; # Number of sampling while 1 deformation

#define material for all bodies:
id_Mat=O.materials.append(FrictMat(young=m_young,poisson=0.3,density=1000,frictionAngle=m_friction))
Mat=O.materials[id_Mat]

# Simulation cell
sq = (4./3.*pi*m_N/m_targetVolfrac)**(1./3.);

width   = sq*m_sphRadius;
length  = sq*m_sphRadius;
height  = sq*m_sphRadius;

O.periodic=True
O.cell.hSize=Matrix3(   1*width, 0, 0,
                        0 ,5*height, 0,
                        0, 0, 1*length);

# Sphere pack
No = sp.makeCloud((0,0,0),O.cell.size,m_sphRadius,0.05,m_N,periodic=True,seed=1)
spheres = [utils.sphere(s[0],s[1]) for s in sp];
O.bodies.append(spheres)

# Setup interaction law
law = Law2_ScGeom_ImplicitLubricationPhys(  activateTangencialLubrication=True,
                                            activateTwistLubrication=True,
                                            activateRollLubrication=True,
                                            resolution = 2,
                                            theta = 1,
                                            SolutionTol = 1.e-8,
                                            MaxIter = 50);

# Setup engines
O.engines = [ForceResetter(),
    InsertionSortCollider([Bo1_Sphere_Aabb(aabbEnlargeFactor=2., label="aabb")],verletDist=-0.2,allowBiggerThanPeriod=False),
    InteractionLoop(
        [Ig2_Sphere_Sphere_ScGeom6D(interactionDetectionFactor=2.,label="Ig2")],
        [Ip2_FrictMat_FrictMat_LubricationPhys(eta=m_viscosity,eps=m_epsilon)],
        [law]
    ),
    NewtonIntegrator(damping=0.),
    GlobalStiffnessTimeStepper(active=1,timeStepUpdateInterval=100,timestepSafetyCoefficient=0.8, defaultDt=1e-6,label="TimeStepper",viscEl=False),
    PDFEngine(filename="PDF.txt", virtPeriod=1./(m_vtkSampling*m_shearRate), numDiscretizeAnglePhi=9,numDiscretizeAngleTheta=13),
    VTKRecorder(fileName=m_vtkDir+'/',recorders=['spheres','velocity','lubrication'], virtPeriod=1./(m_vtkSampling*m_shearRate),label="VtkRecorder"),
    PyRunner(command="UpPlot()",virtPeriod=min(1./(m_sampling*m_shearRate), 0.1), label="UpdatePlot"),
    PyRunner(command="SavePlot()",realPeriod=600,label="SaveDataPlot"),
    PyRunner(command="checkStartShear()", iterPeriod=10, label="beginCheck")
    ];

plot.plots={'time':('totalStress_yy',
                    'normalContactStress_yy',
                    'shearContactStress_yy',
                    'normalLubrifStress_yy',
                    'shearLubrifStress_yy',
                    'kineticStress_yy'),
            'time2':('phi'),
            'time3':('totalStress_xy',
                     'normalContactStress_xy',
                     'shearContactStress_xy',
                     'normalLubrifStress_xy',
                     'shearLubrifStress_xy',
                     'kineticStress_xy')};

plot.plot(subPlots=True);

O.dt = 1e-6;

# Firstly, compress to target volumic fraction
O.cell.velGrad = Matrix3(0,0,0, 0,-10,0, 0,0,0);


def SavePlot():
    global m_savefile;
    plot.saveDataTxt(m_savefile);

def UpPlot():
        global m_stopOnStrain;
        [normalContactStress, shearContactStress, normalLubrifStress, shearLubrifStress, potentialStress] = Law2_ScGeom_ImplicitLubricationPhys.getTotalStresses();
        kineticStress = getTotalDynamicStress();
        
        totalStress = normalContactStress + shearContactStress + normalLubrifStress + shearLubrifStress + potentialStress + kineticStress;

        phi = 1.-porosity();
        
        if abs(O.cell.hSize[0,1]/O.cell.hSize[0,0]) > 1:
            flipCell();
            
        plot.addData(
                totalStress = totalStress,
                totalStress2 = getStress(),
                kineticStress = kineticStress,
                normalContactStress = normalContactStress,
                shearContactStress = shearContactStress,
                normalLubrifStress = normalLubrifStress,
                shearLubrifStress = shearLubrifStress,
                potentialStress = potentialStress,
                phi = phi,
                iter = O.iter,
                strain = O.cell.trsf,
                time = O.time,
                time2 = O.time,
                time3 = O.time,
                velGrad = O.cell.velGrad);

        if (m_stopOnStrain > 0) & (O.cell.trsf[0,1] > m_stopOnStrain):
            SaveAndQuit();

def checkStartShear():
    global m_shearRate;
    phi = 1. - porosity();
    start = m_targetVolfrac < phi;
        
    if start:
        print("Start shear.");
        O.cell.velGrad = Matrix3(0,m_shearRate, 0, 0,0,0, 0,0,0);
        O.cell.trsf = Matrix3(1,0,0, 0,1,0, 0,0,1);
        beginCheck.dead = 1;
       
def SaveAndQuit():
    print("Quit condition reach.");
    SavePlot();
    O.stopAtIter = O.iter+1;
