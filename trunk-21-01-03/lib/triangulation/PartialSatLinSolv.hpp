/*************************************************************************
*  Copyright (C) 2019 by Robert Caulk <rob.caulk@gmail.com>              *
*  Copyright (C) 2019 by Bruno Chareyre <bruno.chareyre@hmg.inpg.fr>     *
*                                                                        *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include "FlowBoundingSphere.hpp"

#ifdef PARTIALSAT

namespace yade {

namespace CGT {

	//typedef CGT::_Tesselation<CGT::TriangulationTypes<PartialSatVertexInfo,PartialSatCellInfo> > PartialSatTesselation;

	template <class _Tesselation> class PartialSatLinSolv : public FlowBoundingSphereLinSolv<_Tesselation, FlowBoundingSphere<_Tesselation>> {
	public:
		typedef _Tesselation         Tesselation;
		typedef Network<Tesselation> _N;
		DECLARE_TESSELATION_TYPES(Network<Tesselation>)
		typedef FlowBoundingSphereLinSolv<_Tesselation, FlowBoundingSphere<_Tesselation>> BaseFlowSolver;
		typedef typename BaseFlowSolver::ETriplet                                         ETriplet;

		///painfull, but we need that for templates inheritance...
		using _N::alphaBoundingCells;
		using _N::boundaries;
		using _N::boundingCells;
		using _N::boundsIds;
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
		using _N::defineFictiousCells;
		;
		using _N::surfaceSolidThroatInPore;
		using _N::tesselation;

		using BaseFlowSolver::bIntrinsicPerm;
		using BaseFlowSolver::checkSphereFacetOverlap;
		using BaseFlowSolver::clampKValues;
		using BaseFlowSolver::computeAllCells;
		using BaseFlowSolver::computeEffectiveRadius;
		using BaseFlowSolver::computeEquivalentRadius;
		using BaseFlowSolver::computeHydraulicRadius;
		using BaseFlowSolver::distanceCorrection;
		using BaseFlowSolver::factorizeOnly;
		using BaseFlowSolver::fluidBulkModulus;
		using BaseFlowSolver::freeSwelling;
		using BaseFlowSolver::freezePorosity;
		using BaseFlowSolver::freezeSaturation;
		using BaseFlowSolver::kFactor;
		using BaseFlowSolver::KOptFactor;
		using BaseFlowSolver::manualCrackPerm;
		using BaseFlowSolver::matricSuctionRatio;
		using BaseFlowSolver::maxKdivKmean;
		using BaseFlowSolver::meanInitialPorosity;
		using BaseFlowSolver::meanKStat;
		using BaseFlowSolver::minKdivKmean;
		using BaseFlowSolver::minPermLength;
		using BaseFlowSolver::noCache;
		using BaseFlowSolver::nUnsatPerm;
		using BaseFlowSolver::pAir;
		using BaseFlowSolver::permClamp;
		using BaseFlowSolver::permeabilityMap;
		using BaseFlowSolver::perVertexPressure;
		using BaseFlowSolver::perVertexUnitForce;
		using BaseFlowSolver::rAverage;
		using BaseFlowSolver::relax;
		using BaseFlowSolver::resetRHS;
		using BaseFlowSolver::setBlocked;
		using BaseFlowSolver::SrM;
		using BaseFlowSolver::SsM;
		using BaseFlowSolver::tolerance;
		using BaseFlowSolver::useKozeny;
		using BaseFlowSolver::viscosity;
		/// More members from LinSolv variant
		using BaseFlowSolver::A;
		using BaseFlowSolver::Achol;
		using BaseFlowSolver::add_T_entry;
		using BaseFlowSolver::alphaBoundValue;
		using BaseFlowSolver::areCellsOrdered;
		using BaseFlowSolver::bodv;
		using BaseFlowSolver::cholT;
		using BaseFlowSolver::com;
		using BaseFlowSolver::computedOnce;
		using BaseFlowSolver::copyLinToCells;
		using BaseFlowSolver::end;
		using BaseFlowSolver::errorCode;
		using BaseFlowSolver::eSolver;
		using BaseFlowSolver::exportTriplets;
		using BaseFlowSolver::factorExists;
		using BaseFlowSolver::factorizedEigenSolver;
		using BaseFlowSolver::fluidCp;
		using BaseFlowSolver::fluidRho;
		using BaseFlowSolver::fullAcolumns;
		using BaseFlowSolver::fullAvalues;
		using BaseFlowSolver::getCHOLMODPerfTimings;
		using BaseFlowSolver::gsB;
		using BaseFlowSolver::gsdV;
		using BaseFlowSolver::gsP;
		using BaseFlowSolver::isFullLinearSystemGSSet;
		using BaseFlowSolver::isLinearSystemSet;
		using BaseFlowSolver::L;
		using BaseFlowSolver::multithread;
		using BaseFlowSolver::N;
		using BaseFlowSolver::ncols;
		using BaseFlowSolver::numFactorizeThreads;
		using BaseFlowSolver::numSolveThreads;
		using BaseFlowSolver::ompThreads;
		using BaseFlowSolver::orderedCells;
		using BaseFlowSolver::partialSatEngine;
		using BaseFlowSolver::reApplyBoundaryConditions;
		using BaseFlowSolver::reuseOrdering;
		using BaseFlowSolver::start;
		using BaseFlowSolver::T_b;
		using BaseFlowSolver::T_bv;
		using BaseFlowSolver::T_cells;
		using BaseFlowSolver::T_index;
		using BaseFlowSolver::T_nnz;
		using BaseFlowSolver::T_x;
		using BaseFlowSolver::thermalEngine;
		using BaseFlowSolver::tripletList;
		using BaseFlowSolver::updatedRHS;
		using BaseFlowSolver::useKeq;
		using BaseFlowSolver::useSolver;
		using BaseFlowSolver::xodv;

		vector<int> indices; //redirection vector containing the rank of cell so that T_cells[indices[cell->info().index]]=cell
		Real        averageK;
		virtual ~PartialSatLinSolv();
		PartialSatLinSolv();

		///Linear system solve
		virtual int       setLinearSystem(Real dt = 0);
		virtual void      copyCellsToLin(Real dt = 0);
		virtual void      interpolate(Tesselation& Tes, Tesselation& NewTes);
		virtual void      computeFacetForcesWithCache(bool onlyCache = false);
		virtual void      computePermeability();
		Real              getCellSaturation(Real x, Real y, Real z);
		std::vector<Real> getCellVelocity(Real x, Real y, Real z);
		Real              getAverageSaturation();
		Real              getAverageSuction();
		Real              getCellPorosity(Real x, Real y, Real z);
		Real              getCellVolume(Real x, Real y, Real z);
		bool              getCellCracked(Real x, Real y, Real z);
		//Real partialSatBoundaryFlux(unsigned int boundaryId);
	};

} //namespace CGTF
} //namespace yade
#include "PartialSatLinSolv.ipp"
#endif //FLOW_ENGINE
