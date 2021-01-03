.. _FoamCouplingEngine:


CFD-DEM coupled simulations with Yade and OpenFOAM
##################################################

The :yref:`FoamCoupling` engine provides a framework for Euler-Lagrange fluid-particle
simulation with the open source finite volume solver `OpenFOAM <https://cfd.direct/openfoam/user-guide/>`_. The coupling
relies on the `Message Passing Interface library (MPI) <https://www.open-mpi.org/software/>`_, as OpenFOAM is
a parallel solver, furthermore communication between the solvers are realised by MPI messages.
The :yref:`FoamCoupling` engine must be enabled with the ENABLE_MPI flag during compilation::

  cmake -DCMAKE_INSTALL_PREFIX=/path/to/install /path/to/source -DENABLE_MPI=1

Yade sends the particle information (particle position, velocity, etc. ) to all the OpenFOAM processes. Each OpenFOAM process searches the particle in the local mesh,
if the particle is found, the hydrodynamic drag force and torque are calculated using the fluid velocity at the particle position (two interpolation methods are available) and the particle velocity.
The hydroynamic force is sent to the Yade process and it is added  to the force container.  The negative of the particle hydrodynamic force (interpolated back to the fluid cell center) is set as source term in the Navier-Stokes equations.
The `OpenFOAM solver <https://github.com/dpkn31/Yade-OpenFOAM-coupling>`_ must also be installed to facilitate the MPI connection between Yade and OpenFOAM.
Technical details on the coupling methodology can be found in [Kunhappan2017]_ and [Kunhappan2018]_.

Background
==========

In the standard Euler-Lagrange modelling of particle laden multiphase flows, the particles are treated as point masses. Two approaches are implemented in the present coupling:

  #. Point force coupling
  #. Volume fraction based force coupling.

In both of the approaches the flow at the particle scale is not resolved and analytical/empirical hydrodynamic force models are used to describe the fluid-particle interactions. For accurate resolution of the
particle volume fraction and hydrodynamic forces on the fluid grid the particle size must be smaller than the fluid cell size.

Point force coupling (`icoFoamYade`)
------------------------------------

In the point force coupling, the particles are assumed to be smaller than the smallest fluid length scales, such that the particle Reynolds Number is
$Re_{p} < 1.0$. The particle Reynolds number is defined as the ratio of inertial forces to viscous forces. For a sphere, the associated length-scale
is the diameter, therefore:

  .. math:: Re_{p} = \frac{\rho_{f}|\vec{U}_{r}|d_{p}}{\mu}
    :label: eq-reynoldsNumber

where in :eq:`eq-reynoldsNumber` $\rho_{f}$ is the fluid density,  $|\vec{U}_{r}|$ is the norm of the relative velocity between the particle and the fluid, $d_{p}$ is the particle
diameter and $\mu$ the fluid dynamic viscosity. In addition to the Reynolds number, another non-dimensional number that characterizes the particle inertia
due to it's mass called Stokes number is defined as:

  .. math:: St_{k} = \frac{\tau_{p} \left| \vec{U}_{f} \right|}{d_{p}}
    :label: eq-stokesNumber

where in equation :eq:`eq-stokesNumber` $\tau_{p}$ is the particle relaxation time defined as:

  .. math:: \tau_{p} = \frac{\rho_{p} d^{2}_{p}}{18 \mu}

For $Re_{p} < 1$ and $St_{k} < 1$, the hydrodynamic force on the particle can be represented as a point force. This force is calculated using the Stoke's
drag force formulation:

  .. math:: \vec{F}_{h} = 3 \pi \mu d_{p} (U_{f} - U_{p})
    :label: eq-stokesForce

The force obtained from :eq:`eq-stokesForce` is applied on the particle and in the fluid side (in the cell where the particle resides), this hydrodynamic force  is formulated as a body/volume
force:

  .. math:: \vec{f}_{h} = \frac{-\vec{F}_{h}}{V_{c} \rho_{f}}
    :label: eq-stokesfluid

where in equation :eq:`eq-stokesfluid` $V_{c}$ is the volume of the cell and $\rho_{f}$ is the fluid density. Hence the Navier-Stokes equations for the combined system is:

.. math:: \frac{\partial \vec{U}}{\partial t} + \nabla \cdot (\vec{U}\vec{U}) = -\frac{\nabla p}{\rho} + \nabla \bar{\bar \tau} + \vec{f}_{h}
  :label: eq-nseqsimple

Along with the continuity equation:

.. math:: \nabla \cdot \vec{U} = 0
  :label: eq-simplecnty


Volume averaged coupling (`pimpleFoamYade`)
--------------------------------------------
In the volume averaged coupling, the effect of the particle volume fraction is included. The Navier-Stokes equations take the following form:

  .. math:: \frac{\partial (\epsilon_{f} \vec{U}_{f}) }{\partial t} + \nabla \cdot ( \epsilon_{f} \vec{U}_{f} \vec{U}_{f}) = -\frac{\nabla p}{\rho} + \epsilon_{f} \nabla \bar{\bar \tau} -K \left(U_{f}-U_{p} \right) + \vec{S}_{u} + \epsilon_{f} \vec{g}
    :label: eq-volfracNS


Along with the continuity equation:

.. math:: \frac{\partial \epsilon_{f}}{\partial t} + \nabla \cdot (\epsilon_{f} \vec{U}_{f}) = 0
  :label: eq-volFracCnty

where in equations :eq:`eq-volfracNS` and :eq:`eq-volFracCnty` $\epsilon_{f}$ is the fluid volume fraction. Note that, we do not solve for $\epsilon_{f}$ directly, but obtain it from the local
particle volume fraction $\epsilon_{s}$, $\epsilon_{f} = 1 - \epsilon_{s}$ . $K$ is the particle drag force parameter, $\vec{U}_{f}$ and $\vec{U}_{p}$ are the fluid and particle velocities respectively. $\vec{S}_{u}$ denotes the explicit source term consisting the effect of other hydrodynamic forces such as the Archimedes/ambient force, added mass force etc. Details on the formulation of these forces are presented in the later parts of this section.

The interpolation and averaging of the Eulerean and Lagrangian quantities are based on a Gaussian envelope $G_{\star}$. In this method, the the effect of the particle
is 'seen' by the neighbouring cells of the cell in which it resides. Let $\vec{x}_{c}$ and $\vec{x}_{p}$ be the fluid cell center and particle position respectively, then the Gaussian filter $G_{\star} \left(\vec{x}_{c}-\vec{x}_{p}\right)$ defined as:

  .. math:: G_{\star} \left(\vec{x}_{c}-\vec{x}_{p}\right)=\left(2\pi\sigma^{2}\right)^{\frac{3}{2}}\exp\left(-\frac{\left|\left|\vec{x}_{c}-\vec{x}_{p}\right|\right|^{2}}{2\sigma^{2}}\right)
    :label: gausseq

with $\sigma$ being the standard deviation of the filter defined as:

  .. math:: \sigma = \delta / \left(2\sqrt{2 \ln 2}\right)
    :label: sigmaeq

where in equation :eq:`sigmaeq` $\delta$ is the cut-off range (at present it's set to $3 \Delta x$, with $\Delta x$ being the fluid cell size.) and follows the rule:

  .. math:: G_{\star} \left(\left| \left| \vec{x}_{c} - \vec{x}_{p} \right| \right| = \delta/2 \right) = \frac{1}{2} G_{\star} \left( \left| \left|  x_{c} -x_{p} \right| \right| = 0 \right)

The particle volume fraction $\epsilon_{s,c}$ for a fluid cell $c$ is calculated by:

  .. math:: \epsilon_{s, c} =  \frac{\sum_{i=1}^{N_{p}} V_{p,i} G_{\star (i,c)}}{V_{c}}
    :label: svolfrac

where in :eq:`svolfrac` $N_{p}$ is the number of particle contributions on the cell $c$, $G_{\star (i,c)}$ is the Gaussian weight obtained from :eq:`gausseq`, $V_{p,i}G_{\star (i,c)}$ forms the individual particle volume contribution.  $V_{c}$ is the fluid cell volume and $\epsilon_{f}+\epsilon_{s}=1$

The averaging and interpolation of an Eulerean quantity $\phi$ from the grid (cells) to the particle position is performed using the following expression:

  .. math:: \widetilde{\phi} = \sum_{i=1}^{N_{c}}  \phi_{i} G_{\star (i,p)}
    :label: fluidinterp

Hydrodynamic Force
^^^^^^^^^^^^^^^^^^
In equation :eq:`eq-volfracNS` the term $K$ is the drag force parameter. In the present implementation, $K$ is based on the Schiller Naumman drag law, which reads as:

  .. math:: K = \frac{3}{4} C_{d} \frac{\rho_{f}}{d_{p}} \left| \left| \vec{\widetilde{U}}_{f} - \vec{U}_{p} \right| \right| \epsilon_{f}^{-h_{exp}}
    :label: dragParam

In equation :eq:`dragParam` $\rho_{f}$ is the fluid density, $d_{p}$ the particle diameter, $h_{exp}$ is defined as the 'hindrance coefficient' with the value set as $h_{exp}=2.65$. The drag force force coefficient  $C_{d}$ is valid for particle Reynolds numbers up to $Re_{p} < 1000$. The expression for $C_{d}$ reads as:

  .. math:: C_{d} = \frac{24}{Re_{p}} \left(1+0.15Re^{0.687}_{p} \right)
    :label: dragCoeff

The expression of hydrodynamic drag force on the particle is:

  .. math:: \vec{F}_{\textrm{drag}} = V_{p}K(\vec{\widetilde{U}}_{f} - {U}_{p})

In the fluid equations, negative of the drag parameter ($-K$) is distributed back to the grid based on equation :eq:`svolfrac`. Since the drag force includes a non-linear dependency on the fluid velocity
$U_{f}$, this term is set as an implicit source term in the fluid solver.


The Archimedes/ambient force experienced by the particle is calculated as:

  .. math:: \vec{F}_{by} = \left(\widetilde{-\nabla p} + \widetilde{\nabla \bar{\bar \tau}} \right) V_{p}
    :label: buoyForce

where in :eq:`buoyForce`, $\widetilde{\nabla p}$ is the averaged pressure gradient at the particle center and $\widetilde{\nabla \bar{\bar \tau}}$ is the averaged divergence of the
viscous stress at the particle position.

Added mass force:

    .. math:: \vec{F}_{am} = C_{m}\left( \frac{D\widetilde{U_{f}}}{Dt} -\frac{dU_{p}}{dt} \right) V_{p}
      :label: amForce

where in eqaution :eq:`amForce`, $\frac{D\widetilde{U}_{f}}{Dt}$ is the material derivative of the fluid velocity.

Therefore the net hydrodynamic force on the particle reads as:

  .. math:: \vec{F}_{\textrm{hyd}} = \vec{F}_{\text{drag}} + \vec{F}_{\text{by}} + \vec{F}_{\text{am}}

And on the fluid side the explicit source term $\vec{S}_{u, c}$  for a fluid cell $c$ is expressed as :

  .. math:: \vec{S}_{u,c} = \frac{ \sum_{i=1}^{N_{p}} -\vec{F}_{\textrm{hyd,i}} \epsilon_{s,c} G_{\star (i,c)} } {\rho_{f} V_{c}}


Setting up a case
=================

In Yade
-------
Setting a case in the Yade side is fairly straight forward.
The python script describing the scene in Yade is based on `this method <https://yade-dev.gitlab.io/trunk/user.html#importing-yade-in-other-python-applications>`_.
Make sure the exact wall/periodic boundary conditions are set in Yade as well as in the OpenFOAM. The particles should not leave the fluid domain. In case a particle has
'escaped' the domain, a warning message would be printed/written to the log file and the simulation will break.

The example in :ysrc:`examples/openfoam/scriptYade.py` demonstrates the coupling.
A symbolic link to Yade is created and it is imported in the script. The MPI environment
is initialized by calling the initMPI() function before instantiating the coupling engine ::

    initMPI()
    fluidCoupling = FoamCoupling()
    fluidCoupling.getRank()


A list of the particle ids and number of particle is passed to the coupling engine ::

    sphereIDs = [b.id for b in O.bodies if type(b.shape)==Sphere]
    numparts = len(sphereIDs);

    fluidCoupling.setNumParticles(numparts)
    fluidCoupling.setIdList(sphereIDs)
    fluidCoupling.isGaussianInterp = False

The type of force/velocity interpolation mode has to be set. For Gaussian envelope interpolation, the :yref:`isGaussianInterp <FoamCoupling::isGaussianInterp>` flag has to be set, also  the solver
`pimpleFoamYade` must be used. The engine is added to the O.engines after the timestepper ::

      O.engines = [
      ForceResetter(),
      ...,
      GlobalStiffnessTimeStepper,
      fluidCoupling ...
      newton ]

Substepping/data exchange interval is set automatically based on the ratio of timesteps as foamDt/yadeDt (see :yref:`exchangeDeltaT <FoamCoupling::exchangeDeltaT>` for details).


In OpenFOAM
-----------

There are two solvers available in this `git <https://github.com/dpkn31/Yade-OpenFOAM-coupling>`_ repository. The solver `icoFoamYade` is based on the point force coupling method and the solver `pimpleFoamYade`
is based on the volume averaged coupling. They are based on the existing `icoFoam <https://openfoamwiki.net/index.php/IcoFoam>`_ and `pimpleFoam <https://openfoamwiki.net/index.php/OpenFOAM_guide/The_PIMPLE_algorithm_in_OpenFOAM>`_
solvers respectively. Any OpenFOAM supported mesh can be used, for more details on the mesh options and meshing see `here <https://cfd.direct/openfoam/user-guide/v6-mesh/>`_. In the present example, the mesh is generated
using `blockMesh` utility of OpenFOAM. The case is set up in the usual OpenFOAM way with the directories `0`, `system` and `constant` ::

  0/
    U                         ## velocity boundary conditions
    p                         ## pressure boundary conditions
    uSource                   ## source term bcs (usually set as calculated).

  system/
    controlDict               ## simulation settings : start time, end time, delta T, solution write control etc.
    blockMeshDict             ## mesh setup for using blockMesh utility : define coordinates of geometry and surfaces. (used for simple geometries -> cartesian mesh.)
    decomposeParDict          ## dictionary for setting domain decomposition, (in the present example scotch is used)
    fvSchemes                 ## selection of finite volume schemes for calculations of divergence, gradients and interpolations.
    fvSolution                ## linear solver selection, setting of relaxation factors and tolerance criterion,

  constant/
    polymesh/                 ## mesh information, generated by blockMesh or other mesh utils.
    transportProperties       ## set the fluid and particle properties. (just density of the particle)

Note: Always set the timestep less than the particle relaxation time scale, this is not claculated automatically yet! Turbulence modelling based on the RANS equations have not been implemented yet. However it is
possible to use the present formulations for fully resolved turbulent flow simulations via DNS. Dynamic/moving mesh problems are not supported yet.
(Let me know if you're interested in implementing any new features.)

To prepare a simulation, follow these steps::

  blockMesh         ## generate the mesh
  decomposePar      ## decompose the mesh

Any type of mesh that is `supported by OpenFOAM <https://cfd.direct/openfoam/user-guide/v6-mesh/>`_ can be used. Dynamic mesh is currently not supported.

Execution
---------

The simulation is executed via the following command::

  mpiexec -n 1 python3 scriptYade.py : -n NUMPROCS icoFoamYade -parallel

The `video`__ below shows the steps involved in compiling and executing the coupled CFD-DEM simulation

  __ https://youtu.be/J_V1ffx71To

  .. youtube:: J_V1ffx71To


Post-Processing
===============

Paraview can be used to visulaize both the Yade solution (use VTKRecorder) and OpenFOAM solution. To visulaize the fluid solution, create an empty file as `name.foam` , open this file in Paraview and in the `properties`
section below the pipeline, change "Reconstructed case" to "Decomposed case" , or you can use the reconstructed case itself but after running the `reconstructPar` utility, but this is time consuming.
