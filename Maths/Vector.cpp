#include "Maths.h"
#include <string.h>
#include <xmmintrin.h>

float2::float2(float a, float b)
{
	x = a;
	y = b;
}

float2::float2(float a)
{
	x = a;
	y = a;
}

float2::float2(void)
{

}

float2::~float2(void)
{

}

float2 operator+(float2 const& v1, float2 const& v2)
{
	return float2(v1.x + v2.x, v1.y + v2.y);
}

float2 operator-(float2 const& v1, float2 const& v2)
{
	return float2(v1.x - v2.x, v1.y - v2.y);
}

float2 operator*(float2 const& v1, float2 const& v2)
{
	return float2(v1.x * v2.x, v1.y * v2.y);
}

float2 operator*(float2 const& v, float x)
{
	return float2(v.x * x, v.y * x);
}

float2 operator*(float x, float2 const& v)
{
	return float2(v.x * x, v.y * x);
}

float2 operator/(float2 const& v, float x)
{
	float inv = 1.f / x;
	return float2(v.x * inv, v.y * inv);
}

float2 operator/(float2 const& v1, float2 const& v2)
{
	return float2(v1.x / v2.x, v1.y / v2.y);
}

float2 float2::operator=(float2 const& vec)
{
	x = vec.x;
	y = vec.y;

	return *this;
}

float2 float2::operator+=(float2 const& vec)
{
	x += vec.x;
	y += vec.y;

	return *this;
}

float2 float2::operator-=(float2 const& vec)
{
	x -= vec.x;
	y -= vec.y;

	return *this;
}

float2 float2::operator=(float value)
{
	x = value;
	y = value;

	return *this;
}


float3::float3(float a, float b, float c)
{
	x = a;
	y = b;
	z = c;
}

float3::float3(float a)
{
	x = a;
	y = a;
	z = a;
}

float3::float3(void)
{

}

float3::~float3(void)
{

}

float3 operator+(float3 const& v1, float3 const& v2)
{
	float3 res;

	res.m128 = _mm_add_ps(v1.m128, v2.m128);

	return res;
}

float3 operator-(float3 const& v1, float3 const& v2)
{
	float3 res;

	res.m128 = _mm_sub_ps(v1.m128, v2.m128);

	return res;
}

float3 operator*(float3 const& v1, float3 const& v2)
{
	float3 res;

	res.m128 = _mm_mul_ps(v1.m128, v2.m128);

	return res;
}

float3 operator*(float3 const& v, float x)
{
	float3 res;

	res.m128 = _mm_mul_ps(v.m128, _mm_broadcast_ss(&x));

	return res;
}

float3 operator*(float x, float3 const& v)
{
	float3 res;

	res.m128 = _mm_mul_ps(v.m128, _mm_broadcast_ss(&x));

	return res;
}

float3 operator/(float3 const& v, float x)
{
	float3 res;

	res.m128 = _mm_mul_ps(v.m128, _mm_rcp_ps(_mm_broadcast_ss(&x)));

	return res;
}

float3 operator/(float3 const& v1, float3 const& v2)
{
	float3 res;

	res.m128 = _mm_div_ps(v1.m128, v2.m128);

	return res;
}

float3 float3::operator=(float3 const& vec)
{
	m128 = vec.m128;

	return *this;
}


float3 float3::operator+=(float3 const& vec)
{
	m128 = _mm_add_ps(vec.m128, m128);

	return *this;
}


float3 float3::operator-=(float3 const& vec)
{
	m128 = _mm_sub_ps(m128, vec.m128);

	return *this;
}


float3 float3::operator=(float value)
{
	m128 = _mm_broadcast_ss(&value);

	return *this;
}


float4::float4(float a, float b, float c, float d)
{
	m128 = _mm_set_ps(d, c, b, a);
}

float4::float4(float3 v3, float d)
{
	x = v3.x;
	y = v3.y;
	z = v3.z;
	w = d;
}

float4::float4(float a, float3 v3)
{
	x = a;
	y = v3.x;
	z = v3.y;
	w = v3.z;
}

float4::float4(void)
{

}

float4::~float4(void)
{
	
}

float4 operator+(float4 const& v1, float4 const& v2)
{
	float4 res;

	res.m128 = _mm_add_ps(v1.m128, v2.m128);

	return res;
}

float4 operator-(float4 const& v1, float4 const& v2)
{
	float4 res;

	res.m128 = _mm_sub_ps(v1.m128, v2.m128);

	return res;
}

float4 operator*(float4 const& v1, float4 const& v2)
{
	float4 res;

	res.m128 = _mm_mul_ps(v1.m128, v2.m128);

	return res;
}

float4 operator*(float4 const& v, float x)
{
	float4 res;

	res.m128 = _mm_mul_ps(v.m128, _mm_broadcast_ss(&x));

	return res;
}

float4 operator*(float x, float4 const& v)
{
	float4 res;

	res.m128 = _mm_mul_ps(v.m128, _mm_broadcast_ss(&x));

	return res;
}

float4 operator/(float4 const& v, float x)
{
	float4 res;
	
	res.m128 = _mm_mul_ps(v.m128, _mm_rcp_ps(_mm_broadcast_ss(&x)));

	return res;
}

float4 float4::operator+=(float4 const& vec)
{
	m128 = _mm_add_ps(m128, vec.m128);

	return *this;
}


float4 float4::operator-=(float4 const& vec)
{
	m128 = _mm_sub_ps(m128, vec.m128);

	return *this;
}


float4 float4::operator=(float4 const& vec)
{
	m128 = vec.m128;

	return *this;
}


float4 float4::operator=(float3 const& vec)
{
	m128 = vec.m128;

	return *this;
}
