/*************************************************************************
*  Copyright (C) 2014 by Bruno Chareyre <bruno.chareyre@grenoble-inp.fr>     *
*  Copyright (C) 2013 by Thomas. Sweijen <T.sweijen@uu.nl>               *
*  Copyright (C) 2012 by Chao Yuan <chao.yuan@3sr-grenoble.fr>           *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifndef FLOW_GUARD
#define FLOW_GUARD

// This is an example of how to derive a new FlowEngine with additional data and possibly completely new behaviour.
// Every functions of the base engine can be overloaded, and new functions can be added

//keep this #ifdef as long as you don't really want to realize a final version publicly, it will save compilation time for everyone else
//when you want it compiled, you can pass -DTWOPHASEFLOW to cmake, or just uncomment the following line
// #define TWOPHASEFLOW
#ifdef TWOPHASEFLOW
#include "FlowEngine_TwoPhaseFlowEngineT.hpp"
#include <Eigen/SparseLU>

#ifdef LINSOLV
#include <cholmod.h>
#endif

namespace yade { // Cannot have #include directive inside.

/// We can add data to the Info types by inheritance
class TwoPhaseCellInfo : public FlowCellInfo_TwoPhaseFlowEngineT {
public:
	bool isWRes; //Flags for marking cell(pore unit) state: isWettingReservoirCell, isNonWettingReservoirCell, isTrappedWettingCell, isTrappedNonWettingCell
	bool isNWRes;
	bool isTrapW;
	bool isTrapNW;
	Real saturation;   //the saturation of single pore (will be used in quasi-static imbibition and dynamic flow)
	Real trapCapP;     //for calculating the pressure of trapped pore, cell->info().p() = pressureNW- trapCapP. OR cell->info().p() = pressureW + trapCapP
	bool hasInterface; //Indicated whether a NW-W interface is present within the pore body
	std::vector<Real> poreThroatRadius;
	Real              poreBodyRadius;
	Real              poreBodyVolume;
	Real              porosity;
	int               windowsID; //a temp cell info for experiment comparison(used by chao)
	Real              solidLine
	        [4]
	        [4]; //the length of intersecting line between sphere and facet. [i][j] is for facet "i" and sphere (facetVertices)"[i][j]". Last component [i][3] for 1/sumLines in the facet "i" (used by chao).

	//DynamicTwoPhaseFlow
	std::vector<Real> entryPressure;
	std::vector<Real> entrySaturation;
	std::vector<int>  poreNeighbors;
	std::vector<int>  poreIdConnectivity;
	std::vector<Real> listOfkNorm;
	std::vector<Real> listOfkNorm2;
	std::vector<Real> listOfEntrySaturation;
	std::vector<Real> listOfEntryPressure;
	std::vector<Real> kNorm2;
	std::vector<Real> listOfThroatArea;
	std::vector<Real> particleSurfaceArea; //Surface area of four particles enclosing one grain-based tetrahedra
	Real              accumulativeDVSwelling;
	Real              saturation2;
	int               numberFacets;
	int               isFictiousId;
	Real              mergedVolume;
	int               mergednr;
	unsigned int      mergedID;
	Real              thresholdSaturation;
	Real              flux;
	Real              accumulativeDV;
	Real              airWaterArea;
	bool              isWResInternal;
	Real              conductivityWRes;
	Real              minSaturation;
	int               poreId;
	bool              airBC;
	bool              waterBC;
	Real              thresholdPressure;
	Real              apparentSolidVolume;
	Real              dvSwelling;
	Real              dvTPF;
	bool              isNWResDef;
	int               invadedFrom;
	int               label; //for marking disconnected clusters. NW-res: 0; W-res: 1; W-clusters by 2,3,4...

	TwoPhaseCellInfo(void)
	{
		saturation2  = 0.0;
		isFictiousId = 0;
		isWRes       = true;
		isNWRes      = false;
		isTrapW      = false;
		isTrapNW     = false;
		saturation   = 1.0;
		hasInterface = false;
		trapCapP     = 0;
		poreThroatRadius.resize(4, 0);
		kNorm2.resize(4, 0);
		poreBodyRadius = 0;
		poreBodyVolume = 0;
		windowsID      = 0;
		for (int k = 0; k < 4; k++)
			for (int l = 0; l < 4; l++)
				solidLine[k][l] = 0;

		//dynamic TwoPhaseFlow
		airBC               = false;
		waterBC             = false;
		numberFacets        = 4;
		mergedVolume        = 0;
		mergednr            = 0;
		mergedID            = 0;
		apparentSolidVolume = 0.0;
		dvSwelling          = 0.0;
		entryPressure.resize(4, 0);
		entrySaturation.resize(4, 0);
		poreIdConnectivity.resize(4, -1);
		particleSurfaceArea.resize(4, 0);
		thresholdSaturation = 0.0;
		flux                = 0.0; //NOTE can potentially be removed, currently not used but might be handy in future work
		accumulativeDV      = 0.0;
		thresholdPressure   = 0.0;
		airWaterArea        = 0.0;
		accumulativeDV      = 0.0;
		minSaturation       = 0.0;
		poreId              = -1;
		isWResInternal      = false;
		dvTPF               = 0.0; //FIXME dvTPF is currently only used to impose pressure as dv() cannot be imposed from FlowEngine currently
		isNWResDef          = false;
		conductivityWRes    = 0.0;
		invadedFrom         = 0;
		label               = -1;
		porosity            = 0.;
	}
};

class TwoPhaseVertexInfo : public FlowVertexInfo_TwoPhaseFlowEngineT {
public:
	//same here if needed
};

typedef TemplateFlowEngine_TwoPhaseFlowEngineT<TwoPhaseCellInfo, TwoPhaseVertexInfo> TwoPhaseFlowEngineT;
REGISTER_SERIALIZABLE(TwoPhaseFlowEngineT);

// A class to represent isolated single-phase cluster (main application in convective drying at the moment)
class PhaseCluster : public Serializable {
	Real totalCellVolume;
	bool factorized;
	// 		CellHandle entryPoreHandle;

public:
	typedef TwoPhaseFlowEngineT::CellHandle                        CellHandle;
	typedef TwoPhaseFlowEngineT::Tesselation                       Tesselation;
	typedef std::pair<std::pair<unsigned int, unsigned int>, Real> Interface_t;
	struct Interface : Interface_t {
		unsigned   outerIndex;   // local index to outer cell from inner cell (from 0 to 3)
		Real       volume;       // for tracking movements of the interface
		Real       capillaryP;   // local capillary pressure
		Real       conductivity; //TODO: to be used instead of default one if not zero
		CellHandle innerCell;
		Interface(Interface_t i)
		        : Interface_t(i)
		        , outerIndex(100)
		        , volume(0)
		        , capillaryP(0)
		        , conductivity(0) {};
	};
	virtual ~PhaseCluster();
	PhaseCluster(Tesselation& t)
	        : PhaseCluster()
	{
		tes = &t;
		LC  = NULL;
		ex  = NULL;
		if (not tes)
			LOG_WARN("invalid initialization");
	}
	// 		PhaseCluster () {tes=NULL; LOG_WARN("avoid default constructor, 'tes' not initialized");}
	Tesselation*       tes; //point back to the full data structure
	vector<CellHandle> pores;
	vector<Interface>  interfaces;
	// 		TwoPhaseFlowEngineT::RTriangulation* tri;
#ifdef LINSOLV
	cholmod_common  comC;
	cholmod_factor* LC;
	cholmod_dense*  ex; //the pressure field
	cholmod_common* pComC;
	// 		cholmod_dense** pEx = &ex;
	// 		cholmod_l_start(&comC);
	void solvePressure();
	void resetSolver()
	{
		if (LC)
			cholmod_l_free_factor(&LC, &comC);
		if (ex)
			cholmod_l_free_dense(&ex, &comC);
		factorized = false;
	}
#endif

	void reset()
	{
		label = entryPore = -1;
		volume = entryRadius = interfacialArea = 0;
		pores.clear();
		interfaces.clear();
		resetSolver();
	}

	//merge another cluster into the current one, interfaces are duplicated selectively
	//start is the cell leading to connectivity (preconditon: there's only one connecting cell)
	void mergeCluster(PhaseCluster& otherCluster, const CellHandle& start)
	{
		this->resetSolver();
		for (auto p = otherCluster.pores.begin(); p != otherCluster.pores.end(); p++)
			(*p)->info().label = label;
		// 			for (auto i=otherCluster.interfaces.begin();i!=otherCluster.interfaces.end();i++) i->first.firstlabel=label;
		pores.insert(pores.end(), otherCluster.pores.begin(), otherCluster.pores.end());
		for (auto itf = otherCluster.interfaces.begin(); itf != otherCluster.interfaces.end(); itf++)
			if (itf->first.second != start->info().id)
				interfaces.push_back(*itf);
		otherCluster.reset();
	}

	vector<int> getPores()
	{
		vector<int> res;
		for (vector<CellHandle>::iterator it = pores.begin(); it != pores.end(); it++)
			res.push_back((*it)->info().id);
		return res;
	}

	boost::python::list getInterfaces(int cellId = -1)
	{
		boost::python::list ints;
		for (vector<Interface>::iterator it = interfaces.begin(); it != interfaces.end(); it++)
			if (cellId == -1 or unsigned(cellId) == it->first.first)
				ints.append(boost::python::make_tuple(it->first.first, it->first.second, it->second, it - interfaces.begin()));
		return ints;
	}
	Real getFlux(unsigned nf)
	{
		const CellHandle& innerCell = interfaces[nf].innerCell;
		return innerCell->info().kNorm()[interfaces[nf].outerIndex]
		        * (innerCell->info().p() + interfaces[nf].capillaryP - innerCell->neighbor(interfaces[nf].outerIndex)->info().p());
	}
	Real getCapVol(unsigned nf) { return interfaces[nf].volume; }
	Real getConductivity(unsigned nf)
	{
		const CellHandle& innerCell = interfaces[nf].innerCell;
		return innerCell->info().kNorm()[interfaces[nf].outerIndex];
	}

	void setCapPressure(unsigned nf, Real pcap) { interfaces[nf].capillaryP = pcap; }
	Real getCapPressure(unsigned nf) { return interfaces[nf].capillaryP; }
	void setCapVol(unsigned nf, Real vcap) { interfaces[nf].volume = vcap; }
	Real updateCapVol(unsigned nf, Real dt)
	{
		interfaces[nf].volume += dt * getFlux(nf); /*LOG_WARN(interfaces[nf].volume);*/
		return interfaces[nf].volume;
	}
	void updateCapVolList(Real dt)
	{
		for (unsigned it = 0; it < interfaces.size(); ++it)
			interfaces[it].volume += dt * getFlux(it);
	}

	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(PhaseCluster,Serializable,"Preliminary.",
		((int,label,-1,,"Unique label of this cluster, should be reflected in pores of this cluster."))
		((Real,volume,0,,"cumulated volume of all pores."))
		((Real,entryRadius,0,,"smallest entry capillary pressure."))
		((int,entryPore,-1,,"the pore of the cluster incident to the throat with smallest entry Pc."))
		((Real,interfacialArea,0,,"interfacial area of the cluster"))
		,((LC,NULL))((ex,NULL))((pComC,&comC)),
		#ifdef LINSOLV
		cholmod_l_start(pComC);//initialize cholmod solver
		factorized=false;
		#endif
		,
		.def("getPores",&PhaseCluster::getPores,"get the list of pores by index")
		.def("getInterfaces",&PhaseCluster::getInterfaces,(boost::python::arg("CellId")=-1),"get the list of interfacial pore-throats associated to a cluster, listed as [id1,id2,area,index] where id2 is the neighbor pore outside the cluster and index is the position in the global cluster's list of interfaces. If CellId>=0 only the interfaces adjacent to the corresponding inner cell are returned.")
		.def("getFlux",&PhaseCluster::getFlux,(boost::python::arg("interface")),"get flux at an interface (i.e. velocity of the menicus), the index to be used is the rank of the interface in the same order as in getInterfaces().")
		.def("setCapPressure",&PhaseCluster::setCapPressure,(boost::python::arg("numf"),boost::python::arg("pCap")),"set local capillary pressure")
		.def("getCapPressure",&PhaseCluster::getCapPressure,(boost::python::arg("numf")),"get local capillary pressure")
		.def("setCapVol",&PhaseCluster::setCapVol,(boost::python::arg("numf"),boost::python::arg("vCap")),"set position of the meniscus - in terms of volume")
		.def("getCapVol",&PhaseCluster::getCapVol,(boost::python::arg("numf")),"get position of the meniscus - in terms of volume")
		.def("getConductivity",&PhaseCluster::getConductivity,(boost::python::arg("numf")),"get conductivity")
		.def("updateCapVol",&PhaseCluster::updateCapVol,(boost::python::arg("numf"),boost::python::arg("dt")),"increments throat's volume of given interface by flux*dt")
		.def("updateCapVolList",&PhaseCluster::updateCapVolList,(boost::python::arg("dt")),"increments throat's volume of all interfaces by flux*dt")
		.def("solvePressure",&PhaseCluster::solvePressure,"Solve 1-phase flow in one single cluster defined by its id.")
		)
	// clang-format on

	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(PhaseCluster);

class TwoPhaseFlowEngine : public TwoPhaseFlowEngineT {
public:
	Real                             airBoundaryPressure = 0.0;
	std::vector<CellHandle>          listOfPores;
	bool                             imposeDeformationFluxTPFSwitch = false;
	Real                             totalCellVolume;
	vector<shared_ptr<PhaseCluster>> clusters; // the list of clusters

	//We can overload every functions of the base engine to make it behave differently
	//if we overload action() like this, this engine is doing nothing in a standard timestep, it can still have useful functions
	virtual void action() {};

	//If a new function is specific to the derived engine, put it here, else go to the base TemplateFlowEngine if it is useful for everyone
	void computePoreBodyVolume();
	void computePoreBodyRadius();
	void computeSolidLine();
	void savePhaseVtk(const char* folder, bool withBoundaries);

	//compute entry pore throat radius (drainage)
	void computePoreThroatRadiusMethod1();       //MS-P method
	void computePoreThroatRadiusTrickyMethod1(); //set the radius of pore throat between side pores negative.
	Real computeEffPoreThroatRadius(CellHandle cell, int j);
	Real computeEffPoreThroatRadiusFine(CellHandle cell, int j);
	Real computeMSPRcByPosRadius(const Vector3r& posA, const Real& rA, const Vector3r& posB, const Real& rB, const Vector3r& posC, const Real& rC);
	Real computeTriRadian(Real a, Real b, Real c);
	Real computeEffRcByPosRadius(const Vector3r& posA, const Real& rA, const Vector3r& posB, const Real& rB, const Vector3r& posC, const Real& rC)
	{
		Real reff = solver->computeEffectiveRadiusByPosRadius(makeCgPoint(posA), rA, makeCgPoint(posB), rB, makeCgPoint(posC), rC);
		return reff < 0 ? 1.0e-10 : reff;
	};
	Real bisection(const Vector3r& posA, const Real& rA, const Vector3r& posB, const Real& rB, const Vector3r& posC, const Real& rC, Real a, Real b);
	Real computeDeltaForce(const Vector3r& posA, const Real& rA, const Vector3r& posB, const Real& rB, const Vector3r& posC, const Real& rC, Real r);

	void computePoreThroatRadiusMethod2(); //radius of the inscribed circle
	void computePoreThroatRadiusMethod3(); //radius of area-equivalent circle

	///begin of invasion (mainly drainage) model
	void initialization();
	void initializeReservoirs();

	void invasion(); //functions can be shared by two modes
	void invasionSingleCell(CellHandle cell);
	void updatePressure();
	Real getMinDrainagePc();
	Real getMaxImbibitionPc();
	Real getSaturation(bool isSideBoundaryIncluded = false);

	void invasion1(); //with-trap
	void updateReservoirs1();
	void WResRecursion(CellHandle cell);
	void NWResRecursion(CellHandle cell);
	void checkTrap(Real pressure);
	void updateReservoirLabel();
	void invasion2(); //without-trap
	void updateReservoirs2();
	///end of invasion model

	//## Clusters ##
	//FIXME: clusters have a pointers to solver->tesselation(), they won't be deserialized correctly, probably needs a post_load() and/or something in TPF.initialization()
	void updateCellLabel();
	void updateSingleCellLabelRecursion(CellHandle cell, PhaseCluster* cluster);
	void clusterGetFacet(PhaseCluster* cluster, CellHandle cell, int facet); //update cluster inetrfacial area and max entry radius wrt to a facet
	void clusterGetPore(PhaseCluster* cluster, CellHandle cell);             //simple add pore to cluster, updating flags and cluster volume
	vector<int>
	clusterInvadePore(PhaseCluster* cluster, CellHandle cell); //remove pore from cluster, if it splits the cluster in many pieces introduce new one(s)
	vector<int> clusterInvadePoreFast(
	        PhaseCluster* cluster,
	        CellHandle
	                cell); //nearly the same as above but faster and using splitCLuster() internally, warning: entry pcap is NOT updated since this function is supposed to be used for viscous invasion, OTOH interfaces are updated
	vector<int> clusterOutvadePore(
	        unsigned startingId,
	        unsigned imbibedId,
	        int      index); //imbibe adjacent pore, including merge clusters if necessary. Returns ids of merged clusters. Giving index of the facet in cluster::interfaces should speedup its removal
	// 	vector<int> clusterAddPore(PhaseCluster* cluster, CellHandle cell);// add a pore to a cluster, merge existing clusters if needed and return the merged ones
	vector<int> splitCluster(
	        PhaseCluster* cluster,
	        CellHandle    cellInit); //an attempt to optimize cluster splitting by skipping some simple cases before triggering a long recursive process
	unsigned    markRecursively(const CellHandle& cell, int newLabel); // mark and count accessible cells with same label as 'cell' and connected to it
	vector<int> pyClusterInvadePore(int cellId)
	{
		int label = solver->T[solver->currentTes].cellHandles[cellId]->info().label;
		if (label < 1) {
			LOG_WARN("the pore is not in a cluster, label=" << label);
			return vector<int>();
		}
		return clusterInvadePore(clusters[label].get(), solver->tesselation().cellHandles[cellId]);
	}
	vector<int> pyClusterInvadePoreFast(int cellId)
	{
		int label = solver->T[solver->currentTes].cellHandles[cellId]->info().label;
		if (label < 1) {
			LOG_WARN("the pore is not in a cluster, label=" << label);
			return vector<int>();
		}
		return clusterInvadePoreFast(clusters[label].get(), solver->T[solver->currentTes].cellHandles[cellId]);
	}
	boost::python::list pyClusters();
	bool                connectedAroundEdge(const RTriangulation& Tri, CellHandle& cell, unsigned facet1, unsigned facet2);
	// 	int getMaxCellLabel();

	//compute forces
	void computeFacetPoreForcesWithCache(bool onlyCache = false);
	void computeCapillaryForce(bool addForces, bool permanently)
	{
		computeFacetPoreForcesWithCache(false);
		if (addForces) {
			for (FiniteVerticesIterator v = solver->tesselation().Triangulation().finite_vertices_begin();
			     v != solver->tesselation().Triangulation().finite_vertices_end();
			     ++v) {
				if (permanently)
					scene->forces.setPermForce(v->info().id(), makeVector3r(v->info().forces));
				else
					scene->forces.addForce(v->info().id(), makeVector3r(v->info().forces));
			}
		}
	}

	//combine with pendular model
	boost::python::list getPotentialPendularSpheresPair()
	{
		RTriangulation&     Tri = solver->T[solver->currentTes].Triangulation();
		boost::python::list bridgeIds;
		FiniteEdgesIterator ed_it = Tri.finite_edges_begin();
		for (; ed_it != Tri.finite_edges_end(); ed_it++) {
			if (detectBridge(ed_it) == true) {
				const VertexInfo& vi1 = (ed_it->first)->vertex(ed_it->second)->info();
				const VertexInfo& vi2 = (ed_it->first)->vertex(ed_it->third)->info();
				const int&        id1 = vi1.id();
				const int&        id2 = vi2.id();
				bridgeIds.append(boost::python::make_tuple(id1, id2));
			}
		}
		return bridgeIds;
	}
	bool detectBridge(RTriangulation::Finite_edges_iterator& edge);

	//Library TwoPhaseFlow
	Real getKappa(int numberFacets);
	Real getChi(int numberFacets);
	Real getLambda(int numberFacets);
	Real getN(int numberFacets);
	Real getDihedralAngle(int numberFacets);

	//Merging Library
	void mergeCells();
	void countFacets();
	void computeMergedVolumes();
	void getMergedCellStats();
	void calculateResidualSaturation();
	void adjustUnresolvedPoreThroatsAfterMerging();
	void actionMergingAlgorithm();
	void checkVolumeConservationAfterMergingAlgorithm();

	//Dynamic Engine
	void actionTPF();
	void solvePressure();
	void setBoundaryConditions();
	void setInitialConditions();
	void setPoreNetwork();
	void setListOfPores();
	void getQuantities();
	Real porePressureFromPcS(CellHandle cell, Real saturation);
	Real getSolidVolumeInCell(CellHandle cell);

	Real getConstantC4(CellHandle cell);
	Real getConstantC3(CellHandle cell);
	Real dsdp(CellHandle cell, Real pw);
	Real poreSaturationFromPcS(CellHandle cell, Real pw);

	void reTriangulate();
	void readTriangulation();
	void initializationTriangulation();
	void assignWaterVolumesTriangulation();
	void equalizeSaturationOverMergedCells();
	void updatePoreUnitProperties();
	void transferConditions();
	void imposeDeformationFluxTPF();
	void updateDeformationFluxTPF();
	void copyPoreDataToCells();
	void verifyCompatibilityBC();
	void makeListOfPoresInCells(bool fast);


	std::vector<Real>                      leftOverVolumePerSphere;
	std::vector<Real>                      untreatedAreaPerSphere;
	std::vector<unsigned int>              finishedUpdating;
	std::vector<Real>                      waterVolume;
	std::vector<std::vector<unsigned int>> tetrahedra;
	std::vector<std::vector<Real>>         solidFractionSpPerTet;
	std::vector<Real>                      deltaVoidVolume;
	std::vector<Real>                      leftOverDVPerSphere;
	std::vector<Real>                      saturationList;
	std::vector<bool>                      hasInterfaceList;
	std::vector<Real>                      listOfFlux;
	std::vector<Real>                      listOfMergedVolume;


	Eigen::SparseMatrix<Real>                                                               aMatrix;
	typedef Eigen::Triplet<Real>                                                            ETriplet;
	std::vector<ETriplet>                                                                   tripletList;
	Eigen::SparseLU<Eigen::SparseMatrix<Real, Eigen::ColMajor>, Eigen::COLAMDOrdering<int>> eSolver;

	int getCell2(Real posX, Real posY, Real posZ)
	{ //Should be fixed properly
		RTriangulation& tri  = solver->T[solver->currentTes].Triangulation();
		CellHandle      cell = tri.locate(CGT::Sphere(posX, posY, posZ));
		return cell->info().id;
	}

	boost::python::list cellporeThroatConductivity(unsigned int id)
	{ // Temporary function to allow for simulations in Python, can be easily accessed in c++
		boost::python::list ids;
		if (id >= solver->T[solver->currentTes].cellHandles.size()) {
			LOG_ERROR("id out of range, max value is " << solver->T[solver->currentTes].cellHandles.size());
			return ids;
		}
		for (unsigned int i = 0; i < 4; i++)
			ids.append(solver->T[solver->currentTes].cellHandles[id]->info().kNorm()[i]);
		return ids;
	}

	boost::python::list solidSurfaceAreaPerParticle(unsigned int id)
	{ // Temporary function to allow for simulations in Python, can be easily accessed in c++
		boost::python::list ids;
		if (id >= solver->T[solver->currentTes].cellHandles.size()) {
			LOG_ERROR("id out of range, max value is " << solver->T[solver->currentTes].cellHandles.size());
			return ids;
		}
		for (unsigned int i = 0; i < 4; i++)
			ids.append(solver->T[solver->currentTes].cellHandles[id]->info().particleSurfaceArea[i]);
		return ids;
	}

	//post-processing
	void savePoreNetwork(const char* folder);
	// 	void saveVtk(const char* folder, bool withBoundaries) {bool initT=solver->noCache; solver->noCache=false; solver->saveVtk(folder,withBoundaries); solver->noCache=initT;}

	boost::python::list cellporeThroatRadius(unsigned int id)
	{
		boost::python::list ids;
		if (id >= solver->T[solver->currentTes].cellHandles.size()) {
			LOG_ERROR("id out of range, max value is " << solver->T[solver->currentTes].cellHandles.size());
			return ids;
		}
		for (unsigned int i = 0; i < 4; i++)
			ids.append(solver->T[solver->currentTes].cellHandles[id]->info().poreThroatRadius[i]);
		return ids;
	}

	boost::python::list getNeighbors(unsigned int id, bool withInfCell)
	{
		boost::python::list   ids;
		const RTriangulation& Tri = solver->tesselation().Triangulation();
		if (id >= solver->tesselation().cellHandles.size()) {
			LOG_ERROR("id out of range, max value is " << solver->T[solver->currentTes].cellHandles.size());
			return ids;
		}
		for (unsigned int i = 0; i < 4; i++) {
			const CellHandle& neighbourCell = solver->tesselation().cellHandles[id]->neighbor(i);
			if (withInfCell == true)
				ids.append(neighbourCell->info().id);
			else if (!Tri.is_infinite(neighbourCell))
				ids.append(neighbourCell->info().id);
		}
		return ids;
	}

	//TODO
	//Dynamic code
	boost::python::list cellEntrySaturation(unsigned int id)
	{ // Temporary function to allow for simulations in Python, can be easily accessed in c++
		boost::python::list ids;
		if (id >= solver->T[solver->currentTes].cellHandles.size()) {
			LOG_ERROR("id out of range, max value is " << solver->T[solver->currentTes].cellHandles.size());
			return ids;
		}
		for (unsigned int i = 0; i < 4; i++)
			ids.append(solver->T[solver->currentTes].cellHandles[id]->info().entrySaturation[i]);
		return ids;
	}

	//FIXME, needs to trigger initSolver() Somewhere, else changing flow.debug or other similar things after first calculation has no effect
	//FIXME, I removed indexing cells from inside UnsatEngine (SoluteEngine shouldl be ok (?)) in order to get pressure computed, problem is they are not indexed at all if flow is not calculated

	void computeOnePhaseFlow()
	{
		scene = Omega::instance().getScene().get();
		if (!solver)
			cerr << "no solver!" << endl;
		solver->gaussSeidel(scene->dt);
		initSolver(*solver);
	}
	///manipulate/get/set on pore geometry
	bool isCellNeighbor(unsigned int cell1, unsigned int cell2);
	void setPoreThroatRadius(unsigned int cell1, unsigned int cell2, Real radius);
	Real getPoreThroatRadius(unsigned int cell1, unsigned int cell2);

	CELL_SCALAR_GETTER(bool, .isWRes, cellIsWRes)
	CELL_SCALAR_GETTER(bool, .isNWRes, cellIsNWRes)
	CELL_SCALAR_GETTER(bool, .isTrapW, cellIsTrapW)
	CELL_SCALAR_GETTER(bool, .isTrapNW, cellIsTrapNW)
	CELL_SCALAR_SETTER(bool, .isNWRes, setCellIsNWRes)
	CELL_SCALAR_SETTER(bool, .isWRes, setCellIsWRes)
	CELL_SCALAR_GETTER(Real, .saturation, cellSaturation)
	CELL_SCALAR_SETTER(Real, .saturation, setCellSaturation)
	CELL_SCALAR_GETTER(bool, .isFictious, cellIsFictious)         //Temporary function to allow for simulations in Python
	CELL_SCALAR_GETTER(bool, .hasInterface, cellHasInterface)     //Temporary function to allow for simulations in Python
	CELL_SCALAR_GETTER(Real, .poreBodyRadius, cellInSphereRadius) //Temporary function to allow for simulations in Python
	CELL_SCALAR_SETTER(Real, .poreBodyRadius, setPoreBodyRadius)  //Temporary function to allow for simulations in Python, for lbm coupling.
	CELL_SCALAR_GETTER(Real, .poreBodyVolume, cellVoidVolume)     //Temporary function to allow for simulations in Python

	CELL_SCALAR_GETTER(Real, .mergedVolume, cellMergedVolume) //Temporary function to allow for simulations in Python
	CELL_SCALAR_SETTER(Real, .dvTPF, setCellDV)               //Temporary function to allow for simulations in Python
	CELL_SCALAR_GETTER(Real, .porosity, cellPorosity)
	CELL_SCALAR_SETTER(bool, .hasInterface, setCellHasInterface) //Temporary function to allow for simulations in Python
	CELL_SCALAR_GETTER(int, .label, cellLabel)
	CELL_SCALAR_GETTER(Real, .volume(), cellVolume) //Temporary function to allow for simulations in Python


	//Dynamic Code
	CELL_SCALAR_GETTER(Real, .thresholdSaturation, cellThresholdSaturation) //Temporary function to allow for simulations in Python
	CELL_SCALAR_GETTER(Real, .mergedID, cellMergedID)                       //Temporary function to allow for simulations in Python


	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(TwoPhaseFlowEngine,TwoPhaseFlowEngineT,"documentation here",
	((Real,surfaceTension,0.0728,,"Water Surface Tension in contact with air at 20 Degrees Celsius is: 0.0728(N/m)"))
	((bool,recursiveInvasion,true,,"If true the invasion stops only when no entry pc is less than current capillary pressure, implying simultaneous invasion of many pores. Else only one pore invasion per invasion step."))
	((bool,initialWetting,true,,"Initial wetting saturated (=true) or non-wetting saturated (=false)"))
	((bool, isPhaseTrapped,true,,"If True, both phases can be entrapped by the other, which would correspond to snap-off. If false, both phases are always connected to their reservoirs, thus no snap-off."))
	((bool, isInvadeBoundary, true,,"Invasion side boundary condition. If True, pores of side boundary can be invaded; if False, the pore throats connecting side boundary are closed, those pores are excluded in saturation calculation."))	
	((bool, drainageFirst, true,,"If true, activate drainage first (initial saturated), then imbibition; if false, activate imbibition first (initial unsaturated), then drainage."))
	((Real,dtDynTPF,0.0,,"Parameter which stores the smallest time step, based on the residence time"))
	((int,entryPressureMethod,1,,"integer to define the method used to determine the pore throat radii and the according entry pressures. 1)radius of entry pore throat based on MS-P method; 2) radius of the inscribed circle; 3) radius of the circle with equivalent surface area of the pore throat."))
// 	((Real,partiallySaturatedPores,false,,"Include partially saturated pores or not?"))

	//BEGIN Latest dynamic/pore merging things (to clean maybe)
	((Real,entryMethodCorrection,float(entryPressureMethod),,"Parameter that is used in computing entry pressure of a pore throat: P_ij =  entryMethodCorrection * surfaceTension / radius_porethroat "))
	//Dynamic TwoPhaseFlow
	((vector<bool>, bndCondIsWaterReservoir, vector<bool>(6,false),,"Boundary conditions, if bndCondIsPressure[] = True, is it air or water boundary condition? True is water reservoir"))
	((unsigned int,maxIDMergedCells,0,,"maximum number of merged ID, this is computed in mergeCells()"))
	((Real,waterPressurePartiallySatPores,0.0,,"water pressure based on the volume-averaged water pressure in partially-saturated pore units (i.e. pore units having an interface)"))
	((Real,waterPressure,0.0,,"Volume-averaged water pressure"))
	((Real,waterSaturation,0.0,,"Water saturation, excluding the boundary cells"))
	((Real,voidVolume,0.0,,"total void volume, excluding boundary cells"))
  	((bool,stopSimulation, false,,"Boolean to indicate that dynamic flow simulations cannot find a solution (or next time step). If True, stop simulations"))
  	((bool,debugTPF, false,,"Print debuging messages two phase flow engine "))
	((Real,airWaterInterfacialArea,0.0,,"Air-water interfacial area, based on the pore-unit assembly and regular-shaped pore units"))
	((Real,areaAveragedPressure,0.0,,"Air-water interfacial area averaged water pressure "))
	((Real,maximumRatioPoreThroatoverPoreBody,0.90,,"maximum ratio of pore throat radius over pore body radius, this is used during merging of tetrahedra."))
	((Real,totalWaterVolume,0.0,,"total watervolume"))
	((string,modelRunName,"dynamicDrainage",,"Name of simulation, to be implemented into output files"))
	((Real,safetyFactorTimeStep,1.0,,"Safey coefficient for time step"))
	((Real,fluxInViaWBC,0.0,,"Total water flux over water boundary conditions"))
	((Real, accumulativeFlux,0.0,,"accumulative influx of water"))
	((Real, truncationPrecision,1e-6,,"threshold at which a saturation is truncated"))
	((unsigned int, numberOfPores, 0,,"Number of pores (i.e. number of tetrahedra, but compensated for merged tetrahedra"))
	((bool, firstDynTPF, true,,"this bool activated the initialization of the dynamic flow engine, such as merging and defining initial values"))
	((bool, keepTriangulation, false,,"this bool activated triangulation or not during initialization"))
 	((bool, remesh, false,,"update triangulation? -- YET TO BE IMPLEMENTED"))                         //FIXME - trinagulation of unsaturated pore units still to be implemented properly
	((bool, deformation, false,,"Boolean to indicate whether simulations of dynamic flow are withing a deformating packing or not. If true, change of void volume due to deformation is considered in flow computations."))
	((int, iterationTPF, -1,,"Iteration number"))
	((Real, initialPC, 2000.0,,"Initial capillary pressure of the water-air inside the packing"))
	((Real, accumulativeDeformationFlux, 0.0,,"accumulative internal flux caused by deformation"))
	((bool, solvePressureSwitch, true,,"solve for pressure during actionTPF()"))
	((Real, deltaTimeTruncation, 0.0,,"truncation of time step, to avoid very small time steps during local imbibition, NOTE it does affect the mass conservation not set to 0"))
	((Real, waterBoundaryPressure, 0.0,,"Water pressure at boundary used in computations, is set automaticaly, but this value can be used to change water pressure during simulations"))
	((Real, waterVolumeTruncatedLost, 0.0,,"Water volume that has been truncated."))
	((bool, getQuantitiesUpdateCont, false,,"Continuous update of various macro-scale quantities or not. Note that the updating quantities is computationally expensive"))
	((Real, simpleWaterPressure, 0.0,,"Water pressure based on averaging over pore volume"))
	((Real, centroidAverageWaterPressure, 0.0,,"Water pressure based on centroid-corrected averaging, see Korteland et al. (2010) - what is the correct definition of average pressure?"))
	((Real, fractionMinSaturationInvasion, -1.0,,"Set the threshold saturation at which drainage can occur (Sthr = fractionMinSaturationInvasion), note that -1 implied the conventional definition of Sthr"))
	((vector<Real>, setFractionParticles, vector<Real>(scene->bodies->size(),0.0),,"Correction fraction for swelling of particles by mismatch of surface area of particles with those from actual surface area in pore units"))
       	((bool,primaryTPF, true,,"Boolean to indicate whether the initial conditions are for primary drainage of imbibition (dictated by drainageFirst) or secondary drainage or imbibition. Note that during simulations, a switch from drainage to imbibition or vise versa can easily be made by changing waterBoundaryPressure"))
       	((bool,swelling, false,,"If true, include swelling of particles during TPF computations"))
	((bool,useFastInvasion, false,,"use fast version of invasion"))
       	
	
	//END Latest dynamic/pore merging

	((bool, isCellLabelActivated, false,, "Activate cell labels for marking disconnected wetting clusters. NW-reservoir label 0; W-reservoir label 1; disconnected W-clusters label from 2. "))
	((bool, computeForceActivated, true,,"Activate capillary force computation. WARNING: turning off means capillary force is not computed at all, but the drainage can still work."))
	((bool, isDrainageActivated, true,, "Activates drainage."))
	((bool, isImbibitionActivated, false,, "Activates imbibition."))

	
	,/*TwoPhaseFlowEngineT()*/,
	,
	.def("getCellIsFictious",&TwoPhaseFlowEngine::cellIsFictious,"Check the connection between pore and boundary. If true, pore throat connects the boundary.")
	.def("setCellIsNWRes",&TwoPhaseFlowEngine::setCellIsNWRes,"set status whether 'wetting reservoir' state")
	.def("setCellIsWRes",&TwoPhaseFlowEngine::setCellIsWRes,"set status whether 'wetting reservoir' state")
	.def("savePhaseVtk",&TwoPhaseFlowEngine::savePhaseVtk,(boost::python::arg("folder")="./phaseVtk",boost::python::arg("withBoundaries")=true),"Save the saturation of local pores in vtk format. Sw(NW-pore)=0, Sw(W-pore)=1. Specify a folder name for output.")
	.def("getCellIsWRes",&TwoPhaseFlowEngine::cellIsWRes,"get status wrt 'wetting reservoir' state")
	.def("getCellIsNWRes",&TwoPhaseFlowEngine::cellIsNWRes,"get status wrt 'non-wetting reservoir' state")
	.def("getCellIsTrapW",&TwoPhaseFlowEngine::cellIsTrapW,"get status wrt 'trapped wetting phase' state")
	.def("getCellIsTrapNW",&TwoPhaseFlowEngine::cellIsTrapNW,"get status wrt 'trapped non-wetting phase' state")
	.def("getCellSaturation",&TwoPhaseFlowEngine::cellSaturation,"get saturation of one pore")
	.def("setCellSaturation",&TwoPhaseFlowEngine::setCellSaturation,"change saturation of one pore")
	.def("computeOnePhaseFlow",&TwoPhaseFlowEngine::computeOnePhaseFlow,"compute pressure and fluxes in the W-phase")
	.def("initialization",&TwoPhaseFlowEngine::initialization,"Initialize invasion setup. Build network, compute pore geometry info and initialize reservoir boundary conditions. ")
	.def("updatePressure",&TwoPhaseFlowEngine::updatePressure,"Apply the values of :yref:`FlowEngine::bndCondValue` to the boundary cells. Note: boundary pressure will be updated automatically in many cases, this function is for some low-level manipulations.")
	.def("getPoreThroatRadiusList",&TwoPhaseFlowEngine::cellporeThroatRadius,(boost::python::arg("cell_ID")),"get 4 pore throat radii of a cell.")
	.def("getNeighbors",&TwoPhaseFlowEngine::getNeighbors,(boost::python::arg("id"),boost::python::arg("withInfCell")=true),"get 4 neigboring cells, optionally exclude the infinite cells if withInfCell is False")
	.def("getCellHasInterface",&TwoPhaseFlowEngine::cellHasInterface,"indicates whether a NW-W interface is present within the cell")
	.def("getCellInSphereRadius",&TwoPhaseFlowEngine::cellInSphereRadius,"get the radius of the inscribed sphere in a pore unit")
	.def("setPoreBodyRadius",&TwoPhaseFlowEngine::setPoreBodyRadius,"set the entry pore body radius.")
	.def("getCellVoidVolume",&TwoPhaseFlowEngine::cellVoidVolume,"get the volume of pore space in each pore unit")
	//Pore merging
	.def("getCellMergedVolume",&TwoPhaseFlowEngine::cellMergedVolume,"get the merged volume of pore space in each pore unit")
	.def("setCellHasInterface",&TwoPhaseFlowEngine::setCellHasInterface,"change wheter a cell has a NW-W interface")
	.def("savePoreNetwork",&TwoPhaseFlowEngine::savePoreNetwork,(boost::python::arg("folder")="./poreNetwork"),"Extract the pore network of the granular material (i.e. based on triangulation of the pore space")
	.def("reTriangulateSpheres",&TwoPhaseFlowEngine::reTriangulate,"apply triangulation, while maintaining saturation")
	.def("actionMergingAlgorithm",&TwoPhaseFlowEngine::actionMergingAlgorithm,"apply triangulation, while maintaining saturation")
	.def("getCell2",&TwoPhaseFlowEngine::getCell2,(boost::python::arg("pos")),"get id of the cell containing (X,Y,Z).")		//should be removed finally, duplicate function
	.def("setCellDeltaVolume",&TwoPhaseFlowEngine::setCellDV,(boost::python::arg("id"),boost::python::arg("value")),"get id of the cell containing (X,Y,Z).")
	.def("mergeCells",&TwoPhaseFlowEngine::mergeCells,"Extract the pore network of the granular material")
	//Dynamic flow
 	.def("calculateResidualSaturation",&TwoPhaseFlowEngine::calculateResidualSaturation,"Calculate the residual saturation for each pore body")
	.def("copyPoreDataToCells",&TwoPhaseFlowEngine::copyPoreDataToCells,"copy data from merged pore units back to grain-based tetrahedra, this should be done before exporting VTK files")
	.def("getCellEntrySaturation",&TwoPhaseFlowEngine::cellEntrySaturation,"get the entry saturation of each pore throat")
	.def("getCellThresholdSaturation",&TwoPhaseFlowEngine::cellThresholdSaturation,"get the saturation of imbibition")
	.def("getCellMergedID",&TwoPhaseFlowEngine::cellMergedID,"get the saturation of imbibition")
	.def("actionTPF",&TwoPhaseFlowEngine::actionTPF,"run 1 time step flow Engine")
	.def("getSolidSurfaceAreaPerParticle",&TwoPhaseFlowEngine::solidSurfaceAreaPerParticle,(boost::python::arg("cell_ID")),"get solid area inside a packing of particles")
// 	.def("readTriangulation",&TwoPhaseFlowEngine::readTriangulation,"get the solid area of various solids in a pore")
	.def("imposeDeformationFluxTPF",&TwoPhaseFlowEngine::imposeDeformationFluxTPF,"Impose fluxes defined in dvTPF")
	.def("getCellPorosity",&TwoPhaseFlowEngine::cellPorosity,"get the porosity of individual cells.")
	.def("setCellHasInterface",&TwoPhaseFlowEngine::setCellHasInterface,"change wheter a cell has a NW-W interface")
	.def("getCellLabel",&TwoPhaseFlowEngine::cellLabel,"get cell label. 0 for NW-reservoir; 1 for W-reservoir; others for disconnected W-clusters.")
	.def("getMinDrainagePc",&TwoPhaseFlowEngine::getMinDrainagePc,"Get the minimum entry capillary pressure for the next drainage step.")
	.def("getMaxImbibitionPc",&TwoPhaseFlowEngine::getMaxImbibitionPc,"Get the maximum entry capillary pressure for the next imbibition step.")
	.def("getSaturation",&TwoPhaseFlowEngine::getSaturation,(boost::python::arg("isSideBoundaryIncluded")),"Get saturation of entire packing. If isSideBoundaryIncluded=false (default), the pores of side boundary are excluded in saturation calculating; if isSideBoundaryIncluded=true (only in isInvadeBoundary=true drainage mode), the pores of side boundary are included in saturation calculating.")
	.def("invasion",&TwoPhaseFlowEngine::invasion,"Run the drainage invasion.")
	.def("computeCapillaryForce",&TwoPhaseFlowEngine::computeCapillaryForce,(boost::python::arg("addForces")=false,boost::python::arg("permanently")=false),"Compute capillary force. Optionaly add them to body forces, for current iteration or permanently.")
// 	.def("saveVtk",&TwoPhaseFlowEngine::saveVtk,(boost::python::arg("folder")="./VTK",boost::python::arg("withBoundaries")=false),"Save pressure field in vtk format. Specify a folder name for output.")
	.def("getPotentialPendularSpheresPair",&TwoPhaseFlowEngine::getPotentialPendularSpheresPair,"Get the list of sphere ID pairs of potential pendular liquid bridge.")
	// Clusters
	.def("getClusters",&TwoPhaseFlowEngine::pyClusters/*,(boost::python::arg("folder")="./VTK")*/,"Get the list of clusters.")
	.def("clusterInvadePore",&TwoPhaseFlowEngine::pyClusterInvadePore,boost::python::arg("cellId"),"drain the pore identified by cellId and update the clusters accordingly.")
	.def("clusterInvadePoreFast",&TwoPhaseFlowEngine::pyClusterInvadePoreFast,boost::python::arg("cellId"),"drain the pore identified by cellId and update the clusters accordingly. This 'fast' version is faster and it also preserves interfaces through cluster splitting. OTOH it does not update entry Pc nor culsters volume (it could if needed)")
	.def("clusterOutvadePore",&TwoPhaseFlowEngine::clusterOutvadePore,(boost::python::arg("startingId"),boost::python::arg("imbibedId"),boost::python::arg("index")=-1),"imbibe the pore identified by imbibedId and merge the newly connected clusters if it happens. startingId->imbibedId defines the throat through which imbibition occurs. Giving index of the facet in cluster::interfaces should speedup its removal")
	// others
	.def("getCellVolume",&TwoPhaseFlowEngine::cellVolume,"get the volume of each cell")
	.def("isCellNeighbor",&TwoPhaseFlowEngine::isCellNeighbor,(boost::python::arg("cell1_ID"), boost::python::arg("cell2_ID")),"check if cell1 and cell2 are neigbors.")
	.def("setPoreThroatRadius",&TwoPhaseFlowEngine::setPoreThroatRadius, (boost::python::arg("cell1_ID"), boost::python::arg("cell2_ID"), boost::python::arg("radius")), "set the pore throat radius between cell1 and cell2.")
	.def("getPoreThroatRadius",&TwoPhaseFlowEngine::getPoreThroatRadius, (boost::python::arg("cell1_ID"), boost::python::arg("cell2_ID")), "get the pore throat radius between cell1 and cell2.")
	.def("getEffRcByPosRadius",&TwoPhaseFlowEngine::computeEffRcByPosRadius, (boost::python::arg("position1"),boost::python::arg("radius1"),boost::python::arg("position2"),boost::python::arg("radius2"),boost::python::arg("position3"),boost::python::arg("radius3")), "get effective radius by three spheres position and radius.(inscribed sphere)")
	.def("getMSPRcByPosRadius",&TwoPhaseFlowEngine::computeMSPRcByPosRadius, (boost::python::arg("position1"),boost::python::arg("radius1"),boost::python::arg("position2"),boost::python::arg("radius2"),boost::python::arg("position3"),boost::python::arg("radius3")), "get entry radius wrt MSP method by three spheres position and radius.")
	)
	// clang-format on

	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(TwoPhaseFlowEngine);

} // namespace yade

#endif //TwoPhaseFLOW

#endif // FLOW_GUARD
