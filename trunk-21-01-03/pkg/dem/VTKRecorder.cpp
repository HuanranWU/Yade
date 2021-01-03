#ifdef YADE_VTK

#include "VTKRecorder.hpp"
#include <lib/compatibility/VTKCompatibility.hpp> // fix InsertNextTupleValue → InsertNextTuple name change (and others in the future)

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkZLibDataCompressor.h>

#ifdef YADE_MPI
#include <core/Subdomain.hpp>
#include <mpi.h>
#include <vtkXMLPMultiBlockDataWriter.h>
#include <vtkXMLPPolyDataWriter.h>
#include <vtkXMLPUnstructuredGridWriter.h>
#endif

// https://codeyarns.com/2014/03/11/how-to-selectively-ignore-a-gcc-warning/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wcomment"
// Code that generates this warning, Note: we cannot do this trick in yade. If we have a warning in yade, we have to fix it! See also https://gitlab.com/yade-dev/trunk/merge_requests/73
// This method will work once g++ bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431#c34 is fixed.
#include <vtkTriangle.h>
#pragma GCC diagnostic pop

#include <vtkHexahedron.h>
#include <vtkLine.h>
#include <vtkQuad.h>
#ifdef YADE_VTK_MULTIBLOCK
#include <vtkMultiBlockDataSet.h>
#include <vtkXMLMultiBlockDataWriter.h>
#endif

#include <core/Scene.hpp>
#include <pkg/common/Box.hpp>
#include <pkg/common/Facet.hpp>
#include <pkg/common/Sphere.hpp>
#include <pkg/dem/ConcretePM.hpp>
#include <pkg/dem/JointedCohesiveFrictionalPM.hpp>
#include <pkg/dem/Lubrication.hpp>
#include <pkg/dem/Shop.hpp>
#include <pkg/dem/WirePM.hpp>
#include <pkg/pfv/Thermal.hpp>
#ifdef YADE_LIQMIGRATION
#include <pkg/dem/ViscoelasticCapillarPM.hpp>
#endif
#include <pkg/dem/HertzMindlin.hpp>
#include <boost/fusion/include/pair.hpp>
#include <boost/fusion/support/pair.hpp>
#include <boost/unordered_map.hpp>

namespace yade { // Cannot have #include directive inside.

YADE_PLUGIN((VTKRecorder));
CREATE_LOGGER(VTKRecorder);

#ifdef YADE_MASK_ARBITRARY
#define GET_MASK(b) b->groupMask.to_ulong()
#else
#define GET_MASK(b) b->groupMask
#endif

void VTKRecorder::action()
{
#ifdef YADE_MPI
	if (parallelMode and !sceneRefreshed) {
		// update scene pointer (to avoid issues after scene broadcast in mpi case init)
		scene = Omega::instance().getScene().get();
		MPI_Comm_size(scene->getComm(), &commSize);
		MPI_Comm_rank(scene->getComm(), &rank);
		sceneRefreshed = true;
	}
#endif
	vector<bool> recActive(REC_SENTINEL, false);
	FOREACH(string & rec, recorders)
	{
		if (rec == "all") {
			recActive[REC_SPHERES]     = true;
			recActive[REC_VELOCITY]    = true;
			recActive[REC_FACETS]      = true;
			recActive[REC_BOXES]       = true;
			recActive[REC_COLORS]      = true;
			recActive[REC_MASS]        = true;
			recActive[REC_INTR]        = true;
			recActive[REC_ID]          = true;
			recActive[REC_MASK]        = true;
			recActive[REC_CLUMPID]     = true;
			recActive[REC_MATERIALID]  = true;
			recActive[REC_STRESS]      = true;
			recActive[REC_FORCE]       = true;
			recActive[REC_COORDNUMBER] = true;
			if (scene->isPeriodic) {
				recActive[REC_PERICELL] = true;
			}
			recActive[REC_BSTRESS] = true;
		} else if (rec == "spheres") {
			recActive[REC_SPHERES] = true;
		} else if (rec == "velocity")
			recActive[REC_VELOCITY] = true;
		else if (rec == "facets")
			recActive[REC_FACETS] = true;
		else if (rec == "boxes")
			recActive[REC_BOXES] = true;
		else if (rec == "mass")
			recActive[REC_MASS] = true;
		else if (rec == "thermal")
			recActive[REC_TEMP] = true;
		else if ((rec == "colors") || (rec == "color"))
			recActive[REC_COLORS] = true;
		else if (rec == "cpm")
			recActive[REC_CPM] = true;
		else if (rec == "wpm")
			recActive[REC_WPM] = true;
		else if (rec == "intr")
			recActive[REC_INTR] = true;
		else if ((rec == "ids") || (rec == "id"))
			recActive[REC_ID] = true;
		else if (rec == "mask")
			recActive[REC_MASK] = true;
		else if ((rec == "clumpids") || (rec == "clumpId"))
			recActive[REC_CLUMPID] = true;
		else if (rec == "materialId")
			recActive[REC_MATERIALID] = true;
		else if (rec == "stress")
			recActive[REC_STRESS] = true;
		else if (rec == "force")
			recActive[REC_FORCE] = true;
		else if (rec == "jcfpm")
			recActive[REC_JCFPM] = true;
		else if (rec == "cracks")
			recActive[REC_CRACKS] = true;
		else if (rec == "moments")
			recActive[REC_MOMENTS] = true;
		else if (rec == "pericell" && scene->isPeriodic)
			recActive[REC_PERICELL] = true;
		else if (rec == "liquidcontrol")
			recActive[REC_LIQ] = true;
		else if (rec == "bstresses")
			recActive[REC_BSTRESS] = true;
		else if (rec == "coordNumber")
			recActive[REC_COORDNUMBER] = true;

		else if (rec == "SPH")
			recActive[REC_SPH] = true;
		else if (rec == "deform")
			recActive[REC_DEFORM] = true;
		else if (rec == "lubrication")
			recActive[REC_LUBRICATION] = true;
		else if (rec == "hertz")
			recActive[REC_HERTZMINDLIN] = true;
		else if (rec == "partialSat")
			recActive[REC_PARTIALSAT] = true;
		else
			LOG_ERROR(
			        "Unknown recorder named `" << rec
			                                   << "' (supported are: all, spheres, velocity, facets, boxes, color, stress, cpm, wpm, intr, id, "
			                                      "clumpId, materialId, jcfpm, cracks, moments, pericell, liquidcontrol, bstresses). Ignored.");
	}
#ifdef YADE_MPI
	if (parallelMode) {
		recActive[REC_SUBDOMAIN] = true;
	}
#endif
	// cpm needs interactions
	if (recActive[REC_CPM])
		recActive[REC_INTR] = true;

	// jcfpm needs interactions
	if (recActive[REC_JCFPM])
		recActive[REC_INTR] = true;

	// wpm needs interactions
	if (recActive[REC_WPM])
		recActive[REC_INTR] = true;

	// liquid control needs interactions
	if (recActive[REC_LIQ])
		recActive[REC_INTR] = true;

	// spheres
	vtkSmartPointer<vtkPointsReal> spheresPos   = vtkSmartPointer<vtkPointsReal>::New();
	vtkSmartPointer<vtkCellArray>  spheresCells = vtkSmartPointer<vtkCellArray>::New();

	vtkSmartPointer<vtkDoubleArrayFromReal> radii = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	radii->SetNumberOfComponents(1);
	radii->SetName("radii");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresSigI = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresSigI->SetNumberOfComponents(1);
	spheresSigI->SetName("sigI");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresSigII = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresSigII->SetNumberOfComponents(1);
	spheresSigII->SetName("sigII");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresSigIII = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresSigIII->SetNumberOfComponents(1);
	spheresSigIII->SetName("sigIII");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresDirI = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresDirI->SetNumberOfComponents(3);
	spheresDirI->SetName("dirI");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresDirII = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresDirII->SetNumberOfComponents(3);
	spheresDirII->SetName("dirII");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresDirIII = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresDirIII->SetNumberOfComponents(3);
	spheresDirIII->SetName("dirIII");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresMass = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresMass->SetNumberOfComponents(1);
	spheresMass->SetName("mass");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresId = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresId->SetNumberOfComponents(1);
	spheresId->SetName("id");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresTemp = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresTemp->SetNumberOfComponents(1);
	spheresTemp->SetName("temp");

#ifdef YADE_SPH
	vtkSmartPointer<vtkDoubleArrayFromReal> spheresRhoSPH = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresRhoSPH->SetNumberOfComponents(1);
	spheresRhoSPH->SetName("SPH_Rho");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresPressSPH = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresPressSPH->SetNumberOfComponents(1);
	spheresPressSPH->SetName("SPH_Press");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresCoordNumbSPH = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresCoordNumbSPH->SetNumberOfComponents(1);
	spheresCoordNumbSPH->SetName("SPH_Neigh");
#endif

#ifdef YADE_DEFORM
	vtkSmartPointer<vtkDoubleArrayFromReal> spheresRealRad = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresRealRad->SetNumberOfComponents(1);
	spheresRealRad->SetName("RealRad");
#endif

#ifdef YADE_LIQMIGRATION
	vtkSmartPointer<vtkDoubleArrayFromReal> spheresLiqVol = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresLiqVol->SetNumberOfComponents(1);
	spheresLiqVol->SetName("Liq_Vol");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresLiqVolIter = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresLiqVolIter->SetNumberOfComponents(1);
	spheresLiqVolIter->SetName("Liq_VolIter");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresLiqVolTotal = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresLiqVolTotal->SetNumberOfComponents(1);
	spheresLiqVolTotal->SetName("Liq_VolTotal");
#endif

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresMask = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresMask->SetNumberOfComponents(1);
	spheresMask->SetName("mask");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresCoordNumb = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresCoordNumb->SetNumberOfComponents(1);
	spheresCoordNumb->SetName("coordNumber");

	vtkSmartPointer<vtkDoubleArrayFromReal> clumpId = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	clumpId->SetNumberOfComponents(1);
	clumpId->SetName("clumpId");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresColors = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresColors->SetNumberOfComponents(3);
	spheresColors->SetName("color");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresLinVelVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresLinVelVec->SetNumberOfComponents(3);
	spheresLinVelVec->SetName("linVelVec"); //Linear velocity in Vector3 form

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresLinVelLen = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresLinVelLen->SetNumberOfComponents(1);
	spheresLinVelLen->SetName("linVelLen"); //Length (magnitude) of linear velocity

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresAngVelVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresAngVelVec->SetNumberOfComponents(3);
	spheresAngVelVec->SetName("angVelVec"); //Angular velocity in Vector3 form

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresAngVelLen = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresAngVelLen->SetNumberOfComponents(1);
	spheresAngVelLen->SetName("angVelLen"); //Length (magnitude) of angular velocity

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresNormalStressVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresNormalStressVec->SetNumberOfComponents(3);
	spheresNormalStressVec->SetName("normalStress");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresShearStressVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresShearStressVec->SetNumberOfComponents(3);
	spheresShearStressVec->SetName("shearStress");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresNormalStressNorm = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresNormalStressNorm->SetNumberOfComponents(1);
	spheresNormalStressNorm->SetName("normalStressNorm");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresLubricationNormalContactStress = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresLubricationNormalContactStress->SetNumberOfComponents(9);
	spheresLubricationNormalContactStress->SetName("lubrication_NormalContactStress");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresLubricationShearContactStress = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresLubricationShearContactStress->SetNumberOfComponents(9);
	spheresLubricationShearContactStress->SetName("lubrication_ShearContactStress");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresLubricationNormalLubricationStress = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresLubricationNormalLubricationStress->SetNumberOfComponents(9);
	spheresLubricationNormalLubricationStress->SetName("lubrication_NormalLubricationStress");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresLubricationShearLubricationStress = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresLubricationShearLubricationStress->SetNumberOfComponents(9);
	spheresLubricationShearLubricationStress->SetName("lubrication_ShearLubricationStress");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresLubricationNormalPotentialStress = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresLubricationNormalPotentialStress->SetNumberOfComponents(9);
	spheresLubricationNormalPotentialStress->SetName("lubrication_NormalPotentialStress");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresMaterialId = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresMaterialId->SetNumberOfComponents(1);
	spheresMaterialId->SetName("materialId");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresForceVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresForceVec->SetNumberOfComponents(3);
	spheresForceVec->SetName("forceVec");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresForceLen = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresForceLen->SetNumberOfComponents(1);
	spheresForceLen->SetName("forceLen");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresTorqueVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresTorqueVec->SetNumberOfComponents(3);
	spheresTorqueVec->SetName("torqueVec");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresTorqueLen = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresTorqueLen->SetNumberOfComponents(1);
	spheresTorqueLen->SetName("torqueLen");

	// facets
	vtkSmartPointer<vtkPointsReal>          facetsPos    = vtkSmartPointer<vtkPointsReal>::New();
	vtkSmartPointer<vtkCellArray>           facetsCells  = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkDoubleArrayFromReal> facetsColors = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	facetsColors->SetNumberOfComponents(3);
	facetsColors->SetName("color");

	vtkSmartPointer<vtkDoubleArrayFromReal> facetsStressVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	facetsStressVec->SetNumberOfComponents(3);
	facetsStressVec->SetName("stressVec");

	vtkSmartPointer<vtkDoubleArrayFromReal> facetsStressLen = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	facetsStressLen->SetNumberOfComponents(1);
	facetsStressLen->SetName("stressLen");

	vtkSmartPointer<vtkDoubleArrayFromReal> facetsMaterialId = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	facetsMaterialId->SetNumberOfComponents(1);
	facetsMaterialId->SetName("materialId");

	vtkSmartPointer<vtkDoubleArrayFromReal> facetsMask = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	facetsMask->SetNumberOfComponents(1);
	facetsMask->SetName("mask");

	vtkSmartPointer<vtkDoubleArrayFromReal> facetsForceVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	facetsForceVec->SetNumberOfComponents(3);
	facetsForceVec->SetName("forceVec");

	vtkSmartPointer<vtkDoubleArrayFromReal> facetsForceLen = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	facetsForceLen->SetNumberOfComponents(1);
	facetsForceLen->SetName("forceLen");

	vtkSmartPointer<vtkDoubleArrayFromReal> facetsTorqueVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	facetsTorqueVec->SetNumberOfComponents(3);
	facetsTorqueVec->SetName("torqueVec");

	vtkSmartPointer<vtkDoubleArrayFromReal> facetsTorqueLen = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	facetsTorqueLen->SetNumberOfComponents(1);
	facetsTorqueLen->SetName("torqueLen");

	vtkSmartPointer<vtkDoubleArrayFromReal> facetsCoordNumb = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	facetsCoordNumb->SetNumberOfComponents(1);
	facetsCoordNumb->SetName("coordNumber");

#ifdef YADE_MPI
	vtkSmartPointer<vtkDoubleArrayFromReal> spheresSubdomain = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresSubdomain->SetNumberOfComponents(1);
	spheresSubdomain->SetName("subdomain");
#endif


	// boxes
	vtkSmartPointer<vtkPointsReal>          boxesPos    = vtkSmartPointer<vtkPointsReal>::New();
	vtkSmartPointer<vtkCellArray>           boxesCells  = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkDoubleArrayFromReal> boxesColors = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	boxesColors->SetNumberOfComponents(3);
	boxesColors->SetName("color");

	vtkSmartPointer<vtkDoubleArrayFromReal> boxesStressVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	boxesStressVec->SetNumberOfComponents(3);
	boxesStressVec->SetName("stressVec");

	vtkSmartPointer<vtkDoubleArrayFromReal> boxesStressLen = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	boxesStressLen->SetNumberOfComponents(1);
	boxesStressLen->SetName("stressLen");

	vtkSmartPointer<vtkDoubleArrayFromReal> boxesMaterialId = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	boxesMaterialId->SetNumberOfComponents(1);
	boxesMaterialId->SetName("materialId");

	vtkSmartPointer<vtkDoubleArrayFromReal> boxesMask = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	boxesMask->SetNumberOfComponents(1);
	boxesMask->SetName("mask");

	vtkSmartPointer<vtkDoubleArrayFromReal> boxesForceVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	boxesForceVec->SetNumberOfComponents(3);
	boxesForceVec->SetName("forceVec");

	vtkSmartPointer<vtkDoubleArrayFromReal> boxesForceLen = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	boxesForceLen->SetNumberOfComponents(1);
	boxesForceLen->SetName("forceLen");

	vtkSmartPointer<vtkDoubleArrayFromReal> boxesTorqueVec = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	boxesTorqueVec->SetNumberOfComponents(3);
	boxesTorqueVec->SetName("torqueVec");

	vtkSmartPointer<vtkDoubleArrayFromReal> boxesTorqueLen = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	boxesTorqueLen->SetNumberOfComponents(1);
	boxesTorqueLen->SetName("torqueLen");

	// interactions
	vtkSmartPointer<vtkPointsReal>          intrBodyPos = vtkSmartPointer<vtkPointsReal>::New();
	vtkSmartPointer<vtkCellArray>           intrCells   = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkDoubleArrayFromReal> intrForceN  = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	intrForceN->SetNumberOfComponents(1);
	intrForceN->SetName("forceN");
	vtkSmartPointer<vtkDoubleArrayFromReal> intrAbsForceT = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	intrAbsForceT->SetNumberOfComponents(3);
	intrAbsForceT->SetName("absForceT");

	// pericell
	vtkSmartPointer<vtkPointsReal> pericellPoints = vtkSmartPointer<vtkPointsReal>::New();
	vtkSmartPointer<vtkCellArray>  pericellHexa   = vtkSmartPointer<vtkCellArray>::New();

	// extras for CPM
	if (recActive[REC_CPM]) {
		CpmStateUpdater csu;
		csu.update(scene);
	}
	vtkSmartPointer<vtkDoubleArrayFromReal> cpmDamage = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	cpmDamage->SetNumberOfComponents(1);
	cpmDamage->SetName("cpmDamage");
	vtkSmartPointer<vtkDoubleArrayFromReal> cpmStress = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	cpmStress->SetNumberOfComponents(9);
	cpmStress->SetName("cpmStress");

	// extras for JCFpm
	vtkSmartPointer<vtkDoubleArrayFromReal> nbCracks = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	nbCracks->SetNumberOfComponents(1);
	nbCracks->SetName("nbCracks");
	vtkSmartPointer<vtkDoubleArrayFromReal> jcfpmDamage = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	jcfpmDamage->SetNumberOfComponents(1);
	jcfpmDamage->SetName("damage");
	vtkSmartPointer<vtkDoubleArrayFromReal> intrIsCohesive = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	intrIsCohesive->SetNumberOfComponents(1);
	intrIsCohesive->SetName("isCohesive");
	vtkSmartPointer<vtkDoubleArrayFromReal> intrIsOnJoint = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	intrIsOnJoint->SetNumberOfComponents(1);
	intrIsOnJoint->SetName("isOnJoint");
	vtkSmartPointer<vtkDoubleArrayFromReal> eventNumber = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	eventNumber->SetNumberOfComponents(1);
	eventNumber->SetName("eventNumber");

	// extras for cracks
	vtkSmartPointer<vtkPointsReal>          crackPos   = vtkSmartPointer<vtkPointsReal>::New();
	vtkSmartPointer<vtkCellArray>           crackCells = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkDoubleArrayFromReal> crackIter  = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	crackIter->SetNumberOfComponents(1);
	crackIter->SetName("iter");
	vtkSmartPointer<vtkDoubleArrayFromReal> crackTime = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	crackTime->SetNumberOfComponents(1);
	crackTime->SetName("time");
	vtkSmartPointer<vtkDoubleArrayFromReal> crackType = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	crackType->SetNumberOfComponents(1);
	crackType->SetName("type");
	vtkSmartPointer<vtkDoubleArrayFromReal> crackSize = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	crackSize->SetNumberOfComponents(1);
	crackSize->SetName("size");
	vtkSmartPointer<vtkDoubleArrayFromReal> crackNorm = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	crackNorm->SetNumberOfComponents(3);
	crackNorm->SetName("norm");
	vtkSmartPointer<vtkDoubleArrayFromReal> crackNrg = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	crackNrg->SetNumberOfComponents(1);
	crackNrg->SetName("nrg");
	vtkSmartPointer<vtkDoubleArrayFromReal> crackOnJnt = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	crackOnJnt->SetNumberOfComponents(1);
	crackOnJnt->SetName("onJnt");

	// extras for moments
	vtkSmartPointer<vtkPointsReal>          momentPos   = vtkSmartPointer<vtkPointsReal>::New();
	vtkSmartPointer<vtkCellArray>           momentCells = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkDoubleArrayFromReal> momentiter  = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	momentiter->SetNumberOfComponents(1);
	momentiter->SetName("momentiter");
	vtkSmartPointer<vtkDoubleArrayFromReal> momenttime = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	momenttime->SetNumberOfComponents(1);
	momenttime->SetName("momenttime");
	vtkSmartPointer<vtkDoubleArrayFromReal> momentSize = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	momentSize->SetNumberOfComponents(1);
	momentSize->SetName("momentSize");
	vtkSmartPointer<vtkDoubleArrayFromReal> momentEventNum = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	momentEventNum->SetNumberOfComponents(1);
	momentEventNum->SetName("momentEventNum");
	vtkSmartPointer<vtkDoubleArrayFromReal> momentNumInts = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	momentNumInts->SetNumberOfComponents(1);
	momentNumInts->SetName("momentNumInts");


#ifdef YADE_LIQMIGRATION
	vtkSmartPointer<vtkDoubleArrayFromReal> liqVol = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	liqVol->SetNumberOfComponents(1);
	liqVol->SetName("liqVol");

	vtkSmartPointer<vtkDoubleArrayFromReal> liqVolNorm = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	liqVolNorm->SetNumberOfComponents(1);
	liqVolNorm->SetName("liqVolNorm");
#endif

	// extras for WireMatPM
	vtkSmartPointer<vtkDoubleArrayFromReal> wpmNormalForce = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	wpmNormalForce->SetNumberOfComponents(1);
	wpmNormalForce->SetName("wpmNormalForce");
	vtkSmartPointer<vtkDoubleArrayFromReal> wpmLimitFactor = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	wpmLimitFactor->SetNumberOfComponents(1);
	wpmLimitFactor->SetName("wpmLimitFactor");

#ifdef PARTIALSAT
	// extras for hertzmindlin
	vtkSmartPointer<vtkDoubleArrayFromReal> intrBrokenHertz = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	intrBrokenHertz->SetNumberOfComponents(1);
	intrBrokenHertz->SetName("broken");
	vtkSmartPointer<vtkDoubleArray> intrDisp = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	intrDisp->SetNumberOfComponents(1);
	intrDisp->SetName("disp");


	vtkSmartPointer<vtkDoubleArrayFromReal> spheresRadiiChange = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresRadiiChange->SetNumberOfComponents(1);
	spheresRadiiChange->SetName("radiiChange");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresSuction = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresSuction->SetNumberOfComponents(1);
	spheresSuction->SetName("suction");

	vtkSmartPointer<vtkDoubleArrayFromReal> spheresIncidentCells = vtkSmartPointer<vtkDoubleArrayFromReal>::New();
	spheresIncidentCells->SetNumberOfComponents(1);
	spheresIncidentCells->SetName("incidentCells");
#endif

	if (recActive[REC_INTR]) {
		// holds information about cell distance between spatial and displayed position of each particle
		vector<Vector3i> wrapCellDist;
		if (scene->isPeriodic) {
			wrapCellDist.resize(scene->bodies->size());
		}
		// save body positions, referenced by ids by vtkLine

		// map to keep real body ids and their number in a vector (intrBodyPos)
		boost::unordered_map<Body::id_t, Body::id_t> bIdVector;
		Body::id_t                                   curId = 0;
		for (const auto& b : *scene->bodies) {
			if (b) {
				if (!scene->isPeriodic) {
					intrBodyPos->InsertNextPoint(b->state->pos);
				} else {
					Vector3r pos = scene->cell->wrapShearedPt(b->state->pos, wrapCellDist[b->id]);
					intrBodyPos->InsertNextPoint(pos);
				}
				bIdVector.insert(std::pair<Body::id_t, Body::id_t>(b->id, curId));
				curId++;
			}
		}
		FOREACH(const shared_ptr<Interaction>& I, *scene->interactions)
		{
			if (!I->isReal())
				continue;
			if (skipFacetIntr) {
				if (!(Body::byId(I->getId1())))
					continue;
				if (!(Body::byId(I->getId2())))
					continue;
				if (!(dynamic_cast<Sphere*>(Body::byId(I->getId1())->shape.get())))
					continue;
				if (!(dynamic_cast<Sphere*>(Body::byId(I->getId2())->shape.get())))
					continue;
			}

			const auto iterId1 = bIdVector.find(I->getId1());
			const auto iterId2 = bIdVector.find(I->getId2());

			if (iterId2 == bIdVector.end() || iterId2 == bIdVector.end())
				continue;

			const auto setId1Line = iterId1->second;
			const auto setId2Line = iterId2->second;

			/* For the periodic boundary conditions,
				find out whether the interaction crosses the boundary of the periodic cell;
				if it does, display the interaction on both sides of the cell, with one of the
				points sticking out in each case.
				Since vtkLines must connect points with an ID assigned, we will create a new additional
				point for each point outside the cell. It might create some data redundancy, but
				let us suppose that the number of interactions crossing the cell boundary is low compared
				to total numer of interactions
			*/
			// how many times to add values defined on interactions, depending on how many times the interaction is saved
			int numAddValues = 1;
			// aperiodic boundary, or interaction is inside the cell
			if (!scene->isPeriodic || (scene->isPeriodic && (I->cellDist == wrapCellDist[I->getId2()] - wrapCellDist[I->getId1()]))) {
				vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
				line->GetPointIds()->SetId(0, setId1Line);
				line->GetPointIds()->SetId(1, setId2Line);
				intrCells->InsertNextCell(line);
			} else {
				assert(scene->isPeriodic);
				// spatial positions of particles
				const Vector3r& p01(Body::byId(I->getId1())->state->pos);
				const Vector3r& p02(Body::byId(I->getId2())->state->pos);
				// create two line objects; each of them has one endpoint inside the cell and the other one sticks outside
				// A,B are the "fake" bodies outside the cell for id1 and id2 respectively, p1,p2 are the displayed points
				// distance in cell units for shifting A away from p1; negated value is shift of B away from p2
				Vector3r        ptA(p01 + scene->cell->hSize * (wrapCellDist[I->getId2()] - I->cellDist).cast<Real>());
				const vtkIdType idPtA = intrBodyPos->InsertNextPoint(ptA);

				Vector3r        ptB(p02 + scene->cell->hSize * (wrapCellDist[I->getId1()] - I->cellDist).cast<Real>());
				const vtkIdType idPtB = intrBodyPos->InsertNextPoint(ptB);

				vtkSmartPointer<vtkLine> line1B(vtkSmartPointer<vtkLine>::New());
				line1B->GetPointIds()->SetId(0, setId2Line);
				line1B->GetPointIds()->SetId(1, idPtB);

				vtkSmartPointer<vtkLine> lineA2(vtkSmartPointer<vtkLine>::New());
				lineA2->GetPointIds()->SetId(0, idPtA);
				lineA2->GetPointIds()->SetId(1, setId2Line);
				numAddValues = 2;
			}
			const NormShearPhys*         phys = YADE_CAST<NormShearPhys*>(I->phys.get());
			const GenericSpheresContact* geom = YADE_CAST<GenericSpheresContact*>(I->geom.get());
			// gives _signed_ scalar of normal force, following the convention used in the respective constitutive law
			Real     fn = phys->normalForce.dot(geom->normal);
			Vector3r fs((Real)math::abs(phys->shearForce[0]), (Real)math::abs(phys->shearForce[1]), (Real)math::abs(phys->shearForce[2]));
			// add the value once for each interaction object that we created (might be 2 for the periodic boundary)
			for (int i = 0; i < numAddValues; i++) {
				intrAbsForceT->InsertNextTuple(fs);
				if (recActive[REC_WPM]) {
					const WirePhys* wirephys = dynamic_cast<WirePhys*>(I->phys.get());
					if (wirephys != NULL && wirephys->isLinked) {
						wpmLimitFactor->InsertNextValue(wirephys->limitFactor);
						wpmNormalForce->InsertNextValue(fn);
						intrForceN->InsertNextValue(NaN);
					} else {
						intrForceN->InsertNextValue(fn);
						wpmNormalForce->InsertNextValue(NaN);
						wpmLimitFactor->InsertNextValue(NaN);
					}
				} else if (recActive[REC_JCFPM]) {
					const JCFpmPhys* jcfpmphys = YADE_CAST<JCFpmPhys*>(I->phys.get());
					intrIsCohesive->InsertNextValue(jcfpmphys->isCohesive);
					intrIsOnJoint->InsertNextValue(jcfpmphys->isOnJoint);
					intrForceN->InsertNextValue(fn);
					eventNumber->InsertNextValue(jcfpmphys->eventNumber);
				} else if (recActive[REC_HERTZMINDLIN]) {
#ifdef PARTIALSAT
					const auto mindlinphys = YADE_CAST<MindlinPhys*>(I->phys.get());
					const auto mindlingeom = YADE_CAST<ScGeom*>(I->geom.get());
					intrBrokenHertz->InsertNextValue(mindlinphys->isBroken);
					intrDisp->InsertNextValue(mindlingeom->penetrationDepth - mindlinphys->initD);
#endif
					intrForceN->InsertNextValue(fn);
				} else {
					intrForceN->InsertNextValue(fn);
				}
#ifdef YADE_LIQMIGRATION
				if (recActive[REC_LIQ]) {
					const ViscElCapPhys* capphys = YADE_CAST<ViscElCapPhys*>(I->phys.get());
					liqVol->InsertNextValue(capphys->Vb);
					liqVolNorm->InsertNextValue(capphys->Vb / capphys->Vmax);
				}
#endif
			}
		}
	}

	//Additional Vector for storing forces
	vector<Shop::bodyState> bodyStates;
	if (recActive[REC_STRESS])
		Shop::getStressForEachBody(bodyStates);

	vector<Matrix3r> bStresses;
	if (recActive[REC_BSTRESS]) {
		Shop::getStressLWForEachBody(bStresses);
	}

	vector<Matrix3r> NCStresses, SCStresses, NLStresses, SLStresses, NPStresses;
	if (recActive[REC_LUBRICATION]) {
		Law2_ScGeom_ImplicitLubricationPhys::getStressForEachBody(NCStresses, SCStresses, NLStresses, SLStresses, NPStresses);
	}


#ifdef YADE_MPI
	const auto& subD = YADE_PTR_CAST<Subdomain>(scene->subD);
	const auto& sz   = parallelMode ? subD->ids.size() : scene->bodies->size();
	for (unsigned bId = 0; bId != sz; ++bId) {
		const auto& b = parallelMode ? (*scene->bodies)[subD->ids[bId]] : (*scene->bodies)[bId];
#else
	for (const auto& b : *scene->bodies) {
#endif
		if (!b)
			continue;
		if (mask != 0 && !b->maskCompatible(mask))
			continue;
		if (recActive[REC_SPHERES]) {
			const Sphere* sphere = dynamic_cast<Sphere*>(b->shape.get());
			if (sphere) {
				if (skipNondynamic && b->state->blockedDOFs == State::DOF_ALL)
					continue;
				vtkIdType pid[1];
				Vector3r  pos(scene->isPeriodic ? scene->cell->wrapShearedPt(b->state->pos) : b->state->pos);
				pid[0] = spheresPos->InsertNextPoint(pos);
				spheresCells->InsertNextCell(1, pid);
				radii->InsertNextValue(sphere->radius);
				if (recActive[REC_BSTRESS]) {
					const Matrix3r&                         bStress = bStresses[b->getId()];
					Eigen::SelfAdjointEigenSolver<Matrix3r> solver(
					        bStress); // bStress is probably not symmetric (= self-adjoint for real matrices), but the solver still works, considering only one half of bStress. Which is good since existence of (real) eigenvalues is not sure for not symmetric bStress..
					Matrix3r dirAll = solver.eigenvectors();
					Vector3r eigenVal
					        = solver.eigenvalues(); // cf http://eigen.tuxfamily.org/dox/classEigen_1_1SelfAdjointEigenSolver.html#a30caf3c3884a7f4a46b8ec94efd23c5e to be sure that eigenVal[i] * dirAll.col(i) = bStress * dirAll.col(i) and that eigenVal[0] <= eigenVal[1] <= eigenVal[2]
					spheresSigI->InsertNextValue(eigenVal[2]);
					spheresSigII->InsertNextValue(eigenVal[1]);
					spheresSigIII->InsertNextValue(eigenVal[0]);
					Vector3r dirI((Real)dirAll(0, 2), (Real)dirAll(1, 2), (Real)dirAll(2, 2));
					Vector3r dirII((Real)dirAll(0, 1), (Real)dirAll(1, 1), (Real)dirAll(2, 1));
					Vector3r dirIII((Real)dirAll(0, 0), (Real)dirAll(1, 0), (Real)dirAll(2, 0));
					spheresDirI->InsertNextTuple(dirI);
					spheresDirII->InsertNextTuple(dirII);
					spheresDirIII->InsertNextTuple(dirIII);
				}
				if (recActive[REC_ID])
					spheresId->InsertNextValue(b->getId());
				if (recActive[REC_MASK])
					spheresMask->InsertNextValue(GET_MASK(b));
				if (recActive[REC_MASS])
					spheresMass->InsertNextValue(b->state->mass);
#ifdef YADE_MPI
				if (recActive[REC_SUBDOMAIN])
					spheresSubdomain->InsertNextValue(b->subdomain);
#endif

#ifdef THERMAL
				if (recActive[REC_TEMP]) {
					auto* thState = b->state.get();
					spheresTemp->InsertNextValue(thState->temp);
				}
#endif
#ifdef PARTIALSAT
				if (recActive[REC_PARTIALSAT]) {
					PartialSatState* state = dynamic_cast<PartialSatState*>(b->state.get());
					spheresRadiiChange->InsertNextValue(state->radiiChange);
					spheresSuction->InsertNextValue(state->suction);
					spheresIncidentCells->InsertNextValue(state->lastIncidentCells);
				}
#endif
				if (recActive[REC_CLUMPID])
					clumpId->InsertNextValue(b->clumpId);
				if (recActive[REC_COLORS]) {
					const Vector3r& color = sphere->color;
					spheresColors->InsertNextTuple(color);
				}
				if (recActive[REC_VELOCITY]) {
					Vector3r vel = Vector3r::Zero();
					if (scene->isPeriodic) { // Take care of cell deformation
						vel = scene->cell->bodyFluctuationVel(b->state->pos, b->state->vel, scene->cell->prevVelGrad)
						        + scene->cell->prevVelGrad * scene->cell->wrapShearedPt(b->state->pos);
					} else {
						vel = b->state->vel;
					}
					spheresLinVelVec->InsertNextTuple(vel);
					spheresLinVelLen->InsertNextValue(vel.norm());
					const Vector3r& angVel = b->state->angVel;
					spheresAngVelVec->InsertNextTuple(angVel);
					spheresAngVelLen->InsertNextValue(angVel.norm());
				}
				if (recActive[REC_STRESS]) {
					const Vector3r& stress = bodyStates[b->getId()].normStress;
					const Vector3r& shear  = bodyStates[b->getId()].shearStress;
					spheresNormalStressVec->InsertNextTuple(stress);
					spheresShearStressVec->InsertNextTuple(shear);
					spheresNormalStressNorm->InsertNextValue(stress.norm());
				}
				if (recActive[REC_LUBRICATION]) {
					const Matrix3r& ncs = NCStresses[b->getId()];
					const Matrix3r& scs = SCStresses[b->getId()];
					const Matrix3r& nls = NLStresses[b->getId()];
					const Matrix3r& sls = SLStresses[b->getId()];
					const Matrix3r& nps = NPStresses[b->getId()];

					spheresLubricationNormalContactStress->InsertNextTuple(ncs);
					spheresLubricationShearContactStress->InsertNextTuple(scs);
					spheresLubricationNormalLubricationStress->InsertNextTuple(nls);
					spheresLubricationShearLubricationStress->InsertNextTuple(sls);
					spheresLubricationNormalPotentialStress->InsertNextTuple(nps);
				}
				if (recActive[REC_FORCE]) {
					scene->forces.sync();
					const Vector3r& f  = scene->forces.getForce(b->getId());
					const Vector3r& t  = scene->forces.getTorque(b->getId());
					Real            fn = f.norm();
					Real            tn = t.norm();
					spheresForceLen->InsertNextValue(fn);
					spheresTorqueLen->InsertNextValue(tn);
					spheresForceVec->InsertNextTuple(f);
					spheresTorqueVec->InsertNextTuple(t);
				}

				if (recActive[REC_CPM]) {
					cpmDamage->InsertNextValue(YADE_PTR_CAST<CpmState>(b->state)->normDmg);
					const Matrix3r& ss = YADE_PTR_CAST<CpmState>(b->state)->stress;
					//Real s[3]={ss[0],ss[1],ss[2]};
					cpmStress->InsertNextTuple(ss);
				}

				if (recActive[REC_JCFPM]) {
					nbCracks->InsertNextValue(YADE_PTR_CAST<JCFpmState>(b->state)->nbBrokenBonds);
					jcfpmDamage->InsertNextValue(YADE_PTR_CAST<JCFpmState>(b->state)->damageIndex);
				}

				if (recActive[REC_COORDNUMBER]) {
					spheresCoordNumb->InsertNextValue(b->coordNumber());
				}
#ifdef YADE_SPH
				if (recActive[REC_SPH]) {
					spheresRhoSPH->InsertNextValue(b->state->rho);
					spheresPressSPH->InsertNextValue(b->state->press);
					spheresCoordNumbSPH->InsertNextValue(b->coordNumber());
				}
#endif

#ifdef YADE_DEFORM
				if (recActive[REC_DEFORM]) {
					const Sphere* sphereDef = dynamic_cast<Sphere*>(b->shape.get());
					spheresRealRad->InsertNextValue(b->state->dR + sphereDef->radius);
				}
#endif

#ifdef YADE_LIQMIGRATION
				if (recActive[REC_LIQ]) {
					spheresLiqVol->InsertNextValue(b->state->Vf);
					const Real tmpVolIter = liqVolIterBody(b);
					spheresLiqVolIter->InsertNextValue(tmpVolIter);
					spheresLiqVolTotal->InsertNextValue(tmpVolIter + b->state->Vf);
				}
#endif
				if (recActive[REC_MATERIALID])
					spheresMaterialId->InsertNextValue(b->material->id);
				continue;
			}
		} // end rec sphere.
		if (recActive[REC_FACETS]) {
			const Facet* facet = dynamic_cast<Facet*>(b->shape.get());
			if (facet) {
				Vector3r                     pos(scene->isPeriodic ? scene->cell->wrapShearedPt(b->state->pos) : b->state->pos);
				const vector<Vector3r>&      localPos   = facet->vertices;
				Matrix3r                     facetAxisT = b->state->ori.toRotationMatrix();
				vtkSmartPointer<vtkTriangle> tri        = vtkSmartPointer<vtkTriangle>::New();
				vtkIdType                    nbPoints   = facetsPos->GetNumberOfPoints();
				for (int i = 0; i < 3; ++i) {
					Vector3r globalPos = pos + facetAxisT * localPos[i];
					facetsPos->InsertNextPoint(globalPos);
					tri->GetPointIds()->SetId(i, nbPoints + i);
				}
				facetsCells->InsertNextCell(tri);
				if (recActive[REC_COLORS]) {
					const Vector3r& color = facet->color;
					facetsColors->InsertNextTuple(color);
				}
				if (recActive[REC_STRESS]) {
					const Vector3r& stress = bodyStates[b->getId()].normStress + bodyStates[b->getId()].shearStress;
					facetsStressVec->InsertNextTuple(stress);
					facetsStressLen->InsertNextValue(stress.norm());
				}
				if (recActive[REC_FORCE]) {
					scene->forces.sync();
					const Vector3r& f  = scene->forces.getForce(b->getId());
					const Vector3r& t  = scene->forces.getTorque(b->getId());
					Real            fn = f.norm();
					Real            tn = t.norm();
					facetsForceLen->InsertNextValue(fn);
					facetsTorqueLen->InsertNextValue(tn);
					facetsForceVec->InsertNextTuple(f);
					facetsTorqueVec->InsertNextTuple(t);
				}

				if (recActive[REC_MATERIALID])
					facetsMaterialId->InsertNextValue(b->material->id);
				if (recActive[REC_MASK])
					facetsMask->InsertNextValue(GET_MASK(b));
				if (recActive[REC_COORDNUMBER]) {
					facetsCoordNumb->InsertNextValue(b->coordNumber());
				}
				continue;
			}
		}
		if (recActive[REC_BOXES]) {
			const Box* box = dynamic_cast<Box*>(b->shape.get());
			if (box) {
				Vector3r                 pos(scene->isPeriodic ? scene->cell->wrapShearedPt(b->state->pos) : b->state->pos);
				Quaternionr              ori(b->state->ori);
				Vector3r                 ext(box->extents);
				vtkSmartPointer<vtkQuad> boxes = vtkSmartPointer<vtkQuad>::New();
				Vector3r                 A     = Vector3r(-ext[0], -ext[1], -ext[2]);
				Vector3r                 B     = Vector3r(-ext[0], +ext[1], -ext[2]);
				Vector3r                 C     = Vector3r(+ext[0], +ext[1], -ext[2]);
				Vector3r                 D     = Vector3r(+ext[0], -ext[1], -ext[2]);

				Vector3r E = Vector3r(-ext[0], -ext[1], +ext[2]);
				Vector3r F = Vector3r(-ext[0], +ext[1], +ext[2]);
				Vector3r G = Vector3r(+ext[0], +ext[1], +ext[2]);
				Vector3r H = Vector3r(+ext[0], -ext[1], +ext[2]);

				A = pos + ori * A;
				B = pos + ori * B;
				C = pos + ori * C;
				D = pos + ori * D;
				E = pos + ori * E;
				F = pos + ori * F;
				G = pos + ori * G;
				H = pos + ori * H;

				addWallVTK(boxes, boxesPos, A, B, C, D);
				boxesCells->InsertNextCell(boxes);

				addWallVTK(boxes, boxesPos, E, H, G, F);
				boxesCells->InsertNextCell(boxes);

				addWallVTK(boxes, boxesPos, A, E, F, B);
				boxesCells->InsertNextCell(boxes);

				addWallVTK(boxes, boxesPos, G, H, D, C);
				boxesCells->InsertNextCell(boxes);

				addWallVTK(boxes, boxesPos, F, G, C, B);
				boxesCells->InsertNextCell(boxes);

				addWallVTK(boxes, boxesPos, D, H, E, A);
				boxesCells->InsertNextCell(boxes);

				for (int i = 0; i < 6; i++) {
					if (recActive[REC_COLORS]) {
						const Vector3r& color = box->color;
						boxesColors->InsertNextTuple(color);
					}
					if (recActive[REC_STRESS]) {
						const Vector3r& stress = bodyStates[b->getId()].normStress + bodyStates[b->getId()].shearStress;
						boxesStressVec->InsertNextTuple(stress);
						boxesStressLen->InsertNextValue(stress.norm());
					}
					if (recActive[REC_FORCE]) {
						scene->forces.sync();
						const Vector3r& f  = scene->forces.getForce(b->getId());
						const Vector3r& t  = scene->forces.getTorque(b->getId());
						Real            fn = f.norm();
						Real            tn = t.norm();
						boxesForceVec->InsertNextTuple(f);
						boxesTorqueVec->InsertNextTuple(t);
						boxesForceLen->InsertNextValue(fn);
						boxesTorqueLen->InsertNextValue(tn);
					}
					if (recActive[REC_MATERIALID])
						boxesMaterialId->InsertNextValue(b->material->id);
					if (recActive[REC_MASK])
						boxesMask->InsertNextValue(GET_MASK(b));
				}
				continue;
			}
		}
	} // end bodies loop.

#ifdef YADE_MPI
	if ((!parallelMode and recActive[REC_PERICELL]) or (scene->subdomain == 0 and recActive[REC_PERICELL])) {
#else
	if (recActive[REC_PERICELL]) {
#endif
		const Matrix3r& hSize = scene->cell->hSize;
		Vector3r        v0    = hSize * Vector3r(0, 0, 1);
		Vector3r        v1    = hSize * Vector3r(0, 1, 1);
		Vector3r        v2    = hSize * Vector3r(1, 1, 1);
		Vector3r        v3    = hSize * Vector3r(1, 0, 1);
		Vector3r        v4    = hSize * Vector3r(0, 0, 0);
		Vector3r        v5    = hSize * Vector3r(0, 1, 0);
		Vector3r        v6    = hSize * Vector3r(1, 1, 0);
		Vector3r        v7    = hSize * Vector3r(1, 0, 0);
		pericellPoints->InsertNextPoint(v0);
		pericellPoints->InsertNextPoint(v1);
		pericellPoints->InsertNextPoint(v2);
		pericellPoints->InsertNextPoint(v3);
		pericellPoints->InsertNextPoint(v4);
		pericellPoints->InsertNextPoint(v5);
		pericellPoints->InsertNextPoint(v6);
		pericellPoints->InsertNextPoint(v7);
		vtkSmartPointer<vtkHexahedron> h = vtkSmartPointer<vtkHexahedron>::New();
		vtkIdList*                     l = h->GetPointIds();
		for (int i = 0; i < 8; i++) {
			l->SetId(i, i);
		}
		pericellHexa->InsertNextCell(h);
	}


	vtkSmartPointer<vtkDataCompressor> compressor;
	if (compress)
		compressor = vtkSmartPointer<vtkZLibDataCompressor>::New();


/* In mpiruns, spheres and other types of bodies are held by workers */
#ifdef YADE_MPI
	vtkSmartPointer<vtkUnstructuredGrid> spheresUg = vtkSmartPointer<vtkUnstructuredGrid>::New();
	if ((recActive[REC_SPHERES] and scene->subdomain != 0) or (!parallelMode and recActive[REC_SPHERES])) {
#else
	vtkSmartPointer<vtkUnstructuredGrid> spheresUg = vtkSmartPointer<vtkUnstructuredGrid>::New();
	if (recActive[REC_SPHERES]) {
#endif
		spheresUg->SetPoints(spheresPos);
		spheresUg->SetCells(VTK_VERTEX, spheresCells);
		spheresUg->GetPointData()->AddArray(radii);
		if (recActive[REC_ID])
			spheresUg->GetPointData()->AddArray(spheresId);
		if (recActive[REC_MASK])
			spheresUg->GetPointData()->AddArray(spheresMask);
		if (recActive[REC_MASS])
			spheresUg->GetPointData()->AddArray(spheresMass);
		if (recActive[REC_TEMP])
			spheresUg->GetPointData()->AddArray(spheresTemp);
		if (recActive[REC_CLUMPID])
			spheresUg->GetPointData()->AddArray(clumpId);
		if (recActive[REC_COLORS])
			spheresUg->GetPointData()->AddArray(spheresColors);
		if (recActive[REC_VELOCITY]) {
			spheresUg->GetPointData()->AddArray(spheresLinVelVec);
			spheresUg->GetPointData()->AddArray(spheresAngVelVec);
			spheresUg->GetPointData()->AddArray(spheresLinVelLen);
			spheresUg->GetPointData()->AddArray(spheresAngVelLen);
		}
#ifdef YADE_MPI
		if (recActive[REC_SUBDOMAIN])
			spheresUg->GetPointData()->AddArray(spheresSubdomain);
#endif
#ifdef YADE_SPH
		if (recActive[REC_SPH]) {
			spheresUg->GetPointData()->AddArray(spheresRhoSPH);
			spheresUg->GetPointData()->AddArray(spheresPressSPH);
			spheresUg->GetPointData()->AddArray(spheresCoordNumbSPH);
		}
#endif

#ifdef YADE_DEFORM
		if (recActive[REC_DEFORM]) {
			spheresUg->GetPointData()->AddArray(spheresRealRad);
		}
#endif

#ifdef YADE_LIQMIGRATION
		if (recActive[REC_LIQ]) {
			spheresUg->GetPointData()->AddArray(spheresLiqVol);
			spheresUg->GetPointData()->AddArray(spheresLiqVolIter);
			spheresUg->GetPointData()->AddArray(spheresLiqVolTotal);
		}
#endif

#ifdef PARTIALSAT
		if (recActive[REC_PARTIALSAT]) {
			spheresUg->GetPointData()->AddArray(spheresRadiiChange);
			spheresUg->GetPointData()->AddArray(spheresSuction);
			spheresUg->GetPointData()->AddArray(spheresIncidentCells);
		}
#endif
		if (recActive[REC_STRESS]) {
			spheresUg->GetPointData()->AddArray(spheresNormalStressVec);
			spheresUg->GetPointData()->AddArray(spheresShearStressVec);
			spheresUg->GetPointData()->AddArray(spheresNormalStressNorm);
		}
		if (recActive[REC_LUBRICATION]) {
			spheresUg->GetPointData()->AddArray(spheresLubricationNormalContactStress);
			spheresUg->GetPointData()->AddArray(spheresLubricationShearContactStress);
			spheresUg->GetPointData()->AddArray(spheresLubricationNormalLubricationStress);
			spheresUg->GetPointData()->AddArray(spheresLubricationShearLubricationStress);
			spheresUg->GetPointData()->AddArray(spheresLubricationNormalPotentialStress);
		}
		if (recActive[REC_FORCE]) {
			spheresUg->GetPointData()->AddArray(spheresForceVec);
			spheresUg->GetPointData()->AddArray(spheresForceLen);
			spheresUg->GetPointData()->AddArray(spheresTorqueVec);
			spheresUg->GetPointData()->AddArray(spheresTorqueLen);
		}
		if (recActive[REC_CPM]) {
			spheresUg->GetPointData()->AddArray(cpmDamage);
			spheresUg->GetPointData()->AddArray(cpmStress);
		}

		if (recActive[REC_JCFPM]) {
			spheresUg->GetPointData()->AddArray(nbCracks);
			spheresUg->GetPointData()->AddArray(jcfpmDamage);
		}
		if (recActive[REC_BSTRESS]) {
			spheresUg->GetPointData()->AddArray(spheresSigI);
			spheresUg->GetPointData()->AddArray(spheresSigII);
			spheresUg->GetPointData()->AddArray(spheresSigIII);
			spheresUg->GetPointData()->AddArray(spheresDirI);
			spheresUg->GetPointData()->AddArray(spheresDirII);
			spheresUg->GetPointData()->AddArray(spheresDirIII);
		}
		if (recActive[REC_MATERIALID])
			spheresUg->GetPointData()->AddArray(spheresMaterialId);
		if (recActive[REC_COORDNUMBER])
			spheresUg->GetCellData()->AddArray(spheresCoordNumb);

#ifdef YADE_VTK_MULTIBLOCK
		if (!multiblock)
#endif

#ifdef YADE_MPI
		{
			vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			if (compress)
				writer->SetCompressor(compressor);
			if (ascii)
				writer->SetDataModeToAscii();
			std::string fn;
			if (!parallelMode) {
				fn = fileName + "spheres." + boost::lexical_cast<string>(scene->iter) + ".vtu";
			} else {
				fn = fileName + "spheres_" + boost::lexical_cast<string>(scene->iter) + "_" + boost::lexical_cast<string>(rank - 1) + ".vtu";
			}
			writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
			writer->SetInputData(spheresUg);
#else
			writer->SetInput(spheresUg);
#endif
			writer->Write();

			if (rank == 1 && parallelMode) {
				vtkSmartPointer<vtkXMLPUnstructuredGridWriter> pwriter = vtkSmartPointer<vtkXMLPUnstructuredGridWriter>::New();
				string                                         pfn = fileName + "spheres_" + boost::lexical_cast<string>(scene->iter) + ".pvtu";
				pwriter->EncodeAppendedDataOff();
				pwriter->SetFileName(pfn.c_str());
				pwriter->SetNumberOfPieces(commSize - 1);
				pwriter->SetStartPiece(0);

#ifdef YADE_VTK6
				pwriter->SetInputData(spheresUg);
#else
				pwriter->SetInput(spheresUg);
#endif
				pwriter->Update();
				pwriter->Write();
			}
		}
#else
		{
			vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			if (compress)
				writer->SetCompressor(compressor);
			if (ascii)
				writer->SetDataModeToAscii();
			string fn = fileName + "spheres." + boost::lexical_cast<string>(scene->iter) + ".vtu";
			writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
			writer->SetInputData(spheresUg);
#else
			writer->SetInput(spheresUg);
#endif
			writer->Write();
		}
#endif
	}


	/* facet bodies can be held by workers or master (like a wall body) */
	vtkSmartPointer<vtkUnstructuredGrid> facetsUg = vtkSmartPointer<vtkUnstructuredGrid>::New();
	if (recActive[REC_FACETS]) {
		facetsUg->SetPoints(facetsPos);
		facetsUg->SetCells(VTK_TRIANGLE, facetsCells);
		if (recActive[REC_COLORS])
			facetsUg->GetCellData()->AddArray(facetsColors);
		if (recActive[REC_STRESS]) {
			facetsUg->GetCellData()->AddArray(facetsStressVec);
			facetsUg->GetCellData()->AddArray(facetsStressLen);
		}
		if (recActive[REC_FORCE]) {
			facetsUg->GetCellData()->AddArray(facetsForceVec);
			facetsUg->GetCellData()->AddArray(facetsForceLen);
			facetsUg->GetCellData()->AddArray(facetsTorqueVec);
			facetsUg->GetCellData()->AddArray(facetsTorqueLen);
		}
		if (recActive[REC_MATERIALID])
			facetsUg->GetCellData()->AddArray(facetsMaterialId);
		if (recActive[REC_MASK])
			facetsUg->GetCellData()->AddArray(facetsMask);
		if (recActive[REC_COORDNUMBER])
			facetsUg->GetCellData()->AddArray(facetsCoordNumb);
#ifdef YADE_VTK_MULTIBLOCK
		if (!multiblock)
#endif
#ifdef YADE_MPI
		{
			vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			if (compress)
				writer->SetCompressor(compressor);
			if (ascii)
				writer->SetDataModeToAscii();
			std::string fn;
			if (!parallelMode) {
				fn = fileName + "facets." + boost::lexical_cast<string>(scene->iter) + ".vtu";
			} else {
				fn = fileName + "facets_" + boost::lexical_cast<string>(scene->iter) + "_" + boost::lexical_cast<string>(rank) + ".vtu";
			}
			writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
			writer->SetInputData(facetsUg);
#else
			writer->SetInput(facetsUg);
#endif
			writer->Write();

			if (rank == 0 && parallelMode) {
				vtkSmartPointer<vtkXMLPUnstructuredGridWriter> pwriter = vtkSmartPointer<vtkXMLPUnstructuredGridWriter>::New();
				string                                         pfn = fileName + "facets_" + boost::lexical_cast<string>(scene->iter) + ".pvtu";
				pwriter->EncodeAppendedDataOff();
				pwriter->SetFileName(pfn.c_str());
				pwriter->SetNumberOfPieces(commSize);
				pwriter->SetStartPiece(0);

#ifdef YADE_VTK6
				pwriter->SetInputData(facetsUg);
#else
				pwriter->SetInput(facetsUg);
#endif
				pwriter->Update();
				pwriter->Write();
			}
		}
#else
		{
			vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			if (compress)
				writer->SetCompressor(compressor);
			if (ascii)
				writer->SetDataModeToAscii();
			string fn = fileName + "facets." + boost::lexical_cast<string>(scene->iter) + ".vtu";
			writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
			writer->SetInputData(facetsUg);
#else
			writer->SetInput(facetsUg);
#endif
			writer->Write();
		}
#endif
	}

	/* mpi case : assuming box bodies  are held by master  */
	vtkSmartPointer<vtkUnstructuredGrid> boxesUg = vtkSmartPointer<vtkUnstructuredGrid>::New();
#ifdef YADE_MPI
	if ((!parallelMode and recActive[REC_BOXES]) or (scene->subdomain == 0 and recActive[REC_BOXES])) {
#else
	if (recActive[REC_BOXES]) {
#endif
		boxesUg->SetPoints(boxesPos);
		boxesUg->SetCells(VTK_QUAD, boxesCells);
		if (recActive[REC_COLORS])
			boxesUg->GetCellData()->AddArray(boxesColors);
		if (recActive[REC_STRESS]) {
			boxesUg->GetCellData()->AddArray(boxesStressVec);
			boxesUg->GetCellData()->AddArray(boxesStressLen);
		}
		if (recActive[REC_FORCE]) {
			boxesUg->GetCellData()->AddArray(boxesForceVec);
			boxesUg->GetCellData()->AddArray(boxesForceLen);
			boxesUg->GetCellData()->AddArray(boxesTorqueVec);
			boxesUg->GetCellData()->AddArray(boxesTorqueLen);
		}
		if (recActive[REC_MATERIALID])
			boxesUg->GetCellData()->AddArray(boxesMaterialId);
		if (recActive[REC_MASK])
			boxesUg->GetCellData()->AddArray(boxesMask);
#ifdef YADE_VTK_MULTIBLOCK
		if (!multiblock)
#endif
		{
			vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			if (compress)
				writer->SetCompressor(compressor);
			if (ascii)
				writer->SetDataModeToAscii();
			string fn = fileName + "boxes." + boost::lexical_cast<string>(scene->iter) + ".vtu";
			writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
			writer->SetInputData(boxesUg);
#else
			writer->SetInput(boxesUg);
#endif
			writer->Write();
		}
	}


	/* all subdomains have interactions in mpi case */
	vtkSmartPointer<vtkPolyData> intrPd = vtkSmartPointer<vtkPolyData>::New();
	if (recActive[REC_INTR]) {
		intrPd->SetPoints(intrBodyPos);
		intrPd->SetLines(intrCells);
		intrPd->GetCellData()->AddArray(intrForceN);
		intrPd->GetCellData()->AddArray(intrAbsForceT);
#ifdef YADE_LIQMIGRATION
		if (recActive[REC_LIQ]) {
			intrPd->GetCellData()->AddArray(liqVol);
			intrPd->GetCellData()->AddArray(liqVolNorm);
		}
#endif
		if (recActive[REC_JCFPM]) {
			intrPd->GetCellData()->AddArray(intrIsCohesive);
			intrPd->GetCellData()->AddArray(intrIsOnJoint);
			intrPd->GetCellData()->AddArray(eventNumber);
		}
		if (recActive[REC_WPM]) {
			intrPd->GetCellData()->AddArray(wpmNormalForce);
			intrPd->GetCellData()->AddArray(wpmLimitFactor);
		}
		if (recActive[REC_HERTZMINDLIN]) {
#ifdef PARTIALSAT
			intrPd->GetCellData()->AddArray(intrBrokenHertz);
			intrPd->GetCellData()->AddArray(intrDisp);
#endif
		}
#ifdef YADE_VTK_MULTIBLOCK
		if (!multiblock)
#endif

#ifdef YADE_MPI
		{
			vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
			if (compress)
				writer->SetCompressor(compressor);
			if (ascii)
				writer->SetDataModeToAscii();
			std::string fn;
			if (!parallelMode) {
				fn = fileName + "intrs." + boost::lexical_cast<string>(scene->iter) + ".vtp";
			} else {
				fn = fileName + "intrs_" + boost::lexical_cast<string>(scene->iter) + "_" + boost::lexical_cast<string>(rank) + ".vtp";
			}
			writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
			writer->SetInputData(intrPd);
#else
			writer->SetInput(intrPd);
#endif
			writer->Write();

			if (rank == 0) {
				vtkSmartPointer<vtkXMLPPolyDataWriter> pwriter = vtkSmartPointer<vtkXMLPPolyDataWriter>::New();
				string                                 pfn     = fileName + "intrs_" + boost::lexical_cast<string>(scene->iter) + ".pvtp";
				pwriter->EncodeAppendedDataOff();
				pwriter->SetFileName(pfn.c_str());
				pwriter->SetNumberOfPieces(commSize);
				pwriter->SetStartPiece(0);

#ifdef YADE_VTK6
				pwriter->SetInputData(intrPd);
#else
				pwriter->SetInput(intrPd);
#endif
				pwriter->Update();
				pwriter->Write();
			}
		}
#else
		{
			vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
			if (compress)
				writer->SetCompressor(compressor);
			if (ascii)
				writer->SetDataModeToAscii();
			string fn = fileName + "intrs." + boost::lexical_cast<string>(scene->iter) + ".vtp";
			writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
			writer->SetInputData(intrPd);
#else
			writer->SetInput(intrPd);
#endif
			writer->Write();
		}
#endif
	}


	vtkSmartPointer<vtkUnstructuredGrid> pericellUg = vtkSmartPointer<vtkUnstructuredGrid>::New();
#ifdef YADE_MPI
	if ((!parallelMode and recActive[REC_PERICELL]) or (scene->subdomain == 0 and recActive[REC_PERICELL])) {
#else
	if (recActive[REC_PERICELL]) {
#endif
		pericellUg->SetPoints(pericellPoints);
		pericellUg->SetCells(12, pericellHexa);
#ifdef YADE_VTK_MULTIBLOCK
		if (!multiblock)
#endif
		{
			vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
			if (compress)
				writer->SetCompressor(compressor);
			if (ascii)
				writer->SetDataModeToAscii();
			string fn = fileName + "pericell." + boost::lexical_cast<string>(scene->iter) + ".vtu";
			writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
			writer->SetInputData(pericellUg);
#else
			writer->SetInput(pericellUg);
#endif
			writer->Write();
		}
	}


	/* in mpi case, just use master to do this */
#ifdef YADE_MPI
	if ((!parallelMode && recActive[REC_CRACKS]) || (scene->subdomain == 0 && recActive[REC_CRACKS])) {
#else
	if (recActive[REC_CRACKS]) {
#endif
		string                               fileCracks = "cracks_" + Key + ".txt";
		std::ifstream                        file(fileCracks.c_str(), std::ios::in);
		vtkSmartPointer<vtkUnstructuredGrid> crackUg = vtkSmartPointer<vtkUnstructuredGrid>::New();

		if (file) {
			while (!file.eof()) {
				std::string line;
				Real        iter, time, p0, p1, p2, type, size, n0, n1, n2, nrg, onJnt;
				while (std::getline(file, line)) { /* writes into string "line", a line of file "file". To go along diff. lines*/
					file >> iter >> time >> p0 >> p1 >> p2 >> type >> size >> n0 >> n1 >> n2 >> nrg >> onJnt;
					vtkIdType pid[1];
					pid[0] = crackPos->InsertNextPoint(Vector3r(p0, p1, p2));
					crackCells->InsertNextCell(1, pid);
					crackIter->InsertNextValue(iter);
					crackTime->InsertNextValue(time);
					crackType->InsertNextValue(type);
					crackSize->InsertNextValue(size);
					Vector3r n(n0, n1, n2);
					crackNorm->InsertNextTuple(n);
					crackNrg->InsertNextValue(nrg);
					crackOnJnt->InsertNextValue(onJnt);
				}
			}
			file.close();
		}
		//
		crackUg->SetPoints(crackPos);
		crackUg->SetCells(VTK_VERTEX, crackCells);
		crackUg->GetPointData()->AddArray(crackIter);
		crackUg->GetPointData()->AddArray(crackTime);
		crackUg->GetPointData()->AddArray(crackType);
		crackUg->GetPointData()->AddArray(crackSize);
		crackUg->GetPointData()->AddArray(
		        crackNorm); //see https://www.mail-archive.com/paraview@paraview.org/msg08166.html to obtain Paraview 2D glyphs conforming to this normal
		crackUg->GetPointData()->AddArray(crackNrg);
		crackUg->GetPointData()->AddArray(crackOnJnt);

		vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
		if (compress)
			writer->SetCompressor(compressor);
		if (ascii)
			writer->SetDataModeToAscii();
		string fn = fileName + "cracks." + boost::lexical_cast<string>(scene->iter) + ".vtu";
		writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
		writer->SetInputData(crackUg);
#else
		writer->SetInput(crackUg);
#endif
		writer->Write();
	}

// doing same thing for moments that we did for cracks:
#ifdef YADE_MPI
	if ((!parallelMode && recActive[REC_MOMENTS]) || (scene->subdomain == 0 && recActive[REC_MOMENTS])) {
#else
	if (recActive[REC_MOMENTS]) {
#endif
		string                               fileMoments = "moments_" + Key + ".txt";
		std::ifstream                        file(fileMoments.c_str(), std::ios::in);
		vtkSmartPointer<vtkUnstructuredGrid> momentUg = vtkSmartPointer<vtkUnstructuredGrid>::New();

		if (file) {
			while (!file.eof()) {
				std::string line;
				Real        i, p0, p1, p2, moment, numInts, eventNum, time;
				while (std::getline(file, line)) { /* writes into string "line", a line of file "file". To go along diff. lines*/
					file >> i >> p0 >> p1 >> p2 >> moment >> numInts >> eventNum >> time;
					vtkIdType pid[1];
					pid[0] = momentPos->InsertNextPoint(Vector3r(p0, p1, p2));
					momentCells->InsertNextCell(1, pid);
					momentSize->InsertNextValue(moment);
					momentiter->InsertNextValue(i);
					momenttime->InsertNextValue(time);
					momentNumInts->InsertNextValue(numInts);
					momentEventNum->InsertNextValue(eventNum);
				}
			}
			file.close();
		}
		//
		momentUg->SetPoints(momentPos);
		momentUg->SetCells(VTK_VERTEX, momentCells);
		momentUg->GetPointData()->AddArray(momentiter);
		momentUg->GetPointData()->AddArray(momenttime);
		momentUg->GetPointData()->AddArray(momentSize);
		momentUg->GetPointData()->AddArray(momentNumInts);
		momentUg->GetPointData()->AddArray(momentEventNum);

		vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer = vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
		if (compress)
			writer->SetCompressor(compressor);
		if (ascii)
			writer->SetDataModeToAscii();
		string fn = fileName + "moments." + boost::lexical_cast<string>(scene->iter) + ".vtu";
		writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
		writer->SetInputData(momentUg);
#else
		writer->SetInput(momentUg);
#endif
		writer->Write();
	}

#ifdef YADE_VTK_MULTIBLOCK
#ifdef YADE_MPI
	if (multiblock) {
		if (parallelMode) {
			LOG_WARN("Multiblock feature in MPI case untested.");
		}
		vtkSmartPointer<vtkMultiBlockDataSet> multiblockDataset = vtkSmartPointer<vtkMultiBlockDataSet>::New();
		int                                   i                 = 0;
		if (recActive[REC_SPHERES])
			multiblockDataset->SetBlock(i++, spheresUg);
		if (recActive[REC_FACETS])
			multiblockDataset->SetBlock(i++, facetsUg);
		if (recActive[REC_INTR])
			multiblockDataset->SetBlock(i++, intrPd);
		if (recActive[REC_PERICELL])
			multiblockDataset->SetBlock(i++, pericellUg);
		vtkSmartPointer<vtkXMLMultiBlockDataWriter> writer = vtkSmartPointer<vtkXMLMultiBlockDataWriter>::New();
		if (ascii)
			writer->SetDataModeToAscii();
		std::string fn;
		if (!parallelMode) {
			fn = fileName + boost::lexical_cast<string>(scene->iter) + ".vtm";
		} else {
			fn = fileName + boost::lexical_cast<string>(scene->iter) + "_" + boost::lexical_cast<string>(rank) + ".vtm";
		}
		writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
		writer->SetInputData(multiblockDataset);
#else
		writer->SetInput(multiblockDataset);
#endif
		writer->Write();

		if (parallelMode and rank == 0) {
			// use vtkController ; controller->GetProcessCount();
			// 					vtkSmartPointer<vtkXMLPMultiBlockDataWriter> parWriter = vtkXMLPMultiBlockDataWriter::New()
			// 					std::string pfn = fileName+boost::lexical_cast<string>(scene->iter+d)+".pvtm";
			// 					parWriter->EncodeAppendedDataOff();
			// 					parWriter->SetFileName(pfn.c_str());
			// 					#ifdef YADE_VTK6
			// 						parWriter->SetInputData(multiblockDataset);
			// 					#else
			// 						parWriter->SetInput(multiblockDataset);
			// 					#endif
			// 					parWriter->SetWriteMetaFile(commSize);
			// 					parWriter->Update();
			LOG_ERROR("PARALLEL MULTIBLOCKDATA NOT IMPLEMENTED. ");
		}
	}
#else
	if (multiblock) {
		vtkSmartPointer<vtkMultiBlockDataSet> multiblockDataset = vtkSmartPointer<vtkMultiBlockDataSet>::New();
		int i = 0;
		if (recActive[REC_SPHERES])
			multiblockDataset->SetBlock(i++, spheresUg);
		if (recActive[REC_FACETS])
			multiblockDataset->SetBlock(i++, facetsUg);
		if (recActive[REC_INTR])
			multiblockDataset->SetBlock(i++, intrPd);
		if (recActive[REC_PERICELL])
			multiblockDataset->SetBlock(i++, pericellUg);
		vtkSmartPointer<vtkXMLMultiBlockDataWriter> writer = vtkSmartPointer<vtkXMLMultiBlockDataWriter>::New();
		if (ascii)
			writer->SetDataModeToAscii();
		string fn = fileName + boost::lexical_cast<string>(scene->iter) + ".vtm";
		writer->SetFileName(fn.c_str());
#ifdef YADE_VTK6
		writer->SetInputData(multiblockDataset);
#else
		writer->SetInput(multiblockDataset);
#endif
		writer->Write();
	}
#endif

#endif
};

void VTKRecorder::addWallVTK(vtkSmartPointer<vtkQuad>& boxes, vtkSmartPointer<vtkPointsReal>& boxesPos, Vector3r& W1, Vector3r& W2, Vector3r& W3, Vector3r& W4)
{
	//Function for exporting walls of boxes
	vtkIdType nbPoints = boxesPos->GetNumberOfPoints();

	boxesPos->InsertNextPoint(W1);
	boxes->GetPointIds()->SetId(0, nbPoints + 0);

	boxesPos->InsertNextPoint(W2);
	boxes->GetPointIds()->SetId(1, nbPoints + 1);

	boxesPos->InsertNextPoint(W3);
	boxes->GetPointIds()->SetId(2, nbPoints + 2);

	boxesPos->InsertNextPoint(W4);
	boxes->GetPointIds()->SetId(3, nbPoints + 3);
};
#undef GET_MASK

} // namespace yade

#endif /* YADE_VTK */
