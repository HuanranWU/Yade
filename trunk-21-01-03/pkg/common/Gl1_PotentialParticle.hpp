/*CWBoon 2015 */
#pragma once
#ifdef YADE_POTENTIAL_PARTICLES
#include <lib/computational-geometry/MarchingCube.hpp>
#include <pkg/common/GLDrawFunctors.hpp>
#include <pkg/common/PeriodicEngines.hpp>
#include <pkg/dem/PotentialParticle.hpp>
#include <pkg/dem/PotentialParticle2AABB.hpp>
#include <vector>

#ifdef YADE_VTK
#include <lib/compatibility/VTKCompatibility.hpp>
#include <vtkImplicitFunction.h>
#include <vtkPolyData.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkXMLUnstructuredGridWriter.h>
// https://codeyarns.com/2014/03/11/how-to-selectively-ignore-a-gcc-warning/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wcomment"
// Code that generates this warning, Note: we cannot do this trick in yade. If we have a warning in yade, we have to fix it! See also https://gitlab.com/yade-dev/trunk/merge_requests/73
// This method will work once g++ bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431#c34 is fixed.
#include <vtkTriangle.h>
#pragma GCC diagnostic pop
#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCylinderSource.h>
#include <vtkExtractVOI.h>
#include <vtkFloatArray.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSampleFunction.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredPoints.h>
#include <vtkStructuredPointsWriter.h>
#include <vtkTextActor.h>
#include <vtkTextActor3D.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVectorText.h>
#include <vtkWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLStructuredGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>

#endif // YADE_VTK

namespace yade { // Cannot have #include directive inside.

#ifdef YADE_VTK
class ImpFunc : public vtkImplicitFunction {
public:
	vtkTypeMacro(ImpFunc, vtkImplicitFunction);
	//void PrintSelf(ostream& os, vtkIndent indent);

	// Description
	// Create a new function
	static ImpFunc* New(void);
	vector<Real>    a;
	vector<Real>    b;
	vector<Real>    c;
	vector<Real>    d;
	Real            k;
	Real            r;
	Real            R;
	Matrix3r        rotationMatrix;
	bool            clump;
	Real            clumpMemberCentreX;
	Real            clumpMemberCentreY;
	Real            clumpMemberCentreZ;
	// Description
	// Evaluate function
	Real FunctionValue(Real x[3]);
#if defined(YADE_REAL_BIT) and (YADE_REAL_BIT != 64)
	// unfortunately vtkImplicitFunction only works with double. The VTK library would need to be patched.
	// The workaround is to convert double arguments to Real. Precision is lost along the way.
	// Or stop using vtkImplicitFunction and use your own functions.
	double FunctionValue(double x[3])
	{
		// convert double arguments to Real (with loss of precision)
		Real r[3] = { static_cast<Real>(x[0]), static_cast<Real>(x[1]), static_cast<Real>(x[2]) };
		return static_cast<double>(FunctionValue(r));
	}
#endif
	virtual double EvaluateFunction(double x[3])
	{
		//return this->vtkImplicitFunction::EvaluateFunction(x);
		return FunctionValue(x);
	};

	virtual double EvaluateFunction(double x, double y, double z) { return this->vtkImplicitFunction::EvaluateFunction(x, y, z); };


	// Description
	// Evaluate gradient for function
	virtual void EvaluateGradient(
	        double /*x*/[3], double /*n*/[3]) {}; // FIXME - better use Vector3r here instead of Real[3] (here I only fix the unused parameter warning).
	//                                                 ↑     you would need to change the virtual function signature in file vtkImplicitFunction.h in VTK library. Which of course cannot be done
	//                                                       without sending a patch to VTK library authors. So better to use your own function instead of vtkImplicitFunction. / Janek

	// If you need to set parameters, add methods here

protected:
	ImpFunc();
	~ImpFunc();
	ImpFunc(const ImpFunc&)
	        : vtkImplicitFunction()
	{
	}
	void operator=(const ImpFunc&) { }

	// Add parameters/members here if you need
};

#endif // YADE_VTK

#ifdef YADE_OPENGL
class Gl1_PotentialParticle : public GlShapeFunctor {
private:
	MarchingCube                 mc;
	Vector3r                     min, max;
	vector<vector<vector<Real>>> scalarField, weights;
	void                         generateScalarField(const PotentialParticle& pp);
	void                         calcMinMax(const PotentialParticle& pp);
	Vector3r                     isoStep;
	struct scalarF {
		vector<Vector3r> triangles;
		vector<Vector3r> normals;
		int              nbTriangles;
	};
	Real                   evaluateF(const PotentialParticle& pp, Real x, Real y, Real z);
	static vector<scalarF> SF;

public:
	virtual void go(const shared_ptr<Shape>&, const shared_ptr<State>&, bool, const GLViewInfo&);

	// clang-format off
		YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_PotentialParticle,GlShapeFunctor,"Renders :yref:`PotentialParticle` object",
			((int,sizeX,20,,"Number of divisions in the X direction for triangulation"))
			((int,sizeY,20,,"Number of divisions in the Y direction for triangulation"))
			((int,sizeZ,20,,"Number of divisions in the Z direction for triangulation"))
			((bool,store,true,,"Whether to store computed triangulation or not"))
			((bool,initialized,false,,"Whether the triangulation is initialized"))
			((Real,aabbEnlargeFactor,1.3,,"Enlargement factor of the Marching Cubes drawing grid, used for displaying purposes. Try different value if the particles are not displayed properly"))
			((bool,wire,false,,"Only show wireframe"))
		);
	// clang-format on
	RENDERS(PotentialParticle);
};
REGISTER_SERIALIZABLE(Gl1_PotentialParticle);
#endif // YADE_OPENGL

#ifdef YADE_VTK
class PotentialParticleVTKRecorder : public PeriodicEngine {
public:
	vtkSmartPointer<ImpFunc> function;

	virtual void action(void);
	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(PotentialParticleVTKRecorder,PeriodicEngine,"Engine recording potential blocks as surfaces into files with given periodicity.",
			((string,fileName,,,"File prefix to save to"))
			((int,sampleX,30,,"Number of divisions in the X direction for triangulation"))
			((int,sampleY,30,,"Number of divisions in the Y direction for triangulation"))
			((int,sampleZ,30,,"Number of divisions in the Z direction for triangulation"))
			((Real,maxDimension,30,,"Maximum allowed distance between consecutive grid lines"))
			((bool,twoDimension,false,,"Whether to render the particles as 2-D"))
			((bool,REC_INTERACTION,false,,"Whether to record contact point and forces"))
			((bool,REC_COLORS,false,,"Whether to record colors"))
			((bool,REC_VELOCITY,false,,"Whether to record velocity"))
			((bool,REC_ID,true,,"Whether to record id"))
			,
			function = ImpFunc::New();
			,

		);
	// clang-format on
};
REGISTER_SERIALIZABLE(PotentialParticleVTKRecorder);

#endif // YADE_VTK

} // namespace yade

#endif // YADE_POTENTIAL_PARTICLES
