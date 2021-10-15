/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 * 
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEODA_CENTER_GEOM_3D_H__
#define __GEODA_CENTER_GEOM_3D_H__

const double FEQ_EPS = 1e-6;

#include <math.h>

#if !wxUSE_GLCANVAS
#error Please set wxUSE_GLCANVAS to 1 in setup.h.
#endif

#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/glu.h>
#    include <OpenGL/gl.h>
#  else
#    include <gl.h>
#    include <glu.h>
#  endif
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif

#ifndef M_PI
#define M_PI 3.1415926535
#endif

template<class T>
class TVec2 {
private:
    T elt[2];

public:
    // Standard constructors
    //
    TVec2(T s=0) { *this = s; }
    TVec2(T x, T y) { elt[0]=x; elt[1]=y; }

    // Copy constructors & assignment operators
    template<class U> TVec2(const TVec2<U>& v) { *this = v; }
    template<class U> TVec2(const U v[2]) { elt[0]=v[0]; elt[1]=v[1]; }
    template<class U> TVec2& operator=(const TVec2<U>& v)
	{ elt[0]=v[0];  elt[1]=v[1];  return *this; }
    TVec2& operator=(T s) { elt[0]=elt[1]=s; return *this; }


    // Descriptive interface
    //
    typedef T value_type;
    static int dim() { return 2; }


    // Access methods
    //
    operator       T*()       { return elt; }
    operator const T*() const { return elt; }

#ifndef HAVE_CASTING_LIMITS
    T& operator[](int i)       { return elt[i]; }
    T  operator[](int i) const { return elt[i]; }
    operator const T*()       { return elt; }
#endif

    // In-place arithmetic methods
    //
    inline TVec2& operator+=(const TVec2& v);
    inline TVec2& operator-=(const TVec2& v);
    inline TVec2& operator*=(T s);
    inline TVec2& operator/=(T s);
};

////////////////////////////////////////////////////////////////////////
//
// Method definitions
//
template<class T> inline TVec2<T>& TVec2<T>::operator+=(const TVec2<T>& v)
	{ elt[0] += v[0];   elt[1] += v[1];   return *this; }

template<class T> inline TVec2<T>& TVec2<T>::operator-=(const TVec2<T>& v)
	{ elt[0] -= v[0];   elt[1] -= v[1];   return *this; }

template<class T> inline TVec2<T>& TVec2<T>::operator*=(T s)
	{ elt[0] *= s;   elt[1] *= s;   return *this; }

template<class T> inline TVec2<T>& TVec2<T>::operator/=(T s)
	{ elt[0] /= s;   elt[1] /= s;   return *this; }

////////////////////////////////////////////////////////////////////////
//
// Operator defintions
//

template<class T>
inline TVec2<T> operator+(const TVec2<T> &u, const TVec2<T> &v)
	{ return TVec2<T>(u[0]+v[0], u[1]+v[1]); }

template<class T>
inline TVec2<T> operator-(const TVec2<T> &u, const TVec2<T> &v)
	{ return TVec2<T>(u[0]-v[0], u[1]-v[1]); }

template<class T> inline TVec2<T> operator-(const TVec2<T> &v)
	{ return TVec2<T>(-v[0], -v[1]); }

#if _MSC_VER==1200
//

  template<class T> inline TVec2<T> operator*(T s, const TVec2<T> &v)
	{ return TVec2<T>(v[0]*s, v[1]*s); }
  template<class T> inline TVec2<T> operator*(const TVec2<T> &v, T s)
	{ return s*v; }

  template<class T> inline TVec2<T> operator/(const TVec2<T> &v, T s)
	{ return TVec2<T>(v[0]/s, v[1]/s); }

#else
  template<class T, class N> inline TVec2<T> operator*(N s, const TVec2<T> &v)
	{ return TVec2<T>(v[0]*s, v[1]*s); }
  template<class T, class N> inline TVec2<T> operator*(const TVec2<T> &v, N s)
	{ return s*v; }

  template<class T, class N> inline TVec2<T> operator/(const TVec2<T> &v, N s)
	{ return TVec2<T>(v[0]/s, v[1]/s); }
#endif

template<class T> inline T operator*(const TVec2<T> &u, const TVec2<T>& v)
	{ return u[0]*v[0] + u[1]*v[1]; }

template<class T> inline TVec2<T> perp(const TVec2<T> &v)
	{ return TVec2<T>(v[1], -v[0]); }


template<class T> inline T norm2(const TVec2<T>& v)  { return v*v; }
template<class T> inline T norm(const TVec2<T>& v)   { return sqrt(norm2(v)); }

template<class T> inline void unitize(TVec2<T>& v)
{
    T l = norm2(v);
    if( l!=1.0 && l!=0.0 )  v /= sqrt(l);
}

typedef TVec2<double> Vec2;
typedef TVec2<float>  Vec2f;


template<class T>
class TVec3 {
private:
    T elt[3];

public:
    TVec3(T s=0) { *this = s; }
    TVec3(T x, T y, T z) { elt[0]=x; elt[1]=y; elt[2]=z; }

    template<class U> TVec3(const TVec3<U>& v) { *this = v; }
#ifndef STDMIX_INCLUDED
    template<class U> TVec3(const U v[3])
    	{ elt[0]=v[0]; elt[1]=v[1]; elt[2]=v[2]; }
#else
    TVec3(const float *v) { elt[0]=v[0]; elt[1]=v[1]; elt[2]=v[2]; }
    TVec3(const double *v) { elt[0]=v[0]; elt[1]=v[1]; elt[2]=v[2]; }
#endif
    template<class U> TVec3& operator=(const TVec3<U>& v)
	{ elt[0]=v[0];  elt[1]=v[1];  elt[2]=v[2];  return *this; }
    TVec3& operator=(T s) { elt[0]=elt[1]=elt[2]=s; return *this; }

    typedef T value_type;
    static int dim() { return 3; }


    operator       T*()       { return elt; }
    operator const T*() const { return elt; }

#ifndef HAVE_CASTING_LIMITS
    T& operator[](int i)       { return elt[i]; }
    T  operator[](int i) const { return elt[i]; }
    operator const T*()       { return elt; }
#endif


    inline TVec3& operator+=(const TVec3& v);
    inline TVec3& operator-=(const TVec3& v);
    inline TVec3& operator*=(T s);
    inline TVec3& operator/=(T s);
};

////////////////////////////////////////////////////////////////////////
//
// Method definitions
//

template<class T> inline TVec3<T>& TVec3<T>::operator+=(const TVec3<T>& v)
	{ elt[0] += v[0];   elt[1] += v[1];   elt[2] += v[2];  return *this; }

template<class T> inline TVec3<T>& TVec3<T>::operator-=(const TVec3<T>& v)
	{ elt[0] -= v[0];   elt[1] -= v[1];   elt[2] -= v[2];  return *this; }

template<class T> inline TVec3<T>& TVec3<T>::operator*=(T s)
	{ elt[0] *= s;   elt[1] *= s;   elt[2] *= s;  return *this; }

template<class T> inline TVec3<T>& TVec3<T>::operator/=(T s)
	{ elt[0] /= s;   elt[1] /= s;   elt[2] /= s;  return *this; }


////////////////////////////////////////////////////////////////////////
//
// Operator definitions
//

template<class T>
inline TVec3<T> operator+(const TVec3<T> &u, const TVec3<T>& v)
	{ return TVec3<T>(u[0]+v[0], u[1]+v[1], u[2]+v[2]); }

template<class T>
inline TVec3<T> operator-(const TVec3<T> &u, const TVec3<T>& v)
	{ return TVec3<T>(u[0]-v[0], u[1]-v[1], u[2]-v[2]); }

template<class T> inline TVec3<T> operator-(const TVec3<T> &v)
	{ return TVec3<T>(-v[0], -v[1], -v[2]); }

#if _MSC_VER==1200

  template<class T> inline TVec3<T> operator*(T s, const TVec3<T> &v)
	{ return TVec3<T>(v[0]*s, v[1]*s, v[2]*s); }
  template<class T> inline TVec3<T> operator*(const TVec3<T> &v, T s)
	{ return s*v; }

  template<class T> inline TVec3<T> operator/(const TVec3<T> &v, T s)
	{ return TVec3<T>(v[0]/s, v[1]/s, v[2]/s); }
#else
  template<class T, class N> inline TVec3<T> operator*(N s, const TVec3<T> &v)
	{ return TVec3<T>(v[0]*s, v[1]*s, v[2]*s); }
  template<class T, class N> inline TVec3<T> operator*(const TVec3<T> &v, N s)
	{ return s*v; }

  template<class T, class N> inline TVec3<T> operator/(const TVec3<T> &v, N s)
	{ return TVec3<T>(v[0]/s, v[1]/s, v[2]/s); }
#endif

template<class T> inline T operator*(const TVec3<T> &u, const TVec3<T>& v)
	{ return u[0]*v[0] + u[1]*v[1] + u[2]*v[2]; }

template<class T> inline TVec3<T> cross(const TVec3<T>& u, const TVec3<T>& v)
{
    return TVec3<T>( u[1]*v[2] - v[1]*u[2],
		-u[0]*v[2] + v[0]*u[2],
		 u[0]*v[1] - v[0]*u[1] );
}

template<class T>
inline TVec3<T> operator^(const TVec3<T>& u, const TVec3<T>& v)
	{ return cross(u, v); }


////////////////////////////////////////////////////////////////////////
//
// Misc. function definitions
//

template<class T> inline T norm2(const TVec3<T>& v)  { return v*v; }
template<class T> inline T norm(const TVec3<T>& v)   { return sqrt(norm2(v)); }

template<class T> inline void unitize(TVec3<T>& v)
{
    T l = norm2(v);
    if( l!=1.0 && l!=0.0 )  v /= sqrt(l);
}


typedef TVec3<double> Vec3;
typedef TVec3<float>  Vec3f;



template<class T>
class TVec4 {
private:
    T elt[4];

public:
    // Standard constructors
    //
    TVec4(T s=0) { *this = s; }
    TVec4(T x, T y, T z, T w) { elt[0]=x; elt[1]=y; elt[2]=z; elt[3]=w; }

    // Copy constructors & assignment operators
    template<class U> TVec4(const TVec4<U>& v) { *this = v; }
    template<class U> TVec4(const TVec3<U>& v,T w)
    	{ elt[0]=v[0];  elt[1]=v[1];  elt[2]=v[2];  elt[3]=w; }
    template<class U> TVec4(const U v[4])
    	{ elt[0]=v[0]; elt[1]=v[1]; elt[2]=v[2]; elt[3]=v[3]; }
    template<class U> TVec4& operator=(const TVec4<U>& v)
	{ elt[0]=v[0];  elt[1]=v[1];  elt[2]=v[2]; elt[3]=v[3]; return *this; }
    TVec4& operator=(T s) { elt[0]=elt[1]=elt[2]=elt[3]=s; return *this; }


    // Descriptive interface
    //
    typedef T value_type;
    static int dim() { return 4; }


    // Access methods
    //
    operator       T*()       { return elt; }
    operator const T*() const { return elt; }

#ifndef HAVE_CASTING_LIMITS
    T& operator[](int i)       { return elt[i]; }
    T  operator[](int i) const { return elt[i]; }
    operator const T*()       { return elt; }
#endif

    // Assignment and in-place arithmetic methods
    //
    inline TVec4& operator+=(const TVec4& v);
    inline TVec4& operator-=(const TVec4& v);
    inline TVec4& operator*=(T s);
    inline TVec4& operator/=(T s);
};

////////////////////////////////////////////////////////////////////////
//
// Method definitions
//

template<class T> inline TVec4<T>& TVec4<T>::operator+=(const TVec4<T>& v)
    { elt[0]+=v[0];  elt[1]+=v[1];  elt[2]+=v[2];  elt[3]+=v[3]; return *this;}

template<class T> inline TVec4<T>& TVec4<T>::operator-=(const TVec4<T>& v)
    { elt[0]-=v[0];  elt[1]-=v[1];  elt[2]-=v[2];  elt[3]-=v[3]; return *this;}

template<class T> inline TVec4<T>& TVec4<T>::operator*=(T s)
    { elt[0] *= s;   elt[1] *= s;   elt[2] *= s;  elt[3] *= s; return *this; }

template<class T> inline TVec4<T>& TVec4<T>::operator/=(T s)
    { elt[0] /= s;   elt[1] /= s;   elt[2] /= s;  elt[3] /= s; return *this; }


////////////////////////////////////////////////////////////////////////
//
// Operator definitions
//

template<class T>
inline TVec4<T> operator+(const TVec4<T> &u, const TVec4<T> &v)
	{ return TVec4<T>(u[0]+v[0], u[1]+v[1], u[2]+v[2], u[3]+v[3]); }

template<class T>
inline TVec4<T> operator-(const TVec4<T> &u, const TVec4<T>& v)
	{ return TVec4<T>(u[0]-v[0], u[1]-v[1], u[2]-v[2], u[3]-v[3]); }

template<class T> inline TVec4<T> operator-(const TVec4<T> &u)
	{ return TVec4<T>(-u[0], -u[1], -u[2], -u[3]); }

#if _MSC_VER==1200
//
  template<class T> inline TVec4<T> operator*(T s, const TVec4<T> &v)
	{ return TVec4<T>(v[0]*s, v[1]*s, v[2]*s, v[3]*s); }
  template<class T> inline TVec4<T> operator*(const TVec4<T> &v, T s)
	{ return s*v; }

  template<class T> inline TVec4<T> operator/(const TVec4<T> &v, T s)
	{ return TVec4<T>(v[0]/s, v[1]/s, v[2]/s, v[3]/s); }
#else
  template<class T, class N> inline TVec4<T> operator*(N s, const TVec4<T> &v)
	{ return TVec4<T>(v[0]*s, v[1]*s, v[2]*s, v[3]*s); }
  template<class T, class N> inline TVec4<T> operator*(const TVec4<T> &v, N s)
	{ return s*v; }

  template<class T, class N> inline TVec4<T> operator/(const TVec4<T> &v, N s)
	{ return TVec4<T>(v[0]/s, v[1]/s, v[2]/s, v[3]/s); }
#endif

template<class T> inline T operator*(const TVec4<T> &u, const TVec4<T> &v)
	{ return u[0]*v[0] + u[1]*v[1] + u[2]*v[2] + u[3]*v[3]; }

////////////////////////////////////////////////////////////////////////
//
// Misc. function definitions
//

template<class T>
inline TVec4<T> cross(const TVec4<T>& a, const TVec4<T>& b, const TVec4<T>& c)
{
    // Code adapted from VecLib4d.c in Graphics Gems V

    T d1 = (b[2] * c[3]) - (b[3] * c[2]);
    T d2 = (b[1] * c[3]) - (b[3] * c[1]);
    T d3 = (b[1] * c[2]) - (b[2] * c[1]);
    T d4 = (b[0] * c[3]) - (b[3] * c[0]);
    T d5 = (b[0] * c[2]) - (b[2] * c[0]);
    T d6 = (b[0] * c[1]) - (b[1] * c[0]);

    return TVec4<T>(- a[1] * d1 + a[2] * d2 - a[3] * d3,
		      a[0] * d1 - a[2] * d4 + a[3] * d5,
		    - a[0] * d2 + a[1] * d4 - a[3] * d6,
		      a[0] * d3 - a[1] * d5 + a[2] * d6);
}

template<class T> inline T norm2(const TVec4<T>& v) { return v*v; }
template<class T> inline T norm(const TVec4<T>& v)  { return sqrt(norm2(v)); }

template<class T> inline void unitize(TVec4<T>& v)
{
    T l = norm2(v);
    if( l!=1.0 && l!=0.0 )  v /= sqrt(l);
}


typedef TVec4<double> Vec4;
typedef TVec4<float>  Vec4f;


class Mat4
{
private:
    Vec4 row[4];

public:
    // Standard constructors
    //
    Mat4() { *this = 0.0; }
    Mat4(const Vec4& r0,const Vec4& r1,const Vec4& r2,const Vec4& r3)
    	{ row[0]=r0; row[1]=r1; row[2]=r2; row[3]=r3; }
    Mat4(const Mat4& m) { *this = m; }

    // Descriptive interface
    //
    typedef double value_type;
    typedef Vec4 vector_type;
    typedef Mat4 inverse_type;
    static int dim() { return 4; }

    // Access methods
    //
    double& operator()(int i, int j)       { return row[i][j]; }
    double  operator()(int i, int j) const { return row[i][j]; }
    Vec4&       operator[](int i)       { return row[i]; }
    const Vec4& operator[](int i) const { return row[i]; }
    inline Vec4 col(int i) const
        { return Vec4(row[0][i],row[1][i],row[2][i],row[3][i]); }

    operator       double*()       { return row[0]; }
    operator const double*()       { return row[0]; }
    operator const double*() const { return row[0]; }

    // Assignment methods
    //
    inline Mat4& operator=(const Mat4& m) {
		row[0] = m[0]; row[1] = m[1]; row[2] = m[2]; row[3] = m[3];
		return *this;
	}
	
    inline Mat4& operator=(double s) {
		row[0]=s;  row[1]=s;  row[2]=s;  row[3]=s;
		return *this;
	}

    inline Mat4& operator+=(const Mat4& m) {
		row[0] += m[0]; row[1] += m[1]; row[2] += m[2]; row[3] += m[3];
		return *this;
	}
	
    inline Mat4& operator-=(const Mat4& m) {
		row[0] -= m[0]; row[1] -= m[1]; row[2] -= m[2]; row[3] -= m[3];
		return *this;
	}
	
    inline Mat4& operator*=(double s) {
		row[0] *= s; row[1] *= s; row[2] *= s; row[3] *= s;
		return *this;
	}
	
    inline Mat4& operator/=(double s) 
	{
		row[0] /= s; row[1] /= s; row[2] /= s; row[3] /= s;
		return *this;
	}

    static Mat4 I();
};


////////////////////////////////////////////////////////////////////////
//
// Operator definitions
//

inline Mat4 operator+(const Mat4& n, const Mat4& m)
	{ return Mat4(n[0]+m[0], n[1]+m[1], n[2]+m[2], n[3]+m[3]); }

inline Mat4 operator-(const Mat4& n, const Mat4& m)
	{ return Mat4(n[0]-m[0], n[1]-m[1], n[2]-m[2], n[3]-m[3]); }

inline Mat4 operator-(const Mat4& n)
	{ return Mat4(-n[0], -n[1], -n[2], -n[3]); }

inline Mat4 operator*(double s, const Mat4& m)
	{ return Mat4(m[0]*s, m[1]*s, m[2]*s, m[3]*s); }
inline Mat4 operator*(const Mat4& m, double s)
	{ return s*m; }

inline Mat4 operator/(const Mat4& m, double s)
	{ return Mat4(m[0]/s, m[1]/s, m[2]/s, m[3]/s); }

inline Vec4 operator*(const Mat4& m, const Vec4& v)
	{ return Vec4(m[0]*v, m[1]*v, m[2]*v, m[3]*v); }

extern Mat4 operator*(const Mat4& n, const Mat4& m);

//
// Transform a homogeneous 3-vector and reproject into normal 3-space
//
inline Vec3 operator*(const Mat4& m, const Vec3& v)
{
    Vec4 u=Vec4(v,1);
    double w=m[3]*u;

    if(w==0.0)  return Vec3(m[0]*u, m[1]*u, m[2]*u);
    else        return Vec3(m[0]*u/w, m[1]*u/w, m[2]*u/w);
}

////////////////////////////////////////////////////////////////////////
//
// Transformations
//

extern Mat4 translation_matrix(const Vec3& delta);

extern Mat4 scaling_matrix(const Vec3& scale);

extern Mat4 rotation_matrix_rad(double theta, const Vec3& axis);

inline Mat4 rotation_matrix_deg(double theta, const Vec3& axis)
	{ return rotation_matrix_rad(theta*M_PI/180.0, axis); }

extern Mat4 perspective_matrix(double fovy, double aspect,
			       double zmin=0.0, double zmax=0.0);

extern Mat4 lookat_matrix(const Vec3& from, const Vec3& at, const Vec3& up);

extern Mat4 viewport_matrix(double w, double h);



class Quat
{
private:
    Vec3 v;			// Vector component
    double s;			// Scalar component

public:
    Quat()                                       { v=0.0; s=1.0; }
    Quat(double x, double y, double z, double w) { v[0]=x;v[1]=y;v[2]=z; s=w; }
    Quat(const Vec3& a, double b)                { v=a; s=b; }
    Quat(const Quat& q)                          { *this=q; }

    // Access methods
    const Vec3& vector() const { return v; }
    Vec3&       vector()       { return v; }
    double      scalar() const { return s; }
    double&     scalar()       { return s; }

    // Assignment and in-place arithmetic methods
    Quat& operator=(const Quat& q);
    Quat& operator+=(const Quat& q);
    Quat& operator-=(const Quat& q);
    Quat& operator=(double d);
    Quat& operator*=(double d);
    Quat& operator/=(double d);

    // Construction of standard quaternions
    static Quat ident();
};


inline Quat operator+(const Quat& q, const Quat& r)
	{ return Quat(q.vector()+r.vector(), q.scalar()+r.scalar()); }

inline Quat operator*(const Quat& q, const Quat& r)
{
    return Quat(cross(q.vector(),r.vector()) +
		r.scalar()*q.vector() +
		q.scalar()*r.vector(),
		q.scalar()*r.scalar() - q.vector()*r.vector());
}

inline Quat operator*(const Quat& q, double s)
	{ return Quat(q.vector()*s, q.scalar()*s); }
inline Quat operator*(double s, const Quat& q)
	{ return Quat(q.vector()*s, q.scalar()*s); }

inline Quat operator/(const Quat& q, double s)
	{ return Quat(q.vector()/s, q.scalar()/s); }


////////////////////////////////////////////////////////////////////////
//
// Standard functions on quaternions
//

inline double norm(const Quat& q)
	{ return q.scalar()*q.scalar() + norm2(q.vector()); }

inline Quat conjugate(const Quat& q) { return Quat(-q.vector(), q.scalar()); }
inline Quat inverse(const Quat& q) { return conjugate(q)/norm(q); }
inline void unitize(Quat& q)  { q /= norm(q); }

extern Quat exp(const Quat& q);
extern Quat log(const Quat& q);
extern Quat axis_to_quat(const Vec3& a, double phi);
extern Mat4 quat_to_matrix(const Quat& q);
extern Mat4 unit_quat_to_matrix(const Quat& q);
extern Quat slerp(const Quat& from, const Quat& to, double t);

class Baseball
{
public:
    Vec3 ctr;			// Describes bounding sphere of object
    double radius;		//

    Quat curquat;		// Current rotation of object
    Vec3 trans;			// Current translation of object

public:
    Baseball();

    // Required initialization method
    void bounding_sphere(const Vec3& v, double r) { ctr=v;	radius=r; }

    // Standard event interface provide by all Ball controllers
    virtual void update_animation() = 0;
    virtual bool mouse_down(int *where, int which) = 0;
    virtual bool mouse_up(int *where, int which) = 0;
    virtual bool mouse_drag(int *where, int *last, int which) = 0;

    // Interface for use during drawing to apply appropriate transformation
    virtual void apply_transform();
    virtual void unapply_transform();
};



class Arcball : public Baseball
{
private:
    Vec2 ball_ctr;
    double ball_radius;

    Quat q_now, q_down, q_drag;	// Quaternions describing rotation
    Vec3 v_from, v_to;		//

    bool is_dragging;

protected:
    Vec3 proj_to_sphere(const Vec2&);
    void update();


public:
    Arcball();

    virtual void update_animation();
    virtual bool mouse_down(int *where, int which);
    virtual bool mouse_up(int *where, int which);
    virtual bool mouse_drag(int *where, int *last, int which);

    virtual void apply_transform();
    virtual void get_transform(Vec3 & c, Vec3 &t, Quat & q);
    virtual void set_transform(const Vec3 & c, const Vec3 & t, const Quat & q); 
};



class SPlane  
{
public:
	SPlane();
	SPlane(Vec3f nor, Vec3f ori)
		: normal(nor), origin(ori) {};
    SPlane(const Vec3f& v1, const Vec3f& v2, const Vec3f& v3);
	virtual ~SPlane();
    
	Vec3f normal, origin;

	bool isPositive(Vec3f& v1, double* distance = NULL);
};

// Other utility methods
int unproject_pixel(int *pixel, double *world, double z);

#endif

