#ifndef MATHS_INC
#define MATHS_INC

#include <math.h>
#include <iostream>
#include <xmmintrin.h>
#include <immintrin.h>

#ifndef min
#define min(a, b) (a < b ? a : b)
#endif

#ifndef max
#define max(a, b) (a < b ? b : a)
#endif

typedef float TVector[3];
typedef float TVector4[4];


inline int Randi(int nMin, int nMax)
{
	return std::rand() % (nMax - nMin) + nMin;
}


inline float Randf(float fMin, float fMax)
{
	return (fMax - fMin) * std::rand() / RAND_MAX + fMin;
}


inline void Copy(float* pDst, const float* pSrc)
{
	pDst[0] = pSrc[0];
	pDst[1] = pSrc[1];
	pDst[2] = pSrc[2];
}

inline void Addi(float* pDst, const float* pSrc)
{
	pDst[0] += pSrc[0];
	pDst[1] += pSrc[1];
	pDst[2] += pSrc[2];
}

inline void Addi(float* pDst, const float* pSrc1, const float* pSrc2)
{
	pDst[0] = pSrc1[0] + pSrc2[0];
	pDst[1] = pSrc1[1] + pSrc2[1];
	pDst[2] = pSrc1[2] + pSrc2[2];
}

inline void Subi(float* pDst, const float* pSrc)
{
	pDst[0] -= pSrc[0];
	pDst[1] -= pSrc[1];
	pDst[2] -= pSrc[2];
}

inline void Subi(float* pDst, const float* pSrc1, const float* pSrc2)
{
	pDst[0] = pSrc1[0] - pSrc2[0];
	pDst[1] = pSrc1[1] - pSrc2[1];
	pDst[2] = pSrc1[2] - pSrc2[2];
}

inline void Normalize(float* pVec)
{
	float fInvNorme = 1.f / sqrtf(pVec[0] * pVec[0] + pVec[1] * pVec[1] + pVec[2] * pVec[2]);
	
	if (isnan(fInvNorme) || fInvNorme > 1e8f)
		return;

	pVec[0] *= fInvNorme;
	pVec[1] *= fInvNorme;
	pVec[2] *= fInvNorme;
}

inline void Normalize(float* pDst, const float* pVec)
{
	float fInvNorme = 1.f / sqrtf(pVec[0] * pVec[0] + pVec[1] * pVec[1] + pVec[2] * pVec[2]);

	pDst[0] = pVec[0] * fInvNorme;
	pDst[1] = pVec[1] * fInvNorme;
	pDst[2] = pVec[2] * fInvNorme;
}

inline float DotProduct(const float* a, const float* b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline void CrossProduct(float* pDst, const float* pSrc1, const float* pSrc2)
{
	pDst[0] = pSrc1[1] * pSrc2[2] - pSrc1[2] * pSrc2[1];
	pDst[1] = pSrc1[2] * pSrc2[0] - pSrc1[0] * pSrc2[2];
	pDst[2] = pSrc1[0] * pSrc2[1] - pSrc1[1] * pSrc2[0];
}

inline void Scale(float* pVec, float factor)
{
	pVec[0] *= factor;
	pVec[1] *= factor;
	pVec[2] *= factor;
}

inline void Scale(float* pDst, float* pSrc, float factor)
{
	pDst[0] = pSrc[0] * factor;
	pDst[1] = pSrc[1] * factor;
	pDst[2] = pSrc[2] * factor;
}

inline float Length(const float* pVec)
{
	return sqrtf(pVec[0] * pVec[0] + pVec[1] * pVec[1] + pVec[2] * pVec[2]);
}


#ifndef clamp
inline float clamp(float x, float xmin, float xmax)
{
	return max(min(x, xmax), xmin);
}


inline int clamp(int x, int xmin, int xmax)
{
	return max(min(x, xmax), xmin);
}
#endif


inline int sign(float x)
{
	return x < 0.f ? -1 : 1;
}


__declspec(align(16)) class float3
{
public:

	float3(float x, float y, float z);
	float3(float x);
	float3(void);
	~float3(void);

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 16);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	inline float length()	
	{ 
		return sqrtf(x*x + y*y + z*z); 
	}

	inline void normalize()
	{
		float l = 1.f / length();
		
		m128 = _mm_mul_ps(m128, _mm_broadcast_ss(&l));
	}

	inline static float3 normalize(float3& v)
	{
		float3 res(v);
		res.normalize();

		return res;
	}

	union
	{
		struct
		{
			float x;
			float y;
			float z;
		};

		__m128 m128;
	};

	inline float* v()
	{
		return &x;
	}

	float3 operator=(float3 const& vec);
	float3 operator=(float value);
	float3 operator+=(float3 const& vec);
	float3 operator-=(float3 const& vec);

	float operator[](int i)
	{
		return *(&x + i);
	}

	float3 operator*=(float v)
	{
		m128 = _mm_mul_ps(m128, _mm_broadcast_ss(&v));

		return *this;
	}

	float3 operator/=(float v)
	{
		m128 = _mm_mul_ps(m128, _mm_rcp_ps(_mm_broadcast_ss(&v)));

		return *this;
	}

	inline static float dotproduct(float3 const& v1, float3 const& v2)
	{ 
		__m128 res = _mm_dp_ps(v1.m128, v2.m128, 0x71);

		return *(float*)&res;
	}

	inline static float3 cross(float3 const& v1, float3 const& v2) 
	{ 
		float3 res;

		__m128 a = _mm_mul_ps(_mm_permute_ps(v1.m128, 0xd2), _mm_permute_ps(v2.m128, 0xc9));

		res.m128 = _mm_fmsub_ps(_mm_permute_ps(v1.m128, 0xc9), _mm_permute_ps(v2.m128, 0xd2), a);

		return res; 
	}
};

float3 operator+(float3 const& v1, float3 const& v2);
float3 operator-(float3 const& v1, float3 const& v2);
float3 operator*(float3 const& v, float x);
float3 operator*(float3 const& v1, float3 const& v2);
float3 operator*(float x, float3 const& v);
float3 operator/(float3 const& v, float x);
float3 operator/(float3 const& v1, float3 const& v2);


__declspec(align(16)) class float4
{
public:

	float4(float x, float y, float z, float w);
	float4(float3 v, float w);
	float4(float x, float3 v);
	float4(void);
	~float4(void);

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 16);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	inline float length()	
	{ 
		return sqrtf(x*x + y*y + z*z); 
	}

	inline void normalize()
	{
		float l = 1.f / length();
		x *= l;
		y *= l;
		z *= l;
	}

	union
	{
		struct
		{
			float x;
			float y;
			float z;
			float w;
		};

		__m128 m128;
	};

	inline float* v()
	{
		return &x;
	}

	float operator[](int i)
	{
		return *(&x + i);
	}

	float4 operator*=(float v)
	{
		m128 = _mm_mul_ps(m128, _mm_broadcast_ss(&v));

		return *this;
	}

	float4 operator/=(float v)
	{
		m128 = _mm_mul_ps(m128, _mm_rcp_ps(_mm_broadcast_ss(&v)));

		return *this;
	}

	float4 operator=(float value)
	{
		m128 = _mm_broadcast_ss(&value);

		return *this;
	}

	float4 operator=(float4 const& vec);
	float4 operator=(float3 const& vec);
	float4 operator+=(float4 const& vec);
	float4 operator-=(float4 const& vec);

};

float4 operator+(float4 const& v1, float4 const& v2);
float4 operator-(float4 const& v1, float4 const& v2);
float4 operator*(float4 const& v1, float4 const& v2);
float4 operator*(float4 const& v, float x);
float4 operator*(float x, float4 const& v);
float4 operator/(float4 const& v, float x);


__declspec(align(16)) class float3x3
{
public:

	float3x3(void);
	float3x3(float x);
	float3x3(float3 v1, float3 v2, float3 v3);
	~float3x3(void);

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 16);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	union
	{
		struct
		{
			float m00;
			float m01;
			float m02;
			float m10;
			float m11;
			float m12;
			float m20;
			float m21;
			float m22;
		};

		__m128 rows[3];
	};

	inline float* m()
	{
		return &m00;
	}

	inline float data(int i) const 
	{
		return *(&m00 + i);
	}

	void inverse(void);
	void transpose(void);

	float3 operator*(float3 const& v);
	float3x3 operator=(float3x3 const& vec);
};

float3x3 inverse(float3x3 const& mat);

float3x3 operator+(float3x3 const& A, float3x3 const& B);
float3x3 operator-(float3x3 const& A, float3x3 const& B);
float3x3 operator*(float3x3 const& A, float3x3 const& B);
float3x3 operator*(float x, float3x3 const& A);


__declspec(align(32)) class float4x4
{
public:

	float4x4(void);
	float4x4(float x);
	float4x4(float4 v1, float4 v2, float4 v3, float4 v4);
	~float4x4(void);

	void* operator new(size_t i)
	{
		return _mm_malloc(i,32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	union
	{
		struct
		{
			float m00;
			float m01;
			float m02;
			float m03;
			float m10;
			float m11;
			float m12;
			float m13;
			float m20;
			float m21;
			float m22;
			float m23;
			float m30;
			float m31;
			float m32;
			float m33;
		};

		__m128 row[4];
		__m256 m256[2];
	};

	inline float* m()
	{
		return &m00;
	}

	inline float data(int i) const
	{
		return *(&m00 + i);
	}

	void transpose(void);
	void Eye(void);

	float4 operator*(float4 const& v);
	float4x4 operator=(float4x4 const& vec);
};

float4x4 inverse(float4x4 const& mat);

bool operator!=(float4x4 const& A, float4x4& B);
bool operator==(float4x4 const& A, float4x4& B);

float4x4 operator+(float4x4 const& A, float4x4 const& B);
float4x4 operator-(float4x4 const& A, float4x4 const& B);
float4x4 operator*(float4x4 const& A, float4x4 const& B);
float4x4 operator*(float x, float4x4 const& A);


class float3x4
{
public:

	float3x4(void);
	float3x4(float x);
	float3x4(float4 v1, float4 v2, float4 v3);
	~float3x4(void);

	union
	{
		struct
		{
			float m00;
			float m01;
			float m02;
			float m03;
			float m10;
			float m11;
			float m12;
			float m13;
			float m20;
			float m21;
			float m22;
			float m23;
		};

		__m128 row[3];
	};

	inline float* m()
	{
		return &m00;
	}

	inline float data(int i) const
	{
		return *(&m00 + i);
	}

	void Eye(void);

	float3 operator*(float4 const& v);
	float3x4 operator=(float3x4 const& vec);
	float3x4 operator=(float4x4 const& mat);
};

float3x4 operator+(float3x4 const& A, float3x4 const& B);
float3x4 operator-(float3x4 const& A, float3x4 const& B);
float3x4 operator*(float x, float3x4 const& A);


namespace Maths
{
	bool	SegTriangleIntersection(TVector Pos, TVector NextPos, TVector4 Vertex[3], float* pDist = NULL);
	bool	CubeTriangleIntersection(float3 Triangle[3], float3& Point);

	float	ComputeBoundingSphere(float3& Center, float* pPoints, int nNumPoints, int nStride);
}

#include "Quaternions.h"


#endif
