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

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../GeoDa.h"

#include "Geom3D.h"

#define MAX_RANGE 3
#define MIN_RANGE -20

#ifndef GDA_SWAP
#define GDA_SWAP(x, y, t) ((t) = (x), (x) = (y), (y) = (t))
#endif

Baseball::Baseball()
{
    curquat = Quat::ident();

    trans[0]=0.0;
    trans[1]=0.0;
    trans[2]=0.0;

    ctr[0]=0.0;
    ctr[1]=0.0;
	
    ctr[2]=0.0;

    radius=1;
}

void Baseball::apply_transform()
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslated(trans[0], trans[1], trans[2]);
    glTranslated(ctr[0], ctr[1], ctr[2]);

    const Mat4 M=unit_quat_to_matrix(curquat);
    glMultMatrixd(M);

    glTranslated(-ctr[0], -ctr[1], -ctr[2]);
}

void Baseball::unapply_transform()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}



Mat4 Mat4::I()
{
    return Mat4(Vec4(1,0,0,0),Vec4(0,1,0,0),Vec4(0,0,1,0),Vec4(0,0,0,1));
}

inline double det(const Mat4& m) { return m[0] * cross(m[1], m[2], m[3]); }

inline double trace(const Mat4& m) { return m(0,0)+m(1,1)+m(2,2)+m(3,3); }

inline Mat4 transpose(const Mat4& m)
	{ return Mat4(m.col(0), m.col(1), m.col(2), m.col(3)); }



Mat4 translation_matrix(const Vec3& d)
{
    return Mat4(Vec4(1, 0, 0, d[0]),
		Vec4(0, 1, 0, d[1]),
		Vec4(0, 0, 1, d[2]),
		Vec4(0, 0, 0, 1));
}

Mat4 scaling_matrix(const Vec3& s)
{
    return Mat4(Vec4(s[0], 0,    0,    0),
		Vec4(0,    s[1], 0,    0),
		Vec4(0,    0,    s[2], 0),
		Vec4(0,    0,    0,    1));
}

Mat4 rotation_matrix_rad(double theta, const Vec3& axis)
{
    double c=cos(theta), s=sin(theta),
	xx=axis[0]*axis[0],  yy=axis[1]*axis[1],  zz=axis[2]*axis[2],
	xy=axis[0]*axis[1],  yz=axis[1]*axis[2],  xz=axis[0]*axis[2];

    double xs=axis[0]*s, ys=axis[1]*s, zs=axis[2]*s;

    Mat4 M;
    M(0,0)=xx*(1-c)+c;  M(0,1)=xy*(1-c)-zs;  M(0,2)=xz*(1-c)+ys;  M(0,3) = 0;
    M(1,0)=xy*(1-c)+zs;  M(1,1)=yy*(1-c)+c;  M(1,2)=yz*(1-c)-xs;  M(1,3)=0;
    M(2,0)=xz*(1-c)-ys;  M(2,1)=yz*(1-c)+xs;  M(2,2)=zz*(1-c)+c;  M(2,3)=0;
    M(3,0)=0;  M(3,1)=0;  M(3,2)=0;  M(3,3)=1;

    return M;
}

Mat4 perspective_matrix(double fovy, double aspect, double zmin, double zmax)
{
    double A, B;
    Mat4 M;

    if( zmax==0.0 )
    {
	A = B = 1.0;
    }
    else
    {
	A = (zmax+zmin)/(zmin-zmax);
	B = (2*zmax*zmin)/(zmin-zmax);
    }

    double f = 1.0/tan(fovy*M_PI/180.0/2.0);
    M(0,0) = f/aspect;
    M(1,1) = f;
    M(2,2) = A;
    M(2,3) = B;
    M(3,2) = -1;
    M(3,3) = 0;

    return M;
}

Mat4 lookat_matrix(const Vec3& from, const Vec3& at, const Vec3&  v_up)
{
    Vec3 up = v_up;;
	unitize(up);

    Vec3 f = at - from;   unitize(f);

    Vec3 s=f^up;
    Vec3 u=s^f;

    unitize(s);
    unitize(u);

    Mat4 M(Vec4(s, 0), Vec4(u, 0), Vec4(-f, 0), Vec4(0, 0, 0, 1));

    return M * translation_matrix(-from);
}

Mat4 viewport_matrix(double w, double h)
{
    return scaling_matrix(Vec3(w/2.0, -h/2.0, 1)) *
	translation_matrix(Vec3(1, -1, 0));
}

Mat4 operator*(const Mat4& n, const Mat4& m)
{
    Mat4 A;
    int i,j;

    for(i=0;i<4;i++)
	for(j=0;j<4;j++)
	    A(i,j) = n[i]*m.col(j);

    return A;
}

Mat4 adjoint(const Mat4& m)
{
    Mat4 A;

    A[0] = cross( m[1], m[2], m[3]);
    A[1] = cross(-m[0], m[2], m[3]);
    A[2] = cross( m[0], m[1], m[3]);
    A[3] = cross(-m[0], m[1], m[2]);
        
    return A;
}

double invert_cramer(Mat4& inv, const Mat4& m)
{
    Mat4 A = adjoint(m);
    double d = A[0] * m[0];

    if( d==0.0 )
	return 0.0;

    inv = transpose(A) / d;
    return d;
}




double invert(Mat4& B, const Mat4& m)
{
    Mat4 A = m;
    int i, j, k;
    double max, t, det, pivot;


    for (i=0; i<4; i++)                 
        for (j=0; j<4; j++)
            B(i, j) = (double)(i==j);

    det = 1.0;
    for (i=0; i<4; i++) {              
        max = -1.;
        for (k=i; k<4; k++)            
            if (fabs(A(k, i)) > max) {
                max = fabs(A(k, i));
                j = k;
            }
        if (max<=0.) return 0.;       
        if (j!=i) {                  
            for (k=i; k<4; k++)
                GDA_SWAP(A(i, k), A(j, k), t);
            for (k=0; k<4; k++)
                GDA_SWAP(B(i, k), B(j, k), t);
            det = -det;
        }
        pivot = A(i, i);
        det *= pivot;
        for (k=i+1; k<4; k++)          
            A(i, k) /= pivot;
        for (k=0; k<4; k++)
            B(i, k) /= pivot;

        for (j=i+1; j<4; j++) {        
            t = A(j, i);                
            for (k=i+1; k<4; k++)       
                A(j, k) -= A(i, k)*t;   
            for (k=0; k<4; k++)
                B(j, k) -= B(i, k)*t;
        }
    }

    for (i=4-1; i>0; i--) {             
        for (j=0; j<i; j++) {          
            t = A(j, i);              
            for (k=0; k<4; k++)         
                B(j, k) -= B(i, k)*t;
        }
    }

    return det;
}



inline Quat& Quat::operator=(const Quat& q) { v=q.v; s=q.s; return *this; }
inline Quat& Quat::operator+=(const Quat& q) { v+=q.v; s+=q.s; return *this; }
inline Quat& Quat::operator-=(const Quat& q) { v-=q.v; s-=q.s; return *this; }

inline Quat& Quat::operator=(double d)  { v=d;  s=d;  return *this; }
inline Quat& Quat::operator*=(double d) { v*=d; s*=d; return *this; }
inline Quat& Quat::operator/=(double d) { v/=d; s/=d; return *this; }

inline Quat Quat::ident() { return Quat(0, 0, 0, 1); }

Quat exp(const Quat& q)
{
    double theta = norm(q.vector());
    double c = cos(theta);

    if( theta > FEQ_EPS )
    {
	double s = sin(theta) / theta;
	return Quat( s*q.vector(), c);
    }
    else
	return Quat(q.vector(), c);
}

Quat log(const Quat& q)
{
    double scale = norm(q.vector());
    double theta = atan2(scale, q.scalar());

    if( scale > 0.0 )  scale=theta/scale;

    return Quat(scale*q.vector(), 0.0);
}

Quat axis_to_quat(const Vec3& a, double phi)
{
    Vec3 u = a;
    unitize(u);

    double s = sin(phi/2.0);
    return Quat(u[0]*s, u[1]*s, u[2]*s, cos(phi/2.0));
}

Mat4 quat_to_matrix(const Quat& q)
{
    Mat4 M;

    const double x = q.vector()[0];
    const double y = q.vector()[1];
    const double z = q.vector()[2];
    const double w = q.scalar();
    const double s = 2/norm(q);

    M(0,0)=1-s*(y*y+z*z); M(0,1)=s*(x*y-w*z);   M(0,2)=s*(x*z+w*y);   M(0,3)=0;
    M(1,0)=s*(x*y+w*z);   M(1,1)=1-s*(x*x+z*z); M(1,2)=s*(y*z-w*x);   M(1,3)=0;
    M(2,0)=s*(x*z-w*y);   M(2,1)=s*(y*z+w*x);   M(2,2)=1-s*(x*x+y*y); M(2,3)=0;
    M(3,0)=0;             M(3,1)=0;             M(3,2)=0;             M(3,3)=1;

    return M;
}

Mat4 unit_quat_to_matrix(const Quat& q)
{
    Mat4 M;

    const double x = q.vector()[0];
    const double y = q.vector()[1];
    const double z = q.vector()[2];
    const double w = q.scalar();

    M(0,0)=1-2*(y*y+z*z); M(0,1)=2*(x*y-w*z);   M(0,2)=2*(x*z+w*y);   M(0,3)=0;
    M(1,0)=2*(x*y+w*z);   M(1,1)=1-2*(x*x+z*z); M(1,2)=2*(y*z-w*x);   M(1,3)=0;
    M(2,0)=2*(x*z-w*y);   M(2,1)=2*(y*z+w*x);   M(2,2)=1-2*(x*x+y*y); M(2,3)=0;
    M(3,0)=0;             M(3,1)=0;             M(3,2)=0;             M(3,3)=1;

    return M;
}

Quat slerp(const Quat& from, const Quat& to, double t)
{
    const Vec3& v_from = from.vector();
    const Vec3& v_to = to.vector();
    const double s_from = from.scalar();
    const double s_to = to.scalar();

    double cosine = v_from*v_to + s_from*s_to;
    double sine = sqrt(1 - cosine*cosine);

    if( (1+cosine) < FEQ_EPS )
    {
	double A = sin( (1-t)*M_PI/2.0 );
	double B = sin( t*M_PI/2.0 );

	return Quat( A*v_from[0] + B*(-v_from[1]),
		     A*v_from[1] + B*(v_from[0]),
		     A*v_from[2] + B*(-s_from),
		     A*v_from[3] + B*(v_from[2]) );
    }

    double A, B;
    if( (1-cosine) < FEQ_EPS )
    {
	A = 1.0 - t;
	B = t;
    }
    else
    {
	double theta = acos(cosine);
	double sine = sin(theta);

	A = sin( (1-t)*theta ) / sine;
	B = sin( t*theta ) / sine;

    }

    return Quat( A*v_from + B*v_to,  A*s_from + B*s_to);
}




static void quat_to_sphere(const Quat& q, Vec3& from, Vec3& to)
{
    const Vec3& v = q.vector();

    double s = sqrt(v[0]*v[0] + v[1]*v[1]);
    if( s==0.0 )
	from = Vec3(0.0, 1.0, 0.0);
    else
	from = Vec3(-v[1]/s, v[0]/s, 0.0);

    to[0] = q.scalar()*from[0] - v[2]*from[1];
    to[1] = q.scalar()*from[1] + v[2]*from[2];
    to[2] = v[0]*from[1] - v[1]*from[0];

    if(q.scalar() < 0.0)  from = -from;
}


static Quat quat_from_sphere(const Vec3& from, const Vec3& to)
{
    Vec3 v;
    v[0] = from[1]*to[2] - from[2]*to[1];
    v[1] = from[2]*to[0] - from[0]*to[2];
    v[2] = from[0]*to[1] - from[1]*to[0];

    double s = from*to;

    return Quat(v, s);
}


Vec3 Arcball::proj_to_sphere(const Vec2& mouse)
{
    Vec2 p = (mouse - ball_ctr) / ball_radius;
    double mag = p*p;

    if( mag > 1.0 )
    {
	double s = sqrt(mag);
	return Vec3(p[0]/s, p[1]/s, 0.0);
    }
    else
    {
	return Vec3(p[0], p[1], sqrt(1-mag));
    }
}

void Arcball::update()
{

    if( is_dragging )
    {
	q_drag = quat_from_sphere(v_from, v_to);
	q_now = q_drag * q_down;
    }
}

Arcball::Arcball()
{
    ball_ctr = Vec2(0, 0);
    ball_radius = 1.0;

    q_now = Quat::ident();
    q_down = Quat::ident();
    q_drag = Quat::ident();

    is_dragging = false;
}

bool Arcball::mouse_down(int *where, int which)
{
    float vp[4];
    glGetFloatv(GL_VIEWPORT, vp);
    float W=vp[2], H=vp[3];

    if( which==1 )
    {
	is_dragging = true;
	Vec2 v( (2.0 * where[0] - W)/W,  (H - 2.0 * where[1])/H );
	v_from = proj_to_sphere(v);
	v_to = v_from;
    }

    return true;
}

bool Arcball::mouse_up(int *where, int which)
{
    is_dragging = false;
    q_down = q_now;
    q_drag = Quat::ident();

    return false;
}

bool Arcball::mouse_drag(int *where, int *last, int which)
{
    float vp[4];
    glGetFloatv(GL_VIEWPORT, vp);
    float W=vp[2], H=vp[3];

    float diam = 2*radius;

    if( which==1 )
    {
	Vec2 v( (2.0 * where[0] - W)/W,  (H - 2.0 * where[1])/H );
	v_to = proj_to_sphere(v);
    }
    else if( which==2 )
    {
	trans[0] += diam * (where[0] - last[0]) / W;
	trans[1] += diam * (last[1] - where[1]) / H;
    }
    else if( which==3 )
    {
	trans[2] += 0.02*diam*(where[1] - last[1]);
	if (trans[2] >= MAX_RANGE) trans[2] = MAX_RANGE;
	if (trans[2] <= MIN_RANGE) trans[2] = MIN_RANGE;
    }
    else
	return false;

    return true;
}

void Arcball::apply_transform()
{
    update();
    curquat = conjugate(q_now);
    Baseball::apply_transform();
}


void Arcball::update_animation()
{
}

void Arcball::get_transform(Vec3 & c, Vec3 &t, Quat & q)
{
  c = ctr;
  t = trans;
  q = q_now;
}

void Arcball::set_transform(const Vec3 & c, const Vec3 &t, const Quat & q)
{
  ctr = c;
  trans = t;
  q_now = q;
  q_down = q;
  q_drag = q;
}

int unproject_pixel(int *pixel, double *world, double z)
{
    GLdouble modelMatrix[16];
    GLdouble projMatrix[16];
    GLint viewport[4];

    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetIntegerv(GL_VIEWPORT, viewport);

    return gluUnProject(pixel[0], viewport[3]-pixel[1], z,
			modelMatrix, projMatrix, viewport,
			world, world+1, world+2);
}



SPlane::SPlane()
    :normal(0.0,0.0,0.0), origin(0.0,0.0,0.0)
{
}
SPlane::SPlane(const Vec3f& v1, const Vec3f& v2, const Vec3f& v3)
{
    origin = v1;
	normal = (v2-v1)^(v3-v1);
	unitize(normal);
}

SPlane::~SPlane()
{
}


bool SPlane::isPositive(Vec3f& v1, double* distance)
{
    double result = (v1-origin)*normal;
	if(distance)
		*distance = result;
	if(result >= 0)
		return true;
	else
	    return false;
}
