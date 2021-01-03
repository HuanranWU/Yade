// 2009 © Václav Šmilauer <eudoxos@arcig.cz>

#include <lib/base/Logging.hpp>
#include <lib/base/Math.hpp>
#include <lib/pyutil/doc_opts.hpp>
#include <pkg/dem/SpherePack.hpp>

CREATE_CPP_LOCAL_LOGGER("_packSpheres.cpp");

// BOOST_PYTHON_MODULE cannot be inside yade namespace, it has 'extern "C"' keyword, which strips it out of any namespaces.
BOOST_PYTHON_MODULE(_packSpheres)
try {
	using SpherePack                       = ::yade::SpherePack;
	using Vector3r                         = ::yade::Vector3r;
	using Matrix3r                         = ::yade::Matrix3r;
	using Real                             = ::yade::Real;
	boost::python::scope().attr("__doc__") = "Creation, manipulation, IO for generic sphere packings.";
	YADE_SET_DOCSTRING_OPTS;
	boost::python::class_<SpherePack>(
	        "SpherePack",
	        "Set of spheres represented as centers and radii. This class is returned by :yref:`yade.pack.randomDensePack`, "
	        ":yref:`yade.pack.randomPeriPack` and others. The object supports iteration over spheres, as in \n\n\t>>> sp=SpherePack()\n\t>>> for "
	        "center,radius in sp: print center,radius\n\n\t>>> for sphere in sp: print sphere[0],sphere[1]   ## same, but without unpacking the tuple "
	        "automatically\n\n\t>>> for i in range(0,len(sp)): print sp[i][0], sp[i][1]   ## same, but accessing spheres by index\n\n\n.. admonition:: "
	        "Special constructors\n\n\tConstruct from list of ``[(c1,r1),(c2,r2),…]``. To convert two same-length lists of ``centers`` and ``radii``, "
	        "construct with ``zip(centers,radii)``.\n",
	        boost::python::init<boost::python::optional<boost::python::list>>(
	                boost::python::args("list"), "Empty constructor, optionally taking list [ ((cx,cy,cz),r), … ] for initial data."))
	        .def("add", &SpherePack::add, "Add single sphere to packing, given center as 3-tuple and radius")
	        .def("toList", &SpherePack::toList, "Return packing data as python list.")
	        .def("fromList", &SpherePack::fromList, "Make packing from given list, same format as for constructor. Discards current data.")
	        .def("fromList",
	             &SpherePack::fromLists,
	             (boost::python::arg("centers"), boost::python::arg("radii")),
	             "Make packing from given list, same format as for constructor. Discards current data.")
	        .def("load", &SpherePack::fromFile, (boost::python::arg("fileName")), "Load packing from external text file (current data will be discarded).")
	        .def("save", &SpherePack::toFile, (boost::python::arg("fileName")), "Save packing to external text file (will be overwritten).")
	        .def("fromSimulation", &SpherePack::fromSimulation, "Make packing corresponding to the current simulation. Discards current data.")
	        //The basic sphere generator
	        .def("makeCloud",
	             &SpherePack::makeCloud,
	             (boost::python::arg("minCorner")      = Vector3r(Vector3r::Zero()),
	              boost::python::arg("maxCorner")      = Vector3r(Vector3r::Zero()),
	              boost::python::arg("rMean")          = -1,
	              boost::python::arg("rRelFuzz")       = 0,
	              boost::python::arg("num")            = -1,
	              boost::python::arg("periodic")       = false,
	              boost::python::arg("porosity")       = 0.65,
	              boost::python::arg("psdSizes")       = std::vector<Real>(),
	              boost::python::arg("psdCumm")        = std::vector<Real>(),
	              boost::python::arg("distributeMass") = false,
	              boost::python::arg("seed")           = -1,
	              boost::python::arg("hSize")          = Matrix3r(Matrix3r::Zero())),
	             R"""(Create a random cloud of particles enclosed in a parallelepiped. The resulting packing is a gas-like state with no contacts between particles initially. Usually used as a first step before reaching a dense packing.

				:param Vector3 minCorner: lower corner of an axis-aligned box
				:param Vector3 maxCorner: upper corner of an axis-aligned box
				:param Matrix3 hSize: base vectors of a generalized box (arbitrary parallelepiped, typically :yref:`Cell::hSize`), superseeds minCorner and maxCorner if defined. For periodic boundaries only.
				:param float rMean: mean radius or spheres
				:param float rRelFuzz: dispersion of radius relative to rMean
				:param int num: number of spheres to be generated. If negative (default), generate as many as possible with stochastic sizes, ending after a fixed number of tries to place the sphere in space, else generate exactly ``num`` spheres with deterministic size distribution.
				:param bool periodic: whether the packing to be generated should be periodic
				:param float porosity: initial guess for the iterative generation procedure (if ``num``>1). The algorithm will be retrying until the number of generated spheres is ``num``. The first iteration tries with the provided porosity, but next iterations increase it if necessary (hence an initialy high porosity can speed-up the algorithm). If ``psdSizes`` is not defined, ``rRelFuzz`` ($z$) and ``num`` ($N$) are used so that the porosity given ($\rho$) is approximately achieved at the end of generation, $r_m=\sqrt[3]{\frac{V(1-\rho)}{\frac{4}{3}\pi(1+z^2)N}}$. The default is $\rho$=0.5. The optimal value depends on ``rRelFuzz`` or  ``psdSizes``.
				:param psdSizes: sieve sizes (particle diameters) when particle size distribution (PSD) is specified.
				:param psdCumm: cummulative fractions of particle sizes given by ``psdSizes``; must be the same length as *psdSizes* and should be non-decreasing.
				:param bool distributeMass: if ``True``, given distribution will be used to distribute sphere's mass rather than radius of them.
				:param seed: number used to initialize the random number generator.
				:returns: number of created spheres, which can be lower than ``num`` depending on the method used.

				.. note::
					- Works in 2D if ``minCorner[k]=maxCorner[k]`` for one coordinate.
					- If ``num`` is defined, then sizes generation is deterministic, giving the best fit of target distribution. It enables spheres placement in descending size order, thus giving lower porosity than the random generation.
					- By default (with ``distributeMass==False``), the distribution is applied to particle count (i.e. particle count percent passing). The typical geomechanics sense of "particle size distribution" is the distribution of *mass fraction* (i.e. mass percent passing); this can be achieved with ``distributeMass=True``.
					- Sphere radius distribution can be specified using one of the following ways:

						1. ``rMean``, ``rRelFuzz`` and ``num`` gives uniform radius distribution in $rMean×(1 \pm rRelFuzz)$. Less than ``num`` spheres can be generated if it is too high.
						2. ``rRelFuzz``, ``num`` and (optional) ``porosity``, which estimates mean radius so that ``porosity`` is attained at the end.  ``rMean`` must be less than 0 (default). ``porosity`` is only an initial guess for the generation algorithm, which will retry with higher porosity until the prescibed *num* is obtained.
						3. ``psdSizes`` and ``psdCumm``, two arrays specifying points of the `particle size distribution <http://en.wikipedia.org/wiki/Particle_size_distribution>`_ function. As many spheres as possible are generated.
						4. ``psdSizes``, ``psdCumm``, ``num``, and (optional) ``porosity``, like above but if ``num`` is not obtained, ``psdSizes`` will be scaled down uniformly, until ``num`` is obtained (see :yref:`appliedPsdScaling<yade._packSpheres.SpherePack.appliedPsdScaling>`).
				)""")

	        .def("psd",
	             &SpherePack::psd,
	             (boost::python::arg("bins") = 50, boost::python::arg("mass") = true),
	             "Return `particle size distribution <http://en.wikipedia.org/wiki/Particle_size_distribution>`__ of the packing.\n\n:param int bins: "
	             "number of bins between minimum and maximum diameter\n:param mass: Compute relative mass rather than relative particle count for each "
	             "bin. Corresponds to :yref:`distributeMass parameter for makeCloud<yade.pack.SpherePack.makeCloud>`.\n:returns: tuple of "
	             "``(cumm,edges)``, where ``cumm`` are cummulative fractions for respective diameters  and ``edges`` are those diameter values. Dimension "
	             "of both arrays is equal to ``bins+1``.")
	        //The variant for clumps
	        .def("makeClumpCloud",
	             &SpherePack::makeClumpCloud,
	             (boost::python::arg("minCorner"),
	              boost::python::arg("maxCorner"),
	              boost::python::arg("clumps"),
	              boost::python::arg("periodic") = false,
	              boost::python::arg("num")      = -1,
	              boost::python::arg("seed")     = -1),
	             R"""(Create a random loose packing of clumps the same way `makeCloud <https://yade-dem.org/doc/yade.pack.html?highlight=makecloud#yade._packSpheres.SpherePack.makeCloud>`_ does with spheres. The parameters ``minCorner``, ``maxCorner``, ``periodic``, ``num`` and ``seed`` are the same as in makeCloud_. The parameter ``clumps`` is a list containing all the different clumps to be appended as ``SpherePack`` objects. Here is an exemple that shows how to create a cloud made of 10 identical clumps :

				.. code-block:: python

					clp = SpherePack([((0,0,0), 1e-2), ((1e-2,0,0), 1e-2)]) # The clump we want a cloud of
					sp = SpherePack()
					sp.makeClumpCloud((0,0,0), (1,1,1), [clp], num=10, seed=42)
					sp.toSimulation() # All the particles in the cloud are now appended to O.bodies
				)""")
	        .def("aabb", &SpherePack::aabb_py, "Get axis-aligned bounding box coordinates, as 2 3-tuples.")
	        .def("dim", &SpherePack::dim, "Return dimensions of the packing in terms of aabb(), as a 3-tuple.")
	        .def("center", &SpherePack::midPt, "Return coordinates of the bounding box center.")
	        .def_readwrite(
	                "cellSize",
	                &SpherePack::cellSize,
	                "Size of periodic cell; is Vector3(0,0,0) if not periodic. (Change this property only if you know what you're doing).")
	        .def_readwrite("isPeriodic", &SpherePack::isPeriodic, "was the packing generated in periodic boundaries?")
	        .def("cellFill",
	             &SpherePack::cellFill,
	             "Repeat the packing (if periodic) so that the results has dim() >= given size. The packing retains periodicity, but changes cellSize. "
	             "Raises exception for non-periodic packing.")
	        .def("cellRepeat",
	             &SpherePack::cellRepeat,
	             "Repeat the packing given number of times in each dimension. Periodicity is retained, cellSize changes. Raises exception for non-periodic "
	             "packing.")
	        .def("relDensity",
	             &SpherePack::relDensity,
	             "Relative packing density, measured as sum of spheres' volumes / aabb volume.\n(Sphere overlaps are ignored.)")
	        .def("translate", &SpherePack::translate, "Translate all spheres by given vector.")
	        .def("rotate",
	             &SpherePack::rotate,
	             (boost::python::arg("axis"), boost::python::arg("angle")),
	             "Rotate all spheres around packing center (in terms of aabb()), given axis and angle of the rotation.")
	        .def("scale", &SpherePack::scale, "Scale the packing around its center (in terms of aabb()) by given factor (may be negative).")
	        .def("hasClumps", &SpherePack::hasClumps, "Whether this object contains clumps.")
	        .def("getClumps",
	             &SpherePack::getClumps,
	             "Return lists of sphere ids sorted by clumps they belong to. The return value is (standalones,[clump1,clump2,…]), where each item is list "
	             "of id's of spheres.")
	        .def("__len__", &SpherePack::len, "Get number of spheres in the packing")
	        .def("__getitem__", &SpherePack::getitem, "Get entry at given index, as tuple of center and radius.")
	        .def("__iter__", &SpherePack::getIterator, "Return iterator over spheres.")
	        .def_readonly("appliedPsdScaling", &SpherePack::appliedPsdScaling, "A factor between 0 and 1, uniformly applied on all sizes of of the PSD.");
	boost::python::class_<SpherePack::_iterator>("SpherePackIterator", boost::python::init<SpherePack::_iterator&>())
	        .def("__iter__", &SpherePack::_iterator::iter)
	        .def("__next__", &SpherePack::_iterator::next);

} catch (...) {
	LOG_FATAL("Importing this module caused an exception and this module is in an inconsistent state now.");
	PyErr_Print();
	PyErr_SetString(PyExc_SystemError, __FILE__);
	boost::python::handle_exception();
	throw;
}
