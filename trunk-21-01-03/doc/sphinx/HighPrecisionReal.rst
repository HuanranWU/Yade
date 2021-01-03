.. _highPrecisionReal:

***************************
High precision calculations
***************************

Yade supports high and arbitrary precision ``Real`` type for performing calculations. All tests and checks pass but still the current support is considered experimental.
The backend library is `boost <https://github.com/boostorg/multiprecision>`__ `multiprecision <https://www.boost.org/doc/libs/1_72_0/libs/multiprecision/doc/html/index.html>`__
along with corresponding `boost <https://github.com/boostorg/math>`__ `math toolkit <https://www.boost.org/doc/libs/1_72_0/libs/math/doc/html/index.html>`__.

The supported types are following:

=============================================== =============== =============================== ==================================================================
type						bits		decimal places [#prec]_		notes
=============================================== =============== =============================== ==================================================================
  ``float``					``32``		``6``				hardware accelerated (not useful, it is only for testing purposes)
  ``double``					``64``		``15``				hardware accelerated
  ``long double``				``80``		``18``				hardware accelerated
  ``boost float128``				``128``		``33``				depending on processor type it may be hardware accelerated, `wrapped by boost <https://www.boost.org/doc/libs/1_72_0/libs/multiprecision/doc/html/boost_multiprecision/tut/floats/float128.html>`__
  ``boost mpfr``				``Nbit``	``Nbit/(log(2)/log(10))``	uses external mpfr library, `wrapped by boost <https://www.boost.org/doc/libs/1_72_0/libs/multiprecision/doc/html/boost_multiprecision/tut/floats/mpfr_float.html>`__
  ``boost cpp_bin_float``			``Nbit``	``Nbit/(log(2)/log(10))``	uses boost only, but is slower
=============================================== =============== =============================== ==================================================================

The last two types are arbitrary precision, and their number of bits ``Nbit`` or decimal places is specified as argument during compilation.

.. note::
	See file :ysrc:`Real.hpp<lib/high-precision/Real.hpp>` for details. All ``Real`` types pass the :ysrccommit:`real type concept<1b4ae97583bd8a6efc74cb0d0/py/high-precision/_math.cpp#L197>` test from `boost concepts <https://www.boost.org/doc/libs/1_72_0/libs/math/doc/html/math_toolkit/real_concepts.html>`__. The support for :ysrc:`Eigen<lib/high-precision/EigenNumTraits.hpp>` and :ysrc:`CGAL <lib/high-precision/CgalNumTraits.hpp>` is done with numerical traits.

.. [#prec] The amount of decimal places in this table is the amount of places which are completely determined by the binary represenation. :ref:`Few additional decimal digits <extraStringDigits>` is necessary to fully reconstruct binary representation. A simple python example to demonstrate this fact: ``for a in range(16): print(1./pow(2.,a))``, shows that every binary digit produces "extra" ``…25`` at the end of decimal representation, but these decimal digits are not completely determined by the binary representation, because for example ``…37`` is impossible to obtain there. More binary bits are necessary to represent ``…37``, but the ``…25`` was produced by the last available bit.

.. _highPrecisionRealInstallation:

Installation
===========================================

The precompiled Yade package uses ``double`` type by default. In order to use high precision type Yade has to be compiled and installed from source code by following the
regular :ref:`installation instructions <install-from-source-code>`. With extra following caveats:

1. Following packages are required to be installed: ``python3-mpmath`` ``libmpfr-dev`` ``libmpfrc++-dev`` (the ``mpfr`` related
   packages are necessary only to use ``boost::multiprecision::mpfr`` type). These packages are already listed in the :ref:`default requirements <prerequisites>`.

2. A g++ compiler version 9.2.1 or higher is required. It shall be noted that upgrading only the compiler on an existing linux installation (an older one, in which packages for different versions of gcc were not introduced) is difficult and it is not recommended. A simpler solution is to upgrade entire linux installation.

3. During cmake invocation specify:

	1. either number of bits as ``REAL_PRECISION_BITS=……``,
	2. or number of requested decimal places as ``REAL_DECIMAL_PLACES=……``, but not both
	3. to use MPFR specify ``ENABLE_MPFR=ON``.

   The arbitrary precision (``mpfr`` or ``cpp_bin_float``) types are used only when more than 128 bits or more than 39 decimal places are requested. In such case if ``ENABLE_MPFR=OFF`` then
   the slower ``cpp_bin_float`` type is used. The difference in decimal places between 39 and 33 stems from the fact that `15 bits are used for exponent <https://en.wikipedia.org/wiki/Quadruple-precision_floating-point_format>`__. Note: a fast quad-double (debian package ``libqd-dev``) implementation with 62 decimal places is `in the works <https://github.com/boostorg/multiprecision/issues/184>`__ with boost multiprecision team.


.. _supported-hp-modules:

Supported modules
===========================================

During :ref:`compilation <yadeCompilation>` several Yade modules can be enabled or disabled by passing an ``ENABLE_*`` command line argument to cmake.
The following table lists which modules are currently working with high precision (those marked with "maybe" were not tested):

=========================================== ============ ============================= ========================
``ENABLE_*`` module name                    HP support   cmake default setting         notes
=========================================== ============ ============================= ========================
``ENABLE_GUI``                              yes          ``ON``                        native support [#supp1]_
``ENABLE_CGAL``                             yes          ``ON``                        native support [#supp1]_
``ENABLE_VTK``                              yes          ``ON``                        supported [#supp3]_
``ENABLE_OPENMP``                           partial      ``ON``                        partial support [#supp5]_
``ENABLE_MPI``                              maybe        ``OFF``                       not tested [#supp6]_
``ENABLE_GTS``                              yes          ``ON``                        supported [#supp2]_
``ENABLE_GL2PS``                            yes          ``ON``                        supported [#supp2]_
``ENABLE_LINSOLV``                          no           ``OFF``                       not supported [#supp7]_
``ENABLE_PFVFLOW``                          no           ``OFF``                       not supported [#supp7]_
``ENABLE_TWOPHASEFLOW``                     no           ``OFF``                       not supported [#supp7]_
``ENABLE_THERMAL``                          no           ``OFF``                       not supported [#supp7]_
``ENABLE_LBMFLOW``                          yes          ``ON``                        supported [#supp2]_
``ENABLE_SPH``                              maybe        ``OFF``                       not tested [#supp9]_
``ENABLE_LIQMIGRATION``                     maybe        ``OFF``                       not tested [#supp9]_
``ENABLE_MASK_ARBITRARY``                   maybe        ``OFF``                       not tested [#supp9]_
``ENABLE_PROFILING``                        maybe        ``OFF``                       not tested [#supp9]_
``ENABLE_POTENTIAL_BLOCKS``                 no           ``OFF``                       not supported [#supp8]_
``ENABLE_POTENTIAL_PARTICLES``              yes          ``ON``                        supported [#supp4]_
``ENABLE_DEFORM``                           maybe        ``OFF``                       not tested [#supp9]_
``ENABLE_OAR``                              maybe        ``OFF``                       not tested [#supp9]_
``ENABLE_FEMLIKE``                          yes          ``ON``                        supported [#supp2]_
``ENABLE_ASAN``                             yes          ``OFF``                       supported [#supp2]_
``ENABLE_MPFR``                             yes          ``OFF``                       native support [#supp1]_
=========================================== ============ ============================= ========================

The unsupported modules are automatically disabled during the cmake stage.

.. rubric:: Footnotes

.. [#supp1] This feature is supported natively, which means that specific numerical traits were written :ysrc:`for Eigen<lib/high-precision/EigenNumTraits.hpp>` and :ysrc:`for CGAL<lib/high-precision/CgalNumTraits.hpp>`, as well as :ysrc:`GUI<gui/qt5/SerializableEditor.py>` and :ysrc:`python support<lib/high-precision/ToFromPythonConverter.hpp>` was added.

.. [#supp3] VTK is supported via the :ysrc:`compatibility layer <lib/compatibility/VTKCompatibility.hpp>` which converts all numbers down to ``double`` type. See :ref:`below <vtk-real-compatibility>`.

.. [#supp5] The OpenMPArrayAccumulator is experimentally supported for ``long double`` and ``float128``. For types ``mpfr`` and ``cpp_bin_float`` the single-threaded version of accumulator is used. File :ysrc:`lib/base/openmp-accu.hpp` needs further testing. If in doubt, compile yade with ``ENABLE_OPENMP=OFF``. In all other places OpenMP multithreading should work correctly.

.. [#supp6] MPI support has not been tested and sending data over network hasn't been tested yet.

.. [#supp2] The module was tested, the ``yade --test`` and ``yade --check`` pass, as well as most of examples are working. But it hasn't been tested extensively for all possible use cases.

.. [#supp7] Not supported, the code uses external cholmod library which supports only ``double`` type. To make it work a native Eigen solver for linear equations should be used.

.. [#supp9] This feature is ``OFF`` by default, the support of this feature has not been tested.

.. [#supp8] Potential blocks use external library coinor for linear programming, this library uses ``double`` type only. To make it work a linear programming routine has to be implemented using Eigen or coinor library should start using C++ templates or a converter/wrapper similar to :ysrc:`LAPACK library <lib/compatibility/LapackCompatibility.hpp>` should be used.

.. [#supp4] The module is enabled by default, the ``yade --test`` and ``yade --check`` pass, as well as most of examples are working. However the calculations are performed at lower ``double`` precision. A wrapper/converter layer for :ysrc:`LAPACK library <lib/compatibility/LapackCompatibility.hpp>` has been implemented. To make it work with full precision these routines should be reimplemented using Eigen.

.. _higher-hp-precision:

Double, quadruple and higher precisions
===========================================

Sometimes a critical section of the calculations in C++ would work better if it was performed in the higher precision to guarantee that it will produce the correct result in the default precision. A simple example is solving a system of linear equations (basically inverting a matrix) where some coefficients are very close to zero. Another example of alleviating such problem is the `Kahan summation algorithm <https://en.wikipedia.org/wiki/Kahan_summation_algorithm>`__.

If  :ref:`requirements <highPrecisionRealInstallation>` are satisfied, Yade supports higher precision multipliers in such a way that ``RealHP<1>`` is the ``Real`` type described above, and every higher number is a multiplier of the ``Real`` precision. ``RealHP<2>`` is double precision of ``RealHP<1>``, ``RealHP<4>`` is quadruple precision and so on. The general formula for amount of decimal places is implemented in :ysrccommit:`RealHP.hpp<26bffeb7ef4fd0d15e4faa025f68f97381621f04/lib/high-precision/RealHP.hpp#L84>` file and the number of decimal places used is simply a multiple N of decimal places in ``Real`` precision, it is used when native types are not available. The family of available native precision types is listed in the :ysrccommit:`RealHPLadder <26bffeb7ef4fd0d15e4faa025f68f97381621f04/lib/high-precision/RealHP.hpp#L100>` type list.

All types listed in :ysrc:`MathEigenTypes.hpp<lib/high-precision/MathEigenTypes.hpp>` follow the same naming pattern: ``Vector3rHP<1>`` is the regular ``Vector3r`` and ``Vector3rHP<N>`` for any supported N uses the precision multiplier N. One could then use an Eigen algorithm for solving a system of linear equations with a higher N using ``MatrixXrHP<N>`` to obtain the result with higher precision. Then continuing calculations in default ``Real`` precision, after the critical section is done. The same naming convention is used for CGAL types, e.g. ``CGAL_AABB_treeHP<N>`` which are declared in file :ysrc:`AliasCGAL.hpp<lib/base/AliasCGAL.hpp>`.

Before we fully move to C++20 standard, one small restriction is in place: the precision multipliers actually supported are determined by these two defines in the :ysrccommit:`RealHPConfig.hpp <39a9a8c975a640dca6217355894c1c3b44963ecb/lib/high-precision/RealHPConfig.hpp#L15>` file:

1. ``#define YADE_EIGENCGAL_HP (1)(2)(3)(4)(8)(10)(20)`` - the multipliers listed here will work in C++ for ``RealHP<N>`` in CGAL and Eigen. They are cheap in compilation time, but have to be listed here nonetheless. After we move code to C++20 this define will be removed and all multipliers will be supported via `single template constraint <https://en.cppreference.com/w/cpp/language/constraints>`__. This inconvenience arises from the fact that both CGAL and Eigen libraries offer template specializations only for a *specific* type, not a generalized family of types. Thus this define is used to declare the required :ysrc:`template specializations<lib/high-precision/RealHPEigenCgal.hpp>`.

.. hint::
	The highest precision available by default N= ``(20)`` corresponds to 300 decimal places when compiling Yade with the default settings, without changing ``REAL_DECIMAL_PLACES=……`` cmake compilation option.

2. ``#define YADE_MINIEIGEN_HP (1)(2)``       - the precision multipliers listed here are exported to python, they are expensive: each one makes compilation longer by 1 minute. Adding more can be useful only for debugging purposes. The double ``RealHP<2>`` type is by default listed here to allow exploring the higher precision types from python. Also please note that ``mpmath`` supports `only one precision <http://mpmath.org/doc/current/basics.html#temporarily-changing-the-precision>`__ at a time. Having different ``mpmath`` variables with different precision is poorly supported, albeit ``mpmath`` authors promise to improve that in the future. Fortunately this is not a big problem for Yade users because the general goal here is to allow more precise calculations in the critical sections of C++ code, not in python. This problem is partially mitigated by *changing* :ysrccommit:`mpmath precision each time <3c49f39078e5b82cf6522b7e8651d40895aac8ef/lib/high-precision/ToFromPythonConverter.hpp#L32>` when a ``C++`` ↔ ``python`` conversion occurs. So one should keep in mind that the variable ``mpmath.mp.dps`` always reflects the precision used by latest conversion performed, even if that conversion took place in GUI (not in the running script). Existing ``mpmath`` variables are not truncated to lower precision, their extra digits are simply ignored until ``mpmath.mp.dps`` is increased again, however the truncation might occur during assignment.

On some occasions it is useful to have an intuitive up-conversion between C++ types of different precisions, say for example to add ``RealHP<1>`` to ``RealHP<2>`` type. The file :ysrccommit:`UpconversionOfBasicOperatorsHP.hpp <26bffeb7ef4fd0d15e4faa025f68f97381621f04/lib/high-precision/UpconversionOfBasicOperatorsHP.hpp#L134>` serves this purpose. This header is not included by default, because more often than not, adding such two different types will be a mistake (efficiency--wise) and compiler will catch them and complain. After including this header this operation will become possible and the resultant type of such operation will be always the higher precision of the two types used. This file should be included only in ``.cpp`` files. If it was included in any ``.hpp`` file then it could pose problems with C++ type safety and will have unexpected consequences. An example usage of this header is in the :ysrccommit:`following test routine<61fc7f208027344e27dc832052b3f8c911a5909e/py/high-precision/_math.cpp#L909>`.


.. warning:: Trying to use N unregistered in ``YADE_MINIEIGEN_HP`` for a ``Vector3rHP<N>`` type inside the ``YADE_CLASS_BASE_DOC_ATTRS_*`` macro to export it to python will not work. Only these N listed in ``YADE_MINIEIGEN_HP`` will work. However it is safe (and intended) to use these from ``YADE_EIGENCGAL_HP`` in the C++ calculations in critical sections of code, without exporting them to python.

Compatibility
===========================================

.. _python-hp-compatibility:

Python
----------------------------------------------

Python has :ysrc:`native support <lib/high-precision/ToFromPythonConverter.hpp>` for high precision types using ``mpmath`` package. Old Yade scripts that use :ref:`supported modules <supported-hp-modules>` can be immediately converted to high precision by switching to ``yade.minieigenHP``. In order to do so, the following line:

.. code-block:: python

	from minieigen import *

has to be replaced with:

.. code-block:: python

	from yade.minieigenHP import *

Respectively ``import minieigen`` has to be replaced with ``import yade.minieigenHP as minieigen``, the old name ``as minieigen`` being used only for the sake of backward compatibility. Then high precision (binary compatible) version of minieigen is used when non ``double`` type is used as ``Real``.

The ``RealHP<N>`` :ref:`higher precision<higher-hp-precision>` vectors and matrices can be accessed in python by using the ``.HPn`` module scope. For example::

	import yade.minieigenHP as mne
	mne.HP2.Vector3(1,2,3) # produces Vector3 using RealHP<2> precision
	mne.Vector3(1,2,3)     # without using HPn module scope it defaults to RealHP<1>

The respective math functions such as::

	import yade.math as mth
	mth.HP2.sqrt(2) # produces square root of 2 using RealHP<2> precision
	mth.sqrt(2)     # without using HPn module scope it defaults to RealHP<1>

are supported as well and work by using the respective C++ function calls, which is usually faster than the ``mpmath`` functions.

.. warning:: There may be still some parts of python code that were not migrated to high precision and may not work well with ``mpmath`` module. See :ref:`debugging section <hp-debugging>` for details.

.. _cpp-hp-compatibility:

C++
----------------------------------------------

Before introducing high precision it was assumed that ``Real`` is actually a `POD <https://en.cppreference.com/w/cpp/named_req/PODType>`__ ``double`` type. It was possible to use ``memset(…)``, ``memcpy(…)`` and similar functions on ``double``. This was not a good approach and even some compiler ``#pragma`` commands were used to silence the compilation warnings. To make ``Real`` work with other types, this assumption had `to be removed <https://gitlab.com/yade-dev/trunk/-/merge_requests/381>`__. A single ``memcpy(…)`` still remains in file :ysrccommit:`openmp-accu.hpp<de696763ea3ab8a88136976fb4d11eb3bd79fcbc/lib/base/openmp-accu.hpp#L42>` and will have to be removed. In future development such raw memory access functions are to be avoided.

All remaining ``double`` were replaced with ``Real`` and any attempts to use ``double`` type in the code will fail in the gitlab-CI pipeline.

Mathematical functions of all high precision types are wrapped using file :ysrc:`MathFunctions.hpp<lib/high-precision/MathFunctions.hpp>`, these are the inline redirections to respective functions of the type that Yade is currently being compiled with. The code will not pass the pipeline checks if ``std::`` is used. All functions that take ``Real`` argument should now call these functions in ``yade::math::`` namespace. Functions which take *only* ``Real`` arguments may omit ``math::`` specifier and use `ADL <https://en.cppreference.com/w/cpp/language/adl>`__ instead. Examples:

1. Call to ``std::min(a,b)`` is replaced with ``math::min(a,b)``, because ``a`` or ``b`` may be ``int`` (non ``Real``) therefore ``math::`` is necessary.
2. Call to ``std::sqrt(a)``  can be replaced with either ``sqrt(a)`` or ``math::sqrt(a)`` thanks to `ADL <https://en.cppreference.com/w/cpp/language/adl>`__, because ``a`` is always ``Real``.

If a new mathematical function is needed it has to be added in the following places:

1. :ysrc:`lib/high-precision/MathFunctions.hpp`
2. :ysrc:`py/high-precision/_math.cpp`, see :yref:`math module<yade.math>` for details.
3. :ysrc:`py/tests/testMath.py`
4. :ysrc:`py/tests/testMathHelper.py`

The tests for a new function are to be added in :ysrc:`py/tests/testMath.py` in one of these functions: ``oneArgMathCheck(…):``, ``twoArgMathCheck(…):``, ``threeArgMathCheck(…):``. A table of approximate expected error tolerances in ``self.defaultTolerances`` is to be supplemented as well. To determine tolerances with better confidence it is recommended to temporarily increase number of tests in the :ysrccommit:`test loop<3c49f39078e5b82cf6522b7e8651d40895aac8ef/py/tests/testMath.py#L593>`. To determine tolerances for currently implemented functions a ``range(1000000)`` in the loop was used.

.. _extraStringDigits:

String conversions
----------------------------------------------

On the ``python`` side it is recommended to use :yref:`yade.math.toHP1(…)<yade.math.toHP1>` to declare ``python`` variables and :yref:`yade.math.radiansHP1(…)<yade.math.radiansHP1>` to convert angles to radians using :yref:`full Pi precision<yade._math.HP1.Pi>`.

On the ``C++`` side it is recommended to use :ysrccommit:`yade::math::toString(…)<3c49f39078e5b82cf6522b7e8651d40895aac8ef/lib/high-precision/RealIO.hpp#L78>` and :ysrccommit:`yade::math::fromStringReal(…)<3c49f39078e5b82cf6522b7e8651d40895aac8ef/lib/high-precision/RealIO.hpp#L80>` conversion functions instead of ``boost::lexical_cast<std::string>(…)``. The ``toString`` and its high precision version :ysrccommit:`toStringHP<3c49f39078e5b82cf6522b7e8651d40895aac8ef/lib/high-precision/RealIO.hpp#L37>` functions (in file :ysrc:`RealIO.hpp<lib/high-precision/RealIO.hpp>`) guarantee full precision during conversion. It is important to note that ``std::to_string`` does `not guarantee this <https://en.cppreference.com/w/cpp/string/basic_string/to_string>`__ and ``boost::lexical_cast`` does `not guarantee this either <https://www.boost.org/doc/libs/1_72_0/doc/html/boost_lexical_cast.html>`__.

For higher precision types it is possible to control in runtime the precision of ``C++`` ↔ ``python`` during the ``RealHP<N>`` string conversion by changing the :yref:`yade.math.RealHPConfig.extraStringDigits10<yade._math.RealHPConfig>` static parameter. Each decimal digit needs $\log_{10}(2)\approx3.3219$ bits. The ``std::numeric_limits<Real>::digits10`` provides information about how many decimal digits are completely determined by binary representation, meaning that these digits are absolutely correct. However to convert back to binary more decimal digits are necessary because $\log_{2}(10)\approx0.3010299$ decimal digits are used by each bit, and the last digit from ``std::numeric_limits<Real>::digits10`` is not sufficient. In general 3 or more in :yref:`extraStringDigits10<yade._math.RealHPConfig>` is enough to have an always working number round tripping. However if one wants to only extract results from python, without feeding them back in to continue calculations then a smaller value of :yref:`extraStringDigits10<yade._math.RealHPConfig>` is recommended, like 0 or 1, to avoid a fake sense of having more precision, when it's not there: these extra decimal digits are not correct in decimal sense. They are only there to have working number round tripping. See also a `short discussion about this <https://github.com/boostorg/multiprecision/pull/249>`__ with boost developers. Also see file :ysrc:`RealHPConfig.cpp<lib/high-precision/RealHPConfig.cpp>` for more details.

.. TODO is that explanation clear enough? A bit more is in lib/high-precision/RealHPConfig.cpp

.. note::
	The parameter ``extraStringDigits10`` does not affect ``double`` conversions, because ``boost::python`` uses an internal converter for this particular type. It might be changed in the future if the need arises. E.g. using a class similar to :ysrc:`ThinRealWrapper<lib/high-precision/ThinRealWrapper.hpp>`.

.. comment TODO once documentation builds on g++ ver > 9.2.1 replace this example with actual code that gets run while building documentation.
It is important to note that creating higher types such as ``RealHP<2>`` from string representation of ``RealHP<1>`` is ambiguous. Consider following example::

	import yade.math as mth

	mth.HP1.getDecomposedReal(1.23)['bits']
	Out[2]: '10011101011100001010001111010111000010100011110101110'

	mth.HP2.getDecomposedReal('1.23')['bits']  # passing the same arg in decimal format to HP2 produces nonzero bits after the first 53 bits of HP1
	Out[3]: '10011101011100001010001111010111000010100011110101110000101000111101011100001010001111010111000010100011110101110'

	mth.HP2.getDecomposedReal(mth.HP1.toHP2(1.23))['bits'] # it is possible to use yade.math.HPn.toHPm(…) conversion, which preserves binary representation
	Out[4]: '10011101011100001010001111010111000010100011110101110000000000000000000000000000000000000000000000000000000000000'

Which of these two ``RealHP<2>`` binary representations is more desirable depends on what is needed:

1. The best binary approximation of a ``1.23`` decimal.
2. Reproducing the 53 binary bits of that number into a higher precision to continue the calculations on **the same** number which was previously in lower precision.

To achieve 1. simply pass the argument ``'1.23'`` as string. To achieve 2. use :yref:`yade.math.HPn.toHPm(…)<yade._math.HP1.toHP2>` conversion, which maintains binary fidelity using a single :ysrccommit:`static_cast<RealHP<m>>(…)<e9f92ab12791fdd27b24989/py/high-precision/_RealHPDiagnostics.cpp#L215>`. Similar problem is discussed in `mpmath <http://mpmath.org/doc/current/basics.html#providing-correct-input>`__ and `boost <https://www.boost.org/doc/libs/1_73_0/libs/multiprecision/doc/html/boost_multiprecision/tut/floats/fp_eg/caveats.html>`__ documentation.

.. hint::
	All ``RealHP<N>`` function arguments that are of type higher than ``double`` can also accept decimal strings. This allows to preserve precision above python default floating point precision.

.. warning::
	On the contrary all the function arguments that are of type ``double`` can not accept decimal strings. To mitigate that one can use ``toHPn(…)`` converters with string arguments.

.. hint::
	To make debugging of this problem easier the function :yref:`yade.math.toHP1(…)<yade.math.toHP1>` will :ysrccommit:`raise RuntimeError<3c49f39078e5b82cf6522b7e8651d40895aac8ef/py/high-precision/math.py#L98>` if the argument is a python float (not a decimal string).

.. warning::
	I cannot stress this problem enough, please try running ``yade --check`` (or ``yade ./checkGravityRungeKuttaCashKarp54.py``) in precision different than ``double`` after changing :ysrccommit:`this line<e9f92ab12791fdd27b24989/scripts/checks-and-tests/checks/checkGravityRungeKuttaCashKarp54.py#L32>` into ``g = -9.81``. In this (particular and simple) case the ``getCurrentPos()`` :ysrccommit:`function<e9f92ab12791fdd27b24989/scripts/checks-and-tests/checks/checkGravityRungeKuttaCashKarp54.py#L102>` fails on the python side because low-precision ``g`` is multiplied by high-precision ``t``.

Eigen and CGAL
----------------------------------------------

Eigen and CGAL libraries have native high precision support.

* All declarations required by Eigen are provided in files :ysrc:`EigenNumTraits.hpp<lib/high-precision/EigenNumTraits.hpp>` and :ysrc:`MathEigenTypes.hpp<lib/high-precision/MathEigenTypes.hpp>`
* All declarations required by CGAL  are provided in files :ysrc:`CgalNumTraits.hpp<lib/high-precision/CgalNumTraits.hpp>` and :ysrc:`AliasCGAL.hpp<lib/base/AliasCGAL.hpp>`


.. _vtk-real-compatibility:

VTK
-------------------------------------------

Since VTK is only used to record results for later viewing in other software, such as `paraview <https://www.paraview.org/>`__, the recording of all decimal places does not seem to be necessary (for now).
Hence all recording commands in ``C++`` convert ``Real`` type down to ``double`` using ``static_cast<double>`` command. This has been implemented via classes ``vtkPointsReal``, ``vtkTransformReal`` and ``vtkDoubleArrayFromReal`` in file :ysrc:`VTKCompatibility.hpp<lib/compatibility/VTKCompatibility.hpp>`. Maybe VTK in the future will support non ``double`` types. If that will be needed, the interface can be updated there.


LAPACK
----------------------------------------------

Lapack is an external library which only supports ``double`` type. Since it is not templatized it is not possible to use it with ``Real`` type. Current solution is to `down-convert arguments <https://gitlab.com/yade-dev/trunk/-/merge_requests/379>`__ to ``double`` upon calling linear equation solver (and other functions), then convert them back to ``Real``. This temporary solution omits all benefits of high precision, so in the future Lapack is to be replaced with Eigen or other templatized libraries which support arbitrary floating point types.

.. _hp-debugging:

Debugging
===========================================

High precision is still in the experimental stages of implementation. Some errors may occur during use. Not all of these errors are caught by the checks and tests. Following examples may be instructive:

1. Trying to `use const references to Vector3r members <https://gitlab.com/yade-dev/trunk/-/merge_requests/406>`__ - a type of problem with results in a segmentation fault during runtime.
2. A part of python code `does not cooperate with mpmath <https://gitlab.com/yade-dev/trunk/-/merge_requests/414>`__ - the checks and tests do not cover all lines of the python code (yet), so more errors like this one are expected. The solution is to put the non compliant python functions into :ysrc:`py/high-precision/math.py`. Then replace original calls to this function with function in ``yade.math``, e.g. ``numpy.linspace(…)`` is replaced with ``yade.math.linspace(…)``.

The most flexibility in debugging is with the ``long double`` type, because special files :ysrc:`ThinRealWrapper.hpp<lib/high-precision/ThinRealWrapper.hpp>`, :ysrc:`ThinComplexWrapper.hpp<lib/high-precision/ThinComplexWrapper.hpp>` were written for that. They are implemented with `boost::operators <https://www.boost.org/doc/libs/1_72_0/libs/utility/operators.htm>`__, using `partially ordered field <https://www.boost.org/doc/libs/1_72_0/libs/utility/operators.htm#ordered_field_operators1>`__. Note that they `do not provide operator++ <https://gitlab.com/yade-dev/trunk/-/merge_requests/407>`__.

A couple of ``#defines`` were introduced in these two files to help debugging more difficult problems:

1. ``YADE_IGNORE_IEEE_INFINITY_NAN`` - it can be used to detect all occurrences when ``NaN`` or ``Inf`` are used. Also it is recommended to use this define when compiling Yade with ``-Ofast`` flag, without  ``-fno-associative-math -fno-finite-math-only -fsigned-zeros``
2. ``YADE_WRAPPER_THROW_ON_NAN_INF_REAL``, ``YADE_WRAPPER_THROW_ON_NAN_INF_COMPLEX`` - can be useful for debugging when calculations go all wrong for unknown reason.

Also refer to :ref:`address sanitizer section <address-sanitizer>`, as it is most useful for debugging in many cases.

.. hint::
	If crash is inside a macro, for example ``YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY``, it is useful to know where inside this macro the problem happens. For this purpose it is possible to use ``g++`` preprocessor to remove the macro and then compile the postprocessed code without the macro. Invoke the preprocessor with some variation of this command::

		g++ -E -P core/Body.hpp -I ./ -I /usr/include/eigen3 -I /usr/include/python3.7m > /tmp/Body.hpp

	Maybe use clang-format so that this file is more readable::

		./scripts/clang-formatter.sh /tmp/Body.hpp

	Be careful because such files tend to be large and clang-format is slow. So sometimes it is more useful to only use the last part of the file, where the macro was postprocessed. Then replace the macro in the original file in question, and then continue debugging. But this time it will be revealed where inside a macro the problem occurs.

.. note::
	When :ref:`asking questions <getting-help>` about High Precision it is recommended to start the question title with ``[RealHP]``.

