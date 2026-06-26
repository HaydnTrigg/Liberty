#pragma once
#ifndef __3DMATH_H__
#define __3DMATH_H__

// This file defines the DA Component interface I3DMathEngine, which is used by a
// number of useful static classes like Matrix, Transform, and Quaternion, which
// are defined in MATRIX.H, XFORM.H, and QUAT.H, respectively.The default
// implementation of this interface will use standard x86 FPU instructions, but
// another implementation will exist for each piece of hardware that is capable
// of accelerating the operations in question.
// 
// Function bodies for the Matrix, Transform, and Quaternion classes are all
// inline, and make calls to the global I3DMathEngine provider where appropriate.
// The app can get a pointer to the global I3DMathEngine provider at any time by
// calling MATH_ENGINE(), defined below in this file.
// 
// The I3DMathEngine interface is a bit of a "catch-all" for potentially -
// acceleratable operations, and as such has a non - zero probability of future
// changes as hardware acceleration schemes proliferate both on PC and other
// platforms.For example, currently no vector - vector operations, such as the
// dot product, are part of I3DMathEngine.Should a 1 - cycle vector dot product
// come into existence on our target hardware, we would have to move the dot
// product into I3DMathEngine.Etc.

/* ---------- headers */

#include <DACOM.h>
#include <Vector.h>
#include <Matrix.h>
#include <Transform.h>
#include <Quaternion.h>

/* ---------- constants */

#ifndef PI
#define PI 3.14159265358979323846f 
#endif

#define IID_I3DMathEngine DACOM_MAKE_IID("I3DMathEngine")

/* ---------- definitions */

struct DA3DMATHDESC : public DACOMDESC
{
	C8* implementation;
	DA3DMATHDESC(const C8* _interface_name = IID_I3DMathEngine) :
		DACOMDESC(_interface_name),
		implementation(nullptr)
	{
		size = sizeof(*this);
	}
};

struct I3DMathEngine : public IDAComponent
{
	// Matrix and Transform operations.

	DEFMETHOD(inverse) (Matrix& dst, const Matrix& m) = 0;
	DEFMETHOD(scale) (Matrix& dst, const Matrix& m, SINGLE s) = 0;

	// Determinant of matrix.

	DEFMETHOD_(SINGLE, det) (const Matrix& m) = 0;

	// Matrix and Transform concatenation.

	DEFMETHOD(mul) (Matrix& dst, const Matrix& m1, const Matrix& m2) = 0;
	// Matrix and Transform concatenation.

	DEFMETHOD(mul) (Transform& dst, const Transform& m1, const Transform& m2) = 0;

	// Matrix and Transform operation on Vectors.

	DEFMETHOD(transform) (Vector& dst, const Matrix& m, const Vector& v) = 0;
	// Matrix and Transform operation on Vectors.

	DEFMETHOD(transform) (Vector& dst, const Transform& t, const Vector& v) = 0;

	// rotate with no translation.

	DEFMETHOD(rotate) (Vector& dst, const Transform& t, const Vector& v) = 0;
	DEFMETHOD(inverse_rotate) (Vector& dst, const Transform& t, const Vector& v) = 0;

	// Transform Vector with Transpose of Matrix. The transpose might also happen
	// to be the inverse in the case of a rotation matrix.

	DEFMETHOD(transpose_transform) (Vector& dst, const Matrix& m, const Vector& v) = 0;
	DEFMETHOD(inverse_transform) (Vector& dst, const Transform& t, const Vector& v) = 0;

	// Same operations on lists of Vectors.

	DEFMETHOD(transform_list) (Vector* dst, const Matrix& m, const Vector* src, int n) = 0;
	DEFMETHOD(transform_list) (Vector* dst, const Transform& t, const Vector* src, int n) = 0;

	DEFMETHOD(transpose_transform_list) (Vector* dst, const Matrix& m, const Vector* src, int n) = 0;
	DEFMETHOD(inverse_transform_list) (Vector* dst, const Transform& t, const Vector* src, int n) = 0;

	// Quaternion conversions.

	DEFMETHOD(matrix_to_quaternion) (Quaternion& dst, const Matrix& m) = 0;
	DEFMETHOD(quaternion_to_matrix) (Matrix& dst, const Quaternion& q) = 0;

	// Quaternion operations.

	DEFMETHOD(transform) (Vector& dst, const Quaternion& q, const Vector& v) = 0;
	DEFMETHOD(mul) (Quaternion& dst, const Quaternion& q1, const Quaternion& q2) = 0;

	// Quaternion interpolation.

	DEFMETHOD(quat_slerp) (Quaternion& dst, const Quaternion& q1, const Quaternion& q2, SINGLE t) = 0;

	DEFMETHOD(general_inverse) (Transform& dst, SINGLE& w, const Transform& m) = 0;
	DEFMETHOD(InvSqrt) (SINGLE& dst, const SINGLE x) = 0;
	DEFMETHOD(Sqrt) (SINGLE& dst, const SINGLE x) = 0;
};

/* ---------- prototypes */

/* #REFACTOR The old I3DMathEngine interface is an absolutely terrible way to implement
* a math library. Calling any math function requires using virtual functions. That's
* often more expensive than the math logic itself.
*
* The old per-module math interface instance is replaced below with a singular math interface
* exposed from the math library.
* Eventually the entire math library should directly use import/export declarations on all
* of the math functions for each of the types. eg. Vector, Quat, Matrix in order to completely
* bypass the usage of the I3DMathEngine interface.
*/

/* #REFACTOR

// the following method is implemented in the static lib MathLib.lib, in the x86math workspace:
I3DMathEngine* __stdcall __MATH_ENGINE(void);

static I3DMathEngine* math_engine = (I3DMathEngine*)0;
inline I3DMathEngine* MATH_ENGINE(void)
{
 if (math_engine == (I3DMathEngine*)0)
 math_engine = __MATH_ENGINE();
 return math_engine;
}

inline void __stdcall MATH_ENGINE_Uninitialize(void)
{
 math_engine = (I3DMathEngine*)0;
}*/

X86MATH_DEC extern I3DMathEngine& MATH_ENGINE;
X86MATH_DEC I3DMathEngine* __stdcall __MATH_ENGINE(void);

X86MATH_DEC extern "C" 
{
	extern void Register_3DMathEngine();
}

/* ---------- globals */

/* ---------- inline code */

#endif // __NEW_HEADER_H__
