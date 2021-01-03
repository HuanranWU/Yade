/* pygts - python package for the manipulation of triangulated surfaces
 *
 *   Copyright (C) 2009 Thomas J. Duck
 *   All rights reserved.
 *
 *   Thomas J. Duck <tom.duck@dal.ca>
 *   Department of Physics and Atmospheric Science,
 *   Dalhousie University, Halifax, Nova Scotia, Canada, B3H 3J5
 *
 * NOTICE
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the
 *   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *   Boston, MA 02111-1307, USA.
 */

#ifndef __PYGTS_H__
#define __PYGTS_H__

#ifndef PYGTS_DEBUG
#define PYGTS_DEBUG 1
#endif /* PYGTS_DEBUG */

// XXX never do #include<Python.h>, see https://www.boost.org/doc/libs/1_71_0/libs/python/doc/html/building/include_issues.html
#include <boost/python/detail/wrap_python.hpp>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <structmember.h>

/* Defined for arrayobject.h which is only included where needed */
#define PY_ARRAY_UNIQUE_SYMBOL PYGTS

#include <glib.h>
#include <gts.h>

// https://codeyarns.com/2014/03/11/how-to-selectively-ignore-a-gcc-warning/
// https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
// Code that generates this warning, Note: we cannot do this trick in yade. If we have a warning in yade, we have to fix it! See also https://gitlab.com/yade-dev/trunk/merge_requests/73
// This method will work once g++ bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431#c34 is fixed.

#include "edge.h"
#include "face.h"
#include "object.h"
#include "point.h"
#include "segment.h"
#include "surface.h"
#include "triangle.h"
#include "vertex.h"

#include "cleanup.h"

#pragma GCC diagnostic pop

// used in several cpp files without having any good header for it
// defined in pygts.cpp
FILE* FILE_from_py_file__raises(PyObject* f_, const char* mode);

// helpers for py3k compatibility
#if PY_MAJOR_VERSION < 3
#ifndef PyLong_AsLong
#define PyLong_AsLong PyInt_AsLong
#endif
#endif


#endif /* __PYGTS_H__ */
