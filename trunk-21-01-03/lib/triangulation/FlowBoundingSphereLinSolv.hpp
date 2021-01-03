/*************************************************************************
*  Copyright (C) 2010 by Bruno Chareyre <bruno.chareyre@grenoble-inp.fr> *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifdef FLOW_ENGINE
#pragma once

// Notes about itegrating this code with high precision Real
//
// cholmod requires Real==double. So it cannot work with arbitrary precision types.
// I tried following solvers in its place (by putting '#define NO_CHOLMOD' and conditioanlly selecting another one, then I removed it):
//    Eigen::BiCGSTAB<Eigen::SparseMatrix<Real>, Eigen::IncompleteLUT<Real> > eSolver;
//    Eigen::CholmodDecomposition<Eigen::SparseMatrix<double>, Eigen::Lower > eSolver;
//    Eigen::BiCGSTAB<Eigen::SparseMatrix<Real>, Eigen::IdentityPreconditioner > eSolver;
//    Eigen::DGMRES<Eigen::SparseMatrix<double>, Eigen::IdentityPreconditioner> eSolver;
//    Eigen::SparseLU<Eigen::SparseMatrix<Real> > eSolver;
// They weren't satisfactory. But it is very close to have all of yade to support Real type. Only the solver needs to be raplaced.
// For a more detailed example see into file lib/compatibility/LapackCompatibility.cpp
//
// So we can use a templatized solver that can work with a non-double Real type, maybe this:
//    https://eigen.tuxfamily.org/dox/group__SparseCholesky__Module.html
// to documentation only this one uses `double`: https://eigen.tuxfamily.org/dox/classEigen_1_1CholmodDecomposition.html
// seem to be general and templatized. So maybe they are faster? especially when compiling with -Ofast ?
//
// Another note: Eigen can use parallelized solvers. Since long time we had #define EIGEN_DONT_PARALLELIZE in Math.hpp (moved to Real.hpp).
// Maybe it's time to undefine it?
//

//#define LINSOLV // should be defined at cmake step
// #define TAUCS_LIB //comment this if TAUCS lib is not available, it will disable PARDISO lib as well

#ifdef LINSOLV
#include <Eigen/CholmodSupport>
#include <Eigen/Sparse>
#include <Eigen/SparseCore>
#include <cholmod.h>
#endif

#ifdef TAUCS_LIB
#define TAUCS_CORE_DOUBLE
#include <complex> //THIS ONE MUST ABSOLUTELY BE INCLUDED BEFORE TAUCS.H!
#include <float.h>
#include <stdlib.h>
//#include <time.h>
extern "C" {
#include "taucs.h"
}
#endif

#include "FlowBoundingSphere.hpp"

///_____ Declaration ____

namespace yade { // Cannot have #include directive inside.

namespace CGT {

	template <class _Tesselation, class FlowType = FlowBoundingSphere<_Tesselation>> class FlowBoundingSphereLinSolv : public FlowType {
	public:
		DECLARE_TESSELATION_TYPES(FlowType)
		typedef typename FlowType::Tesselation Tesselation;
		using FlowType::boundary;
		using FlowType::cavityDV;
		using FlowType::computedOnce;
		using FlowType::controlCavityPressure;
		using FlowType::controlCavityVolumeChange;
		using FlowType::currentTes;
		using FlowType::debugOut;
		using FlowType::equivalentCompressibility;
		using FlowType::factorizeOnly; // used for backgroundAction()
		using FlowType::fluidBulkModulus;
		using FlowType::fluidCp;
		using FlowType::fluidRho;
		using FlowType::getCHOLMODPerfTimings;
		using FlowType::multithread;
		using FlowType::ompThreads;
		using FlowType::phiZero;
		using FlowType::pressureChanged;
		using FlowType::reApplyBoundaryConditions;
		using FlowType::relax;
		using FlowType::resetNetwork;
		using FlowType::resetRHS;
		using FlowType::reuseOrdering;
		using FlowType::saveMesh;
		using FlowType::T;
		using FlowType::tesselation;
		using FlowType::thermalEngine;
		using FlowType::thermalPorosity;
		using FlowType::tolerance;
		using FlowType::useSolver;
		using FlowType::yMaxId;
		using FlowType::yMinId;
#ifdef PARTIALSAT
		using FlowType::partialSatEngine;
#endif

		//! TAUCS DECs
		vector<FiniteCellsIterator> orderedCells;
		bool                        isLinearSystemSet;
		bool                        isFullLinearSystemGSSet;
		bool                        areCellsOrdered; //true when orderedCells is filled, turn it false after retriangulation
		bool                        updatedRHS;
		struct timeval              start, end;

#ifdef LINSOLV
		//Eigen's sparse matrix and solver
		Eigen::SparseMatrix<Real> A;
		//Eigen::SparseMatrix<::yade::math::Complex,RowMajor> Ga; for row major stuff?
		typedef Eigen::Triplet<Real> ETriplet;
		std::vector<ETriplet>        tripletList; //The list of non-zero components in Eigen sparse matrix
		// cholmod requires Real==double. So it cannot work with arbitrary precision types. Below is an example to make it work:
		// #ifdef NO_CHOLMOD
		//	Eigen::BiCGSTAB<Eigen::SparseMatrix<Real>, Eigen::IncompleteLUT<Real> > eSolver;
		// #else
		Eigen::CholmodDecomposition<Eigen::SparseMatrix<double>, Eigen::Lower> eSolver;
		// #endif
		bool factorizedEigenSolver;
		void exportMatrix(const char* filename)
		{
			std::ofstream f;
			f.open(filename);
			f << A;
			f.close();
		};
		void exportTriplets(const char* filename)
		{
			std::ofstream f;
			f.open(filename);
			for (int k = 0; k < A.outerSize(); ++k)
				for (Eigen::SparseMatrix<Real>::InnerIterator it(A, k); it; ++it)
					f << it.row() << " " << it.col() << " " << it.value() << endl;
			f.close();
		};
		//Multi-threading seems to work fine for Cholesky decomposition, but it fails for the solve phase in which -j1 is the fastest,
		//here we specify both thread numbers independently
		int numFactorizeThreads;
		int numSolveThreads;
#endif
#ifdef SUITESPARSE_VERSION_4
		// cholmod direct solver (useSolver=4)

		cholmod_triplet* cholT;
		cholmod_factor*  L;
		cholmod_factor*  M;
		cholmod_factor*  N;
		cholmod_sparse*  Achol;
		cholmod_common   com;
		bool             factorExists;
#ifdef PFV_GPU
#define CHOLMOD(name) cholmod_l_##name
		void add_T_entry(cholmod_triplet* T, long r, long c, Real x)
		{
			size_t k         = T->nnz;
			((long*)T->i)[k] = r;
			((long*)T->j)[k] = c;
			((Real*)T->x)[k] = x;
			T->nnz++;
		}
#else
#define CHOLMOD(name) cholmod_##name
		void add_T_entry(cholmod_triplet* T, int r, int c, Real x)
		{
			size_t k         = T->nnz;
			((int*)T->i)[k]  = r;
			((int*)T->j)[k]  = c;
			((Real*)T->x)[k] = x;
			T->nnz++;
		}
#endif
		void CHOLMOD(wildcard)() { cout << "using cholmod in form of " << __func__ << endl; };
#endif

#ifdef TAUCS_LIB
		taucs_ccs_matrix  SystemMatrix;
		taucs_ccs_matrix* T_A;
		taucs_ccs_matrix* Fccs;
		void*             F; //The taucs factor
#endif


		int T_nnz;
		int ncols;
		int T_size;

		Real pTime1, pTime2;
		int  pTimeInt, pTime1N, pTime2N;

		Real               ZERO;
		vector<Real>       T_an;    //(size*5);
		vector<int>        T_jn;    //(size+1);
		vector<int>        T_ia;    //(size*5);
		vector<Real>       T_f;     //(size); // right-hand size vector object
		vector<CellHandle> T_cells; //(size)
		int                T_index;

		vector<Real> T_b;
		vector<Real> T_bv;
		vector<Real> T_x, P_x;
		vector<Real> bodv;
		vector<Real> xodv;
		int*         perm;
		int*         invperm;
		bool         pardisoInitialized;
		//! END TAUCS DECs


		//! Pardiso
		int*  ia;
		int*  ja;
		Real* a;
		int   nnz;
		int   mtype; /* Real symmetric positive def. matrix */
		Real* b;
		Real* x;    // the unknown vector to solve Ax=b
		int   nrhs; /* Number of right hand sides. */
		void* pt[64];
		int   iparm[64];
		Real  dparm[64];
		int   maxfct, mnum, phase, error, msglvl, solver;
		int   num_procs;
		char* var;
		int   i;
		Real  ddum; /* Double dummy */
		int   idum; /* Integer dummy. */
		//! end pardiso

		/// EXTERNAL_GS part
		vector<vector<Real>>  fullAvalues;  //contains Kij's and 1/(sum Kij) in 5th value (for use in GuaussSeidel)
		vector<vector<Real*>> fullAcolumns; //contains columns numbers
		vector<Real>          gsP;          //a vector of pressures
		vector<Real>          gsdV;         //a vector of dV
		vector<Real>          gsB;          //a vector of dV

	public:
		virtual ~FlowBoundingSphereLinSolv();
		FlowBoundingSphereLinSolv();

		///Linear system solve
		virtual int setLinearSystem(Real dt);
		void        vectorizedGaussSeidel(Real dt);
		virtual int setLinearSystemFullGS(Real dt);
		void        augmentConductivityMatrix(Real dt);
		void        setNewCellTemps(bool addToDeltaTemp);
		void        initializeInternalEnergy();

		int taucsSolveTest();
		int taucsSolve(Real dt);
		int pardisoSolveTest();
		int pardisoSolve(Real dt);
		int eigenSolve(Real dt);
		int cholmodSolve(Real dt);

		void copyGsToCells();
		void copyCellsToGs(Real dt);

		void         copyLinToCells();
		virtual void copyCellsToLin(Real dt);
		void         swapFwd(Real* v, int i);
		void         swapFwd(int* v, int i);
		void         sortV(int k1, int k2, int* is, Real* ds);

		virtual void gaussSeidel(Real dt)
		{
			switch (useSolver) {
				case 0: vectorizedGaussSeidel(dt); break;
				case 1: taucsSolve(dt); break;
				case 2: pardisoSolve(dt); break;
				case 3: eigenSolve(dt); break;
				case 4: cholmodSolve(dt); break;
			}
			computedOnce = true;
		}
		virtual void resetLinearSystem();
		virtual void resetRHS() { updatedRHS = false; };
	};

} //namespace CGT

} // namespace yade


///_____ Implementation ____

#include "FlowBoundingSphereLinSolv.ipp"

#endif
