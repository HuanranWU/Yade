###############
Installation
###############

* Linux systems
  Yade can be installed from packages (pre-compiled binaries) or source code. The choice depends on what you need: if you don't plan to modify Yade itself, package installation is easier. In the contrary case, you must download and install the source code.

* Other Operating Systems:
  Jump to the `last section <https://yade-dem.org/doc/installation.html#yubuntu>`_ of this page.

Packages
----------

**Stable packages**


Since 2011, all Ubuntu (starting from 11.10, Oneiric) and Debian (starting from Wheezy) versions
have Yade in their main repositories. There are only stable releases in place.
To install Yade, run the following::

	sudo apt-get install yade

After that you can normally start Yade using the command ``yade`` or ``yade-batch``.

To check which version of Yade is included in your specific distribution, visit
`Ubuntu <https://launchpad.net/ubuntu/+source/yade>`_ or
`Debian <http://packages.qa.debian.org/y/yade.html>`_.
The `Debian-Backports <http://backports.debian.org/Instructions>`_
repository is updated regularly to bring the newest Yade version to the users of stable
Debians (more on this `here <https://lists.launchpad.net/yade-dev/msg15033.html>`_ for Debian 9 and 10).

**Daily packages**

Pre-built packages updated more frequently than the stable versions are provided for all currently supported Debian and Ubuntu
versions and available on `yade-dem.org/packages <http://yade-dem.org/packages/>`_ .

These are "daily" versions of the packages which are being updated regularly and, hence, include
all the newly added features.

To install the daily-version you need to add the repository to your
/etc/apt/sources.list.

- Debian 9 **stretch**::

	sudo bash -c 'echo "deb http://www.yade-dem.org/packages/ stretch main" >> /etc/apt/sources.list'


- Debian 10 **buster**::

	sudo bash -c 'echo "deb http://www.yade-dem.org/packages/ buster main" >> /etc/apt/sources.list'


- Debian 11 **bullseye**::

	sudo bash -c 'echo "deb http://www.yade-dem.org/packages/ bullseye main" >> /etc/apt/sources.list'


- Ubuntu 16.04 **xenial**::

	sudo bash -c 'echo "deb http://www.yade-dem.org/packages/ xenial main" >> /etc/apt/sources.list'


- Ubuntu 18.04 **bionic**::

	sudo bash -c 'echo "deb http://www.yade-dem.org/packages/ bionic main" >> /etc/apt/sources.list'


- Ubuntu 20.04 **focal**::

	sudo bash -c 'echo "deb http://www.yade-dem.org/packages/ focal main" >> /etc/apt/sources.list'


Add the PGP-key AA915EEB as trusted and install ``yadedaily``::

	wget -O - http://www.yade-dem.org/packages/yadedev_pub.gpg | sudo apt-key add -
	sudo apt-get update
	sudo apt-get install yadedaily


After that you can normally start Yade using the command ``yadedaily`` or ``yadedaily-batch``.
``yadedaily`` on older distributions can have some disabled features due to older library
versions, shipped with particular distribution.


The Git-repository for packaging stuff is available on `GitLab <https://gitlab.com/yade-dev/trunk/tree/master/scripts/ppa_ci>`_.


If you do not need ``yadedaily``-package anymore, just remove the
corresponding line in /etc/apt/sources.list and the package itself::

	sudo apt-get remove yadedaily

To remove our key from keyring, execute the following command::

	sudo apt-key remove AA915EEB

Daily and stable Yade versions can coexist without any conflicts, i.e., you can use ``yade`` and ``yadedaily``
at the same time.

.. _install-from-source-code:

Source code
------------

Installation from source code is reasonable, when you want to add or
modify constitutive laws, engines, functions etc. Installing the latest
trunk version allows one to use newly added features, which are not yet
available in packaged versions.

Download
^^^^^^^^^^

If you want to install from source, you can install either a release
(numbered version, which is frozen) or the current development version
(updated by the developers frequently). You should download the development
version (called ``trunk``) if you want to modify the source code, as you
might encounter problems that will be fixed by the developers. Release
versions will not be updated (except for updates due to critical and
easy-to-fix bugs), but generally they are more stable than the trunk.

#. Releases can be downloaded from the `download page <https://launchpad.net/yade/+download>`_, as compressed archive. Uncompressing the archive gives you a directory with the sources.
#. The development version (``trunk``) can be obtained from the `code repository <https://gitlab.com/yade-dev/>`_ at GitLab.

We use `GIT <http://git-scm.com/>`_ (the ``git`` command) for code
management (install the ``git`` package on your system and create a `GitLab account <https://gitlab.com/users/sign_in>`__)::

		git clone git@gitlab.com:yade-dev/trunk.git

will download the whole code repository of the ``trunk``. Check out :ref:`yade-gitrepo-label`
for more details on how to collaborate using ``git``.

Alternatively, a read-only checkout is possible via https without a GitLab account (easier if you don't want to modify the trunk version)::

		git clone https://gitlab.com/yade-dev/trunk.git

For those behind a firewall, you can download the sources from our `GitLab <https://gitlab.com/yade-dev>`__ repository as compressed archive.

Release and trunk sources are compiled in exactly the same way.

.. _prerequisites:

Prerequisites
^^^^^^^^^^^^^

Yade relies on a number of external software to run; they are checked before the compilation starts.
Some of them are only optional.

* `cmake <http://www.cmake.org/>`_ build system
* `gcc <https://gcc.gnu.org/>`_ compiler (g++); other compilers will not work; you need g++>=4.2 for openMP support
* `boost <http://www.boost.org/>`_ 1.47 or later
* `Qt <http://www.qt.io/>`_ library
* `freeglut3 <http://freeglut.sourceforge.net>`_
* `libQGLViewer <http://www.libqglviewer.com>`_
* `python <http://www.python.org>`_, `numpy <https://www.numpy.org/>`_, `ipython <https://ipython.org/>`_, `sphinx <https://www.sphinx-doc.org/en/master/>`_, `mpi4py <https://mpi4py.readthedocs.io/en/stable/>`_
* `matplotlib <http://matplotlib.sf.net>`_
* `eigen <http://eigen.tuxfamily.org>`_ algebra library (minimal required version 3.2.1)
* `gdb <http://www.gnu.org/software/gdb>`_ debugger
* `sqlite3 <http://www.sqlite.org>`_ database engine
* `Loki <http://loki-lib.sf.net>`_ library
* `VTK <http://www.vtk.org/>`_ library (optional but recommended)
* `CGAL <http://www.cgal.org/>`_ library (optional)
* `SuiteSparse <http://www.suitesparse.com>`_ sparse algebra library (fluid coupling, optional, requires eigen>=3.1)
* `OpenBLAS <http://www.openblas.net/>`_ optimized and parallelized alternative to the standard blas+lapack (fluid coupling :yref:`FlowEngine`, optional)
* `Metis <http://glaros.dtc.umn.edu/gkhome/metis/metis/overview/>`_ matrix preconditioning (fluid coupling, optional)
* `OpenMPI <https://www.open-mpi.org/software/>`_ library for parallel distributed computing (For MPI and OpenFOAM coupling, optional)
* `python3-mpi4py <https://bitbucket.org/mpi4py/>`_ MPI for Python (For MPI, optional)
* `coin-or <https://github.com/coin-or/Clp>`_ COIN-OR Linear Programming Solver (For :yref:`PotentialBlock`, optional)
* `mpfr <https://www.mpfr.org/>`_ in ``C++`` and `mpmath <http://mpmath.org/>`_ in ``python`` for high precision ``Real`` or for CGAL exact predicates (optional)

Most of the list above is very likely already packaged for your distribution. In case you are confronted
with some errors concerning not available packages (e.g., package libmetis-dev is not available) it may be necessary
to add yade external ppa from https://launchpad.net/~yade-users/+archive/external (see below) as well as http://www.yade-dem.org/packages (see the top of this page)::

	sudo add-apt-repository ppa:yade-users/external
	sudo apt-get update

The following commands have to be executed in the command line of your corresponding
distribution. Just copy&paste to the terminal. Note, to execute these commands you
need root privileges.

* **Ubuntu 18.04, 18.10**, **Debian 9, 10** and their derivatives::

		sudo apt install cmake git freeglut3-dev libloki-dev libboost-all-dev fakeroot \
		dpkg-dev build-essential g++ python3-dev python3-ipython python3-matplotlib \
		libsqlite3-dev python3-numpy python3-tk gnuplot libgts-dev python3-pygraphviz \
		libvtk6-dev libeigen3-dev python3-xlib python3-pyqt5 pyqt5-dev-tools python3-mpi4py \
		python3-pyqt5.qtwebkit gtk2-engines-pixbuf python3-pyqt5.qtsvg libqglviewer-dev-qt5 \
		python3-pil libjs-jquery python3-sphinx python3-git libxmu-dev libxi-dev libcgal-dev \
		help2man libbz2-dev zlib1g-dev libopenblas-dev libsuitesparse-dev \
		libmetis-dev python3-bibtexparser python3-future coinor-clp coinor-libclp-dev \
		python3-mpmath libmpfr-dev libmpfrc++-dev

* For **Ubuntu 16.04** ``libqglviewer-dev-qt5`` is to be replaced by ``libqglviewer-dev`` and ``python3-ipython`` by ``ipython3``.

* The packages ``python3-mpmath libmpfr-dev libmpfrc++-dev`` in above list are required only if one wants to use high precision calculations. The latter two only if `mpfr <https://www.mpfr.org/>`_ will be used. See :ref:`high precision documentation<highPrecisionReal>` for more details.

* For building documentation (the ``make doc`` invocation explained below) additional package ``texlive-xetex`` is required. On some multi-language systems an error ``Building format(s) --all. This may take some time... fmtutil failed.`` may occur, in that case a package ``locales-all`` is required.

Some of the packages (for example, cmake, eigen3) are mandatory, some of them
are optional. Watch for notes and warnings/errors, which are shown
by ``cmake`` during the configuration step. If the missing package is optional,
some of Yade features will be disabled (see the messages at the end of the configuration).

Some packages listed here are relatively new and they can be absent
in your distribution (for example, libmetis-dev). They can be
installed from `yade-dem.org/packages <http://yade-dem.org/packages/>`_ or
from our `external PPA <https://launchpad.net/~yade-users/+archive/external/>`_.
If not installed the related features will be disabled automatically.

If you are using other distributions than Debian or its derivatives you should
install the software packages listed above. Their names in other distributions can differ from the
names of the Debian-packages.


.. warning:: If you have Ubuntu 14.04 Trusty, you need to add -DCMAKE_CXX_FLAGS=-frounding-math
 during the configuration step of compilation (see below) or to install libcgal-dev
 from our `external PPA <https://launchpad.net/~yade-users/+archive/external/>`_.
 Otherwise the following error occurs on AMD64 architectures::

    terminate called after throwing an instance of 'CGAL::Assertion_exception'
    what():  CGAL ERROR: assertion violation!
    Expr: -CGAL_IA_MUL(-1.1, 10.1) != CGAL_IA_MUL(1.1, 10.1)
    File: /usr/include/CGAL/Interval_nt.h
    Line: 209
    Explanation: Wrong rounding: did you forget the  -frounding-math  option if you use GCC (or  -fp-model strict  for Intel)?
    Aborted



.. _yadeCompilation:

Compilation
^^^^^^^^^^^

You should create a separate build-place-folder, where Yade will be configured
and where the source code will be compiled. Here is an example for a folder structure::

	myYade/       		## base directory
		trunk/		## folder for source code in which you use git
		build/		## folder in which the sources will be compiled; build-directory; use cmake here
		install/	## install folder; contains the executables

Then, inside this build-directory you should call ``cmake`` to configure the compilation process::

	cmake -DCMAKE_INSTALL_PREFIX=/path/to/installfolder /path/to/sources

For the folder structure given above call the following command in the folder "build"::

	cmake -DCMAKE_INSTALL_PREFIX=../install ../trunk

Additional options can be configured in the same line with the following
syntax::

	cmake -DOPTION1=VALUE1 -DOPTION2=VALUE2
	
For example::
    
    cmake -DENABLE_POTENTIAL_BLOCKS=ON

The following cmake options are available: (see the `source code <https://gitlab.com/yade-dev/trunk/blob/master/CMakeLists.txt>`_ for a most up-to-date list)

	* CMAKE_INSTALL_PREFIX: path where Yade should be installed (/usr/local by default)
	* LIBRARY_OUTPUT_PATH: path to install libraries (lib by default)
	* DEBUG: compile in debug-mode (OFF by default)
	* ENABLE_LOGGER: use `boost::log <https://www.boost.org/doc/libs/release/libs/log/>`_ library for logging separately for each class (ON by default)
	* MAX_LOG_LEVEL: :ref:`set maximum level <maximum-log-level>` for LOG_* macros compiled with ENABLE_LOGGER, (default is 5)
	* ENABLE_USEFUL_ERRORS: enable useful compiler errors which help a lot in error-free development (ON by default)
	* CMAKE_VERBOSE_MAKEFILE: output additional information during compiling (OFF by default)
	* SUFFIX: suffix, added after binary-names (version number by default)
	* NOSUFFIX: do not add a suffix after binary-name (OFF by default)
	* YADE_VERSION: explicitly set version number (is defined from git-directory by default)
	* ENABLE_GUI: enable GUI option (ON by default)
	* ENABLE_CGAL: enable CGAL option (ON by default)
	* ENABLE_VTK: enable VTK-export option (ON by default)
	* ENABLE_OPENMP: enable OpenMP-parallelizing option (ON by default)
	* ENABLE_MPI: Enable MPI enviroment and communication, required distributed memory and for Yade-OpenFOAM coupling (ON by default)
	* ENABLE_GTS: enable GTS-option (ON by default)
	* ENABLE_GL2PS: enable GL2PS-option (ON by default)
	* ENABLE_LINSOLV: enable LINSOLV-option (ON by default)
	* ENABLE_PFVFLOW: enable PFVFLOW-option, FlowEngine (ON by default)
	* ENABLE_TWOPHASEFLOW: enable TWOPHASEFLOW-option, TwoPhaseFlowEngine (ON by default)
	* ENABLE_LBMFLOW: enable LBMFLOW-option, LBM_ENGINE (ON by default)
	* ENABLE_SPH: enable SPH-option, Smoothed Particle Hydrodynamics (OFF by default)
	* ENABLE_LIQMIGRATION: enable LIQMIGRATION-option, see [Mani2013]_ for details (OFF by default)
	* ENABLE_MASK_ARBITRARY: enable MASK_ARBITRARY option (OFF by default)
	* ENABLE_PROFILING: enable profiling, e.g., shows some more metrics, which can define bottlenecks of the code (OFF by default)
	* ENABLE_POTENTIAL_BLOCKS: enable potential blocks option (ON by default)
	* ENABLE_POTENTIAL_PARTICLES: enable potential particles option (ON by default)
	* ENABLE_DEFORM: enable constant volume deformation engine (OFF by default)
	* ENABLE_OAR: generate a script for oar-based task scheduler (OFF by default)
	* ENABLE_MPFR: use `mpfr <https://www.mpfr.org/>`_ in ``C++`` and `mpmath <http://mpmath.org/>`_ in ``python``. It can be used for higher precision ``Real`` or for CGAL exact predicates (OFF by default)
	* REAL_PRECISION_BITS, REAL_DECIMAL_PLACES: specify either of them to use a custom calculation precision. By default double (64 bits, 15 decimal places) precision is used as ``Real`` type. See :ref:`high precision documentation<highPrecisionReal>` for additional details.
	* runtimePREFIX: used for packaging, when install directory is not the same as runtime directory (/usr/local by default)
	* CHUNKSIZE: specifiy the chunk size if you want several sources to be compiled at once. Increases compilation speed but RAM-consumption during compilation as well (1 by default)
	* VECTORIZE: enables vectorization and alignment in Eigen3 library, experimental (OFF by default)
	* USE_QT5: use QT5 for GUI (ON by default)
	* CHOLMOD_GPU link Yade to custom SuiteSparse installation and activate GPU accelerated PFV (OFF by default)
	* SUITESPARSEPATH: define this variable with the path to a custom suitesparse install
	* PYTHON_VERSION: force Python version to the given one, e.g. ``-DPYTHON_VERSION=3.5``. Set to -1 to automatically use the last version on the system (-1 by default)

For using more extended parameters of cmake, please follow the corresponding
documentation on `https://cmake.org/documentation <https://cmake.org/documentation/>`_.

.. warning:: Only Qt5 is supported. On Debian/Ubuntu operating systems libQGLViewer
 of version 2.6.3 and higher are compiled against Qt5 (for other operating systems
 refer to the package archive of your distribution). If you mix Qt-versions a
 ``Segmentation fault`` will appear just after Yade is started. To provide
 necessary build dependencies for Qt5, install ``python-pyqt5 pyqt5-dev-tools``.


If cmake finishes without errors, you will see all enabled
and disabled options at the end. Then start the actual compilation process with::

	make

The compilation process can take a considerable amount of time, be patient.
If you are using a multi-core systems you can use the parameter ``-j`` to speed-up the compilation
and split the compilation onto many cores. For example, on 4-core machines
it would be reasonable to set the parameter ``-j4``. Note, Yade requires
approximately 3GB RAM per core for compilation, otherwise the swap-file will be used
and compilation time dramatically increases.

The installation is performed with the following command::

	make install

The ``install`` command will in fact also recompile if source files have been modified.
Hence there is no absolute need to type the two commands separately. You may receive make errors if you don't have permission to write into the target folder.
These errors are not critical but without writing permissions Yade won't be installed in /usr/local/bin/.

After the compilation finished successfully,
the new built can be started by navigating to /path/to/installfolder/bin and calling yade via (based on version yade-2014-02-20.git-a7048f4)::

    cd /path/to/installfolder/bin
    ./yade-2014-02-20.git-a7048f4

.. comment: is it possible to invoke python yade.config.revision and put it above as a text in the doc?

For building the documentation you should at first execute the command ``make install``
and then ``make doc`` to build it. The generated files will be stored in your current
install directory /path/to/installfolder/share/doc/yade-your-version. Once again writing permissions are necessary for installing into /usr/local/share/doc/. To open your local documentation go into the folder html and open the file index.html with a browser.

``make manpage`` command generates and moves manpages in a standard place.
``make check`` command executes standard test to check the functionality of the compiled program.

Yade can be compiled not only by GCC-compiler, but also by `CLANG <http://clang.llvm.org/>`_
front-end for the LLVM compiler. For that you set the environment variables CC and CXX
upon detecting the C and C++ compiler to use::

	export CC=/usr/bin/clang
	export CXX=/usr/bin/clang++
	cmake -DOPTION1=VALUE1 -DOPTION2=VALUE2

Clang does not support OpenMP-parallelizing for the moment, that is why the
feature will be disabled.

Supported linux releases
^^^^^^^^^^^^^^^^^^^^^^^^

`Currently supported <https://gitlab.com/yade-dev/trunk/pipelines?scope=branches>`_ [#buildLog]_ linux releases and their respective `docker <https://docs.docker.com/>`_ `files <https://docs.docker.com/engine/reference/builder/>`_ are:

* `Ubuntu 16.04 xenial <https://gitlab.com/yade-dev/docker-yade/blob/ubuntu16-py3/Dockerfile>`_
* `Ubuntu 18.04 bionic <https://gitlab.com/yade-dev/docker-yade/blob/ubuntu18.04/Dockerfile>`_
* `Debian 9 stretch <https://gitlab.com/yade-dev/docker-yade/blob/debian-stretch/Dockerfile>`_
* `Debian 10 buster <https://gitlab.com/yade-dev/docker-yade/blob/debian-buster/Dockerfile>`_
* `openSUSE 15 <https://gitlab.com/yade-dev/docker-yade/blob/suse15/Dockerfile>`_

These are the bash commands used to prepare the linux distribution and environment for installing and testing yade.
These instructions are automatically performed using the `gitlab continuous integration <https://docs.gitlab.com/ee/ci/quick_start/>`_ service after
each merge to master. This makes sure that yade always works correctly on these linux distributions.
In fact yade can be installed manually by following step by step these instructions in following order:

1. Bash commands in the respective Dockerfile to install necessary packages,

2. do ``git clone https://gitlab.com/yade-dev/trunk.git``,

3. then the ``cmake_*`` commands in the `.gitlab-ci.yml file <https://gitlab.com/yade-dev/trunk/blob/master/.gitlab-ci.yml>`_ for respective distribution,

4. then the ``make_*`` commands to compile yade,

5. and finally the ``--check`` and ``--test`` commands.

6. Optionally documentation can be built with ``make doc`` command, however currently it is not guaranteed to work on all linux distributions due to frequent interface changes in `sphinx <http://www.sphinx-doc.org/en/master/>`_.

These instructions use ``ccache`` and ``ld.gold`` to :ref:`speed-up compilation <speed-up>` as described below.

.. [#buildLog] To see details of the latest build log click on the *master* branch.

Python 2 backward compatibility
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Following the end of Python 2 support (beginning of 2020), Yade compilation on a Python 2 ecosystem is no longer garanteed since the `6e097e95 <https://gitlab.com/yade-dev/trunk/-/tree/6e097e95368a9c63ce169a040f418d30c7ba307c>`_ trunk version. Python 2-compilation of the latter is still possible using the above ``PYTHON_VERSION`` cmake option, requiring Python 2 version of prerequisites packages whose list can be found in the corresponding paragraph (Python 2 backward compatibility) of the `historical doc <https://gitlab.com/yade-dev/trunk/-/blob/6e097e95368a9c63ce169a040f418d30c7ba307c/doc/sphinx/installation.rst>`_.

Ongoing development of Yade now assumes a Python 3 environment, and you may refer to some notes about :ref:`converting Python 2 scripts into Python 3<convert-python2-to3>` if needed.

.. _speed-up:

Speed-up compilation
---------------------

Compile time
^^^^^^^^^^^^^^^^^^^^^

When spliting the compilation on many cores (``make -jN``), ``N`` is limited by the available cores and memory. It is possible to use more cores if remote computers are available, distributing the compilation with `distcc <https://wiki.archlinux.org/index.php/Distcc>`_  (see distcc documentation for configuring slaves and master)::

	export CC="distcc gcc"
	export CXX="distcc g++"
	cmake [options as usual]
	make -jN

In addition, and independently of distcc, caching previous compilations with `ccache <https://ccache.samba.org/>`_ can speed up re-compilation::

	cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache [options as usual]

The two tools can be combined very simply, adding to the above exports::

	export CCACHE_PREFIX="distcc"

Link time
^^^^^^^^^^^^^^^^^^^^^

The link time can be reduced roughly 2 minutes by changing the default linker from ``ld`` to ``ld.gold``. They are both in the same package ``binutils`` (on opensuse15 it is package ``binutils-gold``). To perform the switch execute these commands as root::

	ld --version
	update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/ld.gold" 20
	update-alternatives --install "/usr/bin/ld" "ld" "/usr/bin/ld.bfd" 10
	ld --version

To switch back run the commands above with reversed priorities ``10`` ↔ ``20``. Alternatively a manual selection can be performed by command: ``update-alternatives --config ld``.

Note: ``ld.gold`` is incompatible with the compiler wrapper ``mpicxx`` in some distributions, which is manifested as an error in the ``cmake`` stage. We do not use ``mpicxx`` for our builds currently, if you want to use it then disable ``ld.gold``.

Cloud Computing
----------------

It is possible to exploit cloud computing services to run Yade. The combo Yade/Amazon Web Service has been found to work well, namely. Detailed instructions for migrating to amazon can be found in the section :ref:`CloudComputing`.

GPU Acceleration
----------------

The FlowEngine can be accelerated with CHOLMOD's GPU accelerated solver. The specific hardware and software requirements are outlined in the section :ref:`GPUacceleration`.

Special builds
--------------

The software can be compiled by a special way to find some specific bugs and problems in it: memory corruptions, data races, undefined behaviour etc.


The listed sanitizers are runtime-detectors. They can only find the problems in the code, if the particular part of the code
is executed. If you have written a new C++ class (constitutive law, engine etc.) try to run your Python script with
the sanitized software to check, whether the problem in your code exist.

.. _address-sanitizer:

AddressSanitizer
^^^^^^^^^^^^^^^^

`AddressSanitizer <https://clang.llvm.org/docs/AddressSanitizer.html>`_ is a memory error detector, which helps to find heap corruptions,
out-of-bounds errors and many other memory errors, leading to crashes and even wrong results.

To compile Yade with this type of sanitizer, use ENABLE_ASAN option::

	cmake -DENABLE_ASAN=1

The compilation time, memory consumption during build and the size of build-files are much higher than during the normall build.
Monitor RAM and disk usage during compilation to prevent out-of-RAM problems.

To find the proper libasan library in your particular distribution, use ``locate`` or ``find /usr -iname "libasan*so"`` command. Then, launch your yade executable in connection with that libasan library, e.g.::

    LD_PRELOAD=/some/path/to/libasan.so yade

By default the leak detector is enabled in the asan build. Yade is producing a lot of leak warnings at the moment.
To mute those warnings and concentrate on other memory errors, one can use detect_leaks=0 option. Accounting for the latter, the full command
to run tests with the AddressSanitized-Yade on Debian 10 Buster is::

	ASAN_OPTIONS=detect_leaks=0:verify_asan_link_order=false yade --test

If you add a new check script, it is being run automatically through the AddressSanitizer in the CI-pipeline.

Yubuntu
-------

If you are not running a Linux system there is a way to create an Ubuntu `live-usb <http://en.wikipedia.org/wiki/Live_USB>`_ on any usb mass-storage device (minimum size 10GB). It is a way to boot the computer on a linux system with Yadedaily pre-installed without affecting the original system. More informations about this alternative are available `here <http://people.3sr-grenoble.fr/users/bchareyre/pubs/yubuntu/>`_ (see the README file first).

Alternatively, images of a linux virtual machine can be downloaded, `here again <http://people.3sr-grenoble.fr/users/bchareyre/pubs/yubuntu/>`_, and they should run on any system with a virtualization software (tested with VirtualBox and VMWare).


