#pragma once
#ifdef YADE_POTENTIAL_PARTICLES
#include <pkg/common/Dispatching.hpp>
#include <pkg/common/ElastMat.hpp>
#include <pkg/common/NormShearPhys.hpp>
#include <pkg/dem/FrictPhys.hpp>
#include <pkg/dem/ScGeom.hpp>
#include <boost/tuple/tuple.hpp>
#include <set>


namespace yade { // Cannot have #include directive inside.


class KnKsPhys : public FrictPhys {
public:
	virtual ~KnKsPhys();
	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS_CTOR(KnKsPhys,FrictPhys,"EXPERIMENTAL. IPhys for :yref:`PotentialParticle`.",
			((Real, frictionAngle, 0.0,,"Friction angle"))
//				((Real,tanFrictionAngle,0.0,,"tangent of fric angle"))
//				((Vector3r, contactDetectionPt, Vector3r::Zero(),,"contact detection result"))
			((Real, viscousDamping, 0.0,,"Viscous damping"))//FIXME: We don't need to store this attr for each contact. It can be stored only in the Law2
//			((Real, unitWidth2D, 1.0,,"Unit width in 2D")) // Moved this attr in Ig2_PP_PP_ScGeom @vsangelidakis
				((Real, maxClosure, 0.0002,,"not fully in use, vmi")) //FIXME: It is used once; check this
//				((Real, u_peak, 0.05,,"peak shear displacement, not fully in use"))
				((Real, u_elastic, 0.0,,"Elastic shear displacement, not fully in use"))
				((Real, brittleLength, 5.0,,"Shear length where strength degrades, not fully in use"))
			((Real, knVol, 0.0,,"Volumetric normal stiffness = Knormal")) //FIXME: We don't need to store this attr for each contact. It can be stored only in the IPhys functor
			((Real, ksVol, 0.0,,"Volumetric shear stiffness = Kshear" )) //FIXME: We don't need to store this attr for each contact. It can be stored only in the IPhys functor
			((Real, kn_i, 5.0,,"Currently, we assume kn_i and Knormal are adopting the same value in Ip2 initialisation")) //FIXME: We don't need to store this attr for each contact. It can be stored only in the IPhys functor
			((Real, ks_i, 5.0,,"Currently, we assume ks_i and Kshear are adopting the same value in Ip2 initialisation")) //FIXME: We don't need to store this attr for each contact. It can be stored only in the IPhys functor
			((Vector3r, normalViscous, Vector3r::Zero(),,"Viscous normal force"))
			((Vector3r, shearViscous, Vector3r::Zero(),,"Viscous shear force (assumed zero at the moment)"))
//				((Real, hwater, 0.0,,"not used, height of pore water"))
//				((bool, rockJointContact, false,," rock joint"))
			((bool, intactRock, false,,"Whether to consider cohesive force in the Mohr-Coulomb criterion, if allowBreakage=False and cohesionBroken=False."))
			((int, jointType, 0,,"jointType"))
//			((Real, Kshear_area,0.0,,"Shear contact stiffness (units=force/length), calculated as the product of the volumetric stiffness and the contact area"))
//			((Real, Knormal_area, 0.0,,"Normal contact stiffness (units=force/length), calculated as the product of the volumetric stiffness and the contact area"))
			((Vector3r, shearDir, Vector3r::Zero(),,"Shear direction"))
//				((vector<Vector3r>, shearForces, ,,"shear force"))
//				((Vector3r, shear, Vector3r::Zero(),,"shear displacement"))
			((Vector3r, prevNormal, Vector3r::Zero(),,"Previous normal"))
//				((Vector3r, normal, Vector3r::Zero(),," normalVector"))
//				((vector<Vector3r>, pointsArea, ,,"not used, intermediate contact points"))
//				((vector<Vector3r>, pointsShear, ,,"not used, points to calculate shear"))
//				((vector<Real>, areaShear, ,,"area to attribute shear"))
//				((vector<Real>, overlapDistances, ,,"not used, overlap distance"))
//				((Real, finalSize, 0.0,,"not used, finalgridsize"))
//				((int, finalGridNo, 0,,"not used, final number of grids"))
//				((vector<Real>, dualityGap, ,,"not used, duality gap for SOCP"))
				((bool, warmstart, false,,"Warmstart for SOCP, not fully in use"))
//				((int, generation, 0,,"not used, number of subdivisions"))
//				((int, triNoMain, 24,,"not used, number of subdivisions"))
//				((int, triNoSub, 6,,"not used, number of subdivisions"))
//				((Vector3r, initial1, Vector3r::Zero(),,"midpoint"))
			((Vector3r, ptOnP1, Vector3r::Zero(),,"Point on particle 1"))
			((Vector3r, ptOnP2, Vector3r::Zero(),,"Point on particle 2"))
//				((vector<bool>, redundantA, ,,"not used, activePlanes for interaction.id1"))
//				((vector<bool>, redundantB, ,,"not used, activePlanes for interaction.id1"))
//				((vector<bool>, activePlanes1, ,,"not used, activePlanes for interaction.id1"))
//				((vector<bool>, activePlanes2, ,,"not used, activePlanes for interaction.id2"))
//				((vector<Real>, activeA1, ,,"not used, activePlanes for interaction.id2"))
//				((vector<Real>, activeB1, ,,"not used, activePlanes for interaction.id2"))
//				((vector<Real>, activeC1, ,,"not used, activePlanes for interaction.id2"))
//				((vector<Real>, activeD1, ,,"not used, activePlanes for interaction.id2"))
//				((vector<Real>, activeA2, ,,"not used, activePlanes for interaction.id2"))
//				((vector<Real>, activeB2, ,,"not used, activePlanes for interaction.id2"))
//				((vector<Real>, activeC2, ,,"not used, activePlanes for interaction.id2"))
//				((vector<Real>, activeD2, ,,"not used, activePlanes for interaction.id2"))
//				((int, noActive1, 0,,"not used, activePlanes for interaction.id1"))
//				((int, noActive2, 0 ,,"not used, activePlanes for interaction.id2"))
//				((int, smallerID, 1 ,,"id of particle with smaller plane"))
			((Real, cumulative_us, 0.0,,"Cumulative shear translation (not fully in use)"))
//				((Real, cumulativeRotation, 0.0,,"cumulative rotation"))
			((Real, mobilizedShear, ,,"Percentage of mobilized shear force as the ratio of the current shear force to the current frictional limit. Represents a quantified measure of the isSliding parameter"))
			((Real, contactArea, 0.0,,"Contact area |yupdate|"))
//				((Real, radCurvFace, ,,"not used, face"))
//				((Real, prevJointLength, 0.0,,"previous joint length"))
//				((Real, radCurvCorner, ,,"not used, corners"))
			((Real, prevSigma, 0.0 ,,"Previous normal stress"))
//				((vector<Real>, prevSigmaList, 0.0 ,,"previous normal stress"))
//			((bool, calJointLength, false,,"Whether to calculate joint length for 2D contacts")) //This attr is replaced by Ig2_PP_PP_ScGeom::calContactArea
//				((bool, useOverlapVol, false,,"calculate overlap volume"))
//				((bool, calContactArea, false,,"calculate contact area"))
			((Real, jointLength, 1.0,,"Approximated contact length"))
				((Real, shearIncrementForCD, 0.0,,"toSeeWhether it is necessary to update contactArea"))
//				((Real, overlappingVol, 0.0 ,,"not used, overlapping vol"))
//				((Real, overlappingVolMulti, 0.0 ,,"not used, overlapping vol"))
//				((Real, gap_normalized, 0.0,,"not used, distance between particles normalized by particle size. Estimated using Taubin Distance"))
//				((Real, gap, 0.0,,"not used, distance between particles normalized by particle size. Estimated using Taubin Distance"))
//				((bool, findCurv, false,,"not used, to get radius of curvature"))
			((bool, useFaceProperties, false,,"Whether to get face properties from the intersecting particles"))
			((Real, cohesion, 0.0,,"Cohesion"))
			((Real, tension, 0.0,,"Tension"))
			((bool, cohesionBroken, true,,"Whether cohesion is already broken. Considered true for particles with isBoundary=True"))
			((bool, tensionBroken, true,,"Whether tension is already broken. Considered true for particles with isBoundary=True"))
//			((bool, twoDimension, false,,"Whether the contact is 2-D"))
			((Real, phi_b, 0.0,,"Basic friction angle (degrees)"))
			((Real, phi_r, 0.0,,"Residual friction angle (degrees)"))
//				((Real, asperity, 3.0,,"not used, asperity height"))
//				((Real, JRC, 0.0,,"not used, Joint Roughness Coefficient"))
//				((Real, JRCmobilized, 0.0,,"not used, Joint Roughness Coefficient"))
//				((Real, JCS, 0.0,,"not used, Joint Roughness Coefficient"))
//				((Real, sigmaC, 0.0,,"not used, Joint Roughness Coefficient"))
//				((Real, u_dilate, 0.0,,"not used, dilation distance"))
//				((Real, dilation_angle, 0.0,,"not used, dilation distance"))
			/* pore water pressure */
//				((Real, lambda0, 0.0,,"not used, initial pore water pressure to stress ratio"))
//				((Real, lambda_present, 0.0,,"not used, Voight&Faust (1992) Sitar et al. (2005)"))
			((Real, u_cumulative, 0.0,,"Cumulative translation")) //FIXME: This was marked as "not used", but it is; check this
//				((Vector3r, prevShearDir, Vector3r::Zero(),,"previous shear direction"))
			((Vector3r, initialShearDir, Vector3r::Zero(),,"Initial shear direction"))
//				((Real, delta_porePressure, 0.0,,"not used, change in pore water pressure"))
//				((Real, porePressure, 0.0,,"not used, pore water pressure"))
//				((Real, bandThickness, 0.1,,"not used, clay layer thickness"))
//				((Real, heatCapacities, 0.0,,"not used, clay layer thickness"))
			((Real, effective_phi, 0.0,,"Friction angle in clay after displacement"))
//				((Real, prevOverlap, 0.0,,"previous overlap"))
//				((Real, h, 0.0,,"not used, cd"))
			((bool,isSliding,false,,"Check if the contact is sliding (useful to calculate the ratio of sliding contacts)"))
			, /* ctor */

			//((Real, cumulativeRotation, 0.0,,"cumulative rotation"))
			//((Quaternionr, initialOrientation1, Quaternionr(1.0,0.0,0.0,0.0),,"orientation1"))
			//((Quaternionr, initialOrientation2, Quaternionr(1.0,0.0,0.0,0.0),,"orientation2")),
			createIndex();

		);
	// clang-format on

	REGISTER_CLASS_INDEX(KnKsPhys, FrictPhys);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(KnKsPhys);


class Ip2_FrictMat_FrictMat_KnKsPhys : public IPhysFunctor {
public:
	virtual void go(const shared_ptr<Material>& pp1, const shared_ptr<Material>& pp2, const shared_ptr<Interaction>& interaction);
	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS(Ip2_FrictMat_FrictMat_KnKsPhys,IPhysFunctor,"EXPERIMENTAL. Ip2 functor for :yref:`KnKsPhys`",
			((Real, Knormal,0.0,,"Volumetric stiffness in the contact normal direction (units: stress/length)"))
			((Real, Kshear,0.0,,"Volumetric stiffness in the contact shear direction (units: stress/length)"))
//			((Real, unitWidth2D, 1.0,,"Unit width in 2D")) // Moved this attr in Ig2_PP_PP_ScGeom @vsangelidakis
			((Real, brittleLength, ,,"Shear length for degradation"))
			((Real, kn_i, ,,"Currently, we assume kn_i and Knormal are adopting the same value in Ip2 initialisation"))
			((Real, ks_i, ,,"Currently, we assume ks_i and Kshear are adopting the same value in Ip2 initialisation"))
//				((Real, u_peak, -1.0,,"not used"))
				((Real, maxClosure, 0.002,,"not fully in use")) //FIXME: It is used once; check this
			((Real, viscousDamping, 0.0,,"Viscous damping"))
			((Real, cohesion, 0.0,,"Cohesion"))
			((Real, tension, 0.0,,"Tension"))
			((bool, cohesionBroken, true,,"Whether cohesion is already broken"))
			((bool, tensionBroken, true,,"Whether tension is already broken"))
			((Real, phi_b, 0.0,,"Basic friction angle"))
//				((bool, useOverlapVol, false,,"not used, calculate overlap volume"))
			((bool, useFaceProperties, false,,"Whether to get face properties from the intersecting particles"))
//			((bool, calJointLength, false,,"Whether to calculate joint length for 2D contacts")) //This attr is replaced by Ig2_PP_PP_ScGeom::calContactArea
//			((bool, twoDimension, true,,"Whether the contact is 2-D"))
		);
	// clang-format on
	FUNCTOR2D(FrictMat, FrictMat);
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Ip2_FrictMat_FrictMat_KnKsPhys);


class Law2_SCG_KnKsPhys_KnKsLaw : public LawFunctor {
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

	//		static Real Real0;
	//OpenMPAccumulator<Real,&Law2_SCG_KnKsPhys_KnKsLaw::Real0> plasticDissipation;
	virtual bool go(shared_ptr<IGeom>& _geom, shared_ptr<IPhys>& _phys, Interaction* I);
	FUNCTOR2D(ScGeom, KnKsPhys);
	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(Law2_SCG_KnKsPhys_KnKsLaw,LawFunctor,"Law for linear compression, without cohesion and Mohr-Coulomb plasticity surface.\n\n.. note::\n This law uses :yref:`ScGeom`; there is also functionally equivalent :yref:`Law2_Dem3DofGeom_FrictPhys_Basic`, which uses :yref:`Dem3DofGeom` (sphere-box interactions are not implemented for the latest).",
			((bool, neverErase,false,,"Keep interactions even if particles go away from each other (only in case another constitutive law is in the scene, e.g. :yref:`Law2_ScGeom_CapillaryPhys_Capillarity`)"))
			((bool, preventGranularRatcheting,false,,"bool to avoid granular ratcheting"))
			((bool, traceEnergy,false,,"Define the total energy dissipated in plastic slips at all contacts."))
			((bool, Talesnick,false,,"Use contact law developed for validation against model test"))
//				((Real,waterLevel,0.0,,"not used"))
			((bool, allowBreakage, false,,"Allow cohesion to break. Once broken, cohesion = 0"))
			((Real, initialOverlapDistance,0.0,,"Initial overlap distance, defining the offset distance for tension overlap, i.e. negative overlap."))
			((bool, allowViscousAttraction, true,,"Whether to allow attractive forces due to viscous damping"))
			((int, normDampDissipIx, -1,(Attr::hidden|Attr::noSave),"Index for normal viscous damping dissipation work (with O.trackEnergy)"))
			((int, shearDampDissipIx, -1,(Attr::hidden|Attr::noSave),"Index for shear viscous damping dissipation work (with O.trackEnergy)"))
			((int, plastDissipIx, -1,(Attr::hidden|Attr::noSave),"Index for plastic dissipation (with O.trackEnergy)"))
			((int, elastPotentialIx, -1,(Attr::hidden|Attr::noSave),"Index for elastic potential energy (with O.trackEnergy)"))
			, /* ctor */
			, /* py */
			.def("elasticEnergy",&Law2_SCG_KnKsPhys_KnKsLaw::elasticEnergy,"Compute and return the total elastic energy in all \"FrictPhys\" contacts. Computed only if :yref:`Law2_SCG_KnKsPhys_KnKsLaw::traceEnergy` is true.")
			.def("normDampDissip",&Law2_SCG_KnKsPhys_KnKsLaw::getnormDampDissip,"Total energy dissipated in normal viscous damping. Computed only if :yref:`Law2_SCG_KnKsPhys_KnKsLaw::traceEnergy` is true.")
			.def("shearDampDissip",&Law2_SCG_KnKsPhys_KnKsLaw::getshearDampDissip,"Total energy dissipated in shear viscous damping. Computed only if :yref:`Law2_SCG_KnKsPhys_KnKsLaw::traceEnergy` is true.")
			.def("plasticDissipation",&Law2_SCG_KnKsPhys_KnKsLaw::getPlasticDissipation,"Total energy dissipated in plastic slips at all FrictPhys contacts. Computed only if :yref:`Law2_SCG_KnKsPhys_KnKsLaw::traceEnergy` is true.")
			.def("initPlasticDissipation",&Law2_SCG_KnKsPhys_KnKsLaw::initPlasticDissipation,"Initialize cummulated plastic dissipation to a value (0 by default).")
			.def("ratioSlidingContacts",&Law2_SCG_KnKsPhys_KnKsLaw::ratioSlidingContacts,"Return the ratio between the number of contacts sliding to the total number at a given time.")

		);
	// clang-format on

	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(Law2_SCG_KnKsPhys_KnKsLaw);

} // namespace yade

#endif // YADE_POTENTIAL_PARTICLES
