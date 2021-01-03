/*************************************************************************
*  2012-2020 Václav Šmilauer                                             *
*  2020      Janek Kozicki                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// compilation wall clock time: 0:05.80
#include <lib/high-precision/Real.hpp>
#include <lib/high-precision/ToFromPythonConverter.hpp>
using namespace ::yade::MathEigenTypes;
// define this for compatibility with minieigen.
#define _COMPLEX_SUPPORT
// file minieigen/expose-converters.cpp
#include <py/high-precision/minieigen/converters.hpp>
template <int N> void expose_converters(bool notDuplicate, const py::scope&)
{
	if (notDuplicate) {
		custom_VectorAnyAny_from_sequence<VectorXrHP<N>>();
		custom_VectorAnyAny_from_sequence<Vector6rHP<N>>();
		custom_VectorAnyAny_from_sequence<Vector6i>();
		custom_VectorAnyAny_from_sequence<Vector3rHP<N>>();
		custom_VectorAnyAny_from_sequence<Vector3i>();
		custom_VectorAnyAny_from_sequence<Vector2rHP<N>>();
		custom_VectorAnyAny_from_sequence<Vector2i>();
		custom_alignedBoxNr_from_seq<N, 2>();
		custom_alignedBoxNr_from_seq<N, 3>();
		custom_Quaternionr_from_axisAngle_or_angleAxis<N>();

		custom_MatrixAnyAny_from_sequence<Matrix3rHP<N>>();
		custom_MatrixAnyAny_from_sequence<Matrix6rHP<N>>();
		custom_MatrixAnyAny_from_sequence<MatrixXrHP<N>>();

#ifdef _COMPLEX_SUPPORT
		custom_VectorAnyAny_from_sequence<Vector2crHP<N>>();
		custom_VectorAnyAny_from_sequence<Vector3crHP<N>>();
		custom_VectorAnyAny_from_sequence<Vector6crHP<N>>();
		custom_VectorAnyAny_from_sequence<VectorXcrHP<N>>();
		custom_MatrixAnyAny_from_sequence<Matrix3crHP<N>>();
		custom_MatrixAnyAny_from_sequence<Matrix6crHP<N>>();
		custom_MatrixAnyAny_from_sequence<MatrixXcrHP<N>>();
#endif
	} // XXX - make sure that it's not necessary to clone attrs to child scope, like I do this in all other files.
}

// explicit instantination - tell compiler to produce a compiled version of expose_converters (it is faster when done in parallel in .cpp files)
YADE_HP_PYTHON_REGISTER(expose_converters)
