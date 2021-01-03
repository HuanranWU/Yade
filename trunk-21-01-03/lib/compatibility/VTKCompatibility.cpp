/*************************************************************************
*  2020 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// these classes serve the purpose of converting Real to double without macros.
// Maybe VTK in the future will support non-double types. If that will be needed,
// the implementation can be updated below.


#ifdef YADE_VTK
#include <lib/compatibility/VTKCompatibility.hpp>

vtkIdType vtkPointsReal::InsertNextPoint(const ::yade::Vector3r& p)
{
	return vtkPoints::InsertNextPoint((static_cast<double>(p[0])), (static_cast<double>(p[1])), (static_cast<double>(p[2])));
}

void vtkSampleFunctionReal::SetModelBounds(const ::yade::Vector3r& min, const ::yade::Vector3r& max)
{
	vtkSampleFunction::SetModelBounds(
	        (static_cast<double>(min[0])),
	        (static_cast<double>(max[0])),
	        (static_cast<double>(min[1])),
	        (static_cast<double>(max[1])),
	        (static_cast<double>(min[2])),
	        (static_cast<double>(max[2])));
}

void vtkTransformReal::Translate(const ::yade::Vector3r& p)
{
	vtkTransform::Translate((static_cast<double>(p[0])), (static_cast<double>(p[1])), (static_cast<double>(p[2])));
}

vtkIdType vtkDoubleArrayFromReal::InsertNextTuple(const ::yade::Vector3r& v)
{
	double t[3] = { static_cast<double>(v[0]), static_cast<double>(v[1]), static_cast<double>(v[2]) };
#if VTK_MAJOR_VERSION < 8
	return vtkDoubleArray::InsertNextTupleValue(t);
#else
	return vtkDoubleArray::InsertNextTuple(t);
#endif
}

vtkIdType vtkDoubleArrayFromReal::InsertNextTuple(const ::yade::Matrix3r& m)
{
	double t[9] = { static_cast<double>(m(0, 0)), static_cast<double>(m(0, 1)), static_cast<double>(m(0, 2)),
		        static_cast<double>(m(1, 0)), static_cast<double>(m(1, 1)), static_cast<double>(m(1, 2)),
		        static_cast<double>(m(2, 0)), static_cast<double>(m(2, 1)), static_cast<double>(m(2, 2)) };
#if VTK_MAJOR_VERSION < 8
	return vtkDoubleArray::InsertNextTupleValue(t);
#else
	return vtkDoubleArray::InsertNextTuple(t);
#endif
}

vtkIdType vtkDoubleArrayFromReal::InsertNextValue(const ::yade::Real& v) { return vtkDoubleArray::InsertNextValue(static_cast<double>(v)); }

#endif
