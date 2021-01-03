// 2010 © Václav Šmilauer <eudoxos@arcig.cz>
#pragma once
#include <lib/base/openmp-accu.hpp>
#include <pkg/common/Callbacks.hpp>

namespace yade { // Cannot have #include directive inside.

class SumIntrForcesCb : public IntrCallback {
public:
	OpenMPAccumulator<int>        numIntr;
	OpenMPAccumulator<Real>       force;
	static void                   go(IntrCallback*, Interaction*);
	virtual IntrCallback::FuncPtr stepInit();
	// clang-format off
	YADE_CLASS_BASE_DOC(SumIntrForcesCb,IntrCallback,"Callback summing magnitudes of forces over all interactions. :yref:`IPhys` of interactions must derive from :yref:`NormShearPhys` (responsability fo the user).");
	// clang-format on
};
REGISTER_SERIALIZABLE(SumIntrForcesCb);

#ifdef YADE_BODY_CALLBACK
class SumBodyForcesCb : public BodyCallback {
	Scene* scene;

public:
	OpenMPAccumulator<int>        numBodies;
	OpenMPAccumulator<Real>       force;
	static void                   go(BodyCallback*, Body*);
	virtual BodyCallback::FuncPtr stepInit();
	// clang-format off
	YADE_CLASS_BASE_DOC(SumBodyForcesCb,BodyCallback,"Callback summing magnitudes of resultant forces over :yref:`dynamic<Body::dynamic>` bodies.");
	// clang-format on
};
REGISTER_SERIALIZABLE(SumBodyForcesCb);
#endif

} // namespace yade
