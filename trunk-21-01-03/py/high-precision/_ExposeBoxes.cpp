/*************************************************************************
*  2012-2020 Václav Šmilauer                                             *
*  2020      Janek Kozicki                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// compilation wall clock time: 0:06.92
#include <lib/high-precision/Real.hpp>
#include <lib/high-precision/ToFromPythonConverter.hpp>
using namespace ::yade::MathEigenTypes;
// file minieigen/expose-boxes.cpp
#include <py/high-precision/minieigen/visitors.hpp>
template <int N> void expose_boxes(bool notDuplicate, const py::scope& topScope)
{
	if (notDuplicate) {
		py::class_<AlignedBox3rHP<N>>("AlignedBox3", "Axis-aligned box object, defined by its minimum and maximum corners", py::init<>())
		        .def(AabbVisitor<AlignedBox3rHP<N>>());

		py::class_<AlignedBox2rHP<N>>("AlignedBox2", "Axis-aligned box object in 2d, defined by its minimum and maximum corners", py::init<>())
		        .def(AabbVisitor<AlignedBox2rHP<N>>());
	} else {
		py::scope().attr("AlignedBox3") = topScope.attr("AlignedBox3");
		py::scope().attr("AlignedBox2") = topScope.attr("AlignedBox2");
	}
}

// explicit instantination - tell compiler to produce a compiled version of expose_converters (it is faster when done in parallel in .cpp files)
YADE_HP_PYTHON_REGISTER(expose_boxes)
