/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include "Engine.hpp"

namespace yade { // Cannot have #include directive inside.

class GlobalEngine : public Engine {
public:
	virtual ~GlobalEngine() {};
	// clang-format off
	YADE_CLASS_BASE_DOC(GlobalEngine,Engine,"Engine that will generally affect the whole simulation (contrary to :yref:`PartialEngine`).");
	// clang-format on
};
REGISTER_SERIALIZABLE(GlobalEngine);

} // namespace yade
