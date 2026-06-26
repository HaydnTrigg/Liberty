/* ---------- headers */

#include "3dMath.h"
#include "fastdot.h"

#include <DACOM.h>
#include <TComponent.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>

/* ---------- constants */

#define CLSID_3DMathEngine "3DMathEngine"
#define IMPLID_x86MathEngine "x86"
#define IID_I3DMathEngine DACOM_MAKE_IID("I3DMathEngine")

// This is an arbitrarily small constant used to circumvent matrix inversions that might overflow due to small determinants.
// The proper way would be to check the ratio, but this will do for now.
static constexpr SINGLE MIN_DET = 1e-8f;
#define Q_EPSILON 0.00001f

/* ---------- definitions */

struct x86MathEngine : public I3DMathEngine, IComponentFactory
{
	BEGIN_DACOM_MAP_INBOUND(x86MathEngine)
		DACOM_INTERFACE_ENTRY(I3DMathEngine)
		END_DACOM_MAP()

	// IDAComponent interface

	DEFMETHOD(QueryInterface) (const C8* interface_name, void** instance) override;
	DEFMETHOD_(U32, AddRef) (void) override;
	DEFMETHOD_(U32, Release) (void) override;

	// IComponentFactory interface

	DEFMETHOD(CreateInstance) (DACOMDESC* descriptor, void** instance) override;

	// I3DMathEngine interface

	// Matrix and Transform operations.

	DEFMETHOD(inverse) (Matrix& dst, const Matrix& m) override;
	DEFMETHOD(scale) (Matrix& dst, const Matrix& m, SINGLE s) override;
	DEFMETHOD(general_inverse) (Transform& dst, SINGLE& w, const Transform& m) override;

	// Determinant of matrix.
	DEFMETHOD_(SINGLE, det) (const Matrix& m) override;

	// Matrix and Transform concatenation.

	DEFMETHOD(mul) (Matrix& dst, const Matrix& m1, const Matrix& m2) override;
	DEFMETHOD(mul) (Transform& dst, const Transform& m1, const Transform& m2) override;

	// Matrix and Transform operation on Vectors.

	DEFMETHOD(transform) (Vector& dst, const Matrix& m, const Vector& v) override;
	DEFMETHOD(transform) (Vector& dst, const Transform& t, const Vector& v) override;

	// rotate with no translation.

	DEFMETHOD(rotate) (Vector& dst, const Transform& t, const Vector& v) override;
	DEFMETHOD(inverse_rotate) (Vector& dst, const Transform& t, const Vector& v) override;

	// Transform Vector with Transpose of Matrix. The transpose might also happen
	// to be the inverse in the case of a rotation matrix.

	DEFMETHOD(transpose_transform) (Vector& dst, const Matrix& m, const Vector& v) override;
	DEFMETHOD(inverse_transform) (Vector& dst, const Transform& t, const Vector& v) override;

	// Same operations on lists of Vectors.

	DEFMETHOD(transform_list) (Vector* dst, const Matrix& m, const Vector* src, int n) override;
	DEFMETHOD(transform_list) (Vector* dst, const Transform& t, const Vector* src, int n) override;

	DEFMETHOD(transpose_transform_list) (Vector* dst, const Matrix& m, const Vector* src, int n) override;
	DEFMETHOD(inverse_transform_list) (Vector* dst, const Transform& t, const Vector* src, int n) override;

	// Quaternion conversions.

	DEFMETHOD(matrix_to_quaternion) (Quaternion& dst, const Matrix& m) override;
	DEFMETHOD(quaternion_to_matrix) (Matrix& dst, const Quaternion& q) override;

	// Quaternion operations.

	DEFMETHOD(transform) (Vector& dst, const Quaternion& q, const Vector& v) override;
	DEFMETHOD(mul) (Quaternion& dst, const Quaternion& q1, const Quaternion& q2) override;

	// Quaternion interpolation.

	DEFMETHOD(quat_slerp) (Quaternion& dst, const Quaternion& q1, const Quaternion& q2, SINGLE t) override;

	// Fast 1/sqrt(x) and sqrt(x)

	DEFMETHOD(InvSqrt) (SINGLE& dst, const SINGLE x) override;
	DEFMETHOD(Sqrt) (SINGLE& dst, const SINGLE x) override;
};

/* ---------- prototypes */

static void mul_vec_by_matrix(Vector& dst, const Matrix& m, const Vector& v);
static void mul_vec_by_transp_matrix(Vector& dst, const Matrix& m, const Vector& v);

/* ---------- globals */

x86MathEngine MathEngine;

//I3DMathEngine* __stdcall __MATH_ENGINE(void)
//{
//	DA3DMATHDESC math_info;
//	I3DMathEngine* math_engine;
//	DACOM_Acquire()->CreateInstance(&math_info, (void**)&math_engine);
//	return math_engine;
//}
//I3DMathEngine& MATH_ENGINE = *__MATH_ENGINE();
I3DMathEngine& MATH_ENGINE = MathEngine;

/* ---------- public code */

GENRESULT DACOM_API x86MathEngine::QueryInterface(const C8* interface_name, void** instance)
{
	*instance = 0;
	return GR_GENERIC;
}

U32 DACOM_API x86MathEngine::AddRef(void)
{
	return 1;
}

U32 DACOM_API x86MathEngine::Release(void)
{
	return 1;
}

// if there are bad things in the descriptor, return an error code
// we answer any request with success, for now.
GENRESULT DACOM_API x86MathEngine::CreateInstance(DACOMDESC* descriptor, void** instance)
{
	DA3DMATHDESC* info = (DA3DMATHDESC*)descriptor;

	*instance = 0;

	if (info == 0 || info->interface_name == 0)
	{
		return GR_INVALID_PARMS;
	}

	if
		(
			info->size == sizeof(DA3DMATHDESC) &&
			(
				strcmp(CLSID_3DMathEngine, info->interface_name) == 0 ||
				strcmp(IID_I3DMathEngine, info->interface_name) == 0
				) &&
			(
				info->implementation == 0 ||
				_stricmp(IMPLID_x86MathEngine, info->implementation) == 0
				)
			)
	{
		((I3DMathEngine*)this)->AddRef();
		*instance = ((I3DMathEngine*)this);
		return GR_OK;
	}

	return GR_INTERFACE_UNSUPPORTED;
}

GENRESULT DACOM_API x86MathEngine::inverse(Matrix& dst, const Matrix& m)
{
	GENRESULT result;

	const SINGLE determinant = det(m);

	if (fabs(determinant) > MIN_DET)
	{
		const SINGLE dt = 1.0f / determinant; //compiler should use fld1 instruction (but doesn't)

		dst.d[0][0] = (m.d[1][1] * m.d[2][2] - m.d[1][2] * m.d[2][1]) * dt;
		dst.d[1][0] = -(m.d[1][0] * m.d[2][2] - m.d[1][2] * m.d[2][0]) * dt;
		dst.d[2][0] = (m.d[1][0] * m.d[2][1] - m.d[1][1] * m.d[2][0]) * dt;
		dst.d[0][1] = -(m.d[0][1] * m.d[2][2] - m.d[0][2] * m.d[2][1]) * dt;
		dst.d[1][1] = (m.d[0][0] * m.d[2][2] - m.d[0][2] * m.d[2][0]) * dt;
		dst.d[2][1] = -(m.d[0][0] * m.d[2][1] - m.d[0][1] * m.d[2][0]) * dt;
		dst.d[0][2] = (m.d[0][1] * m.d[1][2] - m.d[0][2] * m.d[1][1]) * dt;
		dst.d[1][2] = -(m.d[0][0] * m.d[1][2] - m.d[0][2] * m.d[1][0]) * dt;
		dst.d[2][2] = (m.d[0][0] * m.d[1][1] - m.d[0][1] * m.d[1][0]) * dt;

		result = GR_OK;
	}
	else
	{
		// this is the adjoint which can still be usefull
		dst.d[0][0] = (m.d[1][1] * m.d[2][2] - m.d[1][2] * m.d[2][1]);
		dst.d[1][0] = -(m.d[1][0] * m.d[2][2] - m.d[1][2] * m.d[2][0]);
		dst.d[2][0] = (m.d[1][0] * m.d[2][1] - m.d[1][1] * m.d[2][0]);
		dst.d[0][1] = -(m.d[0][1] * m.d[2][2] - m.d[0][2] * m.d[2][1]);
		dst.d[1][1] = (m.d[0][0] * m.d[2][2] - m.d[0][2] * m.d[2][0]);
		dst.d[2][1] = -(m.d[0][0] * m.d[2][1] - m.d[0][1] * m.d[2][0]);
		dst.d[0][2] = (m.d[0][1] * m.d[1][2] - m.d[0][2] * m.d[1][1]);
		dst.d[1][2] = -(m.d[0][0] * m.d[1][2] - m.d[0][2] * m.d[1][0]);
		dst.d[2][2] = (m.d[0][0] * m.d[1][1] - m.d[0][1] * m.d[1][0]);

		result = GR_INVALID_PARMS;
	}

	return result;
}

GENRESULT DACOM_API x86MathEngine::general_inverse(Transform& dst, SINGLE& w, const Transform& t)
{
	// assumes last row is 0 0 0 1

	GENRESULT result;

	const SINGLE determinant = t.d[0][0] * (t.d[1][1] * t.d[2][2] - t.d[2][1] * t.d[1][2]) -
		t.d[0][1] * (t.d[1][0] * t.d[2][2] - t.d[2][0] * t.d[1][2]) +
		t.d[0][2] * (t.d[1][0] * t.d[2][1] - t.d[2][0] * t.d[1][1]);

	SINGLE dt;
	if (fabs(determinant) > MIN_DET)
	{
		dt = 1.0f / determinant;
		result = GR_OK;
	}
	else
	{
		dt = 1.0f; // this will at least give us the adjoint
		result = GR_INVALID_PARMS;
	}

	dst.d[0][0] = (t.d[1][1] * t.d[2][2] - t.d[2][1] * t.d[1][2]) * dt;
	dst.d[0][1] = -(t.d[0][1] * t.d[2][2] - t.d[2][1] * t.d[0][2]) * dt;
	dst.d[0][2] = (t.d[0][1] * t.d[1][2] - t.d[1][1] * t.d[0][2]) * dt;

	dst.d[1][0] = -(t.d[1][0] * t.d[2][2] - t.d[2][0] * t.d[1][2]) * dt;
	dst.d[1][1] = (t.d[0][0] * t.d[2][2] - t.d[2][0] * t.d[0][2]) * dt;
	dst.d[1][2] = -(t.d[0][0] * t.d[1][2] - t.d[1][0] * t.d[0][2]) * dt;

	dst.d[2][0] = (t.d[1][0] * t.d[2][1] - t.d[2][0] * t.d[1][1]) * dt;
	dst.d[2][1] = -(t.d[0][0] * t.d[2][1] - t.d[2][0] * t.d[0][1]) * dt;
	dst.d[2][2] = (t.d[0][0] * t.d[1][1] - t.d[1][0] * t.d[0][1]) * dt;

	dst.translation.x = -(t.d[0][1] * t.d[1][2] * t.translation.z +
		t.d[1][1] * t.d[2][2] * t.translation.x +
		t.d[2][1] * t.d[0][2] * t.translation.y -
		t.d[0][1] * t.d[2][2] * t.translation.y -
		t.d[1][1] * t.d[0][2] * t.translation.z -
		t.d[2][1] * t.d[1][2] * t.translation.x) * dt;

	dst.translation.y = (t.d[0][0] * t.d[1][2] * t.translation.z +
		t.d[1][0] * t.d[2][2] * t.translation.x +
		t.d[2][0] * t.d[0][2] * t.translation.y -
		t.d[0][0] * t.d[2][2] * t.translation.y -
		t.d[1][0] * t.d[0][2] * t.translation.z -
		t.d[2][0] * t.d[1][2] * t.translation.x) * dt;

	dst.translation.z = -(t.d[0][0] * t.d[1][1] * t.translation.z +
		t.d[1][0] * t.d[2][1] * t.translation.x +
		t.d[2][0] * t.d[0][1] * t.translation.y -
		t.d[0][0] * t.d[2][1] * t.translation.y -
		t.d[1][0] * t.d[0][1] * t.translation.z -
		t.d[2][0] * t.d[1][1] * t.translation.x) * dt;

	w = (t.d[0][0] * t.d[1][1] * t.d[2][2] +
		t.d[1][0] * t.d[2][1] * t.d[0][2] +
		t.d[2][0] * t.d[0][1] * t.d[1][2] -
		t.d[2][0] * t.d[1][1] * t.d[0][2] -
		t.d[0][0] * t.d[2][1] * t.d[1][2] -
		t.d[1][0] * t.d[0][1] * t.d[2][2]) * dt;

	return result;
}

GENRESULT DACOM_API x86MathEngine::scale(Matrix& dst, const Matrix& m, SINGLE s)
{
	dst.d[0][0] = m.d[0][0] * s;
	dst.d[0][1] = m.d[0][1] * s;
	dst.d[0][2] = m.d[0][2] * s;
	dst.d[1][0] = m.d[1][0] * s;
	dst.d[1][1] = m.d[1][1] * s;
	dst.d[1][2] = m.d[1][2] * s;
	dst.d[2][0] = m.d[2][0] * s;
	dst.d[2][1] = m.d[2][1] * s;
	dst.d[2][2] = m.d[2][2] * s;

	return GR_OK;
}

SINGLE DACOM_API x86MathEngine::det(const Matrix& m)
{
	return (m.d[0][0] * m.d[1][1] * m.d[2][2] +
		m.d[0][1] * m.d[1][2] * m.d[2][0] +
		m.d[0][2] * m.d[1][0] * m.d[2][1] -
		m.d[0][0] * m.d[1][2] * m.d[2][1] -
		m.d[0][1] * m.d[1][0] * m.d[2][2] -
		m.d[0][2] * m.d[1][1] * m.d[2][0]);
}

GENRESULT DACOM_API x86MathEngine::mul(Matrix& dst, const Matrix& m1, const Matrix& m2)
{
	//FOR SOME UNKNOWN REASON, THE VC++ 5.0 OPTIMIZER CHOKES THE UNROLLED VERSION
	for (int i = 0; i < 3; i++)
	{
		dst.d[i][0] = m1.d[i][0] * m2.d[0][0] + m1.d[i][1] * m2.d[1][0] + m1.d[i][2] * m2.d[2][0];
		dst.d[i][1] = m1.d[i][0] * m2.d[0][1] + m1.d[i][1] * m2.d[1][1] + m1.d[i][2] * m2.d[2][1];
		dst.d[i][2] = m1.d[i][0] * m2.d[0][2] + m1.d[i][1] * m2.d[1][2] + m1.d[i][2] * m2.d[2][2];
	}

	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::mul(Transform& dst, const Transform& m1, const Transform& m2)
{
	dst.d[0][0] = m1.d[0][0] * m2.d[0][0] +
		m1.d[0][1] * m2.d[1][0] +
		m1.d[0][2] * m2.d[2][0];

	dst.d[0][1] = m1.d[0][0] * m2.d[0][1] +
		m1.d[0][1] * m2.d[1][1] +
		m1.d[0][2] * m2.d[2][1];

	dst.d[0][2] = m1.d[0][0] * m2.d[0][2] +
		m1.d[0][1] * m2.d[1][2] +
		m1.d[0][2] * m2.d[2][2];

	dst.translation.x = m1.d[0][0] * m2.translation.x +
		m1.d[0][1] * m2.translation.y +
		m1.d[0][2] * m2.translation.z +
		m1.translation.x;

	dst.d[1][0] = m1.d[1][0] * m2.d[0][0] +
		m1.d[1][1] * m2.d[1][0] +
		m1.d[1][2] * m2.d[2][0];

	dst.d[1][1] = m1.d[1][0] * m2.d[0][1] +
		m1.d[1][1] * m2.d[1][1] +
		m1.d[1][2] * m2.d[2][1];

	dst.d[1][2] = m1.d[1][0] * m2.d[0][2] +
		m1.d[1][1] * m2.d[1][2] +
		m1.d[1][2] * m2.d[2][2];

	dst.translation.y = m1.d[1][0] * m2.translation.x +
		m1.d[1][1] * m2.translation.y +
		m1.d[1][2] * m2.translation.z +
		m1.translation.y;

	dst.d[2][0] = m1.d[2][0] * m2.d[0][0] +
		m1.d[2][1] * m2.d[1][0] +
		m1.d[2][2] * m2.d[2][0];

	dst.d[2][1] = m1.d[2][0] * m2.d[0][1] +
		m1.d[2][1] * m2.d[1][1] +
		m1.d[2][2] * m2.d[2][1];

	dst.d[2][2] = m1.d[2][0] * m2.d[0][2] +
		m1.d[2][1] * m2.d[1][2] +
		m1.d[2][2] * m2.d[2][2];

	dst.translation.z = m1.d[2][0] * m2.translation.x +
		m1.d[2][1] * m2.translation.y +
		m1.d[2][2] * m2.translation.z +
		m1.translation.z;

	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::transform(Vector& dst, const Matrix& m, const Vector& v)
{
	mul_vec_by_matrix(dst, m, v);
	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::rotate(Vector& dst, const Transform& t, const Vector& v)
{
	mul_vec_by_matrix(dst, t, v);
	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::transform(Vector& dst, const Transform& t, const Vector& v)
{
	dst.x = t.d[0][0] * v.x + t.d[0][1] * v.y + t.d[0][2] * v.z + t.translation.x;
	dst.y = t.d[1][0] * v.x + t.d[1][1] * v.y + t.d[1][2] * v.z + t.translation.y;
	dst.z = t.d[2][0] * v.x + t.d[2][1] * v.y + t.d[2][2] * v.z + t.translation.z;

	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::inverse_rotate(Vector& dst, const Transform& t, const Vector& v)
{
	mul_vec_by_transp_matrix(dst, t, v);
	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::transpose_transform(Vector& dst, const Matrix& m, const Vector& v)
{
	// Transform Vector with Transpose of Matrix. The transpose might also happen
	// to be the inverse in the case of a rotation matrix.
	mul_vec_by_transp_matrix(dst, m, v);
	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::inverse_transform(Vector& dst, const Transform& t, const Vector& v)
{
	// Invert translation of source vector
	const SINGLE _x = v.x - t.translation.x;
	const SINGLE _y = v.y - t.translation.y;
	const SINGLE _z = v.z - t.translation.z;

	// Multiply inverse-translated source vector by inverted rotation transform

	dst.x = t.d[0][0] * _x + t.d[1][0] * _y + t.d[2][0] * _z;
	dst.y = t.d[0][1] * _x + t.d[1][1] * _y + t.d[2][1] * _z;
	dst.z = t.d[0][2] * _x + t.d[1][2] * _y + t.d[2][2] * _z;

	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::transform_list(Vector* dst, const Matrix& m, const Vector* src, int n)
{
	const FastDot fd_0(m.d[0][0], m.d[0][1], m.d[0][2]);
	const FastDot fd_1(m.d[1][0], m.d[1][1], m.d[1][2]);
	const FastDot fd_2(m.d[2][0], m.d[2][1], m.d[2][2]);

	if (fd_0.GetSavedOps() + fd_1.GetSavedOps() + fd_2.GetSavedOps() < 4) // not worth it
	{
		for (const Vector* const end = src + n; src < end; src++, dst++)
		{
			const SINGLE _x = src->x;
			const SINGLE _y = src->y;
			const SINGLE _z = src->z;
			dst->x = m.d[0][0] * _x + m.d[0][1] * _y + m.d[0][2] * _z;
			dst->y = m.d[1][0] * _x + m.d[1][1] * _y + m.d[1][2] * _z;
			dst->z = m.d[2][0] * _x + m.d[2][1] * _y + m.d[2][2] * _z;
		}
	}
	else
	{
		for (const Vector* const end = src + n; src < end; src++, dst++)
		{
			dst->x = fd_0.dot(*src);
			dst->y = fd_1.dot(*src);
			dst->z = fd_2.dot(*src);
		}
	}

	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::transform_list(Vector* dst, const Transform& t, const Vector* src, int n)
{
	if (t.translation.x != 0.0f || t.translation.y != 0.0f || t.translation.z != 0.0f)
	{
		const FastDot fd_0(t.d[0][0], t.d[0][1], t.d[0][2]);
		const FastDot fd_1(t.d[1][0], t.d[1][1], t.d[1][2]);
		const FastDot fd_2(t.d[2][0], t.d[2][1], t.d[2][2]);

		if (fd_0.GetSavedOps() + fd_1.GetSavedOps() + fd_2.GetSavedOps() < 4) // not worth it
		{
			for (const Vector* const end = src + n; src < end; src++, dst++)
			{
				const SINGLE _x = src->x;
				const SINGLE _y = src->y;
				const SINGLE _z = src->z;

				dst->x = (t.d[0][0] * _x) +
					(t.d[0][1] * _y) +
					(t.d[0][2] * _z) +
					(t.translation.x);

				dst->y = (t.d[1][0] * _x) +
					(t.d[1][1] * _y) +
					(t.d[1][2] * _z) +
					(t.translation.y);

				dst->z = (t.d[2][0] * _x) +
					(t.d[2][1] * _y) +
					(t.d[2][2] * _z) +
					(t.translation.z);
			}
		}
		else
		{
			for (const Vector* const end = src + n; src < end; src++, dst++)
			{
				dst->x = fd_0.dot(*src) + t.translation.x;
				dst->y = fd_1.dot(*src) + t.translation.y;
				dst->z = fd_2.dot(*src) + t.translation.z;
			}
		}

		return GR_OK;
	}
	else
	{
		return transform_list(dst, static_cast<const Matrix&>(t), src, n);
	}
}

GENRESULT DACOM_API x86MathEngine::transpose_transform_list(Vector* dst, const Matrix& m, const Vector* src, int n)
{
	// doing one transpose should be faster than indexing backwards each itteration -ms
	return transform_list(dst, m.get_transpose(), src, n);
}

GENRESULT DACOM_API x86MathEngine::inverse_transform_list(Vector* dst, const Transform& t, const Vector* src, int n)
{
	// doing one fake inverse should be faster than indexing backwards each itteration -ms
	Vector inv_tran;
	transpose_transform(inv_tran, static_cast<const Matrix&>(t), -t.translation);
	const Transform inverse_t(t.Matrix::get_transpose(), inv_tran);

	return transform_list(dst, inverse_t, src, n);
}

GENRESULT DACOM_API x86MathEngine::matrix_to_quaternion(Quaternion& dst, const Matrix& m)
{
	static const int nxt[3] = { 1, 2, 0 };

	const SINGLE trace = m.d[0][0] + m.d[1][1] + m.d[2][2];

	if (trace > 0.0f)
	{
		SINGLE st = (SINGLE)sqrt(trace + 1.0f);
		dst.w = st * 0.5f;
		st = 0.5f / st;

		dst.x = (m.d[2][1] - m.d[1][2]) * st;
		dst.y = (m.d[0][2] - m.d[2][0]) * st;
		dst.z = (m.d[1][0] - m.d[0][1]) * st;
	}
	else
	{
		int i = 0;
		if (m.d[1][1] > m.d[0][0])
		{
			i = 1;
		}
		if (m.d[2][2] > m.d[i][i])
		{
			i = 2;
		}

		int j = nxt[i];
		int k = nxt[j];

		SINGLE st = (SINGLE)sqrt((m.d[i][i] - (m.d[j][j] + m.d[k][k])) + 1.0f);

		dst.d[i + 1] = st * 0.5f;
		st = 0.5f / st;
		dst.w = (m.d[k][j] - m.d[j][k]) * st;
		dst.d[j + 1] = (m.d[j][i] + m.d[i][j]) * st;
		dst.d[k + 1] = (m.d[k][i] + m.d[i][k]) * st;
	}

	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::quaternion_to_matrix(Matrix& dst, const Quaternion& q)
{
	const SINGLE s = 2.0f / (q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);

	const SINGLE xs = q.x * s;
	const SINGLE ys = q.y * s;
	const SINGLE zs = q.z * s;

	const SINGLE wx = q.w * xs;
	const SINGLE wy = q.w * ys;
	const SINGLE wz = q.w * zs;

	const SINGLE xx = q.x * xs;
	const SINGLE xy = q.x * ys;
	const SINGLE xz = q.x * zs;
	const SINGLE yy = q.y * ys;
	const SINGLE yz = q.y * zs;
	const SINGLE zz = q.z * zs;

	dst.d[0][0] = 1.0f - (yy + zz);
	dst.d[0][1] = xy - wz;
	dst.d[0][2] = xz + wy;

	dst.d[1][0] = xy + wz;
	dst.d[1][1] = 1.0f - (xx + zz);
	dst.d[1][2] = yz - wx;

	dst.d[2][0] = xz - wy;
	dst.d[2][1] = yz + wx;
	dst.d[2][2] = 1.0f - (xx + yy);

	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::quat_slerp(Quaternion& dst, const Quaternion& q1, const Quaternion& q2, SINGLE t)
{
	// Make sure we're taking the shorter arc.

	Quaternion sum, dif, qs;

	sum.w = q1.w + q2.w;
	sum.x = q1.x + q2.x;
	sum.y = q1.y + q2.y;
	sum.z = q1.z + q2.z;

	dif.w = q1.w - q2.w;
	dif.x = q1.x - q2.x;
	dif.y = q1.y - q2.y;
	dif.z = q1.z - q2.z;

	SINGLE s1 = dif.w * dif.w + dif.x * dif.x + dif.y * dif.y + dif.z * dif.z;
	SINGLE s2 = sum.w * sum.w + sum.x * sum.x + sum.y * sum.y + sum.z * sum.z;

	if (s1 > s2)
	{
		qs.w = -q2.w;
		qs.x = -q2.x;
		qs.y = -q2.y;
		qs.z = -q2.z;
	}
	else
	{
		qs = q2;
	}

	SINGLE cos_omega = q1.w * qs.w + q1.x * qs.x + q1.y * qs.y + q1.z * qs.z;

	// Check for cases where rotations are nearly opposite, which causes numerical weirdness.
	if ((1.0f + cos_omega) > Q_EPSILON)
	{
		// Check for case where rotations are very close, which also causes weirdness. 
		if ((1.0f - cos_omega) > Q_EPSILON)
		{
			SINGLE omega = acos(cos_omega);
			SINGLE sin_omega = sin(omega);

			s1 = sin((1.0f - t) * omega) / sin_omega;
			s2 = sin(t * omega) / sin_omega;
		}
		else
		{
			// Too close, do straight linear interpolation.
			s1 = 1.0f - t;
			s2 = t;
		}

		dst.w = s1 * q1.w + s2 * qs.w;
		dst.x = s1 * q1.x + s2 * qs.x;
		dst.y = s1 * q1.y + s2 * qs.y;
		dst.z = s1 * q1.z + s2 * qs.z;
	}
	else
	{
		dst.w = q1.z;
		dst.x = -q1.y;
		dst.y = q1.x;
		dst.z = -q1.w;

		SINGLE half_pi = SINGLE(PI / 2.0f);

		s1 = sin((1.0f - t) * half_pi);
		s2 = sin(t * half_pi);

		dst.x = s1 * q1.x + s2 * dst.x;
		dst.y = s1 * q1.y + s2 * dst.y;
		dst.z = s1 * q1.z + s2 * dst.z;
	}

	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::transform(Vector& dst, const Quaternion& q, const Vector& v)
{
	dst = v * (q.s * q.s - dot_product(q.v, q.v));
	dst += q.v * 2.0f * dot_product(q.v, v);
	dst += cross_product(q.v, v) * q.s * 2.0f;

	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::mul(Quaternion& dst, const Quaternion& q1, const Quaternion& q2)
{
	dst.s = q1.s * q2.s - dot_product(q1.v, q2.v);
	dst.v = q1.v * q2.s + q2.v * q1.s + cross_product(q1.v, q2.v);

	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::InvSqrt(SINGLE& dst, const SINGLE x)
{
	dst = 1.0f / sqrt(x);
	return GR_OK;
}

GENRESULT DACOM_API x86MathEngine::Sqrt(SINGLE& dst, const SINGLE x)
{
	dst = sqrt(x);
	return GR_OK;
}

extern "C"
{
	void Register_3DMathEngine()
	{
		ICOManager* DACOM = DACOM_Acquire();
		DACOM->RegisterComponent(&MathEngine, IID_I3DMathEngine);
		DACOM->RegisterComponent(&MathEngine, CLSID_3DMathEngine);
	}
}

/* ---------- private code */

static void mul_vec_by_matrix(Vector& dst, const Matrix& m, const Vector& v)
{
	dst.x = m.d[0][0] * v.x + m.d[0][1] * v.y + m.d[0][2] * v.z;
	dst.y = m.d[1][0] * v.x + m.d[1][1] * v.y + m.d[1][2] * v.z;
	dst.z = m.d[2][0] * v.x + m.d[2][1] * v.y + m.d[2][2] * v.z;
}

static void mul_vec_by_transp_matrix(Vector& dst, const Matrix& m, const Vector& v)
{
	dst.x = m.d[0][0] * v.x + m.d[1][0] * v.y + m.d[2][0] * v.z;
	dst.y = m.d[0][1] * v.x + m.d[1][1] * v.y + m.d[2][1] * v.z;
	dst.z = m.d[0][2] * v.x + m.d[1][2] * v.y + m.d[2][2] * v.z;
}

/* ---------- reverse engineering */