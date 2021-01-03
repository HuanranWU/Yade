/*************************************************************************
*  Copyright (C) 2004 by Janek Kozicki                                   *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#pragma once

#ifndef YADE_OPENGL
#error "This build doesn't support openGL. Therefore, this header must not be used."
#endif

#include <lib/base/Math.hpp>
#include <type_traits>

// https://stackoverflow.com/questions/7064039/how-to-prevent-non-specialized-template-instantiation/7064062
template <typename T> struct dontCallThis : std::false_type {
};

#include <GL/gl.h>
#include <GL/glut.h>

namespace forCtags {
struct OpenGLWrapper {
}; // for ctags
}

//namespace yade { // Does not work with ABI format of freeglut, possibly due to extern "C". OpenGLWrapper must be in top namespace.

///	Primary Templates

template <typename Type> inline void glRotate(Type, Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glScale(Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glScalev(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glTranslate(Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glTranslatev(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glVertex2(Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glVertex3(Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glVertex2v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glVertex3v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glNormal3(Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glNormal3v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glIndex(Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glIndexv(Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glColor3(Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glColor4(Type, Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glColor3v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glColor4v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glTexCoord1(Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glTexCoord2(Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glTexCoord3(Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glTexCoord4(Type, Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glTexCoord1v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glTexCoord2v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glTexCoord3v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glTexCoord4v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glRasterPos2(Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glRasterPos3(Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glRasterPos4(Type, Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glRasterPos2v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glRasterPos3v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glRasterPos4v(const Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glRect(Type, Type, Type, Type) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glMaterial(GLenum face, GLenum pname, Type param) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glMaterialv(GLenum face, GLenum pname, Type param) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }
template <typename Type> inline void glMultMatrix(const Type*) { static_assert(dontCallThis<Type>::value, "Bad arg Type"); }

#if defined(YADE_REAL_BIT) and (YADE_REAL_BIT != 64)
template <> inline void glMultMatrix<yade::Real>(const yade::Real* m)
{
	double mm[16];
	for (int i = 0; i < 16; i++)
		mm[i] = static_cast<double>(m[i]);
	glMultMatrixd(mm);
}

///	Template Specializations

template <> inline void glRotate<yade::Real>(yade::Real angle, yade::Real x, yade::Real y, yade::Real z)
{
	glRotated((static_cast<double>(angle)), (static_cast<double>(x)), (static_cast<double>(y)), (static_cast<double>(z)));
}

template <typename Type1, typename Type2, typename Type3, typename Type4> inline void glRotate(Type1 angle, Type2 x, Type3 y, Type4 z)
{
	glRotated((static_cast<double>(angle)), (static_cast<double>(x)), (static_cast<double>(y)), (static_cast<double>(z)));
}

template <> inline void glScale<yade::Real>(yade::Real x, yade::Real y, yade::Real z)
{
	glScaled((static_cast<double>(x)), (static_cast<double>(y)), (static_cast<double>(z)));
}

template <> inline void glTranslate<yade::Real>(yade::Real x, yade::Real y, yade::Real z)
{
	glTranslated((static_cast<double>(x)), (static_cast<double>(y)), (static_cast<double>(z)));
}

template <typename Type1, typename Type2, typename Type3> inline void glTranslate(Type1 x, Type2 y, Type3 z)
{
	glTranslated((static_cast<double>(x)), (static_cast<double>(y)), (static_cast<double>(z)));
}

template <> inline void glVertex2<yade::Real>(yade::Real x, yade::Real y) { glVertex2d((static_cast<double>(x)), (static_cast<double>(y))); }

template <> inline void glVertex3<yade::Real>(yade::Real x, yade::Real y, yade::Real z)
{
	glVertex3d((static_cast<double>(x)), (static_cast<double>(y)), (static_cast<double>(z)));
}
template <typename Type1, typename Type2, typename Type3> inline void glVertex3(Type1 x, Type2 y, Type3 z)
{
	glVertex3d((static_cast<double>(x)), (static_cast<double>(y)), (static_cast<double>(z)));
}

template <> inline void glVertex2v<yade::Vector3r>(const yade::Vector3r v)
{
	const double mm[3] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])) };
	glVertex2dv(mm);
}
template <> inline void glVertex3v<yade::Vector3r>(const yade::Vector3r v)
{
	const double mm[3] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])) };
	glVertex3dv(mm);
}

template <> inline void glNormal3v<yade::Vector3r>(const yade::Vector3r v)
{
	const double mm[3] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])) };
	glNormal3dv(mm);
}
template <> inline void glIndexv<const yade::Vector3r>(const yade::Vector3r c)
{
	const double mm[3] = { (static_cast<double>(c[0])), (static_cast<double>(c[1])), (static_cast<double>(c[2])) };
	glIndexdv(mm);
}
template <> inline void glColor3v<yade::Vector3r>(const yade::Vector3r v)
{
	const double mm[3] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])) };
	glColor3dv(mm);
}
template <> inline void glColor4v<yade::Vector4r>(const yade::Vector4r v)
{
	const double m4[4] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])), (static_cast<double>(v[3])) };
	glColor4dv(m4);
}
template <> inline void glTexCoord1v<yade::Vector3r>(const yade::Vector3r v)
{
	const double mm[3] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])) };
	glTexCoord1dv(mm);
}
template <> inline void glTexCoord2v<yade::Vector3r>(const yade::Vector3r v)
{
	const double mm[3] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])) };
	glTexCoord2dv(mm);
}
template <> inline void glTexCoord3v<yade::Vector3r>(const yade::Vector3r v)
{
	const double mm[3] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])) };
	glTexCoord3dv(mm);
}
template <> inline void glTexCoord4v<yade::Vector4r>(const yade::Vector4r v)
{
	const double m4[4] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])), (static_cast<double>(v[3])) };
	glTexCoord4dv(m4);
}
template <> inline void glRasterPos2v<yade::Vector3r>(const yade::Vector3r v)
{
	const double mm[3] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])) };
	glRasterPos2dv(mm);
}
template <> inline void glRasterPos3v<yade::Vector3r>(const yade::Vector3r v)
{
	const double mm[3] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])) };
	glRasterPos3dv(mm);
}
template <> inline void glRasterPos4v<yade::Vector4r>(const yade::Vector4r v)
{
	const double m4[4] = { (static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])), (static_cast<double>(v[3])) };
	glRasterPos4dv(m4);
}


template <> inline void glNormal3<yade::Real>(yade::Real nx, yade::Real ny, yade::Real nz)
{
	glNormal3d((static_cast<double>(nx)), (static_cast<double>(ny)), (static_cast<double>(nz)));
}
template <typename Type1, typename Type2, typename Type3> inline void glNormal3(Type1 x, Type2 y, Type3 z)
{
	glNormal3d((static_cast<double>(x)), (static_cast<double>(y)), (static_cast<double>(z)));
}


template <> inline void glIndex<yade::Real>(yade::Real c) { glIndexd(static_cast<double>(c)); }
template <> inline void glColor3<yade::Real>(yade::Real red, yade::Real green, yade::Real blue)
{
	glColor3d((static_cast<double>(red)), (static_cast<double>(green)), (static_cast<double>(blue)));
}
template <> inline void glColor4<yade::Real>(yade::Real red, yade::Real green, yade::Real blue, yade::Real alpha)
{
	glColor4d((static_cast<double>(red)), (static_cast<double>(green)), (static_cast<double>(blue)), (static_cast<double>(alpha)));
}
template <> inline void glTexCoord1<yade::Real>(yade::Real s) { glTexCoord1d(static_cast<double>(s)); }
template <> inline void glTexCoord2<yade::Real>(yade::Real s, yade::Real t) { glTexCoord2d((static_cast<double>(s)), (static_cast<double>(t))); }
template <> inline void glTexCoord3<yade::Real>(yade::Real s, yade::Real t, yade::Real r)
{
	glTexCoord3d((static_cast<double>(s)), (static_cast<double>(t)), (static_cast<double>(r)));
}
template <> inline void glTexCoord4<yade::Real>(yade::Real s, yade::Real t, yade::Real r, yade::Real q)
{
	glTexCoord4d((static_cast<double>(s)), (static_cast<double>(t)), (static_cast<double>(r)), (static_cast<double>(q)));
}
template <> inline void glRasterPos2<yade::Real>(yade::Real x, yade::Real y) { glRasterPos2d((static_cast<double>(x)), (static_cast<double>(y))); }
template <> inline void glRasterPos3<yade::Real>(yade::Real x, yade::Real y, yade::Real z)
{
	glRasterPos3d((static_cast<double>(x)), (static_cast<double>(y)), (static_cast<double>(z)));
}
template <> inline void glRasterPos4<yade::Real>(yade::Real x, yade::Real y, yade::Real z, yade::Real w)
{
	glRasterPos4d((static_cast<double>(x)), (static_cast<double>(y)), (static_cast<double>(z)), (static_cast<double>(w)));
}
template <> inline void glRect<yade::Real>(yade::Real x1, yade::Real y1, yade::Real x2, yade::Real y2)
{
	glRectd((static_cast<double>(x1)), (static_cast<double>(y1)), (static_cast<double>(x2)), (static_cast<double>(y2)));
}

inline void gluCylinder(GLUquadric* a, yade::Real b, yade::Real c, yade::Real d, int e, int f)
{
	gluCylinder(a, (static_cast<double>(b)), (static_cast<double>(c)), (static_cast<double>(d)), e, f);
}
inline void glutSolidSphere(yade::Real a, int b, int c) { glutSolidSphere(static_cast<double>(a), b, c); }
inline void glutWireSphere(yade::Real a, int b, int c) { glutWireSphere(static_cast<double>(a), b, c); }
inline void glutSolidTorus(yade::Real a, yade::Real b, int c, int d) { glutSolidTorus((static_cast<double>(a)), (static_cast<double>(b)), c, d); }
inline void glutWireTorus(yade::Real a, yade::Real b, int c, int d) { glutWireTorus((static_cast<double>(a)), (static_cast<double>(b)), c, d); }
inline void glutSolidCube(yade::Real a) { glutSolidCube(static_cast<double>(a)); }
inline void glutWireCube(yade::Real a) { glutWireCube(static_cast<double>(a)); }

template <typename Type1, typename Type2, typename Type3, typename Type4> inline void glClearColor(Type1 a, Type2 b, Type3 c, Type4 d)
{
	glClearColor(GLclampf(static_cast<double>(a)), GLclampf(static_cast<double>(b)), GLclampf(static_cast<double>(c)), GLclampf(static_cast<double>(d)));
}
inline void glLineWidth(yade::Real a) { glLineWidth(GLfloat(static_cast<double>(a))); }

#if (YADE_REAL_BIT == 32)
// this is only testing compilation with Real as float. To avoid ambiguous function calls int↔float, int↔double. Such low precision Real is not useful at all. Except for testing builds ;)
inline void glutSolidCube(int a) { glutSolidCube(static_cast<double>(a)); }
inline void glutWireCube(int a) { glutWireCube(static_cast<double>(a)); }
#endif

#else

template <> inline void glVertex2v<yade::Vector3r>(const yade::Vector3r v) { glVertex2dv((double*)&v); }
template <> inline void glVertex3v<yade::Vector3r>(const yade::Vector3r v) { glVertex3dv((double*)&v); }
template <> inline void glNormal3v<yade::Vector3r>(const yade::Vector3r v) { glNormal3dv((double*)&v); }
template <> inline void glIndexv<const yade::Vector3r>(const yade::Vector3r c) { glIndexdv((double*)&c); }
template <> inline void glColor3v<yade::Vector3r>(const yade::Vector3r v) { glColor3dv((double*)&v); }
template <> inline void glColor4v<yade::Vector4r>(const yade::Vector4r v) { glColor4dv((double*)&v); }
template <> inline void glTexCoord1v<yade::Vector3r>(const yade::Vector3r v) { glTexCoord1dv((double*)&v); }
template <> inline void glTexCoord2v<yade::Vector3r>(const yade::Vector3r v) { glTexCoord2dv((double*)&v); }
template <> inline void glTexCoord3v<yade::Vector3r>(const yade::Vector3r v) { glTexCoord3dv((double*)&v); }
template <> inline void glTexCoord4v<yade::Vector4r>(const yade::Vector4r v) { glTexCoord4dv((double*)&v); }
template <> inline void glRasterPos2v<yade::Vector3r>(const yade::Vector3r v) { glRasterPos2dv((double*)&v); }
template <> inline void glRasterPos3v<yade::Vector3r>(const yade::Vector3r v) { glRasterPos3dv((double*)&v); }
template <> inline void glRasterPos4v<yade::Vector4r>(const yade::Vector4r v) { glRasterPos4dv((double*)&v); }

inline void glClearColor(double a, double b, double c, double d) { glClearColor(GLclampf(a), GLclampf(b), GLclampf(c), GLclampf(d)); }
inline void glLineWidth(double a) { glLineWidth(GLfloat(a)); }

#endif

template <> inline void glMultMatrix<double>(const double* m) { glMultMatrixd(m); }

template <> inline void glRotate<double>(double angle, double x, double y, double z) { glRotated(angle, x, y, z); }

template <> inline void glScale<double>(double x, double y, double z) { glScaled(x, y, z); }
template <> inline void glScalev<yade::Vector3r>(const yade::Vector3r v)
{
	glScaled((static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])));
}

template <> inline void glTranslate<double>(double x, double y, double z) { glTranslated(x, y, z); }
template <> inline void glTranslatev<yade::Vector3r>(const yade::Vector3r v)
{
	glTranslated((static_cast<double>(v[0])), (static_cast<double>(v[1])), (static_cast<double>(v[2])));
}

template <> inline void glVertex2<double>(double x, double y) { glVertex2d(x, y); }
template <> inline void glVertex2<int>(int x, int y) { glVertex2i(x, y); }

template <> inline void glVertex3<double>(double x, double y, double z) { glVertex3d(x, y, z); }
template <> inline void glVertex3<int>(int x, int y, int z) { glVertex3i(x, y, z); }

template <> inline void glVertex2v<yade::Vector3i>(const yade::Vector3i v) { glVertex2iv((int*)&v); }
template <> inline void glVertex3v<yade::Vector3i>(const yade::Vector3i v) { glVertex3iv((int*)&v); }


template <> inline void glNormal3<double>(double nx, double ny, double nz) { glNormal3d(nx, ny, nz); }
template <> inline void glNormal3<int>(int nx, int ny, int nz) { glNormal3i(nx, ny, nz); }
template <> inline void glNormal3v<yade::Vector3i>(const yade::Vector3i v) { glNormal3iv((int*)&v); }

template <> inline void glIndex<double>(double c) { glIndexd(c); }
template <> inline void glIndex<int>(int c) { glIndexi(c); }
template <> inline void glIndex<unsigned char>(unsigned char c) { glIndexub(c); }
template <> inline void glIndexv<const yade::Vector3i>(const yade::Vector3i c) { glIndexiv((int*)&c); }

template <> inline void glColor3<double>(double red, double green, double blue) { glColor3d(red, green, blue); }
template <> inline void glColor3<int>(int red, int green, int blue) { glColor3i(red, green, blue); }
template <> inline void glColor3v<yade::Vector3i>(const yade::Vector3i v) { glColor3iv((int*)&v); }

template <> inline void glColor4<double>(double red, double green, double blue, double alpha) { glColor4d(red, green, blue, alpha); }
template <> inline void glColor4<int>(int red, int green, int blue, int alpha) { glColor4i(red, green, blue, alpha); }
template <> inline void glColor4v<yade::Vector4i>(const yade::Vector4i v) { glColor4iv((int*)&v); }

template <> inline void glTexCoord1<double>(double s) { glTexCoord1d(s); }
template <> inline void glTexCoord1<int>(int s) { glTexCoord1i(s); }

template <> inline void glTexCoord2<double>(double s, double t) { glTexCoord2d(s, t); }
template <> inline void glTexCoord2<int>(int s, int t) { glTexCoord2i(s, t); }

template <> inline void glTexCoord3<double>(double s, double t, double r) { glTexCoord3d(s, t, r); }
template <> inline void glTexCoord3<int>(int s, int t, int r) { glTexCoord3i(s, t, r); }

template <> inline void glTexCoord4<double>(double s, double t, double r, double q) { glTexCoord4d(s, t, r, q); }
template <> inline void glTexCoord4<int>(int s, int t, int r, int q) { glTexCoord4i(s, t, r, q); }

template <> inline void glTexCoord1v<yade::Vector3i>(const yade::Vector3i v) { glTexCoord1iv((int*)&v); }
template <> inline void glTexCoord2v<yade::Vector3i>(const yade::Vector3i v) { glTexCoord2iv((int*)&v); }
template <> inline void glTexCoord3v<yade::Vector3i>(const yade::Vector3i v) { glTexCoord3iv((int*)&v); }
template <> inline void glTexCoord4v<yade::Vector4i>(const yade::Vector4i v) { glTexCoord4iv((int*)&v); }

template <> inline void glRasterPos2<double>(double x, double y) { glRasterPos2d(x, y); }
template <> inline void glRasterPos2<int>(int x, int y) { glRasterPos2i(x, y); }

template <> inline void glRasterPos3<double>(double x, double y, double z) { glRasterPos3d(x, y, z); }
template <> inline void glRasterPos3<int>(int x, int y, int z) { glRasterPos3i(x, y, z); }

template <> inline void glRasterPos4<double>(double x, double y, double z, double w) { glRasterPos4d(x, y, z, w); }
template <> inline void glRasterPos4<int>(int x, int y, int z, int w) { glRasterPos4i(x, y, z, w); }


template <> inline void glRasterPos2v<yade::Vector3i>(const yade::Vector3i v) { glRasterPos2iv((int*)&v); }
template <> inline void glRasterPos3v<yade::Vector3i>(const yade::Vector3i v) { glRasterPos3iv((int*)&v); }
template <> inline void glRasterPos4v<yade::Vector4i>(const yade::Vector4i v) { glRasterPos4iv((int*)&v); }


template <> inline void glRect<double>(double x1, double y1, double x2, double y2) { glRectd(x1, y1, x2, y2); }
template <> inline void glRect<int>(int x1, int y1, int x2, int y2) { glRecti(x1, y1, x2, y2); }

template <> inline void glMaterial<double>(GLenum face, GLenum pname, double param) { glMaterialf(face, pname, float(param)); }
template <> inline void glMaterial<int>(GLenum face, GLenum pname, int param) { glMateriali(face, pname, param); }
template <> inline void glMaterialv<yade::Vector4i>(GLenum face, GLenum pname, const yade::Vector4i params) { glMaterialiv(face, pname, (int*)&params); }

template <> inline void glMaterialv<yade::Vector4r>(GLenum face, GLenum pname, const yade::Vector4r params)
{
	const GLfloat _p[4] = { static_cast<float>(params[0]), static_cast<float>(params[1]), static_cast<float>(params[2]), static_cast<float>(params[3]) };
	glMaterialfv(face, pname, _p);
}
template <> inline void glMaterialv<yade::Vector3r>(GLenum face, GLenum pname, const yade::Vector3r params)
{
	const GLfloat _p[4] = { static_cast<float>(params[0]), static_cast<float>(params[1]), static_cast<float>(params[2]), static_cast<float>(1) };
	glMaterialfv(face, pname, _p);
}


template <typename Type> inline void glOneWire(Type& t, unsigned int a, unsigned int b)
{
	glVertex3v(t->v[a]);
	glVertex3v(t->v[b]);
}

template <typename Type> inline void glOneFace(Type& t, unsigned int a, unsigned int b, unsigned int c)
{
	const yade::Vector3r center = (t->v[0] + t->v[1] + t->v[2] + t->v[3]) * .25;
	yade::Vector3r       n      = (t->v[b] - t->v[a]).cross(t->v[c] - t->v[a]);
	n.normalize();
	const yade::Vector3r faceCenter = (t->v[a] + t->v[b] + t->v[c]) / 3.;
	if ((faceCenter - center).dot(n) < 0)
		n = -n;
	glNormal3v(n);
	glVertex3v(t->v[a]);
	glVertex3v(t->v[b]);
	glVertex3v(t->v[c]);
}

//} // namespace yade
