#ifdef YADE_POTENTIAL_BLOCKS

#pragma once
#include <lib/base/openmp-accu.hpp>
#include <pkg/common/Dispatching.hpp>
#include <pkg/common/ElastMat.hpp>
#include <pkg/common/NormShearPhys.hpp>
#include <pkg/dem/FrictPhys.hpp>
#include <pkg/dem/ScGeom.hpp>
#include <boost/tuple/tuple.hpp>
#include <set>

#ifdef USE_TIMING_DELTAS
#define TIMING_DELTAS_CHECKPOINT(cpt) timingDeltas->checkpoint(cpt)
#define TIMING_DELTAS_START() timingDeltas->start()
#else
#define TIMING_DELTAS_CHECKPOINT(cpt)
#define TIMING_DELTAS_START()
#endif

namespace yade { // Cannot have #include directive inside.

class KnKsPBPhys : public FrictPhys {
public:
	virtual ~KnKsPBPhys();
	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(KnKsPBPhys,FrictPhys,"EXPERIMENTAL. IPhys for :yref:`PotentialBlock`.",
//			((vector<Real>,lambdaIPOPT,0.0,,"not used, lagrane multiplier for equality constraints"))
//			((vector<int>,cstatCPLEX,,,"not used"))
//			((vector<int>,rstatCPLEX,,,"not used"))
		((Real, frictionAngle,0.0,,"Friction angle"))
//			((Real, tanFrictionAngle,0.0,,"tangent of fric angle"))
//			((Vector3r,contactDetectionPt,Vector3r::Zero(),,"contact detection result"))
		((Real, viscousDamping, 0.0,,"Viscous damping")) //FIXME: We don't need to store this attr for each contact. It can be stored only in the IPhys functor
//		((Real, unitWidth2D, 1.0,,"Unit width in 2D")) // Moved this attr in Ig2_PB_PB_ScGeom @vsangelidakis
//			((Real, maxClosure, 0.0002,Attr::hidden,"not used, vmi"))
//			((Real, u_peak, 0.05,,"peak shear displacement, not fully in use"))
			((Real, u_elastic, 0.0,,"Elastic shear displacement, not fully in use"))
//			((Real, brittleLength, 5.0,,"shear length where strength degrades, not fully in use"))
		((Real, knVol, 0.0,,"Volumetric normal stiffness = Knormal")) //FIXME: We don't need to store this attr for each contact. It can be stored only in the IPhys functor
		((Real, ksVol, 0.0,,"Volumetric shear stiffness = Kshear" )) //FIXME: We don't need to store this attr for each contact. It can be stored only in the IPhys functor
		((Real, kn_i, 5.0,,"initial normal stiffness, user must provide input during initialisation")) //FIXME: We don't need to store this attr for each contact. It can be stored only in the IPhys functor
		((Real, ks_i, 5.0,,"initial shear stiffness, user must provide input during initialisation")) //FIXME: We don't need to store this attr for each contact. It can be stored only in the IPhys functor
		((Vector3r, normalViscous, Vector3r::Zero(),,"Viscous normal force"))
		((Vector3r, shearViscous, Vector3r::Zero(),,"Viscous shear force (assumed zero at the moment)"))
//			((Real, hwater, 0.0,Attr::hidden,"not used, height of pore water"))
//			((bool, rockJointContact, false,Attr::hidden,"rock joint"))
		((bool, intactRock, false,,"Whether to consider cohesive force in the Mohr-Coulomb criterion, if allowBreakage=False and cohesionBroken=False"))
		((int, jointType, 0,,"jointType"))
//			((Real, Kshear_area,0.0,,"Shear contact stiffness (units=force/length), calculated as the product of the volumetric stiffness and the contact area"))
//			((Real, Knormal_area, 0.0,,"Normal contact stiffness (units=force/length), calculated as the product of the volumetric stiffness and the contact area"))
		((Vector3r, shearDir, Vector3r::Zero(),,"Shear direction"))
//			((vector<Vector3r>, shearForces, ,,"shear force"))
//			((Vector3r, shear, Vector3r::Zero(),,"shear displacement"))
		((Vector3r, prevNormal, Vector3r::Zero(),,"Previous contact normal"))
//			((Vector3r, normal, Vector3r::Zero(),," normalVector"))
//			((vector<Vector3r>, pointsArea, ,Attr::hidden,"not used, intermediate contact points"))
//			((vector<Vector3r>, pointsShear, ,Attr::hidden,"not used, points to calculate shear"))
//			((vector<Real>, areaShear, ,Attr::hidden,"not used, area to attribute shear"))
//			((vector<Real>, overlapDistances, ,Attr::hidden,"not used, overlap distance"))
//			((Real, finalSize, 0.0,Attr::hidden,"not used, finalgridsize"))
//			((int, finalGridNo, 0,Attr::hidden,"not used, final number of grids"))
//			((vector<Real>, dualityGap, ,Attr::hidden,"not used, duality gap for SOCP"))
			((bool, warmstart, false,,"Warmstart for SOCP, not fully in use"))
//			((int, generation, 0,Attr::hidden,"not used, number of subdivisions"))
//			((int, triNoMain, 24,Attr::hidden,"not used, number of subdivisions"))
//			((int, triNoSub, 6,Attr::hidden,"not used, number of subdivisions"))
//			((Vector3r, initial1, Vector3r::Zero(),,"midpoint"))
		((Vector3r, ptOnP1, Vector3r::Zero(),,"Point on particle 1"))
		((Vector3r, ptOnP2, Vector3r::Zero(),,"Point on particle 2"))
//			((vector<bool>, redundantA, ,Attr::hidden,"not used, activePlanes for interaction.id1"))
//			((vector<bool>, redundantB, ,Attr::hidden,"not used, activePlanes for interaction.id1"))
//			((vector<bool>, activePlanes1, ,Attr::hidden,"not used, activePlanes for interaction.id1"))
//			((vector<bool>, activePlanes2, ,Attr::hidden,"not used, activePlanes for interaction.id2"))
//			((vector<Real>, activeA1, ,Attr::hidden,"not used, activePlanes for interaction.id2"))
//			((vector<Real>, activeB1, ,Attr::hidden,"not used, activePlanes for interaction.id2"))
//			((vector<Real>, activeC1, ,Attr::hidden,"not used, activePlanes for interaction.id2"))
//			((vector<Real>, activeD1, ,Attr::hidden,"not used, activePlanes for interaction.id2"))
//			((vector<Real>, activeA2, ,Attr::hidden,"not used, activePlanes for interaction.id2"))
//			((vector<Real>, activeB2, ,Attr::hidden,"not used, activePlanes for interaction.id2"))
//			((vector<Real>, activeC2, ,Attr::hidden,"not used, activePlanes for interaction.id2"))
//			((vector<Real>, activeD2, ,Attr::hidden,"not used, activePlanes for interaction.id2"))
//			((int, noActive1, 0, Attr::hidden,"not used, activePlanes for interaction.id1"))
//			((int, noActive2, 0, Attr::hidden,"not used, activePlanes for interaction.id2"))
			((int, smallerID,1 ,," id of particle with smaller plane")) //FIXME: Check whether we can use this
		((Real, cumulative_us, 0.0,,"Cumulative translation"))
//			((Real, cumulativeRotation, 0.0,,"cumulative rotation"))
		((Real, mobilizedShear, ,,"Percentage of mobilized shear force as the ratio of the current shear force to the current frictional limit. Represents a quantified measure of the isSliding parameter"))
		((Real, contactArea, 0.0,,"Contact area |yupdate|"))
//			((Real, radCurvFace, ,Attr::hidden,"not used, face"))
//			((Real, prevJointLength, 0.0,,"previous joint length"))
//			((Real, radCurvCorner, ,Attr::hidden,"not used, corners"))
		((Real, prevSigma,0.0,,"Previous normal stress"))
//			((vector<Real>, prevSigmaList,0.0 ,Attr::hidden,"not used, previous normal stress"))
//		((bool, calJointLength, false,,"Whether to calculate joint length for 2D contacts")) //This attr is replaced by Ig2_PB_PB_ScGeom::calContactArea
//			((bool, useOverlapVol, false,,"calculate overlap volume"))
//			((bool, calContactArea, false,,"calculate contact area"))
		((Real, jointLength, 1.0,,"Approximated contact length"))
			((Real, shearIncrementForCD, 0.0,,"toSeeWhether it is necessary to update contactArea")) //FIXME: calculated but not used; check if we have a use for it
//			((Real, overlappingVol,0.0 ,,"overlapping vol"))
//			((Real, overlappingVolMulti,0.0 ,,"overlapping vol"))
//			((Real, gap_normalized, 0.0,Attr::hidden,"not used, distance between particles normalized by particle size. Estimated using Taubin Distance"))
//			((Real, gap, 0.0,Attr::hidden,"not used, distance between particles normalized by particle size. Estimated using Taubin Distance"))
//			((bool, findCurv, false,Attr::hidden,"not used, to get radius of curvature"))
		((bool, useFaceProperties, false,,"Whether to get face properties from the intersecting particles"))
		((Real, cohesion, 0.0,,"Cohesion (stress units)"))
		((Real, tension, 0.0,,"Tension (stress units)"))
		((bool, cohesionBroken, true,,"Whether cohesion is already broken. Considered true for particles with isBoundary=True"))
		((bool, tensionBroken, true,,"Whether tension is already broken. Considered true for particles with isBoundary=True"))
//		((bool, twoDimension, false,,"Whether the contact is 2-D")) // Moved this attr in Ig2_PB_PB_ScGeom @vsangelidakis
		((Real, phi_b, 0.0,,"Basic friction angle (degrees)"))
		((Real, phi_r, 0.0,,"Residual friction angle (degrees)"))
//			((Real, asperity, 3.0,,"not used, asperity height"))
//			((Real, JRC, 0.0,,"not used, Joint Roughness Coefficient"))
//			((Real, JRCmobilized, 0.0,,"not used, Joint Roughness Coefficient"))
//			((Real, JCS, 0.0,,"not used, Joint Roughness Coefficient"))
//			((Real, sigmaC, 0.0,,"not used, Joint Roughness Coefficient"))
//			((Real, u_dilate, 0.0,,"not used, dilation distance"))
//			((Real, dilation_angle, 0.0 ,,"not used, dilation distance"))
		/* pore water pressure */
//			((Real, lambda0, 0.0 ,,"not used, initial pore water pressure to stress ratio"))
//			((Real, lambda_present, 0.0 ,,"not used, Voight&Faust (1992) Sitar et al. (2005)"))
			((Real, u_cumulative, 0.0,,"Cumulative translation")) //FIXME: (not used) This is used for the PPs; check whether we can use it here for the PBs as well
//			((Vector3r, prevShearDir, Vector3r::Zero(),,"previous shear direction"))
			((Vector3r, initialShearDir, Vector3r::Zero(),,"Initial shear direction")) //FIXME: (not used) This is used for the PPs; check whether we can use it here for the PBs as well
//			((Real, delta_porePressure, 0.0,Attr::hidden,"not used, change in pore water pressure"))
//			((Real, porePressure, 0.0,Attr::hidden,"not used, pore water pressure"))
//			((Real, bandThickness, 0.1,Attr::hidden,"not used, clay layer thickness"))
//			((Real, heatCapacities, 0.0,Attr::hidden,"not used, clay layer thickness"))
		((Real, effective_phi, 0.0,,"Friction angle in clay after displacement"))
//			((Real, prevOverlap, 0.0,,"previous overlap"))
//			((Real, h, 0.0,Attr::hidden,"not used, cd"))
		((bool,isSliding,false,,"Check if the contact is sliding (useful to calculate the ratio of sliding contacts)"))
		, /* ctor*/
		//((Real, cumulativeRotation, 0.0,,"cumulative rotation"))
		//((Quaternionr, initialOrientation1, Quaternionr(1.0,0.0,0.0,0.0),,"orientation1"))
		//((Quaternionr, initialOrientation2, Quaternionr(1.0,0.0,0.0,0.0),,"orientation2")),
		createIndex();
	);
	// clang-format on
	REGISTER_CLASS_INDEX(KnKsPBPhys, FrictPhys);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(KnKsPBPhys);


class Ip2_FrictMat_FrictMat_KnKsPBPhys : public IPhysFunctor {
public:
	virtual void go(const shared_ptr<Material>& pp1, const shared_ptr<Material>& pp2, const shared_ptr<Interaction>& interaction);
	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS(Ip2_FrictMat_FrictMat_KnKsPBPhys,IPhysFunctor,"EXPERIMENTAL. Ip2 functor for :yref:`KnKsPBPhys`",
		((Real, Knormal, ,,"Volumetric stiffness in the contact normal direction (units: stress/length)"))
		((Real, Kshear, ,,"Volumetric stiffness in the contact shear direction (units: stress/length)"))
//		((Real, unitWidth2D, 1.0,,"Unit width in 2D")) // Moved this attr in Ig2_PB_PB_ScGeom @vsangelidakis
//			((Real, brittleLength, ,,"shear length for degradation"))
		((Real, kn_i, ,,"Volumetric stiffness in the contact normal direction (units: stress/length) when isBoundary=True for one of the PBs")) //initial normal stiffness, user need to initialise
		((Real, ks_i, ,,"Volumetric stiffness in the contact shear direction (units: stress/length) when isBoundary=True for one of the PBs")) //initial shear stiffness, user need to initialise
//			((Real, u_peak, -1.0,,"peak displacement, not fully in use"))
//			((Real, maxClosure, 0.002,Attr::hidden,"not used"))
		((Real, viscousDamping, 0.0,,"Viscous damping"))
		((Real, cohesion, 0.0,,"Cohesion (stress units)"))
		((Real, tension, 0.0,,"Tension (stress units)"))
		((bool, cohesionBroken, true,,"Whether cohesion is already broken"))
		((bool, tensionBroken, true,,"Whether tension is already broken"))
		((Real, phi_b, 0.0,,"Basic friction angle (degrees)"))
//			((bool, useOverlapVol, false,Attr::hidden,"calculate overlap volume (not used)"))
		((bool, useFaceProperties, false,,"Whether to get face properties from the intersecting particles"))
//		((bool, calJointLength, false,,"Whether to calculate joint length for 2D contacts")) //This attr is replaced by Ig2_PB_PB_ScGeom::calContactArea
//		((bool, twoDimension, false,,"Whether the contact is 2-D")) // Moved this attr in Ig2_PB_PB_ScGeom @vsangelidakis
		);
	// clang-format on
	FUNCTOR2D(FrictMat, FrictMat);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Ip2_FrictMat_FrictMat_KnKsPBPhys);


class Law2_SCG_KnKsPBPhys_KnKsPBLaw : public LawFunctor {
public:
	OpenMPAccumulator<Real> plasticDissipation; // Energy dissipation due to sliding
	OpenMPAccumulator<Real> normDampDissip;     // Energy dissipated by normal damping
	OpenMPAccumulator<Real> shearDampDissip;    // Energy dissipated by tangential damping

	Real elasticEnergy();
	Real getPlasticDissipation();
	void initPlasticDissipation(Real initVal = 0);
	Real ratioSlidingContacts();
	Real getnormDampDissip();
	Real getshearDampDissip();

	//		Real stressUpdate(shared_ptr<IPhys>& ip, const Vector3r Fs_prev, const Vector3r du, const Vector3r prev_us, const Real ks /*shear stiffness */,const Real fN, const Real dFn, const Real phi_b, Vector3r & newFs);
	Real stressUpdateVec(
	        shared_ptr<IPhys>& ip,
	        const Vector3r     Fs_prev,
	        const Vector3r     du,
	        const Real         prev_us,
	        const Real         ks /*shear stiffness */,
	        const Real         fN,
	        const Real         phi_b,
	        Vector3r&          newFs);
	Real stressUpdateVecTalesnick(
	        shared_ptr<IPhys>& ip,
	        const Vector3r     Fs_prev,
	        const Vector3r     du,
	        const Real         prev_us,
	        const Real         ks /*shear stiffness */,
	        const Real         fN,
	        const Real         phi_b,
	        Vector3r&          newFs,
	        const Real         upeak);

	virtual bool go(shared_ptr<IGeom>& _geom, shared_ptr<IPhys>& _phys, Interaction* I);
	FUNCTOR2D(ScGeom, KnKsPBPhys);
	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Law2_SCG_KnKsPBPhys_KnKsPBLaw,LawFunctor,"Law for linear compression, without cohesion and Mohr-Coulomb plasticity surface.\n\n.. note::\n This law uses :yref:`ScGeom`; there is also functionally equivalent :yref:`Law2_Dem3DofGeom_FrictPhys_Basic`, which uses :yref:`Dem3DofGeom` (sphere-box interactions are not implemented for the latest).",
		((bool, neverErase, false,,"Keep interactions even if particles go away from each other (only in case another constitutive law is in the scene, e.g. :yref:`Law2_ScGeom_CapillaryPhys_Capillarity`)"))
		((bool, preventGranularRatcheting, false,,"bool to avoid granular ratcheting"))
		((bool, traceEnergy, false,,"Whether to calculate energy terms (elastic potential energy (normal and shear), plastic dissipation due to friction and dissipation of energy (normal and tangential) due to viscous damping)"))
		((bool, Talesnick, false,,"Use contact law developed for validation against model test"))
//			((Real, waterLevel, 0.0,Attr::hidden,"not used"))
		((bool, allowBreakage, false,,"Allow cohesion to break. Once broken, cohesion = 0"))
		((Real, initialOverlapDistance, 0.0,,"Initial overlap distance, defining the offset distance for tension overlap, i.e. negative overlap."))
		((bool, allowViscousAttraction, true,,"Whether to allow attractive forces due to viscous damping"))
		((int, normDampDissipIx, -1,(Attr::hidden|Attr::noSave),"Index for normal viscous damping dissipation work (with O.trackEnergy)"))
		((int, shearDampDissipIx, -1,(Attr::hidden|Attr::noSave),"Index for shear viscous damping dissipation work (with O.trackEnergy)"))
		((int, plastDissipIx, -1,(Attr::hidden|Attr::noSave),"Index for plastic dissipation (with O.trackEnergy)"))
		((int, elastPotentialIx, -1,(Attr::hidden|Attr::noSave),"Index for elastic potential energy (with O.trackEnergy)"))
		, /* ctor */
		, /* py */
		.def("elasticEnergy",&Law2_SCG_KnKsPBPhys_KnKsPBLaw::elasticEnergy,"Compute and return the total elastic energy in all \"FrictPhys\" contacts. Computed only if :yref:`Law2_SCG_KnKsPBPhys_KnKsPBLaw::traceEnergy` is true.")
		.def("normDampDissip",&Law2_SCG_KnKsPBPhys_KnKsPBLaw::getnormDampDissip,"Total energy dissipated in normal viscous damping. Computed only if :yref:`Law2_SCG_KnKsPBPhys_KnKsPBLaw::traceEnergy` is true.")
		.def("shearDampDissip",&Law2_SCG_KnKsPBPhys_KnKsPBLaw::getshearDampDissip,"Total energy dissipated in shear viscous damping. Computed only if :yref:`Law2_SCG_KnKsPBPhys_KnKsPBLaw::traceEnergy` is true.")
		.def("plasticDissipation",&Law2_SCG_KnKsPBPhys_KnKsPBLaw::getPlasticDissipation,"Total energy dissipated in plastic slips at all FrictPhys contacts. Computed only if :yref:`Law2_SCG_KnKsPBPhys_KnKsPBLaw::traceEnergy` is true.")
		.def("initPlasticDissipation",&Law2_SCG_KnKsPBPhys_KnKsPBLaw::initPlasticDissipation,"Initialize cummulated plastic dissipation to a value (0 by default).")
		.def("ratioSlidingContacts",&Law2_SCG_KnKsPBPhys_KnKsPBLaw::ratioSlidingContacts,"Return the ratio between the number of contacts sliding to the total number at a given time.")
		);
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Law2_SCG_KnKsPBPhys_KnKsPBLaw);

} // namespace yade

#endif // YADE_POTENTIAL_BLOCKS
