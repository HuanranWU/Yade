/*************************************************************************
*  Copyright (C) 2008 by Bruno Chareyre                                  *
*  bruno.chareyre@grenoble-inp.fr                                            *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#ifdef YADE_CGAL

#include "TesselationWrapper.hpp"
#include <lib/high-precision/Constants.hpp>
#include <lib/triangulation/Timer.h>
#include <pkg/dem/Shop.hpp>
#include <pkg/dem/SpherePack.hpp>

// https://codeyarns.com/2014/03/11/how-to-selectively-ignore-a-gcc-warning/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunused-function"
// Code that generates this warning, Note: we cannot do this trick in yade. If we have a warning in yade, we have to fix it! See also https://gitlab.com/yade-dev/trunk/merge_requests/73
// This method will work once g++ bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431#c34 is fixed.
#include <lib/pyutil/numpy_boost.hpp>
#pragma GCC diagnostic pop

namespace yade { // Cannot have #include directive inside.

using math::max;
using math::min; // using inside .cpp file is ok.

YADE_PLUGIN((TesselationWrapper));
CREATE_LOGGER(TesselationWrapper);

// helper macro do assign Matrix3r values to subarrays
#define MATRIX3R_TO_NUMPY(mat, arr)                                                                                                                            \
	arr[0] = mat(0, 0);                                                                                                                                    \
	arr[1] = mat(0, 1);                                                                                                                                    \
	arr[2] = mat(0, 2);                                                                                                                                    \
	arr[3] = mat(1, 0);                                                                                                                                    \
	arr[4] = mat(1, 1);                                                                                                                                    \
	arr[5] = mat(1, 2);                                                                                                                                    \
	arr[6] = mat(2, 0);                                                                                                                                    \
	arr[7] = mat(2, 1);                                                                                                                                    \
	arr[8] = mat(2, 2);

//spatial sort traits to use with a pair of CGAL::sphere pointers and integer.
//template<class _Triangulation>
struct RTraits_for_spatial_sort : public CGT::SimpleTriangulationTypes::RTriangulation::Geom_traits {
	//typedef typename _Triangulation::Geom_traits Gt;
	typedef CGT::SimpleTriangulationTypes::RTriangulation::Geom_traits Gt;
	typedef std::pair<const CGT::Sphere*, Body::id_t>                  Point_3;

	struct Less_x_3 {
		bool operator()(const Point_3& p, const Point_3& q) const { return Gt::Less_x_3()(p.first->point(), q.first->point()); }
	};
	struct Less_y_3 {
		bool operator()(const Point_3& p, const Point_3& q) const { return Gt::Less_y_3()(p.first->point(), q.first->point()); }
	};
	struct Less_z_3 {
		bool operator()(const Point_3& p, const Point_3& q) const { return Gt::Less_z_3()(p.first->point(), q.first->point()); }
	};
	Less_x_3 less_x_3_object() const { return Less_x_3(); }
	Less_y_3 less_y_3_object() const { return Less_y_3(); }
	Less_z_3 less_z_3_object() const { return Less_z_3(); }
};


//function inserting points into a triangulation (where YADE::Sphere is converted to CGT::Sphere)
//and setting the info field to the bodies id.
//Possible improvements : use bodies pointers to avoid one copy, use aabb's lists to replace the shuffle/sort part
// template <class Triangulation>
void build_triangulation_with_ids(const shared_ptr<BodyContainer>& bodies, TesselationWrapper& TW, bool reset = true)
{
	if (reset)
		TW.clear();
	typedef SimpleTesselation::RTriangulation              RTriangulation;
	SimpleTesselation&                                     Tes = *(TW.Tes);
	RTriangulation&                                        T   = Tes.Triangulation();
	std::vector<CGT::Sphere>                               spheres;
	std::vector<std::pair<const CGT::Sphere*, Body::id_t>> pointsPtrs;
	spheres.reserve(bodies->size());
	pointsPtrs.reserve(bodies->size());
	Tes.vertexHandles.clear();
	Tes.vertexHandles.resize(bodies->size() + 6, NULL); //+6 extra slots in case boundaries will be added latter as additional vertices

	Body::id_t  Ng                = 0;
	Body::id_t& MaxId             = Tes.maxId;
	TW.mean_radius                = 0;
	int                nonSpheres = 0;
	shared_ptr<Sphere> sph(new Sphere);
	int                Sph_Index = sph->getClassIndexStatic();
	Scene*             scene     = Omega::instance().getScene().get();
	for (const auto& bi : *bodies) {
		if (bi->shape->getClassIndex() == Sph_Index) {
			const Sphere* s = YADE_CAST<Sphere*>(bi->shape.get());
			//FIXME: is the scene periodicity verification useful in the next line ? Tesselation seems to work in both periodic and non-periodic conditions with "scene->cell->wrapShearedPt(bi->state->pos)". I keep the verification to be consistent with all other uses of "wrapShearedPt" function.
			const Vector3r& pos = scene->isPeriodic ? scene->cell->wrapShearedPt(bi->state->pos) : bi->state->pos;
			const Real      rad = s->radius;
			CGT::Sphere     sp(CGT::Point(pos[0], pos[1], pos[2]), rad * rad);
			spheres.push_back(sp);
			pointsPtrs.push_back(std::make_pair(&(spheres[Ng] /*.point()*/), bi->getId()));
			TW.Pmin = CGT::Point(min(TW.Pmin.x(), pos.x() - rad), min(TW.Pmin.y(), pos.y() - rad), min(TW.Pmin.z(), pos.z() - rad));
			TW.Pmax = CGT::Point(max(TW.Pmax.x(), pos.x() + rad), max(TW.Pmax.y(), pos.y() + rad), max(TW.Pmax.z(), pos.z() + rad));
			Ng++;
			TW.mean_radius += rad;
			MaxId = max(MaxId, bi->getId());
		} else
			++nonSpheres;
	}
	TW.mean_radius /= Ng;
	TW.rad_divided = true;
	spheres.resize(Ng);
	pointsPtrs.resize(Ng);
	std::random_shuffle(pointsPtrs.begin(), pointsPtrs.end());
	spatial_sort(pointsPtrs.begin(), pointsPtrs.end(), RTraits_for_spatial_sort() /*, CGT::RTriangulation::Weighted_point*/);

	RTriangulation::Cell_handle hint;

	TW.n_spheres = 0;
	for (std::vector<std::pair<const CGT::Sphere*, Body::id_t>>::const_iterator p = pointsPtrs.begin(); p != pointsPtrs.end(); ++p) {
		RTriangulation::Locate_type lt;
		RTriangulation::Cell_handle c;
		int                         li, lj;
		c                               = T.locate(*(p->first), lt, li, lj, hint);
		RTriangulation::Vertex_handle v = T.insert(*(p->first), lt, c, li, lj);
		if (v == RTriangulation::Vertex_handle())
			hint = c;
		else {
			v->info().setId((unsigned int)p->second);
			//Vh->info().isFictious = false;//false is the default
			Tes.maxId                    = math::max(Tes.maxId, (int)p->second);
			Tes.vertexHandles[p->second] = v;
			hint                         = v->cell();
			++TW.n_spheres;
		}
	}
	//cerr << " loaded : " << Ng<<", triangulated : "<<TW.n_spheres<<", mean radius = " << TW.mean_radius<<endl;
}

double thickness = 0;

TesselationWrapper::~TesselationWrapper()
{
	if (Tes)
		delete Tes;
}

void TesselationWrapper::clear(void)
{
	Tes->Clear();
	Pmin        = CGT::Point(inf, inf, inf);
	Pmax        = CGT::Point(-inf, -inf, -inf);
	mean_radius = 0;
	n_spheres   = 0;
	rad_divided = false;
	bounded     = false;
	Tes->vertexHandles.clear();
	facet_it = Tes->Triangulation().finite_edges_end();
}

void TesselationWrapper::clear2(void) //for testing purpose
{
	Tes->Clear();

	//   Pmin = Point(inf, inf, inf);
	//  Pmax = Point(-inf, -inf, -inf);
	//  mean_radius = 0;
	//  n_spheres = 0;
	//  rad_divided = false;
	// bounded = false;
	//  facet_it = Tes->Triangulation().finite_edges_end ();
}

void TesselationWrapper::insertSceneSpheres(bool reset)
{
	Scene* scene = Omega::instance().getScene().get();
	// 	Real_timer clock;
	//         clock.start();
	const shared_ptr<BodyContainer>& bodies = scene->bodies;
	build_triangulation_with_ids(bodies, *this, reset);
	// 	clock.top("Triangulation");
}

Real TesselationWrapper::Volume(unsigned int id) { return ((unsigned int)Tes->Max_id() >= id) ? Tes->Volume(id) : -1; }

bool TesselationWrapper::insert(double x, double y, double z, double rad, unsigned int id)
{
	checkMinMax(x, y, z, rad);
	mean_radius += rad;
	++n_spheres;
	return (Tes->insert(x, y, z, rad, id) != NULL);
}

void TesselationWrapper::checkMinMax(double x, double y, double z, double rad)
{
	Pmin = CGT::Point(min(Pmin.x(), x - rad), min(Pmin.y(), y - rad), min(Pmin.z(), z - rad));
	Pmax = CGT::Point(max(Pmax.x(), x + rad), max(Pmax.y(), y + rad), max(Pmax.z(), z + rad));
}


bool TesselationWrapper::move(double x, double y, double z, double rad, unsigned int id)
{
	checkMinMax(x, y, z, rad);
	if (Tes->move(x, y, z, rad, id) != NULL)
		return true;
	else {
		std::cerr << "Tes->move(x,y,z,rad,id)==NULL" << std::endl;
		return false;
	}
}

void TesselationWrapper::computeTesselation(void)
{
	if (not(Tes->vertexHandles.size() > 0))
		insertSceneSpheres();
	if (!rad_divided) {
		mean_radius /= n_spheres;
		rad_divided = true;
	}
	Tes->compute();
}

void TesselationWrapper::computeTesselation(double pminx, double pmaxx, double pminy, double pmaxy, double pminz, double pmaxz)
{
	if (not(Tes->vertexHandles.size() > 0))
		insertSceneSpheres();
	addBoundingPlanes(pminx, pmaxx, pminy, pmaxy, pminz, pmaxz);
	computeTesselation();
}

void TesselationWrapper::computeVolumes(void)
{
	if (not(Tes->vertexHandles.size() > 0))
		insertSceneSpheres();
	if (!bounded)
		addBoundingPlanes();
	computeTesselation();
	Tes->computeVolumes();
}
unsigned int TesselationWrapper::NumberOfFacets(bool initIters)
{
	if (initIters)
		InitIter();
	return Tes->Triangulation().number_of_finite_edges();
}

void TesselationWrapper::InitIter(void)
{
	facet_begin = Tes->Triangulation().finite_edges_begin();
	facet_end   = Tes->Triangulation().finite_edges_end();
	facet_it    = facet_begin;
}

bool TesselationWrapper::nextFacet(std::pair<unsigned int, unsigned int>& facet)
{
	if (facet_end == facet_it)
		return false;
	facet.first  = facet_it->first->vertex(facet_it->second)->info().id();
	facet.second = facet_it->first->vertex((facet_it)->third)->info().id();
	++facet_it;
	return true;
}

void TesselationWrapper::addBoundingPlanes(Real pminx, Real pmaxx, Real pminy, Real pmaxy, Real pminz, Real pmaxz)
{
	if (!bounded) {
		if (!rad_divided) {
			mean_radius /= n_spheres;
			rad_divided = true;
		}
		// Insert the 6 additional vertices in the right place (usually they will be ids 0 to 5 when walls/facets/boxes are used, but not always)
		// append them at the end if the initial list is full
		int freeIds[6];
		int i = 0;
		for (int k = 0; k < 6; k++) {
			while (Tes->vertexHandles[i] != NULL)
				++i;
			freeIds[k] = i;
		}
		// now insert
		Tes->vertexHandles[freeIds[0]] = Tes->insert(
		        0.5 * (pminx + pmaxx), pminy - far * (pmaxx - pminx), 0.5 * (pmaxz + pminz), far * (pmaxx - pminx) + thickness, freeIds[0], true);
		Tes->vertexHandles[freeIds[1]] = Tes->insert(
		        0.5 * (pminx + pmaxx), pmaxy + far * (pmaxx - pminx), 0.5 * (pmaxz + pminz), far * (pmaxx - pminx) + thickness, freeIds[1], true);
		Tes->vertexHandles[freeIds[2]] = Tes->insert(
		        pminx - far * (pmaxy - pminy), 0.5 * (pmaxy + pminy), 0.5 * (pmaxz + pminz), far * (pmaxy - pminy) + thickness, freeIds[2], true);
		Tes->vertexHandles[freeIds[3]] = Tes->insert(
		        pmaxx + far * (pmaxy - pminy), 0.5 * (pmaxy + pminy), 0.5 * (pmaxz + pminz), far * (pmaxy - pminy) + thickness, freeIds[3], true);
		Tes->vertexHandles[freeIds[4]] = Tes->insert(
		        0.5 * (pminx + pmaxx), 0.5 * (pmaxy + pminy), pminz - far * (pmaxy - pminy), far * (pmaxy - pminy) + thickness, freeIds[4], true);
		Tes->vertexHandles[freeIds[5]] = Tes->insert(
		        0.5 * (pminx + pmaxx), 0.5 * (pmaxy + pminy), pmaxz + far * (pmaxy - pminy), far * (pmaxy - pminy) + thickness, freeIds[5], true);

		bounded = true;
	}
}

void TesselationWrapper::addBoundingPlanes(void) { addBoundingPlanes(Pmin.x(), Pmax.x(), Pmin.y(), Pmax.y(), Pmin.z(), Pmax.z()); }

void TesselationWrapper::setState(bool state) { mma.setState(state ? 2 : 1); }

void TesselationWrapper::loadState(string filename, bool stateNumber, bool bz2)
{
	CGT::TriaxialState& TS = stateNumber ? *(mma.analyser->TS1) : *(mma.analyser->TS0);
	TS.from_file(filename.c_str(), bz2);
}

void TesselationWrapper::saveState(string filename, bool stateNumber, bool bz2)
{
	CGT::TriaxialState& TS = stateNumber ? *(mma.analyser->TS1) : *(mma.analyser->TS0);
	TS.to_file(filename.c_str(), bz2);
}

void TesselationWrapper::defToVtkFromStates(string inputFile1, string inputFile2, string outputFile, bool bz2)
{
	mma.analyser->DefToFile(inputFile1.c_str(), inputFile2.c_str(), outputFile.c_str(), bz2);
}

void createSphere(shared_ptr<Body>& body, Vector3r position, Real radius, bool /*big*/, bool /*dynamic*/)
{
	body            = shared_ptr<Body>(new Body);
	body->groupMask = 2;
	shared_ptr<Sphere> iSphere(new Sphere);
	body->state->blockedDOFs = State::DOF_NONE;
	body->state->pos         = position;
	iSphere->radius          = radius;
	body->shape              = iSphere;
}

void TesselationWrapper::defToVtkFromPositions(string inputFile1, string inputFile2, string outputFile, bool /*bz2*/)
{
	SpherePack sp1, sp2;
	sp1.fromFile(inputFile1);
	sp2.fromFile(inputFile2);
	size_t imax = sp1.pack.size();
	if (imax != sp2.pack.size())
		LOG_ERROR("The files have different numbers of spheres");
	shared_ptr<Body> body;
	scene->bodies->clear();
	for (size_t i = 0; i < imax; i++) {
		const SpherePack::Sph& sp(sp1.pack[i]);
		LOG_DEBUG("sphere (" << sp.c << " " << sp.r << ")");
		createSphere(body, sp.c, sp.r, false, true);
		scene->bodies->insert(body);
	}
	mma.setState(1);
	scene->bodies->clear();
	for (size_t i = 0; i < imax; i++) {
		const SpherePack::Sph& sp(sp2.pack[i]);
		LOG_DEBUG("sphere (" << sp.c << " " << sp.r << ")");
		createSphere(body, sp.c, sp.r, false, true);
		scene->bodies->insert(body);
	}
	mma.setState(2);
	mma.analyser->DefToFile(outputFile.c_str());
}

void TesselationWrapper::defToVtk(string outputFile) { mma.analyser->DefToFile(outputFile.c_str()); }

boost::python::dict TesselationWrapper::getVolPoroDef(bool deformation)
{
	delete Tes;
	CGT::TriaxialState* ts;
	if (deformation) { //use the final state to compute volumes
		/*const vector<CGT::Tenseur3>& def =*/mma.analyser->computeParticlesDeformation();
		Tes = &mma.analyser->TS1->tesselation();
		ts  = mma.analyser->TS1;
	} else {
		Tes = &mma.analyser->TS0->tesselation(); //no reason to use the final state if we don't want to compute deformations, keep using the initial
		ts  = mma.analyser->TS0;
	}
	RTriangulation& Tri = Tes->Triangulation();
	Pmin                = ts->box.base;
	Pmax                = ts->box.sommet;
	//if (!scene->isPeriodic) addBoundingPlanes();
	computeVolumes();
	int bodiesDim = Tes->Max_id() + 1; //=scene->bodies->size();
	cerr << "bodiesDim=" << bodiesDim << endl;
	int dim1[] = { bodiesDim };
	int dim2[] = { bodiesDim, 9 };
	/// This is the code that needs numpy include
	//numpy_boost<Body::id_t,1> id(dim1);
	numpy_boost<Real, 1> vol(dim1);
	numpy_boost<Real, 1> poro(dim1);
	numpy_boost<Real, 2> def(dim2);
	//for(const auto & b :  *scene->bodies){
	for (RTriangulation::Finite_vertices_iterator V_it = Tri.finite_vertices_begin(); V_it != Tri.finite_vertices_end(); V_it++) {
		//id[]=V_it->info().id()
		//if(!b) continue;
		const Body::id_t id        = V_it->info().id();
		Real             sphereVol = 4.188790 * math::pow((V_it->point().weight()), 1.5); // 4/3*PI*R³ = 4.188...*R³
		vol[id]                    = V_it->info().v();
		poro[id]                   = (V_it->info().v() - sphereVol) / V_it->info().v();
		if (deformation)
			MATRIX3R_TO_NUMPY(mma.analyser->ParticleDeformation[id], def[id]);
		//cerr << V_it->info().v()<<" "<<ParticleDeformation[id]<<endl;
	}
	boost::python::dict ret;
	ret["vol"]  = vol;
	ret["poro"] = poro;
	if (deformation)
		ret["def"] = def;
	return ret;
}

#ifdef ALPHASHAPE
boost::python::list TesselationWrapper::getAlphaFaces(double alpha)
{
	vector<AlphaFace> faces;
	Tes->setAlphaFaces(faces, alpha);
	boost::python::list ret;
	for (auto f = faces.begin(); f != faces.end(); f++)
		ret.append(boost::python::make_tuple(Vector3i(f->ids[0], f->ids[1], f->ids[2]), makeVector3r(f->normal)));
	return ret;
}

boost::python::list TesselationWrapper::getAlphaCaps(double alpha, double shrinkedAlpha, bool fixedAlpha)
{
	vector<AlphaCap> caps;
	Tes->setExtendedAlphaCaps(caps, alpha, shrinkedAlpha, fixedAlpha);
	boost::python::list ret;
	for (auto f = caps.begin(); f != caps.end(); f++)
		ret.append(boost::python::make_tuple(f->id, makeVector3r(f->normal)));
	//    cerr<<"number of caps="<<caps.size()<<endl;
	return ret;
}

void TesselationWrapper::applyAlphaForces(Matrix3r stress, double alpha, double shrinkedAlpha, bool fixedAlpha)
{
	Scene* scene = Omega::instance().getScene().get();
	if (Tes->Triangulation().number_of_vertices() <= 0)
		build_triangulation_with_ids(scene->bodies, *this, true); //if not already triangulated do it now
	vector<AlphaCap> caps;
	Tes->setExtendedAlphaCaps(caps, alpha, shrinkedAlpha, fixedAlpha);
	for (auto f = caps.begin(); f != caps.end(); f++)
		scene->forces.setPermForce(f->id, stress * makeVector3r(f->normal));
}

boost::python::list TesselationWrapper::getAlphaGraph(double alpha, double shrinkedAlpha, bool fixedAlpha)
{
	vector<Vector3r>    segments = Tes->getExtendedAlphaGraph(alpha, shrinkedAlpha, fixedAlpha);
	boost::python::list ret;
	for (auto f = segments.begin(); f != segments.end(); f++)
		ret.append(*f);
	return ret;
}

boost::python::list TesselationWrapper::getAlphaVertices(double alpha)
{
	vector<int>         vertices = Tes->getAlphaVertices(alpha);
	boost::python::list ret;
	for (auto f = vertices.begin(); f != vertices.end(); f++)
		ret.append(*f);
	return ret;
}
#endif //ALPHASHAPE

} // namespace yade

#endif /* YADE_CGAL */
