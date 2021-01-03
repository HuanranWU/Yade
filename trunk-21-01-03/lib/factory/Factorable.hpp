/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#include "ClassFactory.hpp"

#include <sstream>
#include <string>

namespace yade { // Cannot have #include directive inside.

//! macro for registering both class and its base
#define REGISTER_CLASS_AND_BASE(cn, bcn)                                                                                                                       \
	REGISTER_CLASS_NAME(cn);                                                                                                                               \
	REGISTER_BASE_CLASS_NAME(bcn);

#define REGISTER_CLASS_NAME(cn)                                                                                                                                \
public:                                                                                                                                                        \
	virtual string getClassName() const { return #cn; };

// FIXME[1] - that macro below should go to another class! factorable has nothing to do with inheritance tree.

#define REGISTER_BASE_CLASS_NAME(bcn)                                                                                                                          \
public:                                                                                                                                                        \
	virtual string getBaseClassName(unsigned int i = 0) const                                                                                              \
	{                                                                                                                                                      \
		string             token;                                                                                                                      \
		vector<string>     tokens;                                                                                                                     \
		string             str = #bcn;                                                                                                                 \
		std::istringstream iss(str);                                                                                                                   \
		while (!iss.eof()) {                                                                                                                           \
			iss >> token;                                                                                                                          \
			tokens.push_back(token);                                                                                                               \
		}                                                                                                                                              \
		if (i >= token.size())                                                                                                                         \
			return "";                                                                                                                             \
		else                                                                                                                                           \
			return tokens[i];                                                                                                                      \
	}                                                                                                                                                      \
                                                                                                                                                               \
public:                                                                                                                                                        \
	virtual int getBaseClassNumber()                                                                                                                       \
	{                                                                                                                                                      \
		string             token;                                                                                                                      \
		vector<string>     tokens;                                                                                                                     \
		string             str = #bcn;                                                                                                                 \
		std::istringstream iss(str);                                                                                                                   \
		while (!iss.eof()) {                                                                                                                           \
			iss >> token;                                                                                                                          \
			tokens.push_back(token);                                                                                                               \
		}                                                                                                                                              \
		return tokens.size();                                                                                                                          \
	}


class Factorable {
public:
	Factorable() { }
	virtual ~Factorable() { }

	virtual string getBaseClassName(unsigned int = 0) const { return ""; }
	virtual int    getBaseClassNumber() { return 0; }

	REGISTER_CLASS_NAME(Factorable);
};

} // namespace yade
