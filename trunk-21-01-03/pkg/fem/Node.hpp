/*************************************************************************
*  Copyright (C) 2013 by Burak ER                                 	 *
*									 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#ifndef NODE_HPP_
#define NODE_HPP_
#include <core/Shape.hpp>

namespace yade { // Cannot have #include directive inside.


class Node : public Shape {
public:
	Node(Real _radius)
	        : radius(_radius)
	{
		createIndex();
	}
	virtual ~Node();
	// clang-format off
		YADE_CLASS_BASE_DOC_ATTRS_CTOR(Node,Shape,"Geometry of node particle.",
		((Real,radius,0.1,,"Radius [m]")),
		createIndex(); /*ctor*/
	);
	// clang-format on
	REGISTER_CLASS_INDEX(Node, Shape);
};

REGISTER_SERIALIZABLE(Node);

}; // namespace yade

#endif
