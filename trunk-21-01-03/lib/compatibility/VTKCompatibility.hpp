// 2019 Janek Kozicki

/*
vtk changed their function name in vtk8, this is annoying. And we don't want to litter the code with
#ifdefs everywhere. Better to clean it up with vim comamnds:

:grep -E "InsertNextTupleValue" --include="*" . -R --exclude ChangeLog --exclude tags --exclude CMakeLists.txt
:%s/InsertNextTupleValue(\([^)]\+\))/INSERT_NEXT_TUPLE(\1)/gc

And use a macro in these places:
*/
#pragma once

#ifdef YADE_VTK
#include <vtkVersion.h>
// fix InsertNextTupleValue → InsertNextTuple name change
#if VTK_MAJOR_VERSION < 8
#define INSERT_NEXT_TUPLE(a) InsertNextTupleValue(a)
#define INSERT_NEXT_TYPED_TUPLE(a) InsertNextTupleValue(a)
#else
#define INSERT_NEXT_TUPLE(a) InsertNextTuple(a)
#define INSERT_NEXT_TYPED_TUPLE(a) InsertNextTypedTuple(a)
#endif
// (and others in the future)

#include <lib/base/Math.hpp>
#include <vtkDoubleArray.h>
#include <vtkPoints.h>
#include <vtkSampleFunction.h>
#include <vtkTransform.h>

// these classes serve the purpose of converting Real to double without macros.
// Maybe VTK in the future will support non-double types. If that will be needed,
// the interface can be updated below.

struct vtkPointsReal : public vtkPoints {
	static vtkPointsReal* New() { return new vtkPointsReal; }; // a design decision made by VTK developers
	vtkIdType             InsertNextPoint(const ::yade::Vector3r&);
};

struct vtkSampleFunctionReal : public vtkSampleFunction {
	static vtkSampleFunctionReal* New() { return new vtkSampleFunctionReal; }; // a design decision made by VTK developers
	void                          SetModelBounds(const ::yade::Vector3r& min, const ::yade::Vector3r& max);
};

struct vtkTransformReal : public vtkTransform {
	static vtkTransformReal* New() { return new vtkTransformReal; }; // a design decision made by VTK developers
	void                     Translate(const ::yade::Vector3r&);
};

struct vtkDoubleArrayFromReal : public vtkDoubleArray {
	static vtkDoubleArrayFromReal* New() { return new vtkDoubleArrayFromReal; }; // a design decision made by VTK developers
	vtkIdType                      InsertNextTuple(const ::yade::Vector3r&);
	vtkIdType                      InsertNextTuple(const ::yade::Matrix3r&);
	vtkIdType                      InsertNextValue(const ::yade::Real&);
};

#endif


/*
# At first I wanted to do this inside cmake, but it turns out that function definitions
# are not supoprted: https://cmake.org/cmake/help/v3.0/prop_dir/COMPILE_DEFINITIONS.html#prop_dir:COMPILE_DEFINITIONS

IF(${VTK_MAJOR_VERSION} LESS 8)
  ADD_DEFINITIONS("-DINSERT_NEXT_TUPLE(a)=InsertNextTupleValue(a)")
ELSE()
  ADD_DEFINITIONS("-DINSERT_NEXT_TUPLE(a)=InsertNextTuple(a)")
ENDIF()
*/
