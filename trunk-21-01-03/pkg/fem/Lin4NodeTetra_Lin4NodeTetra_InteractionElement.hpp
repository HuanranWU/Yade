/*************************************************************************
*  Copyright (C) 2013 by Burak ER                                 	 *
*									 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include <lib/base/Logging.hpp>
#include <lib/base/Math.hpp>
#include <core/Body.hpp>
#include <core/PartialEngine.hpp>
#include <pkg/fem/DeformableCohesiveElement.hpp>
#include <map>
#include <stdexcept>

namespace yade { // Cannot have #include directive inside.


class Lin4NodeTetra_Lin4NodeTetra_InteractionElement : public DeformableCohesiveElement {
public:
	friend class If2_2xLin4NodeTetra_LinCohesiveStiffPropDampElastMat;
	virtual ~Lin4NodeTetra_Lin4NodeTetra_InteractionElement();
	void initialize(void);

	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS_INIT_CTOR_PY(Lin4NodeTetra_Lin4NodeTetra_InteractionElement,DeformableCohesiveElement,"Tetrahedral Deformable Element Composed of Nodes",
		,
		,
		createIndex(); /*ctor*/
		initialize();
		,
		/*py*/

	);
	// clang-format on
	DECLARE_LOGGER;

	REGISTER_CLASS_INDEX(Lin4NodeTetra_Lin4NodeTetra_InteractionElement, DeformableCohesiveElement);
};

REGISTER_SERIALIZABLE(Lin4NodeTetra_Lin4NodeTetra_InteractionElement);

}; // namespace yade
