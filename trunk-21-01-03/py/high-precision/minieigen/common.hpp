/*************************************************************************
*  2012-2020 Václav Šmilauer                                             *
*  2020      Janek Kozicki                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once
// common types, funcs, includes; should be included by all other files

// BEGIN workaround for
// * http://eigen.tuxfamily.org/bz/show_bug.cgi?id=528
// * https://sourceforge.net/tracker/index.php?func=detail&aid=3584127&group_id=202880&atid=983354
// (only needed with gcc <= 4.7)
#include <stdlib.h>
#include <sys/stat.h>
// END workaround

#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <Eigen/Geometry>
#include <Eigen/SVD>

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/python.hpp>
namespace py = boost::python;
#include <boost/lexical_cast.hpp>
#include <boost/static_assert.hpp>

/*** getters and setters with bound guards ***/
static inline void IDX_CHECK(Index i, Index MAX)
{
	if (i < 0 || i >= MAX) {
		PyErr_SetString(
		        PyExc_IndexError,
		        ("Index " + boost::lexical_cast<std::string>(i) + " out of range 0.." + boost::lexical_cast<std::string>(MAX - 1)).c_str());
		py::throw_error_already_set();
	}
}
static inline void IDX2_CHECKED_TUPLE_INTS(py::tuple tuple, const Index max2[2], Index arr2[2])
{
	Index l = py::len(tuple);
	if (l != 2) {
		PyErr_SetString(PyExc_IndexError, "Index must be integer or a 2-tuple");
		py::throw_error_already_set();
	}
	for (int _i = 0; _i < 2; _i++) {
		py::extract<Index> val(tuple[_i]);
		if (!val.check()) {
			PyErr_SetString(PyExc_ValueError, ("Unable to convert " + boost::lexical_cast<std::string>(_i) + "-th index to integer.").c_str());
			py::throw_error_already_set();
		}
		Index v = val();
		IDX_CHECK(v, max2[_i]);
		arr2[_i] = v;
	}
}

static inline std::string object_class_name(const py::object& obj) { return py::extract<std::string>(obj.attr("__class__").attr("__name__"))(); }
