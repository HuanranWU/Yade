/*************************************************************************
*  Copyright (C) 2010 by Bruno Chareyre <bruno.chareyre@hmg.inpg.fr>     *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifdef PARTIALSAT


#if CGAL_VERSION_NR < CGAL_VERSION_NUMBER(4, 11, 0)
#include "CGAL/constructions/constructions_on_weighted_points_cartesian_3.h"
#endif
#include "vector"
#include <CGAL/Width_3.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <new>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>

#ifdef YADE_OPENMP
#include <omp.h>
#endif
namespace yade { // Cannot have #include directive inside.
namespace CGT {

	template <class _Tesselation> PartialSatLinSolv<_Tesselation>::~PartialSatLinSolv()
	{
#ifdef SUITESPARSE_VERSION_4
		if (useSolver == 4) {
			if (getCHOLMODPerfTimings)
				gettimeofday(&start, NULL);
			CHOLMOD(free_sparse)(&Achol, &com);
			CHOLMOD(free_factor)(&L, &com);
			CHOLMOD(finish)(&com);
			if (getCHOLMODPerfTimings) {
				gettimeofday(&end, NULL);
				cout << "CHOLMOD Time to finalize multithreaded com "
				     << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) << endl;
			}
		}
#endif
	}
	template <class _Tesselation>
	PartialSatLinSolv<_Tesselation>::PartialSatLinSolv()
	        : BaseFlowSolver()
	{
	}

	template <class _Tesselation> int PartialSatLinSolv<_Tesselation>::setLinearSystem(Real dt)
	{
#ifdef SUITESPARSE_VERSION_4
// if (!multithread && factorExists && useSolver==4){
// 	if (getCHOLMODPerfTimings) gettimeofday (&start, NULL);
// 	CHOLMOD(free_sparse)(&Achol, &com);
// 	//cholmod_l_free_triplet(&cholT, &com);
// 	//if (!reuseOrdering) {  // turning off for now
// 		CHOLMOD(free_factor)(&L, &com);
// 		CHOLMOD(finish)(&com);
// 		if (getCHOLMODPerfTimings){
// 			gettimeofday (&end, NULL);
// 			cout << "CHOLMOD Time to finalize singlethreaded com " << ((end.tv_sec *1000000   + end.tv_usec ) - (start.tv_sec * 1000000 + start.tv_usec )) << endl;
// 		}
// 		CHOLMOD(start)(&com);
// 	//}
// 	com.nmethods= 1; // nOrderingMethods; //1;
// 	com.method[0].ordering = CHOLMOD_METIS; // orderingMethod; //CHOLMOD_METIS;
// 	factorExists=false;
//
// }
#endif

		if (getCHOLMODPerfTimings)
			gettimeofday(&start, NULL);

		RTriangulation& Tri     = T[currentTes].Triangulation();
		int             n_cells = Tri.number_of_finite_cells();
		vector<int>     clen;
		vector<int>     is;
		vector<int>     js;
		vector<Real>    vs;
		if (!areCellsOrdered) {
			T_nnz = 0;
			ncols = 0;
			///Ordered cells
			orderedCells.clear();
			//
			const FiniteCellsIterator cellEnd = Tri.finite_cells_end();
			for (FiniteCellsIterator cell = Tri.finite_cells_begin(); cell != cellEnd; cell++) {
				orderedCells.push_back(cell);
				cell->info().index = 0;
				if (!cell->info().Pcondition && !cell->info().blocked)
					++ncols;
			}
			orderedCells.shrink_to_fit();
			//		//Segfault on 14.10, and useless overall since SuiteSparse has preconditionners (including metis)
			// 		spatial_sort(orderedCells.begin(),orderedCells.end(), CellTraits_for_spatial_sort<RTriangulation>());
			T_cells.clear();
			//
			T_index           = 0;
			isLinearSystemSet = false;
			areCellsOrdered   = true;
		}
		if (!isLinearSystemSet) {
			int n = 3 * (ncols + 1); //number of non-zero in triangular matrix
			is.resize(n);
			js.resize(n);
			vs.resize(n);
			T_x.clear();
			T_x.resize(ncols);
			T_x.shrink_to_fit();
			T_b.clear();
			T_b.resize(ncols);
			T_b.shrink_to_fit();
			T_bv.clear();
			T_bv.resize(ncols);
			T_bv.shrink_to_fit();
			//bodv.resize(ncols);
			//xodv.resize(ncols);
			//gsB.resize(ncols+1);
			T_cells.resize(ncols + 1);
			T_cells.shrink_to_fit();
			T_nnz = 0;
		}
		for (int kk = 0; kk < ncols; kk++)
			T_b[kk] = 0;
		///Ordered cells
		int        index = 0, nIndex = 0;
		CellHandle neighbourCell;
		for (int i = 0; i < n_cells; i++) {
			FiniteCellsIterator& cell = orderedCells[i];
			///Non-ordered cells
			// 	for (FiniteCellsIterator cell = Tri.finite_cells_begin(); cell != cell_end; cell++) {
			if (!cell->info().Pcondition && !cell->info().blocked) {
				index = cell->info().index;
				if (index == 0) {
					T_cells[++T_index] = cell;
					cell->info().index = index = T_index;
				}
				if (!isLinearSystemSet) {
					//Add diagonal term
					is[T_nnz] = index;
					js[T_nnz] = index;
					vs[T_nnz] = 0;
					for (int j = 0; j < 4; j++)
						if (!cell->neighbor(j)->info().blocked)
							vs[T_nnz] += (cell->info().kNorm())[j];
					// 				vs[T_nnz] = (cell->info().kNorm())[0]+ (cell->info().kNorm())[1]+ (cell->info().kNorm())[2]+ (cell->info().kNorm())[3];
					if (fluidBulkModulus > 0)
						vs[T_nnz] += (1.f / (dt * fluidBulkModulus * cell->info().invVoidVolume()));
					if (!freezeSaturation && partialSatEngine && !isnan(cell->info().dsdp)) {
						vs[T_nnz] += cell->info().dsdp / (cell->info().invVoidVolume() * dt);
						//if (useKeq) vs[T_nnz] += 1./(dt*cell->info().equivalentBulkModulus*cell->info().invVoidVolume());
					}
					++T_nnz;
				}
				for (int j = 0; j < 4; j++) {
					neighbourCell = cell->neighbor(j);
					nIndex        = neighbourCell->info().index;
					if (Tri.is_infinite(neighbourCell))
						continue;
					if (!isLinearSystemSet && !(neighbourCell->info().Pcondition || neighbourCell->info().blocked)) {
						if (nIndex == 0) {
							T_cells[++T_index]          = neighbourCell;
							neighbourCell->info().index = nIndex = T_index;
						} else if (index > nIndex) {
							is[T_nnz] = index;
							js[T_nnz] = nIndex;
							vs[T_nnz] = -(cell->info().kNorm())[j];
							T_nnz++;
						}
					} else if (neighbourCell->info().Pcondition && !neighbourCell->info().blocked) {
						//ADD TO b, FIXME : use updated volume change
						T_b[index - 1] += cell->info().kNorm()[j] * neighbourCell->info().p();
					}
				}
			}
		}

		if (getCHOLMODPerfTimings) {
			gettimeofday(&end, NULL);
			cout << "CHOLMOD Time to build linear equations " << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec))
			     << endl;
		}
		updatedRHS = true;
		if (!isLinearSystemSet) {
			if (useSolver == 1 || useSolver == 2) {
				cerr << " TAUCS is not supported for PartialSatClayEngine " << endl;
#ifdef LINSOLV
			} else if (useSolver == 3) {
				tripletList.clear();
				tripletList.resize(T_nnz);
				tripletList.shrink_to_fit();
				for (int k = 0; k < T_nnz; k++)
					tripletList[k] = ETriplet(is[k] - 1, js[k] - 1, vs[k]);
				A.resize(ncols, ncols);
				A.data().squeeze();
				A.setFromTriplets(tripletList.begin(), tripletList.end());
#endif
#ifdef SUITESPARSE_VERSION_4
			} else if (useSolver == 4) {
				if (getCHOLMODPerfTimings)
					gettimeofday(&start, NULL);
				cholmod_triplet* trip = CHOLMOD(allocate_triplet)(ncols, ncols, T_nnz, 1, CHOLMOD_REAL, &com);

				// set all the values for the cholmod triplet matrix
				for (int k = 0; k < T_nnz; k++) {
					add_T_entry(trip, is[k] - 1, js[k] - 1, vs[k]);
				}
				if (factorExists) {
					CHOLMOD(free_sparse)(&Achol, &com);
					CHOLMOD(free_factor)(&L, &com);
				}
				Achol = CHOLMOD(triplet_to_sparse)(trip, trip->nnz, &com);
				CHOLMOD(free_triplet)(&trip, &com);
				//trip=0;
				if (getCHOLMODPerfTimings) {
					CHOLMOD(print_sparse)(Achol, "Achol", &com);
					gettimeofday(&end, NULL);
					cout << "CHOLMOD Time to allocate matrix "
					     << ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) << endl;
				}
#endif
			}
			isLinearSystemSet = true;
		}
		return ncols;
	}

	// clang-format off
	template <class _Tesselation> void PartialSatLinSolv<_Tesselation>::copyCellsToLin(Real dt)
	{
		for (int ii = 1; ii <= ncols; ii++) {
			T_bv[ii - 1] = T_b [ii - 1]; // - T_cells[ii]->info().dv(); // considering the volume change of tetrahedrals and the contribution to pressure change
			if (fluidBulkModulus > 0)
				T_bv[ii - 1] += T_cells[ii]->info().p() / (fluidBulkModulus * dt * T_cells[ii]->info().invVoidVolume());
			if (!freezeSaturation && partialSatEngine && !isnan(T_cells[ii]->info().invVoidVolume())) {
				T_bv[ii - 1] += T_cells[ii]->info().p() * T_cells[ii]->info().dsdp
				        / (T_cells[ii]->info().invVoidVolume() * dt); // positive or negative??
				//if (useKeq) T_bv[ii-1] += T_cells[ii]->info().p()/(T_cells[ii]->info().equivalentBulkModulus*dt*T_cells[ii]->info().invVoidVolume());
			}
		}
	}

	template <class _Tesselation> void PartialSatLinSolv<_Tesselation>::interpolate(Tesselation& Tes, Tesselation& NewTes)
	{
		CellHandle      oldCell;
		RTriangulation& Tri = Tes.Triangulation();
		//#ifdef YADE_OPENMP
		const long size = NewTes.cellHandles.size();
		//#pragma omp parallel for num_threads(ompThreads>0 ? ompThreads : 1)
		for (long i = 0; i < size; i++) {
			CellHandle& newCell = NewTes.cellHandles[i];
			//cout << "cell i "<< i << endl;
			//#else
			//for (typename VectorCell::iterator cellIt=NewTes.cellHandles.begin(); cellIt!=NewTes.cellHandles.end(); cellIt++){
			//	CellHandle& newCell = *cellIt;
			//#endif
			if (newCell->info().isGhost || newCell->info().isAlpha) continue; // a new alpha cell will have vertices that are not in the original Tes.vertex().
			CVector center(0, 0, 0);
			//cout <<"get center of alpha cell? " << newCell->info().isAlpha << endl;
			if (newCell->info().fictious() == 0)
				for (int k = 0; k < 4; k++) {
					// if (newCell->vertex(k)->info().isAlpha) {
					//  	cout << "vertex alpha is alpha with ID" << endl;
					//  	cout << newCell->vertex(k)->info().id() << endl;
					// }
					center = center + 0.25 * (Tes.vertex(newCell->vertex(k)->info().id())->point().point() - CGAL::ORIGIN);
				}
			else {
				Real boundPos = 0;
				int  coord    = 0;
				for (int k = 0; k < 4; k++) {
					if (!newCell->vertex(k)->info().isFictious)
						center = center
						        + (1. / (4. - newCell->info().fictious()))
						                * (Tes.vertex(newCell->vertex(k)->info().id())->point().point() - CGAL::ORIGIN);
				}
				for (int k = 0; k < 4; k++) {
					if (newCell->vertex(k)->info().isFictious) {
						coord    = boundary(newCell->vertex(k)->info().id()).coordinate;
						boundPos = boundary(newCell->vertex(k)->info().id()).p[coord];
						center   = CVector(
                                                        coord == 0 ? boundPos : center[0],
                                                        coord == 1 ? boundPos : center[1],
                                                        coord == 2 ? boundPos : center[2]);
					}
				}
			}
			//cout << "using center" << endl;
			//cout << center[0] << " " << center[1] << " "<< center[2] << endl;
			oldCell = Tri.locate(CGT::Sphere(center[0], center[1], center[2]));
			//cout << "old cell located" << endl;
			if (oldCell->info().isAlpha) { // if a new cell appears without a past interpolate, we call it an alpha
				newCell->info().isAlpha    = true;
				newCell->info().Pcondition = true;
				newCell->info().p()        = alphaBoundValue;
				alphaBoundingCells.push_back(newCell);
				continue;
			}
			if (!newCell->info().Pcondition) newCell->info().getInfo(oldCell->info());
			if (!newCell->info().Tcondition && thermalEngine) newCell->info().temp() = oldCell->info().temp();
			//cout << "saturation interp" <<endl;
			if (!newCell->info().Pcondition) newCell->info().sat() = oldCell->info().sat();
			newCell->info().crack             = oldCell->info().crack;
			newCell->info().crackNum          = oldCell->info().crackNum;
			newCell->info().initialSaturation = oldCell->info().initialSaturation;
			newCell->info().initialPorosity   = oldCell->info().initialPorosity;

			newCell->info().porosity = oldCell->info().porosity;
			newCell->info().vSolids  = oldCell->info().vSolids;
			newCell->info().blocked = oldCell->info().blocked; //used for carrying clumped minerals through sim - maybe an issue for blocking cracks
			newCell->info().initiallyCracked = oldCell->info().initiallyCracked;
			// shouldnt need this stuff...
			newCell->info().dsdp = oldCell->info().dsdp;
			newCell->info().Po = oldCell->info().Po;
			newCell->info().lambdao = oldCell->info().lambdao;
			newCell->info().oldPressure = oldCell->info().oldPressure;
			//cout << "blocked interp" << endl;
			/*		if (freezePorosity) {*/
			/*			newCell->info().lambdao = oldCell->info().lambdao;*/
			/*			newCell->info().Po = oldCell->info().Po;*/
			/*		}*/
		}
	}

	template <class _Tesselation> void PartialSatLinSolv<_Tesselation>::computeFacetForcesWithCache(bool onlyCache)
	{
		RTriangulation& Tri = T[currentTes].Triangulation();
		CVector         nullVect(0, 0, 0);
		//reset forces
		if (!onlyCache)
			for (FiniteVerticesIterator v = Tri.finite_vertices_begin(); v != Tri.finite_vertices_end(); ++v)
				v->info().forces = nullVect;

		/*	#ifdef parallel_forces*/
		/*	if (noCache) {*/
		/*		perVertexUnitForce.clear(); perVertexPressure.clear();*/
		/*		perVertexUnitForce.resize(T[currentTes].maxId+1);*/
		/*		perVertexPressure.resize(T[currentTes].maxId+1);}*/
		/*	#endif*/
		CellHandle   neighbourCell;
		VertexHandle mirrorVertex;
		CVector      tempVect;
		//FIXME : Ema, be carefull with this (noCache), it needs to be turned true after retriangulation
		if (noCache) {
			for (VCellIterator cellIt = T[currentTes].cellHandles.begin(); cellIt != T[currentTes].cellHandles.end(); cellIt++) {
				CellHandle& cell = *cellIt;
				//reset cache
				for (int k = 0; k < 4; k++)
					cell->info().unitForceVectors[k] = nullVect;

				for (int j = 0; j < 4; j++)
					if (!Tri.is_infinite(cell->neighbor(j))) {
						neighbourCell        = cell->neighbor(j);
						const CVector& Surfk = cell->info().facetSurfaces[j];
						//FIXME : later compute that fluidSurf only once in hydraulicRadius, for now keep full surface not modified in cell->info for comparison with other forces schemes
						//The ratio void surface / facet surface
						//Area of the facet (i.e. the triangle)
						Real area = sqrt(Surfk.squared_length());
						if (area <= 0)
							cerr << "AREA <= 0!!" << endl;
						CVector                     facetNormal   = Surfk / area;
						const std::vector<CVector>& crossSections = cell->info().facetSphereCrossSections;
						//This is the cross-sectional area of the throat
						CVector fluidSurfk = cell->info().facetSurfaces[j] * cell->info().facetFluidSurfacesRatio[j];
						/// handle fictious vertex since we can get the projected surface easily here
						if (cell->vertex(j)->info().isFictious and !cell->info().isAlpha) {
							//projection of facet on the boundary
							Real projSurf                  = std::abs(Surfk[boundary(cell->vertex(j)->info().id()).coordinate]);
							tempVect                       = -projSurf * boundary(cell->vertex(j)->info().id()).normal;
							cell->vertex(j)->info().forces = cell->vertex(j)->info().forces + tempVect * cell->info().p();
							//define the cached value for later use with cache*p
							cell->info().unitForceVectors[j] = cell->info().unitForceVectors[j] + tempVect;
						}
						/// Apply weighted forces f_k=sqRad_k/sumSqRad*f
						CVector facetUnitForce = -fluidSurfk * cell->info().solidSurfaces[j][3];
						CVector facetForce;
						if (((cell->info().isFictious or cell->info().isAlpha) && freeSwelling) /*|| cell->info().isExposed*/)
							facetForce = pAir * facetUnitForce; // forces computed on boundaries of free swelling case based on air pressure only
						else {
							facetForce = cell->info().p() * facetUnitForce * cell->info().sat() * matricSuctionRatio;
							if (useKeq) {
								Real delP = -cell->info().dv() * cell->info().equivalentBulkModulus/ cell->info().volume(); // assume that each step starts with 0facet force due to volume changes (if there was no volume change, then there should be no force due to compressible mixture in cell)
								facetForce += delP * facetUnitForce;
							}
						}


						for (int y = 0; y < 3; y++) {
							//1st the drag (viscous) force weighted by surface of spheres in the throat
							cell->vertex(facetVertices[j][y])->info().forces = cell->vertex(facetVertices[j][y])->info().forces
							        + facetForce * cell->info().solidSurfaces[j][y]; // no viscous drag in partiallysaturated clay
							//(add to cached value)
							cell->info().unitForceVectors[facetVertices[j][y]] = cell->info().unitForceVectors[facetVertices[j][y]]
							        + facetUnitForce * cell->info().solidSurfaces[j][y];
							//2nd the partial integral of pore pressure, which boils down to weighting by partial cross-sectional area
							//uncomment to get total force / comment to get only viscous forces (Bruno)
							if (!cell->vertex(facetVertices[j][y])->info().isFictious and !cell->vertex(facetVertices[j][y])->info().isAlpha) {
								if (((cell->info().isFictious or cell->info().isAlpha)
								     && freeSwelling) /*|| cell->info().isExposed*/) {
									cell->vertex(facetVertices[j][y])->info().forces
									        = cell->vertex(facetVertices[j][y])->info().forces
									        - facetNormal * pAir* crossSections [j][y]; // forces exerted by boundary cells computed not according to suction
								} else {
									cell->vertex(facetVertices[j][y])->info().forces = cell->vertex(facetVertices[j][y])->info().forces
									        - facetNormal * cell->info().p() * crossSections[j][y] * cell->info().sat()
									                * matricSuctionRatio; // when saturation increases at beginning of imbibition on edges, the force increases and we see contraction
								} //add to cached value
								cell->info().unitForceVectors[facetVertices[j][y]]
								        = cell->info().unitForceVectors[facetVertices[j][y]]
								        - facetNormal * crossSections[j][y];
							}
						}
						/*					#ifdef parallel_forces*/
						/*					perVertexUnitForce[cell->vertex(j)->info().id()].push_back(&(cell->info().unitForceVectors[j]));*/
						/*					if  ((cell->info().isFictious && freeSwelling) || cell->info().isExposed) perVertexPressure[cell->vertex(j)->info().id()].push_back(&(pAir));*/
						/*					else perVertexPressure[cell->vertex(j)->info().id()].push_back(&(cell->info().p()));*/
						/*					#endif*/
					}
			}
			noCache = false; //cache should always be defined after execution of this function
			if (onlyCache)
				return;
		} else { //use cached values

			//		#ifndef parallel_forces
			//	#pragma omp parallel for
			for (FiniteCellsIterator cell = Tri.finite_cells_begin(); cell != Tri.finite_cells_end(); cell++) {
				for (int yy = 0; yy < 4; yy++) {
					if (((cell->info().isFictious or cell->info().isAlpha) && freeSwelling) /*|| cell->info().isExposed*/) {
						cell->vertex(yy)->info().forces = cell->vertex(yy)->info().forces + cell->info().unitForceVectors[yy] * pAir;
					} else {
						cell->vertex(yy)->info().forces = cell->vertex(yy)->info().forces
						        + cell->info().unitForceVectors[yy] * cell->info().p() * cell->info().sat() * matricSuctionRatio;
						if (useKeq) {
							Real delP = -cell->info().dv() * cell->info().equivalentBulkModulus
							        / cell->info().volume(); // assume that each step starts with 0facet force due to volume changes (if there was no volume change, then there should be no force due to compressible mixture in cell)
							cell->vertex(yy)->info().forces
							        = cell->vertex(yy)->info().forces + cell->info().unitForceVectors[yy] * delP;
						}
					}
				}
			}

			/*		#else*/
			/*		#pragma omp parallel for num_threads(ompThreads)*/
			/*		for (int vn=0; vn<= T[currentTes].maxId; vn++) {*/
			/*			if (T[currentTes].vertexHandles[vn]==NULL) continue;*/
			/*			VertexHandle& v = T[currentTes].vertexHandles[vn];*/
			/*			const int& id =  v->info().id();*/
			/*			CVector tf (0,0,0);*/
			/*			int k=0;*/
			/*			for (vector<const Real*>::iterator c = perVertexPressure[id].begin(); c != perVertexPressure[id].end(); c++)*/
			/*				tf = tf + (*(perVertexUnitForce[id][k++]))*(**c);*/
			/*			v->info().forces = tf;*/
			/*		}*/
			/*		#endif*/
		}
		if (debugOut) {
			CVector totalForce = nullVect;
			for (FiniteVerticesIterator v = Tri.finite_vertices_begin(); v != Tri.finite_vertices_end(); ++v) {
				if (!v->info().isFictious)
					totalForce = totalForce + v->info().forces;
				else if (boundary(v->info().id()).flowCondition == 1)
					totalForce = totalForce + v->info().forces;
			}
			cout << "totalForce = " << totalForce << endl;
		}
	}
	// clang-format on

	template <class _Tesselation> void PartialSatLinSolv<_Tesselation>::computePermeability()
	{
		if (debugOut)
			cout << "----Computing_Permeability------" << endl;
		RTriangulation& Tri = T[currentTes].Triangulation();
		VSolidTot = 0, Vtotalissimo = 0, vPoral = 0, sSolidTot = 0, vTotalPorosity = 0, vPoralPorosity = 0;
		FiniteCellsIterator cellEnd = Tri.finite_cells_end();

		CellHandle neighbourCell;

		Real k = 0, distance = 0, radius = 0;
		int  surfneg = 0;
		int  NEG = 0, POS = 0, pass = 0;

		bool ref   = Tri.finite_cells_begin()->info().isvisited;
		Real meanK = 0, STDEV = 0, meanRadius = 0, meanDistance = 0;
		//Real infiniteK = 1e10;

		for (VCellIterator cellIt = T[currentTes].cellHandles.begin(); cellIt != T[currentTes].cellHandles.end(); cellIt++) {
			CellHandle& cell = *cellIt;
			if (cell->info().blocked) {
				setBlocked(cell);
				cell->info().isvisited = !ref;
			}
			//if (cell->info().crack) return;
			Point& p1 = cell->info();
			for (int j = 0; j < 4; j++) {
				neighbourCell = cell->neighbor(j);
				Point& p2     = neighbourCell->info();
				if (!Tri.is_infinite(neighbourCell) && (neighbourCell->info().isvisited == ref || computeAllCells)) {
					//compute and store the area of sphere-facet intersections for later use
					VertexHandle W[3];
					for (int kk = 0; kk < 3; kk++) {
						W[kk] = cell->vertex(facetVertices[j][kk]);
					}
					Sphere& v0                               = W[0]->point();
					Sphere& v1                               = W[1]->point();
					Sphere& v2                               = W[2]->point();
					cell->info().facetSphereCrossSections[j] = CVector(
					        W[0]->info().isFictious ? 0
					                                : 0.5 * v0.weight()
					                        * acos((v1.point() - v0.point()) * (v2.point() - v0.point())
					                               / sqrt((v1.point() - v0.point()).squared_length()
					                                      * (v2.point() - v0.point()).squared_length())),
					        W[1]->info().isFictious ? 0
					                                : 0.5 * v1.weight()
					                        * acos((v0.point() - v1.point()) * (v2.point() - v1.point())
					                               / sqrt((v1.point() - v0.point()).squared_length()
					                                      * (v2.point() - v1.point()).squared_length())),
					        W[2]->info().isFictious ? 0
					                                : 0.5 * v2.weight()
					                        * acos((v0.point() - v2.point()) * (v1.point() - v2.point())
					                               / sqrt((v1.point() - v2.point()).squared_length()
					                                      * (v2.point() - v0.point()).squared_length())));
					//FIXME: it should be possible to skip completely blocked cells, currently the problem is it segfault for undefined areas
					// 				if (cell->info().blocked) continue;//We don't need permeability for blocked cells, it will be set to zero anyway
					pass += 1;
					CVector l = p1 - p2;
					distance  = sqrt(l.squared_length());
					if (!rAverage)
						radius = 2 * computeHydraulicRadius(cell, j);
					else
						radius = (computeEffectiveRadius(cell, j) + computeEquivalentRadius(cell, j)) * 0.5;
					if (radius < 0)
						NEG++;
					else
						POS++;
					if (radius == 0) {
						cout << "INS-INS PROBLEM!!!!!!!" << endl;
					}
					Real fluidArea = 0;
					if (distance != 0) {
						if (minPermLength > 0 && distanceCorrection)
							distance = std::max(minPermLength * radius, distance);
						const CVector& Surfk         = cell->info().facetSurfaces[j];
						Real           area          = sqrt(Surfk.squared_length());
						const CVector& crossSections = cell->info().facetSphereCrossSections[j];
						Real           S0            = 0;
						S0                           = checkSphereFacetOverlap(v0, v1, v2);
						if (S0 == 0)
							S0 = checkSphereFacetOverlap(v1, v2, v0);
						if (S0 == 0)
							S0 = checkSphereFacetOverlap(v2, v0, v1);
						//take absolute value, since in rare cases the surface can be negative (overlaping spheres)
						fluidArea = std::abs(area - crossSections[0] - crossSections[1] - crossSections[2] + S0);
						cell->info().facetFluidSurfacesRatio[j] = fluidArea / area;
						// kFactor<0 means we replace Poiseuille by Darcy localy, yielding a particle size-independent bulk conductivity
						if (kFactor > 0)
							cell->info().kNorm()[j] = kFactor * (fluidArea * pow(radius, 2)) / (8 * viscosity * distance);
						else if (partialSatEngine) {
							Real avgSat = (cell->info().sat() + neighbourCell->info().sat()) / 2.;
							//cout << "avgSat" << avgSat << endl;
							if (avgSat < SrM)
								avgSat = SrM;
							Real SeM = (avgSat - SrM) / (SsM - SrM);
							if (SeM < SrM)
								SeM = SrM;
							Real avgPoro     = (cell->info().porosity + neighbourCell->info().porosity) / 2.;
							Real avgPoroOrig = (cell->info().initialPorosity + neighbourCell->info().initialPorosity) / 2.;
							if (useKozeny) { // Kozeny 1927
								Real OneMinusPoroSqed   = pow(1. - avgPoro, 2);
								cell->info().kNorm()[j] = -kFactor * (pow(avgPoro, 3) / OneMinusPoroSqed)
								        * (OneMinusPoroSqed / pow(avgPoroOrig, 3));
							} else {
								if (SeM < 0)
									cerr << "negative equivalent saturation, linear system will be unstable" << endl;
								Real kM = -kFactor
								        * exp(bIntrinsicPerm
								              * (avgPoro
								                 - meanInitialPorosity)); //avgPoroOrig)); consider making all perm relative to the mean instaed of the initial...
								Real perm               = kM * pow(SeM, nUnsatPerm) * area / distance;
								cell->info().kNorm()[j] = (permClamp > 0 and perm > permClamp) ? permClamp : perm;
								//cout << "id " << cell->info().id <<  " perm " << cell->info().kNorm()[j] << " SeM " << SeM << " kM " << kM << " avgPoro " << avgPoro << endl;
							}
						} else
							cell->info().kNorm()[j] = -kFactor * area / distance;

						meanDistance += distance;
						meanRadius += radius;
						if (!cell->info().crack)
							meanK += (cell->info().kNorm())[j];

						if (!neighbourCell->info().isGhost)
							(neighbourCell->info().kNorm())[Tri.mirror_index(cell, j)] = (cell->info().kNorm())[j];
						if (k < 0 && debugOut) {
							surfneg += 1;
							cout << "__ k<0 __" << k << " "
							     << " fluidArea " << fluidArea << " area " << area << " " << crossSections[0] << " "
							     << crossSections[1] << " " << crossSections[2] << " " << W[0]->info().id() << " "
							     << W[1]->info().id() << " " << W[2]->info().id() << " " << p1 << " " << p2 << " test " << endl;
						}
					} else {
						cout << "infinite K1!" << endl;
						cell->info().kNorm()[j] = manualCrackPerm;
						//k = manualCrackPerm; //infiniteK;

					} //Will be corrected in the next loop
					if (!neighbourCell->info().isGhost)
						(neighbourCell->info().kNorm())[Tri.mirror_index(cell, j)] = (cell->info().kNorm())[j];
				}
			}
			cell->info().isvisited = !ref;
		}
		if (debugOut)
			cout << "surfneg est " << surfneg << endl;
		meanK /= pass;
		averageK = meanK; //for use in DFN
		meanRadius /= pass;
		meanDistance /= pass;
		Real globalK;
		if (kFactor > 0)
			globalK = kFactor * meanDistance * vPoral
			        / (sSolidTot * 8. * viscosity); //An approximate value of macroscopic permeability, for clamping local values below
		else
			globalK = meanK;
		if (debugOut) {
			cout << "PassCompK = " << pass << endl;
			cout << "meanK = " << meanK << endl;
			cout << "globalK = " << globalK << endl;
			cout << "maxKdivKmean*globalK = " << maxKdivKmean * globalK << endl;
			cout << "minKdivKmean*globalK = " << minKdivKmean * globalK << endl;
			cout << "meanTubesRadius = " << meanRadius << endl;
			cout << "meanDistance = " << meanDistance << endl;
		}
		ref  = Tri.finite_cells_begin()->info().isvisited;
		pass = 0;

		if (clampKValues)
			for (VCellIterator cellIt = T[currentTes].cellHandles.begin(); cellIt != T[currentTes].cellHandles.end(); cellIt++) {
				CellHandle& cell = *cellIt;
				for (int j = 0; j < 4; j++) {
					neighbourCell = cell->neighbor(j);
					if (!Tri.is_infinite(neighbourCell) && neighbourCell->info().isvisited == ref) {
						pass++;
						(cell->info().kNorm())[j]
						        = std::max(minKdivKmean * globalK, std::min((cell->info().kNorm())[j], maxKdivKmean * globalK));
						(neighbourCell->info().kNorm())[Tri.mirror_index(cell, j)] = (cell->info().kNorm())[j];
					}
				}
			}
		if (debugOut)
			cout << "PassKcorrect = " << pass << endl;
		if (debugOut)
			cout << "POS = " << POS << " NEG = " << NEG << " pass = " << pass << endl;

		// A loop to compute the standard deviation of the local K distribution, and use it to include/exclude K values higher then (meanK +/- K_opt_factor*STDEV)
		if (meanKStat) {
			std::ofstream k_opt_file("k_stdev.txt", std::ios::out);
			ref  = Tri.finite_cells_begin()->info().isvisited;
			pass = 0;
			for (FiniteCellsIterator cell = Tri.finite_cells_begin(); cell != cellEnd; cell++) {
				for (int j = 0; j < 4; j++) {
					neighbourCell = cell->neighbor(j);
					if (!Tri.is_infinite(neighbourCell) && neighbourCell->info().isvisited == ref) {
						pass++;
						STDEV += pow(((cell->info().kNorm())[j] - meanK), 2);
					}
				}
				cell->info().isvisited = !ref;
			}
			STDEV = sqrt(STDEV / pass);
			if (debugOut)
				cout << "PassSTDEV = " << pass << endl << "STATISTIC K" << endl;
			Real k_min = 0, k_max = meanK + KOptFactor * STDEV;
			cout << "Kmoy = " << meanK << " Standard Deviation = " << STDEV << endl << "kmin = " << k_min << " kmax = " << k_max << endl;
			ref  = Tri.finite_cells_begin()->info().isvisited;
			pass = 0;
			for (FiniteCellsIterator cell = Tri.finite_cells_begin(); cell != cellEnd; cell++) {
				for (int j = 0; j < 4; j++) {
					neighbourCell = cell->neighbor(j);
					if (!Tri.is_infinite(neighbourCell) && neighbourCell->info().isvisited == ref) {
						pass += 1;
						if ((cell->info().kNorm())[j] > k_max) {
							(cell->info().kNorm())[j]                                  = k_max;
							(neighbourCell->info().kNorm())[Tri.mirror_index(cell, j)] = (cell->info().kNorm())[j];
						}
						k_opt_file << KOptFactor << " " << (cell->info().kNorm())[j] << endl;
					}
				}
				cell->info().isvisited = !ref;
			}
			if (debugOut)
				cout << "PassKopt = " << pass << endl;
		}
		if (debugOut) {
			FiniteVerticesIterator verticesEnd = Tri.finite_vertices_end();
			Real                   Vgrains     = 0;
			int                    grains      = 0;
			for (FiniteVerticesIterator vIt = Tri.finite_vertices_begin(); vIt != verticesEnd; vIt++) {
				if (!vIt->info().isFictious && !vIt->info().isGhost) {
					grains += 1;
					Vgrains += 1.33333333 * M_PI * pow(vIt->point().weight(), 1.5);
				}
			}
			cout << grains << "grains - "
			     << "vTotal = " << vTotal << " Vgrains = " << Vgrains << " vPoral1 = " << (vTotal - Vgrains) << endl;
			cout << "Vtotalissimo = " << Vtotalissimo / 2 << " VSolidTot = " << VSolidTot / 2 << " vPoral2 = " << vPoral / 2
			     << " sSolidTot = " << sSolidTot << endl
			     << endl;
			if (!rAverage)
				cout << "------Hydraulic Radius is used for permeability computation------" << endl << endl;
			else
				cout << "------Average Radius is used for permeability computation------" << endl << endl;
			cout << "-----computed_Permeability-----" << endl;
		}
	}

	template <class _Tesselation> Real PartialSatLinSolv<_Tesselation>::getCellSaturation(Real X, Real Y, Real Z)
	{
		if (noCache && T[!currentTes].Max_id() <= 0)
			return 0; //the engine never solved anything
		RTriangulation& Tri  = T[noCache ? (!currentTes) : currentTes].Triangulation();
		CellHandle      cell = Tri.locate(CGT::Sphere(X, Y, Z));
		return cell->info().sat();
	}

	template <class _Tesselation> std::vector<Real> PartialSatLinSolv<_Tesselation>::getCellVelocity(Real X, Real Y, Real Z)
	{
		//if (noCache && T[!currentTes].Max_id()<=0) return 0;//the engine never solved anything
		RTriangulation& Tri  = T[noCache ? (!currentTes) : currentTes].Triangulation();
		CellHandle      cell = Tri.locate(CGT::Sphere(X, Y, Z));
		//if ( cell->info().averageVelocity().squared_length() == 0 ) return 0;
		std::vector<Real> velocityVector { cell->info().averageVelocity()[0], cell->info().averageVelocity()[1], cell->info().averageVelocity()[2] };
		return velocityVector;
	}

	template <class _Tesselation> Real PartialSatLinSolv<_Tesselation>::getCellVolume(Real X, Real Y, Real Z)
	{
		if (noCache && T[!currentTes].Max_id() <= 0)
			return 0; //the engine never solved anything
		RTriangulation& Tri  = T[noCache ? (!currentTes) : currentTes].Triangulation();
		CellHandle      cell = Tri.locate(CGT::Sphere(X, Y, Z));
		return cell->info().volume(); //sqrt( cell->info().averageVelocity().squared_length());
	}

	template <class _Tesselation> Real PartialSatLinSolv<_Tesselation>::getCellPorosity(Real X, Real Y, Real Z)
	{
		if (noCache && T[!currentTes].Max_id() <= 0)
			return 0; //the engine never solved anything
		RTriangulation& Tri  = T[noCache ? (!currentTes) : currentTes].Triangulation();
		CellHandle      cell = Tri.locate(CGT::Sphere(X, Y, Z));
		return cell->info().porosity; //sqrt( cell->info().averageVelocity().squared_length());
	}

	template <class _Tesselation> bool PartialSatLinSolv<_Tesselation>::getCellCracked(Real X, Real Y, Real Z)
	{
		if (noCache && T[!currentTes].Max_id() <= 0)
			return 0; //the engine never solved anything
		RTriangulation& Tri  = T[noCache ? (!currentTes) : currentTes].Triangulation();
		CellHandle      cell = Tri.locate(CGT::Sphere(X, Y, Z));
		return cell->info().crack; //sqrt( cell->info().averageVelocity().squared_length());
	}

	template <class _Tesselation> Real PartialSatLinSolv<_Tesselation>::getAverageSaturation()
	{
		if (noCache && T[!currentTes].Max_id() <= 0)
			return 0; //the engine never solved anything
		//RTriangulation& Tri = T[noCache?(!currentTes):currentTes].Triangulation();
		Real saturationTotal = 0;
		int  numCells        = 0;
		for (VCellIterator cellIt = T[currentTes].cellHandles.begin(); cellIt != T[currentTes].cellHandles.end(); cellIt++) {
			CellHandle& cell = *cellIt;
			if (cell->info().Pcondition or cell->info().blocked)
				continue;
			saturationTotal += cell->info().sat();
			numCells += 1;
		}
		return saturationTotal / numCells; //sqrt( cell->info().averageVelocity().squared_length());
	}

	template <class _Tesselation> Real PartialSatLinSolv<_Tesselation>::getAverageSuction()
	{
		if (noCache && T[!currentTes].Max_id() <= 0)
			return 0; //the engine never solved anything
		//RTriangulation& Tri = T[noCache?(!currentTes):currentTes].Triangulation();
		Real suctionTotal = 0;
		int  numCells     = 0;
		for (VCellIterator cellIt = T[currentTes].cellHandles.begin(); cellIt != T[currentTes].cellHandles.end(); cellIt++) {
			CellHandle& cell = *cellIt;
			if (cell->info().Pcondition or cell->info().blocked)
				continue;
			suctionTotal += pAir - cell->info().p();
			numCells += 1;
		}
		return suctionTotal / numCells; //sqrt( cell->info().averageVelocity().squared_length());
	}


	// need to make sure the flux considers the saturation change between pores
	// template <class _Tesselation>
	// Real PartialSatLinSolv<_Tesselation>::partialSatBoundaryFlux(unsigned int boundaryId)
	// {
	// 	if (noCache && T[!currentTes].Max_id()<=0) return 0;
	// 	bool tes = noCache?(!currentTes):currentTes;
	// 	RTriangulation& Tri = T[tes].Triangulation();
	// 	Real Q1=0;
	//
	// 	VectorCell tmpCells;
	// 	tmpCells.resize(10000);
	// 	VCellIterator cells_it = tmpCells.begin();
	//
	// 	VCellIterator cell_up_end = Tri.incident_cells(T[tes].vertexHandles[boundaryId],cells_it);
	// 	for (VCellIterator it = tmpCells.begin(); it != cell_up_end; it++)
	// 	{
	// 		const CellHandle& cell = *it;
	// 		if (cell->info().isGhost) continue;
	// 		Q1 -= cell->info().dv();
	// 		for (int j2=0; j2<4; j2++)
	// 			Q1 += (cell->info().kNorm())[j2]*(cell->neighbor(j2)->info().shiftedP()-cell->info().shiftedP());
	// 	}
	// 	return Q1;
	// }

} //namespace CGT
} //namespace yade

#endif //FLOW_ENGINE
