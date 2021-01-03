#pragma once
#include <core/GlobalEngine.hpp>
#include <stdexcept>

namespace yade { // Cannot have #include directive inside.

class FieldApplier : public GlobalEngine {
	virtual void action() { throw std::runtime_error("FieldApplier must not be used in simulations directly (FieldApplier::action called)."); }
	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS(FieldApplier,GlobalEngine,"Base for engines applying force files on particles. Not to be used directly.",
		((int,fieldWorkIx,-1,(Attr::hidden|Attr::noSave),"Index for the work done by this field, if tracking energies."))
	);
	// clang-format on
};
REGISTER_SERIALIZABLE(FieldApplier);

} // namespace yade
