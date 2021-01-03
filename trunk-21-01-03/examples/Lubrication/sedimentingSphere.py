#!/usr/local/bin/yade-trunk -x
# -*- coding: utf-8 -*-
# encoding: utf-8

readParamsFromTable(roughness=0, eta=10, a=1.e-2, rho=1e3, g=10., young=5.e7, u0=1.)

from yade.params import table

from yade import plot
import math
import os

m_simuParams = {k: yade.params.table.__dict__[k] for k in table.__dict__['__all__']};
m_savefile = "data_sediment";

m_tau = table.eta/(table.a*table.rho*table.g);
m_tprim = m_tau + math.sqrt(m_tau**2 + 2.*table.u0*table.a/(table.g));

m_tmax = max(200.*m_tau, 10.*m_tprim);

i = 0;
while os.path.exists(m_savefile+'.'+str(i)+'.txt'):
    i=i+1;

m_savefile += '.'+str(i)+'.txt';
open(m_savefile, 'a').close(); # Create the empty file asap

def SaveAndQuit():
    print("Quit condition reach.");
    saveToFile();
    O.stopAtIter = O.iter+1;

def upGraph():
        
        norm = 0;
        tangContact = Vector3(0,0,0);
        tangLub = Vector3(0,0,0);
        normContact = 0;
        normLub = 0;
        ue = 0;
        u = 0;
        
        [nc, sc, nl, sl] =  Law2_ScGeom_ImplicitLubricationPhys.getStressForEachBody();
        
        if O.interactions.has(1,0):
            if O.interactions[1,0].isReal:
                g = O.interactions[1,0].geom;
                ph = O.interactions[1,0].phys;

                norm = ph.normalForce.dot(g.normal);
                normContact = ph.normalContactForce.dot(g.normal);
                normLub = ph.normalLubricationForce.dot(g.normal);
                tangContact = ph.shearContactForce;
                tangLub = ph.shearLubricationForce;
                ue = ph.ue;
                u = ph.u;
        
        plot.addData(
                Fn = norm,
                ue = ue,
                Fnc = normContact,
                Fnl = normLub,
                Fsl = tangLub,
                Fsc = tangContact,
                nc = nc[0],
                nl = nl[0],
                u = u,
                t = O.time,
                t2 = O.time,
                t3 = O.time,
                t4 = O.time)
        
        if O.time > m_tmax:
            SaveAndQuit();
		
def saveToFile():
    global m_savefile, m_simuParams;
    plot.saveDataTxt(m_savefile, headers=m_simuParams);

# ElectrostaticMat
mat = O.materials.append(CohFrictMat(density=table.rho,young=table.young,poisson=0.3,frictionAngle=radians(60)))

# add two spheres
O.bodies.append([
        sphere(center=(0,0,0),radius=table.a,material=mat,fixed=True),
        sphere(center=(0.0,(table.u0+2)*table.a,0),radius=table.a,material=mat)]);

law = Law2_ScGeom_ImplicitLubricationPhys(  activateTangencialLubrication=True,
                                            activateTwistLubrication=True,
                                            activateRollLubrication=True,
                                            theta=1.,
                                            resolution=2,
                                            MaxDist=50,
                                            SolutionTol=1.e-10,
                                            MaxIter=100);

O.engines=[ForceResetter(),
           InsertionSortCollider([Bo1_Sphere_Aabb(aabbEnlargeFactor=3)]),
           InteractionLoop([Ig2_Sphere_Sphere_ScGeom(interactionDetectionFactor=3)],
                           [Ip2_FrictMat_FrictMat_LubricationPhys(eta=table.eta,eps=table.roughness)],
                           #[Ip2_ElastMat_ElastMat_LubricationPhys(eta=100,eps=0.)],
                           [law]),
           NewtonIntegrator(damping=0., gravity=(0,-table.g,0),label="newton" ),
           GlobalStiffnessTimeStepper(active=0,timeStepUpdateInterval=100,timestepSafetyCoefficient=0.8, defaultDt=1e-10,label="TimeStepper",viscEl=False),
           
           PyRunner(command='upGraph()',virtPeriod=m_tau/10),
           PyRunner(command='saveToFile()',realPeriod=600),
           #PyRunner(command='trajectoire()',iterPeriod=1)
           ]

O.dt = 1.e-7;
#O.bodies[1].state.vel = (0,-1e-2,0);
#O.bodies[1].state.blockedDOFs = 'xyzXYZ';

plot.plots = {'t':('Fn','Fnc','Fnl'),'t3':('Fsc_x','Fsc_y', 'Fsl_x', 'Fsl_y'), 't2':('u','ue'), 't4':('nc_yy','nl_yy')};
plot.plot(subPlots=True);
O.saveTmp();
        
#O.run(1);
#O.run(long(0.302/O.dt));

if 'YADE_BATCH' in os.environ:
    O.run();
    waitIfBatch();
