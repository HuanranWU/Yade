#!/usr/local/bin/yade-trunk -x
# -*- coding: utf-8 -*-
# encoding: utf-8

readParamsFromTable(roughness=1.e-2*0, eta=100., a=1.e-3, rho=1e5, young=1.e9, shearRate=30)

from yade.params import table

from yade import plot
import math
import os

m_simuParams = {k: yade.params.table.__dict__[k] for k in table.__dict__['__all__']};
m_savefile = "data_shearTwoSpheres";

mul = 100;
dist = 0.05;

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
                t4 = O.time,
                pos0 = O.bodies[0].state.pos,
                pos1 = O.bodies[1].state.pos)

        if(O.bodies[1].state.pos[0] - O.bodies[0].state.pos[0] > 6.*table.a):
            SaveAndQuit();

       

def saveToFile():
    global m_savefile, m_simuParams;
    plot.saveDataTxt(m_savefile, headers=m_simuParams);
    


O.periodic=True
O.cell.hSize=Matrix3(mul*table.a, 0, 0,
                        0 ,mul*table.a, 0,
                        0, 0, mul*table.a);

def stockesFlow(b):
    global mul;
    
    y0 = mul/2.*table.a;
    dy = (b.state.pos[1]%(mul*table.a)-y0);
    
    
    v0 = Vector3(dy*table.shearRate,0.,0.);
    dv = b.state.vel-v0;
    
    #print("%f, %f, %f, %f"%(y0,dy,v0,dv));
    
    O.forces.setPermF(b.id, -6.*math.pi*table.eta*table.a*dv);
    O.forces.setPermT(b.id, -8.*math.pi*table.eta*table.a**3*(b.state.angVel*(2.*math.pi)-Vector3(0.,0.,-0.5*table.shearRate)));
    
def applyStockesFlow():
    stockesFlow(O.bodies[0]);
    stockesFlow(O.bodies[1]);

# ElectrostaticMat
mat = O.materials.append(CohFrictMat(density=table.rho,young=table.young,poisson=0.3,frictionAngle=radians(60)))

# add two spheres
O.bodies.append([
        sphere(center=(mul/2.*table.a,(mul/2)*table.a,mul/2.*table.a),radius=table.a,material=mat),
        sphere(center=((mul/2.-6)*table.a,(mul/2.+dist)*table.a,mul/2.*table.a),radius=table.a,material=mat)]);

law = Law2_ScGeom_ImplicitLubricationPhys(  activateTangencialLubrication=True,
                                            activateTwistLubrication=True,
                                            activateRollLubrication=True,
                                            theta=1.,
                                            resolution=2,
                                            MaxDist=4.0,
                                            SolutionTol=1.e-9,
                                            MaxIter=100);

O.engines=[ForceResetter(),
           InsertionSortCollider([Bo1_Sphere_Aabb(aabbEnlargeFactor=10)]),
           InteractionLoop([Ig2_Sphere_Sphere_ScGeom(interactionDetectionFactor=10)],
                           [Ip2_FrictMat_FrictMat_LubricationPhys(eta=table.eta,eps=table.roughness)],
                           #[Ip2_ElastMat_ElastMat_LubricationPhys(eta=100,eps=0.)],
                           [law]),
           NewtonIntegrator(damping=0., gravity=(0,0,0),label="newton" ),
           GlobalStiffnessTimeStepper(active=1,timeStepUpdateInterval=100,timestepSafetyCoefficient=0.5, defaultDt=1.e-5, maxDt=1.e-5, label="TimeStepper",viscEl=False),
           
           PyRunner(command='upGraph()',virtPeriod=0.01),
           PyRunner(command='saveToFile()',realPeriod=600),
           PyRunner(command='applyStockesFlow()',iterPeriod=1)
           ]
           
#O.bodies[0].state.blockedDOFs = 'xyzXYZ';
#O.bodies[1].state.blockedDOFs = 'XYZ';

plot.plots = {'t':('Fn','Fnc','Fnl'),'t2':('u','ue'), 'pos1_x':('pos1_y')};
plot.plot(subPlots=True);
O.bodies[0].state.angVel = (0,0.,-0.5*table.shearRate/(2*math.pi));
O.bodies[1].state.angVel = (0,0.,-0.5*table.shearRate/(2*math.pi));
O.saveTmp();

if 'YADE_BATCH' in os.environ :
    for i in range(6,-20,-1):
        d = 10.**(i/10);
        for theta in range(5,91,5):
            O.bodies[1].state.vel = (d*sin(theta)*table.shearRate*table.a,0.,0.);
            O.bodies[1].state.pos = Vector3((mul/2.-6)*table.a,(mul/2.+d*sin(theta))*table.a,(mul/2+d*cos(theta))*table.a);
            O.run(0, True);
            O.loadTmp();
