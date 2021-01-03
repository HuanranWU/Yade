.. _FEMxDEM:

##############################################################
FEM-DEM hierarchical multiscale modeling with Yade and Escript
##############################################################

.. comment: original title was:
.. comment: Parallel hierarchical multiscale modeling of granular media by coupling FEM and DEM with open-source codes Escript and YADE

Authors: Ning Guo and Jidong Zhao

Institution: Hong Kong University of Science and Technology

Escript download page: https://launchpad.net/escript-finley

mpi4py download page (optional, require MPI): https://bitbucket.org/mpi4py/mpi4py

Tested platforms: Desktop with Ubuntu 10.04, 32 bit; Server with Ubuntu 12.04, 14.04, 64 bit; Cluster with Centos 6.2, 6.5, 64 bit;

Introduction
^^^^^^^^^^^^^^^^
The code is built upon two open source packages: Yade for DEM modules and Escript for FEM modules. It implements the hierarchical multiscale model (FEMxDEM) for simulating the boundary value problem (BVP) of granular media. FEM is used to discretize the problem domain. Each Gauss point of the FEM mesh is embedded a representative volume element (RVE) packing simulated by DEM which returns local material constitutive responses to FEM. Typically, hundreds to thousands of RVEs are involved in a medium-sized problem which is critically time consuming. Hence parallelization is achieved in the code through either multiprocessing on a supercomputer or mpi4py on a HPC cluster (require MPICH or Open MPI). The MPI implementation in the code is quite experimental. The "mpipool.py" is contributed by Lisandro Dalcin, the author of mpi4py package. Please refer to the examples for the usage of the code.

Finite element formulation
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note:: This and the following section are a short excerpt from [Guo2014]_ to provide some theoretical background. Yade users of FEM-DEM coupling are welcome to improve the following two sections.

In this coupled FEM/DEM framework on hierarchical multiscale modelling of granular media, the
geometric domain $\Omega$ of a given BVP is first discretised into a suitable FEM mesh.
After the finite element discretisation, one ends up with the following equation system to be solved,

.. math:: \mat{K}\vec{u}=\vec{f},
   :label: FDeq1

where $\mat{K}$ is the stiffness matrix, $\vec{u}$ is the unknown displacement vector at the FEM nodes
and $\vec{f}$ is the
nodal force vector lumped from the applied boundary traction. For a typical linear elastic problem,
$\mat{K}$ can be formulated from the elastic modulus, and equation :eq:`FDeq1` can be solved directly. Whilst in
the case involving nonlinearity such as for granular media where $\vec{K}$ depends on state parameters
and loading history, Newton–Raphson iterative method needs to be adopted and the stiffness matrix
is replaced with the tangent matrix $\mat{K}_t$, which is assembled from the tangent operator:

.. math:: \mat{K}_t=\int_{\Omega} \mat{B}^T \mat{D} \mat{B} dV,
   :label: FDeq2

where $\mat{B}$ is the deformation matrix (i.e. gradient of the shape function), and $\mat{D}$ is the matrix form
of the rank four tangent operator tensor $\tens{D}$. During each Newton–Raphson iteration, both
$\mat{K}_t$ and internal stress $\mat{\sigma}$ are updated,
and the scheme tries to minimise the residual force $\vec{R}$ to find a converged solution:

.. math:: \vec{R}=\int_{\Omega} \vec{B}^T\vec{\sigma} dV - \vec{f}.
   :label: FDeq3


The tangent operator and the stress tensor at each local Gauss integration point are
pivotal variables in the aforementioned calculation and need to be evaluated before each iteration and
loading step. A continuum-based conventional FEM usually assumes a constitutive relation for the
material and derives the tangent matrix and the stress increment based on this constitutive
assumption (e.g. using the elasto-plastic modulus $\vec{D}^{ep}$ in equation :eq:`FDeq2` to
assemble $\vec{K}_t$ and to integrate
stress). The coupled FEM/DEM multiscale approach obtains the two quantities from the
embedded discrete element assembly at each Gauss point and avoids the needs for phenomenological
assumptions.

Multiscale solution procedure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The hierarchical multiscale modelling procedure is schematically summarised in the following steps:

1. Discretise the problem domain by suitable FEM mesh and attach each Gauss point with a DEM assembly prepared with suitable initial state.

2. Apply one global loading step, that is, imposed by FEM boundary condition on $\partial\Omega$.

   a) Determine the current tangent operator for each RVE.
   b) Assemble the global tangent matrix using equation :eq:`FDeq2` and obtain a trial solution of displacement $\vec{u}$ by solving Equation :eq:`FDeq1` with FEM.
   c) Interpolate the deformation $\nabla\vec{u}$ at each Gauss point of the FEM mesh and run the DEM simulation for the corresponding RVE using $\nabla\vec{u}$ as the DEM boundary conditions.
   d) Derive the updated total stress for each RVE and use it to evaluate the residual by equation :eq:`FDeq3` for the FEM domain.
   e) Repeat the aforementioned steps from (a) to (d) until convergence is reached and finish the current loading step.

3. Proceed to the next loading step and repeat Step 2.

In interpolating the deformation $\vec{u}$ from the FEM solution for DEM boundary conditions in Step 2(c), we consider both the infinitesimal strain $\vec{\epsilon}$ and rotation $\vec{\omega}$

.. math:: \nabla\vec{u}=\underbrace{\frac{1}{2}(\nabla\vec{u}+\nabla\vec{u}^T)}_{\vec{\epsilon}} + \underbrace{\frac{1}{2}(\nabla\vec{u}-\nabla\vec{u}^T)}_{\vec{\omega}}
   :label: FDeq4

The corresponding RVE packing will deform according to this prescribed boundary condition.

It is also instructive to add a few remarks on the evolution of stress from the RVE in Step 2(d).
In traditional FEM, the stress is updated based on an incremental manner to tackle the nonlinear
material response. If small strain is assumed, the incremental stress–strain relation may potentially
cause inaccurate numerical results when large deformation occurs in the material, which calls for
an alternative formulation for large deformation. This issue indeed can be naturally circumvented
in the current hierarchical framework. In our framework, the DEM assembly at each Gauss point
will memorise its past state history (e.g. pressure level, void ratio and fabric structure) and will be
solved with the current applied boundary condition (including both stretch and rotation) at each
loading and iteration step. Towards the end of each loading step, instead of using an incremental
stress update scheme, the total true stress (Cauchy stress) is derived directly over the solved DEM
assembly through homogenisation and is then returned to the FEM solver for the
global solution. In this way, we do not have to resort to the use of other objective stress measures
to deal with large deformation problems. However, we note that a proper strain measurement is
still required and the FEM mesh should not be severely distorted, otherwise, remeshing of the FEM
domain will be required.

More detailed description of the solution procedure can be found in  [Guo2013]_, [Guo2014]_, [Guo2014b]_, [Guo2014c]_,  [Guo2015]_.


Work on the YADE side
^^^^^^^^^^^^^^^^^^^^^^^^
The version of YADE should be at least rev3682 in which Bruno added the stringToScene function. Before installation, I added some functions to the source code (in "yade" subfolder). But only one function ("Shop::getStressAndTangent" in "./pkg/dem/Shop.cpp") is necessary for the FEMxDEM coupling, which returns the stress tensor and the tangent operator of a discrete packing. The former is homogenized using the Love's formula and the latter is homogenized as the elastic modulus. After installation and we get the executable file: yade-versionNo. We then generate a .py file linked to the executable file by "ln yade-versionNo yadeimport.py". This .py file will serve as a wrapped library of YADE. Later on, we will import all YADE functions into the python script through "from yadeimport import \*" (see simDEM.py file).

Open a python terminal. Make sure you can run ::

	import sys
	sys.path.append('where you put yadeimport.py')
	from yadeimport import *
	Omega().load('your initial RVE packing, e.g. 0.yade.gz')

If you are successful, you should also be able to run :: 

	from simDEM import *

Work on the Escript side
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
No particular requirement. But make sure the modules are callable in python, which means the main folder of Escript should be in your PYTHONPATH and LD_LIBRARY_PATH. The modules are wrapped as a class in msFEM\*.py.

Open a python terminal. Make sure you can run::

	from esys.escript import *
	from esys.escript.linearPDEs import LinearPDE
	from esys.finley import Rectangle

(Note: Escript is used for the current implementation. It can be replaced by any other FEM package provided with python bindings, e.g. FEniCS (http://fenicsproject.org). But the interface files "msFEM\*.py" need to be modified.)

Example tests
^^^^^^^^^^^^^^^^

After Steps 1 & 2, one should be able to run all the scripts for the multiscale analysis. The initial RVE packing (default name "0.yade.gz") should be provided by the user (e.g. using YADE to prepare a consolidated packing), which will be loaded by simDEM.py when the problem is initialized. The sample is initially uniform as long as the same RVE packing is assigned to all the Gauss points in the problem domain. It is also possible for the user to specify different RVEs at different Gauss points to generate an inherently inhomogeneous sample.

While simDEM.py is always required, only one msFEM\*.py is needed for a single test. For example, in a 2D (3D) dry test, msFEM2D.py (msFEM3D.py) is needed; similarly for a coupled hydro-mechanical problem (2D only, saturated), msFEMup.py is used which incorporates the u-p formulation. Multiprocessing is used by default. To try MPI parallelization, please set useMPI=True when constructing the problem in the main script. Example tests given in the "example" subfolder are listed below.
Note: The initial RVE packing (named 0.yade.gz by default) needs to be generated, e.g. using prepareRVE.py in "example" subfolder for a 2D packing (similarly for 3D).

#.	**2D drained biaxial compression test on dry dense sand** (biaxialSmooth.py)
	*Note*: Test description and result were presented in [Guo2014]_ and [Guo2014c]_.
#.	**2D passive failure under translational mode of dry sand retained by a rigid and frictionless wall** (retainingSmooth.py)
	*Note:* Rolling resistance model (CohFrictMat) is used in the RVE packing. Test description and result were presented in [Guo2015]_.
#.	**2D half domain footing settlement problem with mesh generated by Gmsh** (footing.py, footing.msh)
	*Note:* Rolling resistance model (CohFrictMat) is used in the RVE packing. Six-node triangle element is generated by Gmsh with three Gauss points each. Test description and result were presented in [Guo2015]_.
#.	**3D drained conventional triaxial compression test on dry dense sand using MPI parallelism** (triaxialRough.py)
	*Note 1:* The simulation is very time consuming. It costs ~4.5 days on one node using multiprocessing (16 processes, 2.0 GHz CPU). When useMPI is switched to True (as in the example script) and four nodes are used (80 processes, 2.2 GHz CPU), the simulation costs less than 24 hours. The speedup is about 4.4 in our test.
	*Note 2:* When MPI is used, mpi4py is required to be installed. The MPI implementation can be either MPICH or Open MPI. The file "mpipool.py" should also be placed in the main folder. Our test is based on openmpi-1.6.5. This is an on-going work. Test description and result will be presented later.
#.	**2D globally undrained biaxial compression test on saturated dense sand with changing permeability using MPI parallelism** (undrained.py)
	*Note:* This is an on-going work. Test description and result will be presented later.

Disclaim
^^^^^^^^^^^^
This work extensively utilizes and relies on some third-party packages as mentioned above. Their contributions are acknowledged. Feel free to use and redistribute the code. But there is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
