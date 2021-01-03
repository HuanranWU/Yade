/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include <lib/base/Math.hpp>
#include <lib/multimethods/Indexable.hpp>
#include <lib/serialization/Serializable.hpp>
#include <core/Dispatcher.hpp>

namespace yade { // Cannot have #include directive inside.

class IPhys : public Serializable, public Indexable {
	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR_PY(IPhys,Serializable,"Physical (material) properties of :yref:`interaction<Interaction>`.",
		/*attrs*/,
		/*ctor*/,
		/*py*/YADE_PY_TOPINDEXABLE(IPhys)
	);
	// clang-format on
	REGISTER_INDEX_COUNTER(IPhys);
};
REGISTER_SERIALIZABLE(IPhys);

} // namespace yade
