Advanced & more
===============

Particle size distribution
--------------------------

See :ref:`periodic-triaxial-test` and :ysrc:`examples/test/psd.py`

Clumps
------

:yref:`Clump`; see :ref:`periodic-triaxial-test`

Testing laws
------------

:yref:`LawTester`, :ysrc:`scripts/checks-and-tests/law-test.py`

New law
-------



Visualization
-------------

See the example :ref:`3d-postprocessing and video recording<3d-postprocessing>`

* :yref:`VTKRecorder` & `Paraview <http://www.paraview.org>`__
* :yref:`makeVideo<yade.utils.makeVideo>`
* :yref:`SnapshotEngine`
* :ysrc:`doc/sphinx/tutorial/05-3d-postprocessing.py`
* :ysrc:`examples/test/force-network-video.py`
* :ysrc:`doc/sphinx/tutorial/make-simulation-video.py`

.. _convert-python2-to3:

Convert python 2 scripts to python 3
------------------------------------

Below is a non-exhaustive list of common things to do to convert your scripts to python 3.

Mandatory:
.............

* ``print ...`` becomes ``print(...)``,
* ``myDict.iterkeys()``, ``myDict.itervalues()``, ``myDict.iteritems()`` becomes ``myDict.keys()``, ``myDict.values()``, ``myDict.items()``,
* ``import cPickle`` becomes ``import pickle``,
* \`\` and ``<>`` operators are no longer recognized,
* inconsistent use of tabs and spaces in indentation is prohibited, for this reason all scripts in yade use tabs for indentation.

Should be checked, but not always mandatory:
.....................................
* (euclidian) division of two integers: ``i1/i2`` becomes ``i1//i2``,
* ``myDict.keys()``, ``myDict.values()``, ``myDict.items()`` becomes sometimes ``list(myDict.keys())``, ``list(myDict.values())``, ``list(myDict.items())`` (depending on your usage),
* ``map()``, ``filter()``, ``zip()`` becomes sometimes ``list(map())``, ``list(filter())``, ``list(zip())`` (depending on your usage),
* string encoding is now UTF8 everywhere, it may cause problems on user inputs/outputs (keyboard, file...) with special chars.

Optional:
............

* ``# encoding: utf-8`` no longer needed
