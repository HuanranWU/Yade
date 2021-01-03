/*************************************************************************
*  Copyright (C) 2009 by Emanuele Catalano <catalano@grenoble-inp.fr>    *
*  Copyright (C) 2009 by Bruno Chareyre <bruno.chareyre@grenoble-inp.fr> *
*  Copyright (C) 2012 by Donia Marzougui <donia.marzougui@grenoble-inp.fr>*
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#ifdef FLOW_ENGINE

#pragma once

#include "Network.hpp"
#include "Timer.h"
#include "basicVTKwritter.hpp"

namespace yade { // Cannot have #include directive inside.

typedef pair<pair<int, int>, vector<Real>> Constriction;

namespace CGT {

	template <class _Tesselation> class FlowBoundingSphere : public Network<_Tesselation> {
	public:
		typedef _Tesselation         Tesselation;
		typedef Network<Tesselation> _N;
		DECLARE_TESSELATION_TYPES(Network<Tesselation>)

		//painfull, but we need that for templates inheritance...
		using _N::alphaBoundingCells;
		using _N::boundaries;
		using _N::boundingCells;
		using _N::boundsIds;
		using _N::conductionBoundingCells;
		using _N::cornerMax;
		using _N::cornerMin;
		using _N::currentTes;
		using _N::debugOut;
		using _N::facetNFictious;
		using _N::facetVertices;
		using _N::Height;
		using _N::idOffset;
		using _N::nOfSpheres;
		using _N::num_particles;
		using _N::Rmoy;
		using _N::sectionArea;
		using _N::sSolidTot;
		using _N::T;
		using _N::thermalBoundingCells;
		using _N::vPoral;
		using _N::vPoralPorosity;
		using _N::VSolidTot;
		using _N::vtkInfiniteCells;
		using _N::vtkInfiniteVertices;
		using _N::vTotal;
		using _N::Vtotalissimo;
		using _N::vTotalPorosity;
		using _N::xMax;
		using _N::xMaxId;
		using _N::xMin;
		using _N::xMinId;
		using _N::yMax;
		using _N::yMaxId;
		using _N::yMin;
		using _N::yMinId;
		using _N::zMax;
		using _N::zMaxId;
		using _N::zMin;
		using _N::zMinId;
		//same for functions
		using _N::addBoundingPlanes;
		using _N::boundary;
		using _N::conductionBoundary;
		using _N::defineFictiousCells;
		using _N::surfaceSolidThroatInPore;
		using _N::tesselation;
		using _N::thermalBoundary;

		virtual ~FlowBoundingSphere();
		FlowBoundingSphere();

		bool slipBoundary;
		Real tolerance;
		Real relax;
		Real ks; //Hydraulic Conductivity
		bool clampKValues, meanKStat, distanceCorrection;
		bool OUTPUT_BOUDARIES_RADII;
		bool noCache;         //flag for checking if cached values cell->unitForceVectors have been defined
		bool computedOnce;    //flag for checking if current triangulation has been computed at least once
		bool pressureChanged; //are imposed pressures modified (on python side)? When it happens, we have to reApplyBoundaryConditions
		int  errorCode;
		bool factorizeOnly;
		bool getCHOLMODPerfTimings;
		bool reuseOrdering;
		bool controlCavityPressure;
		bool controlCavityVolumeChange;
		bool averageCavityPressure;
		Real cavityDV;
		Real alphaBound;
		Real alphaBoundValue;

		bool thermalEngine;
		Real fluidRho;
		Real fluidCp;
		bool sphericalVertexAreaCalculated = 0;
		Real thermalPorosity;

#ifdef PARTIALSAT
		// partialsat engine necessities for proper inheritence
		bool partialSatEngine;
		Real pAir;
		bool freeSwelling;
		Real matricSuctionRatio;
		Real nUnsatPerm;
		Real SrM, SsM;
		bool freezePorosity;
		bool useKeq;
		bool useKozeny;
		Real bIntrinsicPerm;
		Real meanInitialPorosity;
		bool freezeSaturation;
		Real permClamp;
		Real manualCrackPerm;
#endif

		//Handling imposed pressures/fluxes on elements in the form of {point,value} pairs, IPCells contains the cell handles corresponding to point
		vector<pair<Point, Real>> imposedP;
		vector<CellHandle>        IPCells;
		vector<pair<Point, Real>> imposedF;
		vector<CellHandle>        IFCells;
		vector<Point>             imposedCavity;
		vector<CellHandle>        cavityCells;
		//Blocked cells, where pressure may be computed in undrained condition
		vector<CellHandle> blockedCells;
		//Pointers to vectors used for user defined boundary pressure
		vector<Real>*pxpos, *ppval;
		void         initNewTri() { noCache = true; /*isLinearSystemSet=false; areCellsOrdered=false;*/ } //set flags after retriangulation
		bool         permeabilityMap;

		bool computeAllCells; //exececute computeHydraulicRadius for all facets and all spheres (Real cpu time but needed for now in order to define crossSections correctly)
		Real KOptFactor;
		Real minKdivKmean;
		Real maxKdivKmean;
		int  Iterations;

		//Handling imposed temperatures on elements in the form of {point,value} pairs, ITCells contains the cell handles corresponding to point
		vector<pair<Point, Real>> imposedT;
		vector<CellHandle>        ITCells;

		bool rAverage;
		int  walls_id[6];
#define parallel_forces
#ifdef parallel_forces
		int                            ompThreads;
		vector<vector<const CVector*>> perVertexUnitForce;
		vector<vector<const Real*>>    perVertexPressure;
#endif
		vector<Real>                                       edgeSurfaces;
		vector<pair<const VertexInfo*, const VertexInfo*>> edgeIds;
		vector<Real>                                       edgeNormalLubF;
		vector<Vector3r>                                   shearLubricationForces;
		vector<Vector3r>                                   shearLubricationTorques;
		vector<Vector3r>                                   pumpLubricationTorques;
		vector<Vector3r>                                   twistLubricationTorques;
		vector<Vector3r>                                   normalLubricationForce;
		vector<Matrix3r>                                   shearLubricationBodyStress;
		vector<Matrix3r>                                   normalLubricationBodyStress;
		vector<Vector3r>                                   deltaNormVel;
		vector<Vector3r>                                   deltaShearVel;
		vector<Vector3r>                                   normalV;
		vector<Real>                                       surfaceDistance;
		vector<int>                                        onlySpheresInteractions;
		vector<Matrix3r>                                   shearStressInteraction;
		vector<Matrix3r>                                   normalStressInteraction;

		void         Localize();
		virtual void computePermeability();
		virtual void gaussSeidel(Real dt = 0);
		virtual void resetNetwork();
		virtual void resetLinearSystem(); //reset both A and B in the linear system A*P=B, done typically after updating the mesh
		virtual void resetRHS() {};       ////reset only B in the linear system A*P=B, done typically after changing values of imposed pressures

		Real        kFactor;      //permeability moltiplicator
		Real        cavityFactor; // permeability factor for cavity cell neighbors
		bool        tempDependentViscosity;
		std::string key;     //to give to consolidation files a name with iteration number
		                     // 		std::vector<Real> pressures; //for automatic write maximum pressures during consolidation
		bool tessBasedForce; //allow the force computation method to be chosen from FlowEngine
		Real minPermLength;  //min branch length for Poiseuille

		Real viscosity;
		Real fluidBulkModulus;
		Real equivalentCompressibility;
		Real netCavityFlux;
		Real phiZero;
		Real cavityFlux;
		Real cavityFluidDensity;
		bool multithread;

		void         displayStatistics();
		void         initializePressure(Real pZero);
		void         initializeTemperatures(Real tZero);
		bool         reApplyBoundaryConditions();
		virtual void computeFacetForcesWithCache(bool onlyCache = false);
		void         saveVtk(const char* folder, bool withBoundaries);
		//write vertices, cells, return ids and no. of fictious neighbors, allIds is an ordered list of cell ids (from begin() to end(), for vtk table lookup),
		// some ids will appear multiple times if withBoundaries==true since boundary cells are splitted into multiple tetrahedra
		void saveMesh(basicVTKwritter& writer, bool withBoundaries, vector<int>& allIds, vector<int>& fictiousN, const char* folder);
#ifdef XVIEW
		void dessineTriangulation(Vue3D& Vue, RTriangulation& T);
		void dessineShortTesselation(Vue3D& Vue, Tesselation& Tes);
#endif
		Real permeameter(Real PInf, Real PSup, Real Section, Real DeltaY, const char* file);
		Real samplePermeability(Real& xMin, Real& xMax, Real& yMin, Real& yMax, Real& zMin, Real& zMax);
		Real computeHydraulicRadius(CellHandle cell, int j);
		Real checkSphereFacetOverlap(const Sphere& v0, const Sphere& v1, const Sphere& v2);

		Real dotProduct(CVector x, CVector y);
		Real computeEffectiveRadius(CellHandle cell, int j);
		Real computeEffectiveRadiusByPosRadius(const Point& posA, const Real& rA, const Point& posB, const Real& rB, const Point& posC, const Real& rC);
		Real computeEquivalentRadius(CellHandle cell, int j);
		//return the list of constriction values
		vector<Real>         getConstrictions();
		vector<Constriction> getConstrictionsFull();
		CVector              cellBarycenter(CellHandle& cell);

		void printVertices();
		void generateVoxelFile();

		void     computeEdgesSurfaces();
		Vector3r computeViscousShearForce(const Vector3r& deltaV, const int& edge_id, const Real& Rh);
		Real     computeNormalLubricationForce(
		            const Real& deltaNormV,
		            const Real& dist,
		            const int&  edge_id,
		            const Real& eps,
		            const Real& stiffness,
		            const Real& dt,
		            const Real& meanRad);
		Vector3r computeShearLubricationForce(
		        const Vector3r& deltaShearV, const Real& dist, const int& edge_id, const Real& eps, const Real& centerDist, const Real& meanRad);
		Vector3r computePumpTorque(const Vector3r& deltaShearAngV, const Real& dist, const int& edge_id, const Real& eps, const Real& meanRad);
		Vector3r computeTwistTorque(const Vector3r& deltaNormAngV, const Real& dist, const int& edge_id, const Real& eps, const Real& meanRad);


		RTriangulation& buildTriangulation(Real x, Real y, Real z, Real radius, unsigned const id);

		bool isInsideSphere(Real& x, Real& y, Real& z);

		void sliceField(const char* filename);
		void comsolField();

		virtual void                   interpolate(Tesselation& Tes, Tesselation& NewTes);
		virtual void                   averageRelativeCellVelocity();
		void                           averageFluidVelocity();
		void                           applySinusoidalPressure(RTriangulation& Tri, Real amplitude, Real averagePressure, Real loadIntervals);
		void                           applyUserDefinedPressure(RTriangulation& Tri, vector<Real>& xpos, vector<Real>& pval);
		bool                           isOnSolid(Real X, Real Y, Real Z);
		Real                           getPorePressure(Real X, Real Y, Real Z);
		Real                           getPoreTemperature(Real X, Real Y, Real Z);
		void                           measurePressureProfile(Real WallUpy, Real WallDowny);
		Real                           averageSlicePressure(Real Y);
		Real                           averagePressure();
		int                            getCell(Real X, Real Y, Real Z);
		Real                           boundaryFlux(unsigned int boundaryId);
		Real                           boundaryArea(unsigned int boundaryId);
		std::vector<std::vector<Real>> boundaryVel(unsigned int booundaryId);
		void                           setBlocked(CellHandle& cell);
		void                           adjustCavityPressure(Real dt, int stepsSinceLastMesh, Real pZero);
		void                           adjustCavityVolumeChange(Real dt, int stepsSinceLastMesh, Real pZero);
		void                           adjustCavityCompressibility(Real pZero);
		Real                           getCavityFlux();
		vector<Real>                   getCellVelocity(Real X, Real Y, Real Z);
		Real                           getCellVolume(Real X, Real Y, Real Z);
		vector<Real>                   averageFluidVelocityOnSphere(unsigned int Id_sph);
		//Solver?
		int  useSolver; //(0 : GaussSeidel, 1:CHOLMOD)
		Real fractionalSolidArea(CellHandle cell, int j);
	};

} //namespace CGT

}; // namespace yade

#include <lib/triangulation/FlowBoundingSphere.ipp>
#ifdef LINSOLV
#include "lib/triangulation/FlowBoundingSphereLinSolv.hpp"
#endif

#endif //FLOW_ENGINE
