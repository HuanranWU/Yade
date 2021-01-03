/*************************************************************************
*  2012-2020 Václav Šmilauer                                             *
*  2020      Janek Kozicki                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

// functions defined in the respective .cpp files
// N is the level of RealHP<N>
template <int N> void expose_matrices1(bool notDuplicate, const py::scope& topScope);
template <int N> void expose_matrices2(bool notDuplicate, const py::scope& topScope);
template <int N> void expose_vectors1(bool notDuplicate, const py::scope& topScope);
template <int N> void expose_vectors2(bool notDuplicate, const py::scope& topScope);
template <int N> void expose_boxes(bool notDuplicate, const py::scope& topScope);
template <int N> void expose_quaternion(bool notDuplicate, const py::scope& topScope);
template <int N> void expose_complex1(bool notDuplicate, const py::scope& topScope); // does nothing if _COMPLEX_SUPPORT is not #defined
template <int N> void expose_complex2(bool notDuplicate, const py::scope& topScope); // does nothing if _COMPLEX_SUPPORT is not #defined
template <int N> void expose_converters(bool notDuplicate, const py::scope& topScope);
