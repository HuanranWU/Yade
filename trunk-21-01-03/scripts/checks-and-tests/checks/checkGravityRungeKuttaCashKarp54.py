# -*- coding: utf-8

# 3 Spheres have an initial velociries: 0, +5, -5
# Their positions and velocities are checked during free fall
# Checks the correctness of RungeKuttaCashKarp54Integrator and GravityEngine
# Also see checkGravity.py

## Omega
from __future__ import print_function
o=Omega() 

## PhysicalParameters 
# In present test these parameters are in fact not used, because the spheres are only falling down.
# However for good measure let us use the full precision Real here also.
from yade.math import toHP1, radiansHP1

Density       = toHP1(2400)
frictionAngle = radiansHP1(35)
sphereRadius  = toHP1('0.05')
tc            = toHP1('0.001')
en            = toHP1('0.3')
et            = toHP1('0.3')

O.dt          = 2*tc

sphereMat=O.materials.append(ViscElMat(density=Density,frictionAngle=frictionAngle,tc=tc,en=en,et=et))


v_down    = toHP1(-5)
v_up      = toHP1( 5)

# Important  !!!! when using yade high-precision be careful because any *quickly* *typed* ad-hoc floating point number can "destroy" the calculations.
# Important  !!!! For example in yade-mpfr150 declaring      g = -9.81 in fact produces number which is not useful to
#            ↓↓↓↓ do a simulation with 150 decimal places :  g = -9.8100000000000004973799150320701301097869873046875
#            ↓↓↓↓ make sure to use, yade.math.toHP1(………), so that the numbers on python side use native yade rpecision.
g         = yade.math.toHP1("-9.81")

tolerance = 10**(-yade.math.getDigits10(1)+3)
print("checkGravityRungeKuttaCashKarp54.py : yade precision is ",yade.math.getDigits10(1)," decimal places. Will use error tolerance of: ", tolerance)

print("Note: for high precision calculations use yade.math.toHP1(...) see this:")
print("g = yade.math.toHP1(-9.81) ## produces: ", (        yade.math.toHP1('-9.81')).__repr__())
print("g = -9.81                  ## produces: ", (-9.81 * yade.math.toHP1('1'    )).__repr__())
print("See https://yade-dem.org/doc/HighPrecisionReal.html#string-conversions for more info.")

id_0    = o.bodies.append(sphere((0,0,0),0.2,material=sphereMat)) # The body has no initial vertical Velocity
id_down = o.bodies.append(sphere((1,0,0),0.2,material=sphereMat)) # The body has an initial vertical Velocity -5
id_up   = o.bodies.append(sphere((2,0,0),0.2,material=sphereMat)) # The body has an initial vertical Velocity +5

O.bodies[id_down].state.vel[1] = v_down
O.bodies[id_up  ].state.vel[1] = v_up

# RungeKuttaCashKarp54Integrator calculates O.dt step with requested precision (rel_err, abs_err).
# The calculations involve checking forces at various positions, resetting state and so on, multiple times. With very small timesteps. But the end result is the dimulation advanced by O.dt
# see https://www.boost.org/doc/libs/1_74_0/libs/numeric/odeint/doc/html/boost/numeric/odeint/runge_kutta_cash_karp54.html
# and https://yade-dem.org/doc/yade.wrapper.html?highlight=rungekuttacashkarp54integrator#yade.wrapper.RungeKuttaCashKarp54Integrator
integrator=RungeKuttaCashKarp54Integrator([
  ForceResetter(),
  GeneralIntegratorInsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb(),]),
  InteractionLoop(
    [Ig2_Sphere_Sphere_ScGeom(), Ig2_Facet_Sphere_ScGeom()],
    [Ip2_ViscElMat_ViscElMat_ViscElPhys()],
    [Law2_ScGeom_ViscElPhys_Basic()],
  ),
  GravityEngine(gravity=[0,g,0]),
]);

# Tolerances can be set for the optimum accuracy. Just for fun use maximum available precision: yade.math.epsilon
integrator.rel_err = yade.math.epsilon() # 1e-20; # yade.math.epsilon()
integrator.abs_err = yade.math.epsilon() # 1e-20;

## Engines 
o.engines=[
   integrator
  ,PyRunner(command='checkPos()',iterPeriod=1),
]

def checkPos():
  #print("Iter=%i, time=%.15f, dt=%.15f" % (O.iter, O.time, O.dt))
  if (abs((O.bodies[id_0   ].state.pos[1] - getCurrentPos(0     ))/O.bodies[id_0   ].state.pos[1]) > tolerance):
    warningMessagePos (0     , O.bodies[id_0   ].state.pos[1], getCurrentPos(0))

  if (abs((O.bodies[id_down].state.pos[1] - getCurrentPos(v_down))/O.bodies[id_down].state.pos[1]) > tolerance):
    warningMessagePos (v_down, O.bodies[id_down].state.pos[1], getCurrentPos(v_down))

  if (abs((O.bodies[id_up  ].state.pos[1] - getCurrentPos(v_up  ))/O.bodies[id_up  ].state.pos[1]) > tolerance):
    warningMessagePos (v_up  , O.bodies[id_up  ].state.pos[1], getCurrentPos(v_up))

  if (abs((O.bodies[id_0   ].state.vel[1] - getCurrentVel(0     ))/O.bodies[id_0   ].state.vel[1]) > tolerance):
    warningMessageVel (0     , O.bodies[id_0   ].state.vel[1], getCurrentVel(0))

  if (abs((O.bodies[id_down].state.vel[1] - getCurrentVel(v_down))/O.bodies[id_down].state.vel[1]) > tolerance):
    warningMessageVel (v_down, O.bodies[id_down].state.vel[1], getCurrentVel(v_down))

  if (abs((O.bodies[id_up  ].state.vel[1] - getCurrentVel(v_up  ))/O.bodies[id_up  ].state.vel[1]) > tolerance):
    warningMessageVel (v_up  , O.bodies[id_up  ].state.vel[1], getCurrentVel(v_up))

# When not using yade.math.HP1(…) to declare g in python, in this particular case, the error occurs in getCurrentPos(inVel=0).
# The low-precision g is multiplied by high-precision t which causes incorrect value after first 15 decimal places.
# In this simple simulation frictionAngle or stiffness are not used, but in any simulation *each* floating point
# number with incorrect precision on python side can cause problems.
def getCurrentPos(inVel=0):
  t = O.time + O.dt
  return inVel*t + g*t*t/2

def getCurrentVel(inVel=0):
  t = O.time + O.dt
  return inVel + g*t

def warningMessagePos(inVel, y_pos, y_pos_need):
  raise YadeCheckError("The body with the initial velocity %.3f, has an y-position %.19f, but it should be %.19f. Iter=%i, time=%.15f, dt=%.15f" % (inVel, y_pos, y_pos_need, O.iter, O.time, O.dt))

def warningMessageVel(inVel, y_vel, y_vel_need):
  raise YadeCheckError("The body with the initial velocity %.3f, has an y-velocity %.19f, but it should be %.19f. Iter=%i, time=%.15f, dt=%.15f" % (inVel, y_vel, y_vel_need, O.iter, O.time, O.dt))

# on higher precisions, with error tolerance of yade.math.epsilon() this can become really slow. So only do 200 iterations.
# also note, that although the error can be very small, it is still accumulating. Increasing number of iterations to 1000000
# will require a larger tolerance declared at start of this script. But declaring larger O.dt (partially) solves this problem,
# because the actual error accumulations happens mostly between the iterations. And less inside the runge_kutta_cash_karp54,
# which focuses on eliminating it.
O.run(241,True)
# Note about error tolerance = 10**(-yade.math.getDigits10(1)+3) as declared above.
# This note might be useful in case of problems on other architectures:
#  O.run(242,True) fails on yade-mpfr150. Passes with higher tolerance.
#  O.run(241,True) fails on yade-long-double when incorrect assignment g = -9.81 is used above.
# If we replace "raise YadeCheckError" with "print" we can see that these errors occur only when id_up sphere's velocity is near zero. Then the errors disappear again.

