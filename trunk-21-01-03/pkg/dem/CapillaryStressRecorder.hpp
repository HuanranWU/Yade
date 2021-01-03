/*************************************************************************
*  Copyright (C) 2006 by luc Scholtes                                    *
*  luc.scholtes@hmg.inpg.fr                                              *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once
#include <pkg/common/Recorder.hpp>

#include <fstream>
#include <string>

namespace yade { // Cannot have #include directive inside.

class TriaxialCompressionEngine;

class CapillaryStressRecorder : public Recorder {
private:
	shared_ptr<TriaxialCompressionEngine> triaxialCompressionEngine;

public:
	virtual void action();

	// clang-format off
	YADE_CLASS_BASE_DOC_ATTRS_CTOR(CapillaryStressRecorder,Recorder,"Records information from capillary meniscii on samples submitted to triaxial compressions. Classical sign convention (tension positiv) is used for capillary stresses. -> New formalism needs to be tested!!!",,initRun=true;);
	// clang-format on
	DECLARE_LOGGER;
};
REGISTER_SERIALIZABLE(CapillaryStressRecorder);

} // namespace yade
