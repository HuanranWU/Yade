// © 2013 Jan Elias, http://www.fce.vutbr.cz/STM/elias.j/, elias.j@fce.vutbr.cz
// https://www.vutbr.cz/www_base/gigadisk.php?i=95194aa9a


#pragma once

#ifdef YADE_CGAL
#include "Polyhedra.hpp"
#include <pkg/common/PeriodicEngines.hpp>


namespace yade { // Cannot have #include directive inside.

//*********************************************************************************
/* Polyhedra Splitter */
class PolyhedraSplitter : public PeriodicEngine {
public:
	virtual void action();
	Real         getStrength(const Real& volume, const Real& strength) const;
	void         Symmetrize(Matrix3r& bStress);

	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(
		PolyhedraSplitter,PeriodicEngine,"Engine that splits polyhedras.\n\n.. warning:: PolyhedraSplitter returns different results depending on CGAL version! For details see https://gitlab.com/yade-dev/trunk/issues/45"
		,
		,
		/*ctor*/
		,/*py*/
	);
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(PolyhedraSplitter);

class SplitPolyTauMax : public PolyhedraSplitter {
public:
	virtual void action();

	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(
		SplitPolyTauMax,PolyhedraSplitter,"Split polyhedra along TauMax."
		,
		,
		/*ctor*/
		,/*py*/
	);
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(SplitPolyTauMax);

class SplitPolyMohrCoulomb : public PolyhedraSplitter {
public:
	virtual void action();

	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(
		SplitPolyMohrCoulomb,PolyhedraSplitter,"Split polyhedra according to Mohr-Coulomb criterion."
		,
		((string,fileName,"",,"Base."))
		,
		/*ctor*/
		,/*py*/
	);
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(SplitPolyMohrCoulomb);

} // namespace yade

#endif // YADE_CGAL
