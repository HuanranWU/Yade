#pragma once
#include <lib/compatibility/VTKCompatibility.hpp> // fix InsertNextTupleValue → InsertNextTuple name change (and others in the future)
#include <pkg/common/PeriodicEngines.hpp>
#include <vtkQuad.h>
#include <vtkSmartPointer.h>
// multiblock features don't seem to exist prioor to 5.2
#if (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION >= 2) || (VTK_MAJOR_VERSION > 5)
#define YADE_VTK_MULTIBLOCK
#endif

namespace yade { // Cannot have #include directive inside.

class VTKRecorder : public PeriodicEngine {
private:
#ifdef YADE_MPI
	int  rank, commSize;
	bool sceneRefreshed = false;
#endif
public:
	enum {
		REC_SPHERES = 0,
		REC_FACETS,
		REC_BOXES,
		REC_COLORS,
		REC_MASS,
		REC_TEMP,
		REC_CPM,
		REC_INTR,
		REC_VELOCITY,
		REC_ID,
		REC_CLUMPID,
		REC_SENTINEL,
		REC_MATERIALID,
		REC_STRESS,
		REC_MASK,
		REC_RPM,
		REC_JCFPM,
		REC_CRACKS,
		REC_MOMENTS,
		REC_WPM,
		REC_PERICELL,
		REC_LIQ,
		REC_BSTRESS,
		REC_FORCE,
		REC_COORDNUMBER,
		REC_SPH,
		REC_DEFORM,
		REC_LUBRICATION,
		REC_SUBDOMAIN,
		REC_PARTIALSAT,
		REC_HERTZMINDLIN
	};
	virtual void action();
	void addWallVTK(vtkSmartPointer<vtkQuad>& boxes, vtkSmartPointer<vtkPointsReal>& boxesPos, Vector3r& W1, Vector3r& W2, Vector3r& W3, Vector3r& W4);
	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(VTKRecorder,PeriodicEngine,"Engine recording snapshots of simulation into series of \\*.vtu files, readable by VTK-based postprocessing programs such as Paraview. Both bodies (spheres and facets) and interactions can be recorded, with various vector/scalar quantities that are defined on them.\n\n:yref:`PeriodicEngine.initRun` is initialized to ``True`` automatically.",
		((bool,compress,false,,"Compress output XML files [experimental]."))
		((bool,ascii,false,,"Store data as readable text in the XML file (sets `vtkXMLWriter <http://www.vtk.org/doc/nightly/html/classvtkXMLWriter.html>`__ data mode to ``vtkXMLWriter::Ascii``, while the default is ``Appended``)"))
		((bool,skipFacetIntr,true,,"Skip interactions that are not of sphere-sphere type (e.g. sphere-facet, sphere-box...), when saving interactions"))
		((bool,skipNondynamic,false,,"Skip non-dynamic spheres (but not facets)."))
		#ifdef YADE_VTK_MULTIBLOCK
			((bool,multiblock,false,,"Use multi-block (``.vtm``) files to store data, rather than separate ``.vtu`` files."))
		#endif
		#ifdef YADE_MPI
			((bool,parallelMode,false,,"For MPI parallel runs, each proc writes their own vtu/vtp files. Master proc writes a pvtu/pvtp file containing metadata about worker vtu files. load the pvtu/pvtp in paraview for visualization."))
		#endif
		((string,fileName,"",,"Base file name; it will be appended with {spheres,intrs,facets}.243100.vtu (unless *multiblock* is ``True``) depending on active recorders and step number (243100 in this case). It can contain slashes, but the directory must exist already."))
		((vector<string>,recorders,vector<string>(1,string("all")),,R"""(List of active recorders (as strings). ``all`` (the default value) enables all base and generic recorders.

**Base recorders**

	Base recorders save the geometry (unstructured grids) on which other data is defined. They are implicitly activated by many of the other recorders. Each of them creates a new file (or a block, if :yref:`multiblock <VTKRecorder.multiblock>` is set).

	``spheres``
		Saves positions and radii (``radii``) of :yref:`spherical<Sphere>` particles.
	``facets``
		Save :yref:`facets<Facet>` positions (vertices).
	``boxes``
		Save :yref:`boxes<Box>` positions (edges).
	``intr``
		Store interactions as lines between nodes at respective particles positions. Additionally stores magnitude of normal (``forceN``) and shear (``absForceT``) forces on interactions (the :yref:`geom<Interaction.geom> must be of type :yref:`NormShearPhys`).

**Generic recorders**

	Generic recorders do not depend on specific model being used and save commonly useful data.

	``id``
		Saves id's (field ``id``) of spheres; active only if ``spheres`` is active.
	``mass``
		Saves masses (field ``mass``) of spheres; active only if ``spheres`` is active.
	``clumpId``
		Saves id's of clumps to which each sphere belongs (field ``clumpId``); active only if ``spheres`` is active.
	``colors``
		Saves colors of :yref:`spheres<Sphere>` and of :yref:`facets<Facet>` (field ``color``); only active if ``spheres`` or ``facets`` are activated.
	``mask``
		Saves groupMasks of :yref:`spheres<Sphere>` and of :yref:`facets<Facet>` (field ``mask``); only active if ``spheres`` or ``facets`` are activated.
	``materialId``
		Saves materialID of :yref:`spheres<Sphere>` and of :yref:`facets<Facet>`; only active if ``spheres`` or ``facets`` are activated.
	``coordNumber``
		Saves coordination number (number of neighbours) of :yref:`spheres<Sphere>` and of :yref:`facets<Facet>`; only active if ``spheres`` or ``facets`` are activated.
	``velocity``
		Saves linear and angular velocities of spherical particles as Vector3 and length(fields ``linVelVec``, ``linVelLen`` and ``angVelVec``, ``angVelLen`` respectively``); only effective with ``spheres``.
	``stress``
		Saves stresses of :yref:`spheres<Sphere>` and of :yref:`facets<Facet>`  as Vector3 and length; only active if ``spheres`` or ``facets`` are activated.
	``force``
		Saves force and torque of :yref:`spheres<Sphere>`, :yref:`facets<Facet>` and :yref:`boxes<Box>` as Vector3 and length (norm); only active if ``spheres``, ``facets`` or ``boxes`` are activated.
	``pericell``
		Saves the shape of the cell (simulation has to be periodic).
	``bstresses``
		Considering the per-particle stress tensors as given by :yref:`bodyStressTensors<yade.utils.bodyStressTensors>`, saves the per-particle principal stresses, sigI (most tensile) $\\geq$ sigII $\\geq$ sigIII (most compressive), and the associated principal directions dirI, dirII, dirIII.

**Specific recorders**

	The following should only be activated in when appropriate engines/contact laws are in use, otherwise crashes can occur due to violation of type presuppositions.

	``cpm``
		Saves data pertaining to the :yref:`concrete model<Law2_ScGeom_CpmPhys_Cpm>`: ``cpmDamage`` (normalized residual strength averaged on particle), ``cpmStress`` (stress on particle); ``intr`` is activated automatically by ``cpm``
	``wpm``
		Saves data pertaining to the :yref:`wire particle model<Law2_ScGeom_WirePhys_WirePM>`: ``wpmForceNFactor`` shows the loading factor for the wire, e.g. normal force divided by threshold normal force.
	``jcfpm``
		Saves data pertaining to the :yref:`rock (smooth)-jointed model<Law2_ScGeom_JCFpmPhys_JointedCohesiveFrictionalPM>`: ``damage`` is defined by :yref:`JCFpmState.tensBreak` + :yref:`JCFpmState.shearBreak`; ``intr`` is activated automatically by ``jcfpm``, and :yref:`on joint<JCFpmPhys.isOnJoint>` or :yref:`cohesive<JCFpmPhys.isCohesive>` interactions can be vizualized.
	``cracks``
		Saves other data pertaining to the :yref:`rock model<Law2_ScGeom_JCFpmPhys_JointedCohesiveFrictionalPM>`: ``cracks`` shows locations where cohesive bonds failed during the simulation, with their types (0/1  for tensile/shear breakages), their sizes (0.5*(R1+R2)), and their normal directions. The :yref:`corresponding attribute<Law2_ScGeom_JCFpmPhys_JointedCohesiveFrictionalPM.recordCracks>` has to be activated, and Key attributes have to be consistent.
	``moments``
		Saves data pertaining to the required corresponding attribute:yref:`acoustic emissions model<Law2_ScGeom_JCFpmPhys_JointedCohesiveFrictionalPM.recordMoments>`: ``moments`` shows locations of acoustic emissions, the number of broken bonds comprising the acoustic emission, the magnitude.
	``thermal``
		Saves temperature of bodies computed using Yade's :yref:`ThermalEngine`.
	``liquid``
		Saves the liquid volume associated with :yref:`capillary models<Law2_ScGeom_ViscElCapPhys_Basic>`.
	``cohfrict``
		Saves interaction information associated with :yref:`the cohesive frictional model<Law2_ScGeom6D_CohFrictPhys_CohesionMoment>`, including isBroken, unp, and breakType.
	``SPH``
		Saves sphere information associated with Yade's SPH module.
	``deform``
		Saves interaction information associated with Yade's deformation module.
	``lubrication``
		Saves lubrications stress from :yref:`LubricationPhys`. ``spheres`` must be active.
	``partialsat``
		Saves suction and radii changes of spheres associated with :yref:`PartialSatClayEngine`. ``spheres`` must be active.
	``hertz``
		Saves bond data from hertzmindlin such as displacement or 'broken' where broken follows a displacement criteria set by user in :yref:`Law2_ScGeom_MindlinPhys_Mindlin`.
)"""))
		((string,Key,"",,"Necessary if :yref:`recorders<VTKRecorder.recorders>` contains 'cracks' or 'moments'. A string specifying the name of file 'cracks___.txt' that is considered in this case (see :yref:`corresponding attribute<Law2_ScGeom_JCFpmPhys_JointedCohesiveFrictionalPM.Key>`)."))
		((int,mask,0,,"If mask defined, only bodies with corresponding groupMask will be exported. If 0, all bodies will be exported.")),
		/*ctor*/
		initRun=true;
	);
	// clang-format on
	DECLARE_LOGGER;
};

REGISTER_SERIALIZABLE(VTKRecorder);

} // namespace yade
