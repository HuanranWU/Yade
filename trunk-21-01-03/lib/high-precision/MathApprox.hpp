/*************************************************************************
*  2004 Olivier Galizzi                                                  *
*  2009 Luc Scholtes                                                     *
*  2010 Václav Šmilauer                                                  *
*  2020 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// note: only Law2_ScGeom_CapillaryPhys_Capillarity*.cpp uses this function fastInvCos0, I'm not sure where to put it.
//
// for history of fastInvCos0 see commits: 7b3ee83ec , 3809c4c1e0 , d83e6302b
// It was in file Math.hpp, template <typename Scalar> struct Math; but it is cleaned up.

// note: as working on https://gitlab.com/yade-dev/trunk/-/issues/97 goes forward, this file might get renamed.

#pragma once
#include <lib/high-precision/Real.hpp>

namespace forCtags {
struct MathApprox {
}; // for ctags
}

namespace yade {
namespace math {

	template <typename Scalar, int Level = levelOfRealHP<Scalar>> inline Scalar fastInvCos0(const Scalar& fValue)
	// note: this function is verbatim moved from Math.hpp
	{
		Scalar fRoot   = sqrt(((Scalar)1.0) - fValue);
		Scalar fResult = -(Scalar)0.0187293;
		fResult *= fValue;
		fResult += (Scalar)0.0742610;
		fResult *= fValue;
		fResult -= (Scalar)0.2121144;
		fResult *= fValue;
		fResult += (Scalar)1.5707288;
		fResult *= fRoot;
		return fResult;
	}

} // namespace math
} // namespace yade
