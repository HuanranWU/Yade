/* CWBoon 2016 */
#ifdef YADE_POTENTIAL_BLOCKS

#pragma once

#include <lib/computational-geometry/MarchingCube.hpp>
#include <vector>
//#include <pkg/dem/PotentialBlock.hpp>
#include <pkg/common/PeriodicEngines.hpp>
#include <pkg/dem/PotentialBlock2AABB.hpp>

#ifdef YADE_VTK

#include <vtkImplicitFunction.h>
#include <vtkPolyData.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

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

#endif //YADE_VTK

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef YADE_OPENGL
#include <lib/opengl/GLUtils.hpp>
#include <lib/opengl/OpenGLWrapper.hpp>
#include <pkg/common/GLDrawFunctors.hpp>
#include <GL/glu.h>
#endif


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

namespace yade { // Cannot have #include directive inside.

#ifdef YADE_OPENGL
/* Draw PotentialBlocks using OpenGL */
class Gl1_PotentialBlock : public GlShapeFunctor {
public:
	virtual void go(const shared_ptr<Shape>&, const shared_ptr<State>&, bool, const GLViewInfo&);
	// clang-format off
			YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_PotentialBlock,GlShapeFunctor,"Renders :yref:`PotentialBlock` object",
				((bool,wire,false,,"Only show wireframe"))
			);
	// clang-format on
	RENDERS(PotentialBlock);
	//		protected:
	//			Vector3r centroid;
};
REGISTER_SERIALIZABLE(Gl1_PotentialBlock);
#endif // YADE_OPENGL

} // namespace yade


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* PREVIOUSLY EXISTING CODE DEVELOPED BY CW BOON USING THE MARCHING CUBES */
/* TODO  CREATE A BOOLEAN TO CHOOSE BETWEEN display="ACTUAL_PARTICLE" OR display="INNER_PP" */

//#ifdef YADE_OPENGL

//class Gl1_PotentialBlock : public GlShapeFunctor
//{
//	private :
//		MarchingCube mc;
//		Vector3r Min,Max;
//		vector<vector<vector<Real> > > scalarField,weights;
//		void generateScalarField(const PotentialBlock& pp);
//		void calcMinMax(const PotentialBlock& cm);
//		Vector3r isoStep;
//		Eigen::Matrix3d rotationMatrix;

//		int display;

//	public :
//		struct scalarF{
//			 vector<vector<vector<float> > > scalarField2;
//			 vector<Vector3r> triangles;
//			 vector<Vector3r> normals;
//			 int nbTriangles;
//		};

//		virtual void go(const shared_ptr<Shape>&, const shared_ptr<State>&,bool,const GLViewInfo&);
//		Real evaluateF(const PotentialBlock& pp, Real x, Real y, Real z);
//		static vector<scalarF> SF ;
//		//void clearMemory();
//
// clang-format off
//	YADE_CLASS_BASE_DOC_STATICATTRS(Gl1_PotentialBlock,GlShapeFunctor,"Renders :yref:`PotentialBlock` object",
//		((int,sizeX,30,,"Number of divisions in the X direction for triangulation"))
//		((int,sizeY,30,,"Number of divisions in the Y direction for triangulation"))
//		((int,sizeZ,30,,"Number of divisions in the Z direction for triangulation"))
//		((bool,store,true,Attr::hidden,"Whether to store initial triangulation")) //USERS DON'T NEED TO HAVE ACCESS TO THIS
//		((bool,initialized,false,Attr::hidden,"Whether the triangulation has been initialized")) //USERS DON'T NEED TO HAVE ACCESS TO THIS
//		((Real,aabbEnlargeFactor,1.3,,"some factor for displaying algorithm, try different value if you have problems with displaying"))
//		((bool,wire,false,,"Only show wireframe"))
////		((vector<scalarF>,SF,"Scalar field used by the Marching cubes algorithm"))
//	);
// clang-format on
//	RENDERS(PotentialBlock);


//};
//REGISTER_SERIALIZABLE(Gl1_PotentialBlock);

//#endif // YADE_OPENGL


namespace yade { // Cannot have #include directive inside.

#ifdef YADE_VTK
class ImpFuncPB : public vtkImplicitFunction {
public:
	vtkTypeMacro(ImpFuncPB, vtkImplicitFunction);
	//void PrintSelf(ostream& os, vtkIndent indent);

	// Create a new function
	static ImpFuncPB* New(void);
	vector<Real>      a;
	vector<Real>      b;
	vector<Real>      c;
	vector<Real>      d;
	Real              k;
	Real              r;
	Real              R;
	Matrix3r          rotationMatrix;
	bool              clump;
	Real              clumpMemberCentreX;
	Real              clumpMemberCentreY;
	Real              clumpMemberCentreZ;

	// Evaluate function
	Real FunctionValue(Real x[3]);
	Real EvaluateFunction(Real x[3])
	{
		//return this->vtkImplicitFunction::EvaluateFunction(x);
		return FunctionValue(x);
	};

	Real EvaluateFunction(Real x, Real y, Real z) { return this->vtkImplicitFunction::EvaluateFunction(x, y, z); };

	// Evaluate gradient for function
	void EvaluateGradient(
	        Real /*x*/[3], Real /*n*/[3]) {}; // FIXME - better use Vector3r here instead of Real[3] (here I only fix the unused parameter warning).

	// If you need to set parameters, add methods here

protected:
	ImpFuncPB();
	~ImpFuncPB();
	ImpFuncPB(const ImpFuncPB&)
	        : vtkImplicitFunction()
	{
	}
	void operator=(const ImpFuncPB&) { }

	// Add parameters/members here if you need
};

/* PotentialBlockVTKRecorder */
class PotentialBlockVTKRecorder : public PeriodicEngine {
public:
	vtkSmartPointer<ImpFuncPB> function;

	virtual void action(void);
	// clang-format off
	  YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(PotentialBlockVTKRecorder,PeriodicEngine,"Engine recording potential blocks as surfaces into files with given periodicity.",
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
		function = ImpFuncPB::New();
		,
		
	  );
	// clang-format on
};
REGISTER_SERIALIZABLE(PotentialBlockVTKRecorder);

#endif //YADE_VTK

} // namespace yade


#endif // YADE_POTENTIAL_BLOCKS
