# -*- coding: utf-8 -*-
# Check lubrication law with one sphere sedimenting one one other.

#from yade import plot
import math
import os

m_density   = 1000;
m_radius    = 0.01;
m_viscosity = 100;
m_roughness = 1e-2;
m_gravity   = -1;
m_tauc      = -m_viscosity/(m_radius*m_density*m_gravity); # Characteristic time.
m_tol       = 1e-5;


def check():        
        
        if O.interactions.has(1,0):
            if O.interactions[1,0].isReal:
                ph = O.interactions[1,0].phys;
                ue = ph.ue;
                u = ph.u;
                
                u_ = m_radius*math.exp(-8/9*O.time/m_tauc);
                ul = m_radius * m_roughness*2 + 4./3.*math.pi*m_radius**3*m_density*m_gravity/ph.kn;
                
                u_ = max(u_,ul); # Take roughness into account
                
                #plot.addData( t = O.time, u=u, u_=u_);
                
                if abs(u-u_) > m_tol:
                    raise YadeCheckError("The moving body should be at position %.5e and is at position %.5e"%(u_,u));

mat = O.materials.append(CohFrictMat(density=m_density,young=1e9,poisson=0.3,frictionAngle=radians(60)))

# add two spheres
O.bodies.append([
        sphere(center=(0,0,0),radius=m_radius,material=mat,fixed=True),
        sphere(center=(0.0,3*m_radius,0),radius=m_radius,material=mat)]);

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
                           [Ip2_FrictMat_FrictMat_LubricationPhys(eta=m_viscosity,eps=m_roughness)],
                           #[Ip2_ElastMat_ElastMat_LubricationPhys(eta=100,eps=0.)],
                           [law]),
           NewtonIntegrator(damping=0., gravity=(0,m_gravity,0),label="newton" ),
           GlobalStiffnessTimeStepper(active=0,timeStepUpdateInterval=100,timestepSafetyCoefficient=0.8, defaultDt=1e-10,label="TimeStepper",viscEl=False),
           PyRunner(command='check()',iterPeriod=10000)
           ]

O.dt = 1.e-5;
#plot.plots = {'t':('u','u_')};

O.run(100000, True)
