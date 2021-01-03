#pragma once
#include <core/GlobalEngine.hpp>

namespace yade { // Cannot have #include directive inside.

class BoundaryController : public GlobalEngine {
	virtual void action()
	{
		{
			throw std::runtime_error("BoundaryController must not be used in simulations directly (BoundaryController::action called).");
		}
	}
	// clang-format off
	YADE_CLASS_BASE_DOC(BoundaryController,GlobalEngine,"Base for engines controlling boundary conditions of simulations. Not to be used directly.");
	// clang-format on
};
REGISTER_SERIALIZABLE(BoundaryController);

} // namespace yade
