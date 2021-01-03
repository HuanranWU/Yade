/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once
#include <core/Body.hpp>
#include <core/Engine.hpp>
#include <vector>

namespace yade { // Cannot have #include directive inside.

class PartialEngine : public Engine {
public:
	virtual ~PartialEngine() {};
	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS(PartialEngine,Engine,"Engine affecting only particular bodies in the simulation, namely those defined in :yref:`ids attribute<PartialEngine::ids>`. See also :yref:`GlobalEngine`.",
		((std::vector<int>,ids,,,":yref:`Ids<Body::id>` list of bodies affected by this PartialEngine."))
	);
	// clang-format on
};
REGISTER_SERIALIZABLE(PartialEngine);

} // namespace yade
