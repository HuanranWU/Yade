#!/usr/local/bin/yade-trunk -x
# -*- coding: utf-8 -*-
# encoding: utf-8

from yade import plot
import math
import os

m_savefile = "data.txt";

# Simulation parameters
m_density   = 1000;
m_gravity   = -10;
m_radius    = 0.01;
m_viscosity = 100;
m_roughness = 1e-3;
m_young     = 1e9;

# This is an exemple of potential defined from python side

class CundallStrackForcePotential(GenericPotential):
    def contactForce(self, u, a):
         if u < a*table.roughness:
             return 10000*(u - a*table.roughness);
         else:
             return 0;
    def potentialForce(self, u, a):
        return 0;
    def hasContact(self, u, a):
        return u < a*table.roughness;

# This is the linear-exponential potential form
e = LinExponentialPotential();
e.computeParametersFromF0Fe(F0=0,Fe=-2, xe=.1)

def SaveAndQuit():
    print("Quit condition reach.");
    saveToFile();
    O.stopAtIter = O.iter+1;

def upGraph():
        
        #[nc, sc, nl, sl, np] =  Law2_ScGeom_ImplicitLubricationPhys.getStressForEachBody();
        
        if O.interactions.has(1,0):
            if O.interactions[1,0].isReal:
                ph = O.interactions[1,0].phys;
        
                plot.addData(
                        Fn = ph.normalForce,
                        Fnc = ph.normalContactForce,
                        Fnl = ph.normalLubricationForce,
                        Fnp = ph.normalPotentialForce,
                        u = ph.u,
                        ue = ph.ue,
                        t = O.time,
                        t2 = O.time)
		
def saveToFile():
    global m_savefile;
    plot.saveDataTxt(m_savefile);

mat = O.materials.append(CohFrictMat(density=m_density,young=m_young,poisson=0.3,frictionAngle=radians(60)))

# add two spheres
O.bodies.append([
        sphere(center=(0,0,0),radius=m_radius,material=mat,fixed=True),
        sphere(center=(0.0,3*m_radius,0),radius=m_radius,material=mat)]);

law = Law2_ScGeom_PotentialLubricationPhys(activateTangencialLubrication=True,
                                            activateTwistLubrication=True,
                                            activateRollLubrication=True,
                                            MaxDist=50,
                                            SolutionTol=1.e-10,
                                            MaxIter=100,
                                            potential=e);

O.engines=[ForceResetter(),
           InsertionSortCollider([Bo1_Sphere_Aabb(aabbEnlargeFactor=3)]),
           InteractionLoop([Ig2_Sphere_Sphere_ScGeom(interactionDetectionFactor=3)],
                           [Ip2_FrictMat_FrictMat_LubricationPhys(eta=m_viscosity,eps=m_roughness)],
                           #[Ip2_ElastMat_ElastMat_LubricationPhys(eta=100,eps=0.)],
                           [law]),
           NewtonIntegrator(damping=0., gravity=(0,m_gravity,0),label="newton" ),
           GlobalStiffnessTimeStepper(active=0,timeStepUpdateInterval=100,timestepSafetyCoefficient=0.8, defaultDt=1e-10,label="TimeStepper",viscEl=False),
           PyRunner(command='upGraph()',iterPeriod=10000),
           PyRunner(command='saveToFile()',realPeriod=600)
           ]

O.dt = 1.e-6;

plot.plots = {'u':('Fn_y','Fnc_y','Fnl_y','Fnp_y'),'t2':('u','ue')};
plot.plot(subPlots=True);
