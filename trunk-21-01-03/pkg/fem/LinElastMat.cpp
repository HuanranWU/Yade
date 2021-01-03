/*************************************************************************
*  Copyright (C) 2013 by Burak ER                                 	 *
*									 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#ifdef YADE_FEM
#include <pkg/fem/LinElastMat.hpp>

namespace yade { // Cannot have #include directive inside.

YADE_PLUGIN((DeformableElementMaterial)(LinIsoElastMat)(LinIsoRayleighDampElastMat));

DeformableElementMaterial::~DeformableElementMaterial(void) { }
LinIsoElastMat::~LinIsoElastMat(void) { }
LinIsoRayleighDampElastMat::~LinIsoRayleighDampElastMat(void) { }

} // namespace yade

#endif //YADE_FEM
