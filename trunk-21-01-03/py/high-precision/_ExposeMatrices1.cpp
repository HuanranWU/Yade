/*************************************************************************
*  2012-2020 Václav Šmilauer                                             *
*  2020      Janek Kozicki                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// compilation wall clock time: 0:24.23 → split into two files → 0:14.75
#include <lib/high-precision/Real.hpp>
#include <lib/high-precision/ToFromPythonConverter.hpp>
using namespace ::yade::MathEigenTypes;
// half of minieigen/expose-matrices.cpp
#include <py/high-precision/minieigen/visitors.hpp>
template <int N> void expose_matrices1(bool notDuplicate, const py::scope& topScope)
{
	if (notDuplicate) {
		py::class_<Matrix3rHP<N>>(
		        "Matrix3",
		        "3x3 float matrix.\n\nSupported operations (``m`` is a Matrix3, ``f`` if a float/int, ``v`` is a Vector3): ``-m``, ``m+m``, ``m+=m``, "
		        "``m-m``, ``m-=m``, ``m*f``, ``f*m``, ``m*=f``, ``m/f``, ``m/=f``, ``m*m``, ``m*=m``, ``m*v``, ``v*m``, ``m==m``, ``m!=m``.\n\nStatic "
		        "attributes: ``Zero``, ``Ones``, ``Identity``.",
		        py::init<>())
		        .def(py::init<QuaternionrHP<N> const&>((py::arg("q"))))
		        .def(MatrixVisitor<Matrix3rHP<N>>());
		py::class_<Matrix6rHP<N>>(
		        "Matrix6",
		        "6x6 float matrix. Constructed from 4 3x3 sub-matrices, from 6xVector6 (rows).\n\nSupported operations (``m`` is a Matrix6, ``f`` if a "
		        "float/int, ``v`` is a Vector6): ``-m``, ``m+m``, ``m+=m``, ``m-m``, ``m-=m``, ``m*f``, ``f*m``, ``m*=f``, ``m/f``, ``m/=f``, ``m*m``, "
		        "``m*=m``, ``m*v``, ``v*m``, ``m==m``, ``m!=m``.\n\nStatic attributes: ``Zero``, ``Ones``, ``Identity``.",
		        py::init<>())
		        .def(MatrixVisitor<Matrix6rHP<N>>());
	} else {
		py::scope().attr("Matrix3") = topScope.attr("Matrix3");
		py::scope().attr("Matrix6") = topScope.attr("Matrix6");
	}
}

// explicit instantination - tell compiler to produce a compiled version of expose_converters (it is faster when done in parallel in .cpp files)
YADE_HP_PYTHON_REGISTER(expose_matrices1)
