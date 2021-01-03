/*CWBoon 2016 */
/* Please cite: */
/* CW Boon, GT Houlsby, S Utili (2015).  A new rock slicing method based on linear programming.  Computers and Geotechnics 65, 12-29. */
/* The numerical library is changed from CPLEX to CLP because subscription to the academic initiative is required to use CPLEX for free */
#ifdef YADE_POTENTIAL_BLOCKS
#pragma once
// XXX never do #include<Python.h>, see https://www.boost.org/doc/libs/1_71_0/libs/python/doc/html/building/include_issues.html
#include <boost/python/detail/wrap_python.hpp>

#include <core/FileGenerator.hpp>


#include <lib/base/Math.hpp>
#include <lib/compatibility/LapackCompatibility.hpp>
#include <lib/multimethods/Indexable.hpp>
#include <lib/serialization/Serializable.hpp>

#include <ClpSimplex.hpp>
#include <ClpSimplexDual.hpp>
#include <CoinBuild.hpp>
#include <CoinHelperFunctions.hpp>
#include <CoinModel.hpp>
#include <CoinTime.hpp>
#include <cassert>
#include <iomanip>

namespace yade { // Cannot have #include directive inside.

class GlobalStiffnessTimeStepper;

class BlockGen : public FileGenerator {
private:
	void createActors(shared_ptr<Scene>& scene);
	//		void positionRootBody(shared_ptr<Scene>& scene); //not used

	shared_ptr<GlobalStiffnessTimeStepper> globalStiffnessTimeStepper;
	std::ofstream                          myfile;

protected:
	//		std::ofstream output2;        // it was always creating files "BlkGen" "BlockGenFindExtreme.txt", but they are not used in the code, so I commented this out, Janek
	//		std::string myfile;
	//		std::string Key;
	//		static std::ofstream output;

public:
	~BlockGen();
	bool                    generate(string&);
	template <class T> Real gen_normal_3(T& generator) { return generator(); }

	struct Discontinuity {
		Vector3r centre;
		Discontinuity(Vector3r pos)
		{
			centre = pos;
			a = b = c = d      = 0; /*persistence = false;*/
			isBoundary         = false;
			sliceBoundaries    = false;
			constructionJoints = false;
			phi_b              = 30.0;
			phi_r              = 30.0;
			JRC                = 15;
			JCS                = pow(10, 6);
			asperity           = 5;
			sigmaC             = JCS;
			cohesion           = 0;
			tension            = 0;
			lambda0            = 0.0;
			heatCapacity       = 0.0;
			hwater             = -1.0;
			intactRock         = false;
			throughGoing       = false;
			jointType          = 0;
		}
		Real a;
		Real b;
		Real c;
		Real d;
		//			Real a_p;
		//			Real b_p;
		//			Real c_p;
		//			Real d_p;
		//			bool persistence;
		bool         isBoundary;
		bool         sliceBoundaries;
		bool         constructionJoints;
		vector<Real> persistence_a;
		vector<Real> persistence_b;
		vector<Real> persistence_c;
		vector<Real> persistence_d;
		/* Joint properties */
		Real phi_b;
		Real phi_r;
		Real JRC;
		Real JCS;
		Real asperity;
		Real sigmaC;
		Real cohesion;
		Real tension;
		Real lambda0;
		Real heatCapacity;
		Real hwater;
		bool intactRock;
		bool throughGoing;
		int  jointType;
	};
	struct Planes {
		vector<int> vertexID;
	};
	struct Block {
		Vector3r tempCentre;
		Vector3r centre;
		Block(Vector3r pos, Real kPP, Real rPP, Real RPP)
		{
			centre     = pos;
			k          = kPP;
			r          = rPP;
			R          = RPP;
			tooSmall   = false;
			isBoundary = false;
			tempCentre = pos;
		}
		vector<Real>         a;
		vector<Real>         b;
		vector<Real>         c;
		vector<Real>         d;
		vector<bool>         redundant;
		vector<bool>         isBoundaryPlane;
		bool                 isBoundary;
		vector<struct Block> subMembers;
		vector<Vector3r>     falseVertex;
		//			vector<Vector3r> node; Real gridVol;
		Real r;
		Real R;
		Real k;
		bool tooSmall;
		/* Joint properties */
		vector<Real>          phi_b;
		vector<Real>          phi_r;
		vector<Real>          JRC;
		vector<Real>          JCS;
		vector<Real>          asperity;
		vector<Real>          sigmaC;
		vector<Real>          cohesion;
		vector<Real>          tension;
		vector<Real>          lambda0;
		vector<Real>          heatCapacity;
		vector<Real>          hwater;
		vector<bool>          intactRock;
		vector<int>           jointType;
		vector<struct Planes> planes;

		Vector3r color;
	};

	//			Real getSignedArea(const Vector3r pt1,const Vector3r pt2, const Vector3r pt3);
	//			Real getDet(const MatrixXr A);
	bool createBlock(shared_ptr<Body>& body, struct BlockGen::Block block, int no);
	bool contactDetectionLPCLPglobal(struct BlockGen::Discontinuity joint, struct BlockGen::Block block, Vector3r& touchingPt);
	bool checkRedundancyLPCLP(struct BlockGen::Discontinuity joint, struct BlockGen::Block block, Vector3r& touchingPt);
	Real inscribedSphereCLP(struct BlockGen::Block block, Vector3r& initialPoint, bool twoDimension);
	bool contactBoundaryLPCLP(struct BlockGen::Discontinuity joint, struct BlockGen::Block block, Vector3r& touchingPt);

	/* Functions that were not used */
	//			Real getCentroidTetrahedron(const MatrixXr A); //not used
	//			bool contactDetection(struct BlockGen::Discontinuity joint, struct BlockGen::Block block, Vector3r& touchingPt); //not used
	//			bool contactDetectionLPCLP(struct BlockGen::Discontinuity joint, struct BlockGen::Block block, Vector3r& touchingPt); //not used
	//			bool startingPointFeasibility(struct BlockGen::Block block, Vector3r& initialPoint); //not used
	//			bool contactBoundaryLPCLPslack(struct BlockGen::Discontinuity joint, struct BlockGen::Block block, Vector3r& touchingPt); //not used
	//			Real evaluateFNoSphere(struct Block block, Vector3r presentTrial); //not used

	//			void calculateInertia(struct Block block, Real& Ixx, Real& Iyy, Real& Izz,Real& Ixy, Real& Ixz, Real& Iyz);
	//			Vector3r calCentroid(struct Block block, Real & blockVol);

	bool checkCentroid(struct Block block, Vector3r presentTrial);

	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(
		BlockGen,FileGenerator,"Prepare a scene for Block Generation using the Potential Blocks."
		,
		/* public */
		//TODO: Many default values could be set empty. Better for the user to get an error than use a default value if they forget to define it.
		((Real, dampingMomentum,0.2,,"Coefficient of global damping"))
//			((Real, maxClosure, 0.0002,,"not used"))
//			((Real, peakDisplacement, 0.02,,"not used"))
//			((Real, brittleLength, 2.0,,"not used"))
//			((Real, damp3DEC,0.8,,"not used"))
		((Real, unitWidth2D,1.0,,"Unit width in 2D (out of plane distance)"))
		((Real, density,2600,,"Density of blocks"))
		((Real, Kn,pow(10,9),,"Volumetric contact normal stiffness"))
		((Real, Ks,pow(10,8),,"Volumetric contact shear stiffness"))
		((Real, frictionDeg,30.0,,"Friction angle [°]"))
		((Vector3r, globalOrigin,Vector3r::Zero(),,"Global origin (reference point) for the discontinuities to be imposed"))
		((Real, inertiaFactor,1.0,,"Scaling of inertia")) //FIXME: This parameter is used, but we need to revise if it is actually useful
		((Real, rForPP,0.1,,"r in Potential Particles"))
		((Real, kForPP,0.0,,"k in Potential Particles"))
		((Real, RForPP,0.0,,"R in Potential Particles"))
//			((int, numberOfGrids,1,,"not used"))
		((bool, probabilisticOrientation,false,,"Whether to generate rock joints randomly"))
		((bool, Talesnick,false,,"Whether to choose the Talesnick contact law, used for validating code previously against model test"))
		((bool, neverErase,false,,"Whether to erase non interacting contacts"))
//		((bool, calJointLength,false,,"Whether to calculate jointLength"))
		((bool, calContactArea,true,,"Whether to calculate jointLength for 2-D contacts and contactArea for 2-D and 3-D contacts"))
		((bool, twoDimension,false,,"Whether the model is 2D"))
		((Real, shrinkFactor,1.0,,"Ratio to shrink r")) //FIXME: This parameter is used, but we need to revise if it is actually useful
		((Real, viscousDamping,0.0,,"Viscous damping"))
		((bool, intactRockDegradation,false,,"Whether to activate degradation of parameters for contact"))
		((Real, initialOverlap,0.0,,"Initial overlap between blocks"))
		((Vector3r, gravity,Vector3r(0.0,-9.81,0.0),,"Gravity"))
		((bool, traceEnergy,false,,"Whether to calculate energy terms (elastic potential energy (normal and shear), plastic dissipation due to friction and dissipation of energy (normal and tangential) due to viscous damping)"))
		((bool, exactRotation,true,,"Whether to handle the rotational motion of aspherical bodies more accurately"))
		((Real, minSize,50.0,,"Minimum size for all blocks"))
//			((Real,minSize2,50.0,,"minimum size for blocks with joint Type=2, minSize2 is smaller than minSize"))
		((Real, maxRatio,3.0,,"Minimum ratio for all blocks"))
//			((Real,maxRatio2,1000.0,,"not used"))
		((Real, boundarySizeXmax,1.0,,"Max X of domain"))
		((Real, boundarySizeYmax,1.0,,"Max Y of domain"))
		((Real, boundarySizeZmax,1.0,,"Max Z of domain"))
		((Real, boundarySizeXmin,1.0,,"Min X of domain"))
		((Real, boundarySizeYmin,1.0,,"Min Y of domain"))
		((Real, boundarySizeZmin,1.0,,"Min Z of domain"))
		((Vector3r, directionA,Vector3r(1,0,0),,"Local x-direction to check minSize"))
		((Vector3r, directionB,Vector3r(0,1,0),,"Local y-direction to check minSize"))
		((Vector3r, directionC,Vector3r(0,0,1),,"Local z-direction to check minSize"))
//			((Real, calAreaStep,10.0,,"length Z of domain"))
//			((Real, extremeDist,0.5,,"boundary to base calculation of octree algorithms"))
//			((Real, subdivisionRatio,0.1,,"smallest size/boundary of octree algorithms"))
		/* Set up GlobalStiffnessTimeStepper */
		((bool, useGlobalStiffnessTimeStepper,false,,"Whether to use :yref:`GlobalStiffnessTimeStepper`"))
		((Real, defaultDt,-1,,"Max time-step. Used as initial value if defined. Later adjusted by the time stepper"))
		((int, timeStepUpdateInterval,50,,"Interval for :yref:`GlobalStiffnessTimeStepper`"))
		/* which contact law to use */
//			((bool, useBartonBandis,false,,"not used"))  // not used: To be deleted (or better, developed) in the future
		((bool, useFaceProperties, false,,"Whether to use face properties"))
//			((bool, useOverlapVol,false,,"not used"))

		/* Add joints from python*/
		((vector<Real>, joint_a, ,,"Introduce discontinuities from Python: List of a coefficients of plane normals"))
		((vector<Real>, joint_b, ,,"Introduce discontinuities from Python: List of b coefficients of plane normals"))
		((vector<Real>, joint_c, ,,"Introduce discontinuities from Python: List of c coefficients of plane normals"))
		((vector<Real>, joint_d, ,,"Introduce discontinuities from Python: List of d coefficients of plane equations"))

		/* Add joints from .csv files: Check for different joint types */
		((bool, persistentPlanes,false,,"Whether to check persistence"))
		((bool, jointProbabilistic,false,,"Whether to check for filename jointProbabilistic"))
		((bool, boundaries,false,,"Whether to check for filename boundaries"))
		((bool, opening,false,,"Whether to check for filename opening"))
		((bool, slopeFace,false,,"Whether to check for filename slopeFace"))
		((bool, sliceBoundaries,false,,"Whether to check for filename sliceBoundaries"))
//			((bool, jointProbabilisticRockBridge,false,,"Whether to check for filename jointProbabilisticRockBridge"))

		((std::string, filenamePersistentPlanes,"./Tunnel/jointPersistent.csv",,"filename to look for joint properties"))
		((std::string, filenameProbabilistic,"./Tunnel/jointProbabilistic.csv",,"filename to look for joint with probabilistic models"))
		((std::string, filenameBoundaries,"./Tunnel/boundaries.csv",,"filename to look for joint with probabilistic models"))
		((std::string, filenameOpening,"./Tunnel/opening.csv",,"filename to look for joint outline of joints"))
		((std::string, filenameSlopeFace,"./Tunnel/opening.csv",,"filename to look for joint outline of joints"))
		((std::string, filenameSliceBoundaries,"./Tunnel/sliceBoundaries.csv",,"filename to look for joint outline of joints"))
//			((std::string,filenameProbabilisticRockBridge,"./Tunnel/jointProbabilistic.csv",,"filename to look for joint with probabilistic models"))
		((Vector3r, color,Vector3r(-1,-1,-1),,"color of generated blocks (random color will be assigned to each sub-block if a color is not specified)"))
		((bool, saveBlockGenData,false,,"Whether to write the data of the block generation in a text file (if true) or display on the terminal (if false)"))
		((std::string,outputFile,"",,"Filename where the data of the block generation are saved. Leave blank if an output file is not needed"))
		//((Real*, array_a,,,"a"))
		, /* init */
		, /* ctor */
		/* constructor for private */
//		Key			="";
//		myfile = "./BlkGen"+Key;
//		output2.open(myfile.c_str(), fstream::app);

		, /* py */
		//.def("setContactProperties",&TriaxialCompressionEngine::setContactProperties,"Assign a new friction angle (degrees) to dynamic bodies and relative interactions")
		 );
	// clang-format on
	DECLARE_LOGGER;
};

REGISTER_SERIALIZABLE(BlockGen);

} // namespace yade

#endif // YADE_POTENTIAL_BLOCKS
