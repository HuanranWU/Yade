// 2018 © William Chèvremont <william.chevremont@univ-grenoble-alpes.fr>

#pragma once

#include <pkg/common/NormShearPhys.hpp>
#include <pkg/common/PeriodicEngines.hpp>
#include <pkg/dem/ScGeom.hpp>
#include <boost/multi_array.hpp>

namespace yade { // Cannot have #include directive inside.

class PDFEngine : public PeriodicEngine {
public:
	class PDFCalculator {
	public:
		PDFCalculator(string const& n)
		        : name(n) {};
		virtual ~PDFCalculator() {};

		virtual vector<string> getSuffixes() const { return vector<string>({ "" }); }
		virtual vector<string> getDatas() const                                                                                    = 0;
		virtual void           cleanData()                                                                                         = 0;
		virtual bool           addData(const shared_ptr<Interaction>&, Real const& dS, Real const& V, int const& N, bool inversed) = 0;

		string name;
	};

	typedef boost::multi_array<shared_ptr<PDFCalculator>, 2> PDF;

	static void  getSpectrums(vector<PDF>&);
	virtual void action();

	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(PDFEngine, PeriodicEngine,
		"Base class for spectrums calculations. Compute Probability Density Functions of normalStress, shearStress, distance, velocity and interactions in spherical coordinates and write result to a file. Column name format is: Data(theta, phi). Convention used: x: phi = 0, y: theta = 0, z: phi = pi/2",
		((uint, numDiscretizeAngleTheta, 20,,"Number of sector for theta-angle"))
		((uint, numDiscretizeAnglePhi, 20,,"Number of sector for phi-angle"))
		//((Real, discretizeRadius, 0.1,,"d/a interval size"))
		((string, filename, "PDF.txt", , "Filename"))
		((bool, firstRun, true, (Attr::hidden | Attr::readonly), ""))
		((bool, warnedOnce, false, , "For one-time warning. May trigger usefull warnings"))
		,,
		//.def("getSpectrums", &LubricationDPFEngine::PyGetSpectrums,(py::arg("nPhi")=40, py::arg("nTheta")=20), "Get Stress spectrums").staticmethod("getSpectrums")
	);
	// clang-format on
	DECLARE_LOGGER;

protected:
	void writeToFile(vector<PDF> const&);
};

REGISTER_SERIALIZABLE(PDFEngine);

template <class Phys> class PDFSpheresStressCalculator : public PDFEngine::PDFCalculator {
public:
	PDFSpheresStressCalculator(Vector3r Phys::*member, string name)
	        : PDFEngine::PDFCalculator(name)
	        , m_member(member)
	        , m_stress(Matrix3r::Zero()) {};
	vector<string> getSuffixes() const { return vector<string>({ "xx", "xy", "xz", "yx", "yy", "yz", "zx", "zy", "zz" }); }
	vector<string> getDatas() const
	{
		vector<string> out;
		for (int i(0); i < 3; i++)
			for (int j(0); j < 3; j++)
				out.push_back(math::toString(m_stress(i, j)));
		return out;
	}
	void cleanData() { m_stress = Matrix3r::Zero(); }
	bool addData(const shared_ptr<Interaction>& I, Real const& dS, Real const& V, int const&, bool)
	{
		if (!I->isReal())
			return false;
		ScGeom* geom = dynamic_cast<ScGeom*>(I->geom.get());
		Phys*   phys = dynamic_cast<Phys*>(I->phys.get());

		if (geom && phys) {
			Real     r = geom->radius1 + geom->radius2 - geom->penetrationDepth;
			Vector3r l = r / (V * dS) * geom->normal;
			m_stress += phys->*(m_member)*l.transpose();
			return true;
		} else
			return false;
	}

private:
	Vector3r Phys::*m_member;
	Matrix3r        m_stress;
};

class PDFSpheresDistanceCalculator : public PDFEngine::PDFCalculator {
public:
	PDFSpheresDistanceCalculator(string name);
	vector<string> getDatas() const;
	void           cleanData();
	bool           addData(const shared_ptr<Interaction>&, Real const& dS, Real const& V, int const& N, bool inversed);

private:
	Real m_h;
	uint m_N;
};

class PDFSpheresVelocityCalculator : public PDFEngine::PDFCalculator {
public:
	PDFSpheresVelocityCalculator(string name);
	vector<string> getSuffixes() const;
	vector<string> getDatas() const;
	void           cleanData();
	bool           addData(const shared_ptr<Interaction>&, Real const& dS, Real const& V, int const& N, bool inversed);

private:
	Vector3r m_vel;
	uint     m_N;
};

class PDFSpheresIntrsCalculator : public PDFEngine::PDFCalculator {
public:
	PDFSpheresIntrsCalculator(
	        string name, bool (*)(shared_ptr<Interaction> const&) = [](shared_ptr<Interaction> const&) { return true; });
	vector<string> getDatas() const;
	void           cleanData();
	bool           addData(const shared_ptr<Interaction>&, Real const& dS, Real const& V, int const& N, bool inversed);

private:
	Real m_P;
	bool (*m_accepter)(shared_ptr<Interaction> const&);
};

} // namespace yade
