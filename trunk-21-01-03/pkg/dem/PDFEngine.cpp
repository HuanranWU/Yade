#include "PDFEngine.hpp"
#include <lib/high-precision/Constants.hpp>
#include <fstream>
#include <iostream>
#include <type_traits>

namespace yade { // Cannot have #include directive inside.

void PDFEngine::getSpectrums(vector<PDFEngine::PDF>& pdfs)
{
	const shared_ptr<Scene>& scene = Omega::instance().getScene();

	vector<int>  nTheta, nPhi;
	vector<Real> dTheta, dPhi;
	Real         V;
	int          N = scene->bodies->size();

	if (!scene->isPeriodic) {
		LOG_WARN("Volume is based on periodic cell volume. Set V = 1");
		V = 1.;
	} else
		V = scene->cell->getVolume();

	for (uint i(0); i < pdfs.size(); i++) {
		nTheta.push_back(pdfs[i].shape()[0]);
		nPhi.push_back(pdfs[i].shape()[1]);
		dTheta.push_back(Mathr::PI / nTheta[i]);
		dPhi.push_back(Mathr::PI / nPhi[i]);
	}

	FOREACH(const shared_ptr<Interaction>& I, *scene->interactions)
	{
		if (!I->isReal())
			continue;
		GenericSpheresContact* geom = dynamic_cast<GenericSpheresContact*>(I->geom.get());

		if (geom) {
			Real theta = acos(geom->normal.y());                    //[0;pi]
			Real phi   = atan2(geom->normal.x(), geom->normal.z()); //[-pi;pi]

			bool inversed = false;
			if (phi < 0) { // Everything has central-symmetry. Let's take only positive angles.
				theta = Mathr::PI - theta;
				phi += Mathr::PI;
				inversed = true;
			}

			for (uint i(0); i < pdfs.size(); i++) {
				// Calculate indexes
				int idT1 = ((int)floor((theta) / dTheta[i])) % nTheta[i];
				int idP1 = ((int)floor((phi) / dPhi[i])) % nPhi[i];

				Real dS = sin(((double)idT1 + 0.5) * dTheta[i]) * dTheta[i] * dPhi[i];

				if (pdfs[i][idT1][idP1])
					pdfs[i][idT1][idP1]->addData(I, dS, V, N, inversed);
			}
		}
	}
}

void PDFEngine::writeToFile(vector<PDFEngine::PDF> const& pdfs)
{
	std::ofstream fid;
	if (firstRun) {
		fid.open(filename);
	} else {
		fid.open(filename, std::ios_base::app);
	}

	if (fid.good() and fid.is_open()) {
		if (firstRun) {
			fid << "# time\t";
			for (uint i(0); i < pdfs.size(); i++) {
				uint nTheta = pdfs[i].shape()[0];
				uint nPhi   = pdfs[i].shape()[1];
				Real dTheta = (Mathr::PI / nTheta);
				Real dPhi   = (Mathr::PI / nPhi);

				for (uint t(0); t < nTheta; t++)
					for (uint p(0); p < nPhi; p++)
						if (pdfs[i][t][p]) {
							vector<string> ss = pdfs[i][t][p]->getSuffixes();

							if (ss.size() > 1)
								for (uint j(0); j < ss.size(); j++)
									fid << pdfs[i][t][p]->name << "_" << ss[j] << "("
									    << ((static_cast<Real>(t) + 0.5) * dTheta) << ","
									    << ((static_cast<Real>(p) + 0.5) * dPhi) << ")\t";
							else
								fid << pdfs[i][t][p]->name << "(" << ((static_cast<Real>(t) + 0.5) * dTheta) << ","
								    << ((static_cast<Real>(p) + 0.5) * dPhi) << ")\t";
						}
			}
			firstRun = false;
			fid << "\n";
		}

		fid << scene->time << "\t";

		for (uint i(0); i < pdfs.size(); i++)
			for (uint t(0); t < pdfs[i].shape()[0]; t++)
				for (uint p(0); p < pdfs[i].shape()[1]; p++)
					if (pdfs[i][t][p]) {
						vector<string> dat = pdfs[i][t][p]->getDatas();

						for (uint j(0); j < dat.size(); j++)
							fid << dat[j] << "\t";
					}
		fid << "\n";
		fid.close();
	} else {
		if (!warnedOnce)
			LOG_ERROR("Unable to open " << filename << " for PDF writing");
		warnedOnce = true;
	}
}

void PDFEngine::action()
{
	vector<PDFEngine::PDF> pdfs;
	pdfs.resize(5);

	for (uint i(0); i < pdfs.size(); i++) {
		pdfs[i].resize(boost::extents[numDiscretizeAngleTheta][numDiscretizeAnglePhi]);
	}

	// Hint: If you want data on particular points, allocate only those pointers.
	for (uint t(0); t < numDiscretizeAngleTheta; t++)
		for (uint p(0); p < numDiscretizeAnglePhi; p++) {
			pdfs[0][t][p] = shared_ptr<PDFCalculator>(new PDFSpheresStressCalculator<NormPhys>(&NormPhys::normalForce, "normalStress"));
			pdfs[1][t][p] = shared_ptr<PDFCalculator>(new PDFSpheresStressCalculator<NormShearPhys>(&NormShearPhys::shearForce, "shearStress"));
			pdfs[2][t][p] = shared_ptr<PDFCalculator>(new PDFSpheresDistanceCalculator("h"));
			pdfs[3][t][p] = shared_ptr<PDFCalculator>(new PDFSpheresVelocityCalculator("v"));
			pdfs[4][t][p] = shared_ptr<PDFCalculator>(new PDFSpheresIntrsCalculator("P"));
		}

	getSpectrums(pdfs); // Where the magic happen :)
	writeToFile(pdfs);
}


CREATE_LOGGER(PDFEngine);

PDFSpheresDistanceCalculator::PDFSpheresDistanceCalculator(string name)
        : PDFEngine::PDFCalculator(name)
        , m_h(0.)
        , m_N(0)
{
}

vector<string> PDFSpheresDistanceCalculator::getDatas() const { return vector<string>({ math::toString(m_h / m_N) }); }

void PDFSpheresDistanceCalculator::cleanData()
{
	m_h = 0.;
	m_N = 0;
}

bool PDFSpheresDistanceCalculator::addData(const shared_ptr<Interaction>& I, Real const&, Real const&, int const&, bool)
{
	if (!I->isReal())
		return false;
	ScGeom* geom = dynamic_cast<ScGeom*>(I->geom.get());
	Real    a((geom->radius1 + geom->radius2) / 2.);

	if (!geom)
		return false;

	m_N++;
	m_h -= geom->penetrationDepth / a;

	return true;
}

PDFSpheresVelocityCalculator::PDFSpheresVelocityCalculator(string name)
        : PDFEngine::PDFCalculator(name)
        , m_vel(Vector3r::Zero())
        , m_N(0)
{
}

vector<string> PDFSpheresVelocityCalculator::getSuffixes() const { return vector<string>({ "x", "y", "z" }); }

vector<string> PDFSpheresVelocityCalculator::getDatas() const
{
	vector<string> ret;
	for (int i(0); i < 3; i++)
		ret.push_back(math::toString(m_vel(i) / m_N));
	return ret;
}

void PDFSpheresVelocityCalculator::cleanData()
{
	m_vel = Vector3r::Zero();
	m_N   = 0;
}


bool PDFSpheresVelocityCalculator::addData(const shared_ptr<Interaction>& I, Real const&, Real const&, int const&, bool inversed)
{
	if (!I->isReal())
		return false;

	// Geometry
	ScGeom* geom = dynamic_cast<ScGeom*>(I->geom.get());
	if (!geom)
		return false;

	// geometric parameters
	// Real a((geom->radius1+geom->radius2)/2.);
	Vector3r relV = geom->getIncidentVel_py(I, false);

	if (inversed)
		relV *= -1;

	m_N++;
	m_vel += relV;

	return true;
}

PDFSpheresIntrsCalculator::PDFSpheresIntrsCalculator(string name, bool (*fp)(shared_ptr<Interaction> const&))
        : PDFEngine::PDFCalculator(name)
        , m_P(0.)
        , m_accepter(fp)
{
}

vector<string> PDFSpheresIntrsCalculator::getDatas() const { return vector<string>({ math::toString(m_P) }); }

void PDFSpheresIntrsCalculator::cleanData() { m_P = 0.; }

bool PDFSpheresIntrsCalculator::addData(const shared_ptr<Interaction>& I, Real const& dS, Real const&, int const& N, bool)
{
	if (!I->isReal())
		return false;

	if (m_accepter(I))
		m_P += 1. / (dS * N);

	return true;
}

} // namespace yade
