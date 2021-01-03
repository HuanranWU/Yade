/*************************************************************************
*  2019 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

// This file is to collect all namespace aliases inside yade namespace
// don't include here <boost/filesystem.hpp> or <Python.h>. Better to include them in places where they are needed.

namespace boost {
namespace python {
}
namespace filesystem {
}
}

namespace yade {
namespace py  = ::boost::python;
namespace bfs = ::boost::filesystem;
}
