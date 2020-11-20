#include "Maths.h"
#include <string.h>
#include <xmmintrin.h>
#include <immintrin.h>


float3x3::float3x3(void)
{

}

float3x3::float3x3(float x)
{

	for (int i = 0; i < 9; i++)
		m()[i] = x;
}

float3x3::float3x3(float3 v1, float3 v2, float3 v3)
{
	memcpy(&m00, v1.v(), 3 * sizeof(float));
	memcpy(&m10, v2.v(), 3 * sizeof(float));
	memcpy(&m20, v3.v(), 3 * sizeof(float));
}

float3x3::~float3x3(void)
{

}

void float3x3::inverse(void)
{

}

void float3x3::transpose(void)
{
	float tmp = m01;
	m01 = m10;
	m10 = tmp;

	tmp = m02;
	m02 = m20;
	m20 = tmp;

	tmp = m12;
	m12 = m21;
	m21 = tmp;
}

float3 float3x3::operator*(float3 const& v)
{
	float3 res;

	res.x = m00*v.x + m01*v.y + m02*v.z;
	res.y = m10*v.x + m11*v.y + m12*v.z;
	res.z = m20*v.x + m21*v.y + m22*v.z;

	return res;
}

float3x3 operator+(float3x3 const& A, float3x3 const& B)
{
	float3x3 C;

	for (int i = 0; i < 9; i++)
	{
		C.m()[i] = A.data(i) + B.data(i);
	}

	return C;
}

float3x3 operator-(float3x3 const& A, float3x3 const& B)
{
	float3x3 C;

	for (int i = 0; i < 9; i++)
	{
		C.m()[i] = A.data(i) - B.data(i);
	}

	return C;
}

float3x3 operator*(float3x3 const& A, float3x3 const& B)
{
	float3x3 C;
	int i, j, k;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			*(&C.m00 + 3 * i + j) = 0.f;

			for (k = 0; k < 3; k++)
			{
				*(&C.m00 + 3 * i + j) += *(&A.m00 + 3 * i + k) * *(&B.m00 + 3 * k + j);
			}
		}
	}

	return C;
}

float3x3 operator*(float x, float3x3 const& A)
{
	float3x3 B;

	for (int i = 0; i < 9; i++)
	{
		B.m()[i] = x*A.data(i);
	}

	return B;
}


float3x3 float3x3::operator=(float3x3 const& mat)
{
	memcpy(&m00, &mat.m00, 9 * sizeof(float));

	return *this;
}


float3x3 inverse(float3x3 const& mat)
{
	float3x3 ret;
	float invdet, det1, det2, det3;

	det1 = mat.m11 * mat.m22 - mat.m21 * mat.m12;
	det2 = mat.m01 * mat.m22 - mat.m21 * mat.m02;
	det3 = mat.m01 * mat.m12 - mat.m11 * mat.m02;

	invdet = 1.f / (mat.m00 * det1 - mat.m10 * det2 + mat.m20 * det3);
	ret.m00 = det1 * invdet;
	ret.m01 = -det2 * invdet;
	ret.m02 = det3 * invdet;

	det1 = mat.m10 * mat.m22 - mat.m20 * mat.m12;
	det2 = mat.m00 * mat.m22 - mat.m20 * mat.m02;
	det3 = mat.m00 * mat.m12 - mat.m10 * mat.m02;
	ret.m10 = -det1 * invdet;
	ret.m11 = det2 * invdet;
	ret.m12 = -det3 * invdet;

	det1 = mat.m10 * mat.m21 - mat.m20 * mat.m11;
	det2 = mat.m00 * mat.m21 - mat.m20 * mat.m01;
	det3 = mat.m00 * mat.m11 - mat.m10 * mat.m01;
	ret.m20 = det1 * invdet;
	ret.m21 = -det2 * invdet;
	ret.m22 = det3 * invdet;

	return ret;
}


float4x4::float4x4(void)
{

}

float4x4::float4x4(float x)
{
	m256[0] = _mm256_broadcast_ss(&x);
	m256[1] = _mm256_broadcast_ss(&x);
}

float4x4::float4x4(float4 v1, float4 v2, float4 v3, float4 v4)
{
	row[0] = v1.m128;
	row[1] = v2.m128;
	row[2] = v3.m128;
	row[3] = v4.m128;
}

float4x4::~float4x4(void)
{

}


__forceinline __m128 Mat2Mul(__m128 vec1, __m128 vec2)
{
	return _mm_add_ps(_mm_mul_ps(vec1, _mm_shuffle_ps(vec2, vec2, 0xcc)), _mm_mul_ps(_mm_shuffle_ps(vec1, vec1, 0xb1), _mm_shuffle_ps(vec2, vec2, 0x66)));
}

__forceinline __m128 Mat2AdjMul(__m128 vec1, __m128 vec2)
{
	return _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(vec1, vec1, 0xf), vec2), _mm_mul_ps(_mm_shuffle_ps(vec1, vec1, 0xa5), _mm_shuffle_ps(vec2, vec2, 0x4e)));

}

__forceinline __m128 Mat2MulAdj(__m128 vec1, __m128 vec2)
{
	return _mm_sub_ps(_mm_mul_ps(vec1, _mm_shuffle_ps(vec2, vec2, 0x33)), _mm_mul_ps(_mm_shuffle_ps(vec1, vec1, 0xb1), _mm_shuffle_ps(vec2, vec2, 0x66)));
}


float4x4 inverse(float4x4 const& mat)
{
	__m128 A = _mm_movelh_ps(mat.row[0], mat.row[1]);
	__m128 B = _mm_movehl_ps(mat.row[1], mat.row[0]);
	__m128 C = _mm_movelh_ps(mat.row[2], mat.row[3]);
	__m128 D = _mm_movehl_ps(mat.row[3], mat.row[2]);

	__m128 detA = _mm_set1_ps(mat.m00 * mat.m11 - mat.m01 * mat.m10);
	__m128 detB = _mm_set1_ps(mat.m02 * mat.m13 - mat.m03 * mat.m12);
	__m128 detC = _mm_set1_ps(mat.m20 * mat.m31 - mat.m21 * mat.m30);
	__m128 detD = _mm_set1_ps(mat.m22 * mat.m33 - mat.m23 * mat.m32);


	__m128 D_C	= Mat2AdjMul(D, C);
	__m128 A_B	= Mat2AdjMul(A, B);
	__m128 X_	= _mm_sub_ps(_mm_mul_ps(detD, A), Mat2Mul(B, D_C));
	__m128 W_	= _mm_sub_ps(_mm_mul_ps(detA, D), Mat2Mul(C, A_B));
	__m128 detM = _mm_mul_ps(detA, detD);
	__m128 Y_	= _mm_sub_ps(_mm_mul_ps(detB, C), Mat2MulAdj(D, A_B));
	__m128 Z_	= _mm_sub_ps(_mm_mul_ps(detC, B), Mat2MulAdj(A, D_C));


	detM = _mm_add_ps(detM, _mm_mul_ps(detB, detC));

	__m128 tr = _mm_mul_ps(A_B, _mm_shuffle_ps(D_C, D_C, 0xd8));
	tr = _mm_hadd_ps(tr, tr);
	tr = _mm_hadd_ps(tr, tr);

	detM = _mm_sub_ps(detM, tr);

	const __m128 adjSignMask = _mm_setr_ps(1.f, -1.f, -1.f, 1.f);

	__m128 rDetM = _mm_div_ps(adjSignMask, detM);

	X_ = _mm_mul_ps(X_, rDetM);
	Y_ = _mm_mul_ps(Y_, rDetM);
	Z_ = _mm_mul_ps(Z_, rDetM);
	W_ = _mm_mul_ps(W_, rDetM);

	float4x4 r;

	r.row[0] = _mm_shuffle_ps(X_, Y_, 0x77);
	r.row[1] = _mm_shuffle_ps(X_, Y_, 0x22);
	r.row[2] = _mm_shuffle_ps(Z_, W_, 0x77);
	r.row[3] = _mm_shuffle_ps(Z_, W_, 0x22);

	return r;
}

void float4x4::transpose(void)
{
	_MM_TRANSPOSE4_PS(row[0], row[1], row[2], row[3]);
}


void float4x4::Eye(void)
{
	float zero = 0.f;

	m256[0] = _mm256_broadcast_ss(&zero);
	m256[1] = _mm256_broadcast_ss(&zero);

	m00 = m11 = m22 = m33 = 1.f;
}


float4 float4x4::operator*(float4 const& v)
{
	float4 res;

	__m128 x = _mm_dp_ps(row[0], v.m128, 0xf1);
	__m128 y = _mm_dp_ps(row[1], v.m128, 0xf2);
	__m128 z = _mm_dp_ps(row[2], v.m128, 0xf4);
	__m128 w = _mm_dp_ps(row[3], v.m128, 0xf8);

	_mm_storeu_ps(&res.x, _mm_add_ps(_mm_add_ps(x, y), _mm_add_ps(z, w)));

	return res;
}


bool operator!=(float4x4 const& A, float4x4& B)
{
	__m256 res1 = _mm256_cmp_ps(A.m256[0], B.m256[0], _CMP_NEQ_OQ);
	__m256 res2 = _mm256_cmp_ps(A.m256[1], B.m256[1], _CMP_NEQ_OQ);

	res1 = _mm256_dp_ps(res1, res1, 0xf1);
	res2 = _mm256_dp_ps(res2, res2, 0xf1);

	return isnan(*(float*)&res1 + ((float*)&res1)[4] + *(float*)&res2 + ((float*)&res2)[4]);
}


bool operator==(float4x4 const& A, float4x4& B)
{
	return !(A != B);
}


float4x4 operator+(float4x4 const& A, float4x4 const& B)
{
	float4x4 C;

	C.m256[0] = _mm256_add_ps(A.m256[0], B.m256[0]);
	C.m256[1] = _mm256_add_ps(A.m256[1], B.m256[1]);

	return C;
}

float4x4 operator-(float4x4 const& A, float4x4 const& B)
{
	float4x4 C;

	C.m256[0] = _mm256_sub_ps(A.m256[0], B.m256[0]);
	C.m256[1] = _mm256_sub_ps(A.m256[1], B.m256[1]);

	return C;
}


static inline __m256 twolincomb_AVX_8(__m256 A01, float4x4 const&B)
{
	__m256 result;
	result = _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x00), _mm256_broadcast_ps(&B.row[0]));
	result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x55), _mm256_broadcast_ps(&B.row[1])));
	result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xaa), _mm256_broadcast_ps(&B.row[2])));
	result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xff), _mm256_broadcast_ps(&B.row[3])));
	return result;
}


float4x4 operator*(float4x4 const& A, float4x4 const& B)
{
	float4x4 C;

	_mm256_zeroupper();

	C.m256[0] = twolincomb_AVX_8(A.m256[0], B);
	C.m256[1] = twolincomb_AVX_8(A.m256[1], B);

	return C;
}

float4x4 operator*(float x, float4x4 const& A)
{
	float4x4 B;

	B.m256[0] = _mm256_mul_ps(A.m256[0], _mm256_broadcast_ss(&x));
	B.m256[1] = _mm256_mul_ps(A.m256[1], _mm256_broadcast_ss(&x));

	return B;
}


float4x4 float4x4::operator=(float4x4 const& mat)
{
	m256[0] = mat.m256[0];
	m256[1] = mat.m256[1];

	return *this;
}


float3x4::float3x4(void)
{

}

float3x4::float3x4(float x)
{

	for (int i = 0; i < 12; i++)
		m()[i] = x;
}

float3x4::float3x4(float4 v1, float4 v2, float4 v3)
{
	row[0] = v1.m128;
	row[1] = v2.m128;
	row[2] = v3.m128;
}

float3x4::~float3x4(void)
{

}


void float3x4::Eye(void)
{
	memset(&m00, 0, 12 * sizeof(float));

	m00 = m11 = m22 = 1.f;
}


float3 float3x4::operator*(float4 const& v)
{
	float3 res;

	__m128 a = _mm_dp_ps(row[0], v.m128, 0xf1);
	__m128 b = _mm_dp_ps(row[1], v.m128, 0xf2);
	__m128 c = _mm_dp_ps(row[2], v.m128, 0xf4);

	res.m128 = _mm_add_ps(a, _mm_add_ps(b, c));

	return res;
}

float3x4 operator+(float3x4 const& A, float3x4 const& B)
{
	float3x4 C;

	C.row[0] = _mm_add_ps(A.row[0], B.row[0]);
	C.row[1] = _mm_add_ps(A.row[1], B.row[1]);
	C.row[2] = _mm_add_ps(A.row[2], B.row[2]);

	return C;
}

float3x4 operator-(float3x4 const& A, float3x4 const& B)
{
	float3x4 C;

	C.row[0] = _mm_sub_ps(A.row[0], B.row[0]);
	C.row[1] = _mm_sub_ps(A.row[1], B.row[1]);
	C.row[2] = _mm_sub_ps(A.row[2], B.row[2]);

	return C;
}


float3x4 operator*(float x, float3x4 const& A)
{
	float3x4 B;

	B.row[0] = _mm_mul_ps(A.row[0], _mm_set1_ps(x));
	B.row[1] = _mm_mul_ps(A.row[1], _mm_set1_ps(x));
	B.row[2] = _mm_mul_ps(A.row[2], _mm_set1_ps(x));

	return B;
}


float3x4 float3x4::operator=(float3x4 const& mat)
{
	row[0] = mat.row[0];
	row[1] = mat.row[1];
	row[2] = mat.row[2];

	return *this;
}


float3x4 float3x4::operator=(float4x4 const& mat)
{
	row[0] = mat.row[0];
	row[1] = mat.row[1];
	row[2] = mat.row[2];

	return *this;
}

