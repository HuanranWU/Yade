
#include <pkg/common/Dispatching.hpp>
#include <pkg/common/ElastMat.hpp>
#include <pkg/common/MatchMaker.hpp>

namespace yade { // Cannot have #include directive inside.

class Ip2_ElastMat_ElastMat_NormPhys : public IPhysFunctor {
public:
	virtual void go(const shared_ptr<Material>& b1, const shared_ptr<Material>& b2, const shared_ptr<Interaction>& interaction);
	FUNCTOR2D(ElastMat, ElastMat);
	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS(Ip2_ElastMat_ElastMat_NormPhys,IPhysFunctor,"Create a :yref:`NormPhys` from two :yref:`ElastMats<ElastMat>`. TODO. EXPERIMENTAL",
	);
	// clang-format on
};
REGISTER_SERIALIZABLE(Ip2_ElastMat_ElastMat_NormPhys);


class Ip2_ElastMat_ElastMat_NormShearPhys : public IPhysFunctor {
public:
	virtual void go(const shared_ptr<Material>& b1, const shared_ptr<Material>& b2, const shared_ptr<Interaction>& interaction);
	FUNCTOR2D(ElastMat, ElastMat);
	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS(Ip2_ElastMat_ElastMat_NormShearPhys,IPhysFunctor,"Create a :yref:`NormShearPhys` from two :yref:`ElastMats<ElastMat>`. TODO. EXPERIMENTAL",
	);
	// clang-format on
};
REGISTER_SERIALIZABLE(Ip2_ElastMat_ElastMat_NormShearPhys);

} // namespace yade
