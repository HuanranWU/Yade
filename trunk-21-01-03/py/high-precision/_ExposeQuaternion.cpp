/*************************************************************************
*  2012-2020 Václav Šmilauer                                             *
*  2020      Janek Kozicki                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// compilation wall clock time: 0:13.29
#include <lib/high-precision/Real.hpp>
#include <lib/high-precision/ToFromPythonConverter.hpp>
using namespace ::yade::MathEigenTypes;
// file minieigen/expose-quaternion.cpp
#include <py/high-precision/minieigen/visitors.hpp>
template <int N> void expose_quaternion(bool notDuplicate, const py::scope& topScope)
{
	if (notDuplicate) {
		py::class_<QuaternionrHP<N>>(
		        "Quaternion",
		        "Quaternion representing rotation.\n\nSupported operations (``q`` is a Quaternion, ``v`` is a Vector3): ``q*q`` (rotation "
		        "composition), ``q*=q``, ``q*v`` (rotating ``v`` by ``q``), ``q==q``, ``q!=q``.\n\nStatic attributes: ``Identity``.\n\n.. note:: "
		        "Quaternion is represented as axis-angle when printed (e.g. ``Identity`` is ``Quaternion((1,0,0),0)``, and can also be constructed "
		        "from the axis-angle representation. This is however different from the data stored inside, which can be accessed by indices ``[0]`` "
		        "(:math:`x`), ``[1]`` (:math:`y`), ``[2]`` (:math:`z`), ``[3]`` (:math:`w`). To obtain axis-angle programatically, use "
		        ":obj:`Quaternion.toAxisAngle` which returns the tuple.",
		        py::init<>())
		        .def(QuaternionVisitor<QuaternionrHP<N>>());
	} else {
		py::scope().attr("Quaternion") = topScope.attr("Quaternion");
	}
}

// explicit instantination - tell compiler to produce a compiled version of expose_converters (it is faster when done in parallel in .cpp files)
YADE_HP_PYTHON_REGISTER(expose_quaternion)
