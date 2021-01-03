/*CWBoon 2016 */
/* Please cite: */
/* CW Boon, GT Houlsby, S Utili (2015).  Designing Tunnel Support in Jointed Rock Masses Via the DEM.  Rock Mechanics and Rock Engineering,  48 (2), 603-632. */
#if defined(YADE_POTENTIAL_BLOCKS) && defined(YADE_VTK)
#pragma once
#include <lib/compatibility/LapackCompatibility.hpp>
#include <pkg/dem/PotentialBlock.hpp>
#include <pkg/dem/PotentialBlock2AABB.hpp>

#include <pkg/common/PeriodicEngines.hpp>
#include <vector>

#include <stdio.h>

#include <vtkPolyData.h>

// https://codeyarns.com/2014/03/11/how-to-selectively-ignore-a-gcc-warning/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wcomment"
// Code that generates this warning, Note: we cannot do this trick in yade. If we have a warning in yade, we have to fix it! See also https://gitlab.com/yade-dev/trunk/merge_requests/73
// This method will work once g++ bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431#c34 is fixed.
#include <vtkTriangle.h>
#pragma GCC diagnostic pop

#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCylinderSource.h>
#include <vtkExtractVOI.h>
#include <vtkFloatArray.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredPoints.h>
#include <vtkStructuredPointsWriter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkWriter.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLStructuredGridWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include <ClpSimplex.hpp>
#include <CoinBuild.hpp>
#include <CoinHelperFunctions.hpp>
#include <CoinModel.hpp>
#include <CoinTime.hpp>

namespace yade { // Cannot have #include directive inside.

class RockLiningGlobal : public PeriodicEngine {
protected:
	Real stiffnessMatrix[36];
	//Real * globalStiffnessMatrix;
	Real globalStiffnessMatrix[3 * 3 * 200 * 200];

public:
#if 0
		struct Bolts{
			Bolts(Vector3r pt1, Vector3r  pt2){startingPoint = pt1; endPoint=pt2; }
			Vector3r startingPoint;
			Vector3r endPoint;

			/* variables stored in sequence starting from the block closest to the opening */
			vector<int> blockIDs; /*blocks intersected */
			vector<Vector3r> localCoordinates; /*local coordinates inside blocks */
			vector<Real> initialLength;
		};
		vector<Bolts> bolt;
#endif

	Vector3r getNodeDistance(
	        const PotentialBlock* cm1,
	        const State*          state1,
	        const PotentialBlock* cm2,
	        const State*          state2,
	        const Vector3r        localPt1,
	        const Vector3r        localPt2);
	bool installLining(
	        const PotentialBlock* cm1,
	        const State*          state1,
	        const Vector3r        startingPt,
	        const Vector3r        direction,
	        const Real            length,
	        Vector3r&             intersectionPt);
	int  insertNode(Vector3r pos, Real mass, Real intervalLength);
	Real evaluateFNoSphereVol(const PotentialBlock* s1, const State* state1, const Vector3r newTrial);
	bool intersectPlane(
	        const PotentialBlock* s1,
	        const State*          state1,
	        const Vector3r        startingPt,
	        const Vector3r        direction,
	        const Real            length,
	        Vector3r&             intersectionPt,
	        const Vector3r        plane,
	        const Real            planeD);
	virtual void action(void);
	// clang-format off
  	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(RockLiningGlobal,PeriodicEngine,"Engine recording potential blocks as surfaces into files with given periodicity.",
		((bool,assembledKglobal,false ,,"global stiffness matrix"))
		((Real,density,0.0 ,,"density"))
		((Real,lumpedMass,0.0 ,,"lumpedMass"))
		((Real,EA,0.0 ,,"EA"))
		((Real,EI,0.0 ,,"EI"))
		((Real,initOverlap,pow(10,-5),,"initialOverlap"))
		((Real,expansionFactor,pow(10,-5),,"alpha deltaT"))
		((Real,contactLength,1.0 ,,"contactLength"))
		((vector<Real>, sigmaMax, ,,"sigma max"))
		((vector<Real>, sigmaMin, ,,"sigma min"))
		((Real,ElasticModulus,0.0 ,,"E"))
		((Real,liningThickness,0.1 ,,"liningThickness"))
		((Real,Inertia,0.0 ,,"I"))
		((vector<Real>,lengthNode, ,,"L"))
		((vector<int>,stickIDs, ,,"L"))
		((Real,Area,0.02 ,,"A"))
		((Real,interfaceStiffness,pow(10,8) ,,"L"))
		((Real,interfaceFriction,30.0 ,,"L"))
		((Real,interfaceCohesion,0.5*pow(10,6) ,,"L"))
		((Real,interfaceTension,0.8*pow(10,6) ,,"L"))
		((int,totalNodes,0 ,,"L"))
		((Vector3r,startingPoint,Vector3r(0,0,0) ,,"startingPt"))
		((vector<int>,blockIDs, ,,"ids"))
		((vector<Vector3r>,localCoordinates, ,,"local coordinates of intersection"))
		((vector<Vector3r>,refPos, ,,"initial u"))
		((vector<Vector3r>,refDir, ,,"initial v"))
		((vector<Quaternionr>,refOri, ,,"initial theta"))
		((vector<Real>,refAngle, ,,"initial theta"))
		((vector<Real>,moment, ,,"moment"))
		((vector<Real>,axialForces, ,,"force"))
		((vector<Real>,shearForces, ,,"force"))
		((vector<Real>,displacement, ,,"force"))
		((vector<Real>,radialDisplacement, ,,"force"))
		((Real,openingRad,5.0 ,,"estimated opening radius"))
		((bool,installed,false ,,"installed?"))
		((bool,openingCreated,false ,,"opening created?"))
		((vector<bool>,ruptured, ,,"ruptured"))
		((Real,axialMax,1000000000 ,,"maximum axial force"))
		((Real,shearMax,1000000000 ,,"maximum shear force"))
		((int,vtkIteratorInterval,10000 ,,"how often to print vtk"))
		((int,vtkRefTimeStep,1 ,,"first timestep to print vtk"))
		((string,fileName,,,"File prefix to save to"))
		((string,name,,,"File prefix to save to"))
		,
			//globalStiffnessMatrix = new Real[totalNodes*3*totalNodes*3];
		,
  	);
	// clang-format on
};
REGISTER_SERIALIZABLE(RockLiningGlobal);

} // namespace yade

#endif // YADE_POTENTIAL_BLOCKS && YADE_VTK
