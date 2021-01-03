/*************************************************************************
*  2012-2020 Václav Šmilauer                                             *
*  2020      Janek Kozicki                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// compilation wall clock time: 0:24.23 → split into two files → 0:14.13
#include <lib/high-precision/Real.hpp>
#include <lib/high-precision/ToFromPythonConverter.hpp>
using namespace ::yade::MathEigenTypes;
// half of minieigen/expose-matrices.cpp
#include <py/high-precision/minieigen/visitors.hpp>
template <int N> void expose_matrices2(bool notDuplicate, const py::scope& topScope)
{
	if (notDuplicate) {
		py::class_<MatrixXrHP<N>>(
		        "MatrixX",
		        "XxX (dynamic-sized) float matrix. Constructed from list of rows (as VectorX).\n\nSupported operations (``m`` is a MatrixX, ``f`` if a "
		        "float/int, ``v`` is a VectorX): ``-m``, ``m+m``, ``m+=m``, ``m-m``, ``m-=m``, ``m*f``, ``f*m``, ``m*=f``, ``m/f``, ``m/=f``, ``m*m``, "
		        "``m*=m``, ``m*v``, ``v*m``, ``m==m``, ``m!=m``.",
		        py::init<>())
		        .def(MatrixVisitor<MatrixXrHP<N>>());
	} else {
		py::scope().attr("MatrixX") = topScope.attr("MatrixX");
	}
}

// explicit instantination - tell compiler to produce a compiled version of expose_converters (it is faster when done in parallel in .cpp files)
YADE_HP_PYTHON_REGISTER(expose_matrices2)
