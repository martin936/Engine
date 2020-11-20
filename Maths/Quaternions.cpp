#include "Engine/Engine.h"
#include "Maths.h"


Quaternion::Quaternion()
{
	x = y = z = w = 0.f;
}


Quaternion::Quaternion(float a)
{
	x = y = z = w = a;
}


Quaternion::Quaternion(float a, float b, float c, float d)
{
	w = a;
	x = b;
	y = c;
	z = d;
}


Quaternion operator+(Quaternion const& H1, Quaternion const& H2)
{
	Quaternion H;

	H.x = H1.x + H2.x;
	H.y = H1.y + H2.y;
	H.z = H1.z + H2.z;
	H.w = H1.w + H2.w;

	return H;
}


Quaternion operator-(Quaternion const& H1, Quaternion const& H2)
{
	Quaternion H;

	H.x = H1.x - H2.x;
	H.y = H1.y - H2.y;
	H.z = H1.z - H2.z;
	H.w = H1.w - H2.w;

	return H;
}


Quaternion operator*(Quaternion const& H1, Quaternion const& H2)
{
	Quaternion H;

	H.w = H1.w * H2.w - H1.x * H2.x - H1.y * H2.y - H1.z * H2.z;
	H.x = H1.x * H2.w + H1.w * H2.x + H1.y * H2.z - H1.z * H2.y;
	H.y = H1.y * H2.w + H1.w * H2.y + H1.z * H2.x - H1.x * H2.z;
	H.z = H1.z * H2.w + H1.w * H2.z + H1.x * H2.y - H1.y * H2.x;

	return H;
}

Quaternion operator*(Quaternion const& H1, float val)
{
	Quaternion H;

	H.w = H1.w * val;
	H.x = H1.x * val;
	H.y = H1.y * val;
	H.z = H1.z * val;

	return H;
}

Quaternion operator/(Quaternion const& H1, float val)
{
	Quaternion H;

	H.w = H1.w / val;
	H.x = H1.x / val;
	H.y = H1.y / val;
	H.z = H1.z / val;

	return H;
}


Quaternion Quaternion::operator=(Quaternion const& H)
{
	x = H.x;
	y = H.y;
	z = H.z;
	w = H.w;

	return *this;
}


Quaternion Quaternion::operator+=(Quaternion const& H)
{
	x += H.x;
	y += H.y;
	z += H.z;
	w += H.w;

	return *this;
}


Quaternion Quaternion::operator-=(Quaternion const& H)
{
	x -= H.x;
	y -= H.y;
	z -= H.z;
	w -= H.w;

	return *this;
}


Quaternion Quaternion::operator=(float a)
{
	x = y = z = w = a;

	return *this;
}


void Quaternion::Normalize()
{
	float norm = 1.f / MAX(1e-4f, sqrtf(w*w + x*x + y*y + z*z));

	w *= norm;
	x *= norm;
	y *= norm;
	z *= norm;
}


float4x4 Quaternion::GetMatrix(Quaternion H, float3 Position)
{
	float4x4 World(0.f);

	World.m00 = 1.f - 2.f * H.y * H.y - 2.f * H.z * H.z;
	World.m10 = 2.f * H.x * H.y - 2.f * H.z * H.w;
	World.m20 = 2.f * H.x * H.z + 2.f * H.y * H.w;
	World.m01 = 2.f * H.x * H.y + 2.f * H.z * H.w;
	World.m11 = 1.f - 2.f * H.x * H.x - 2.f * H.z * H.z;
	World.m21 = 2.f * H.y * H.z - 2.f * H.x * H.w;
	World.m02 = 2.f * H.x * H.z - 2.f * H.y * H.w;
	World.m12 = 2.f * H.y * H.z + 2.f * H.x * H.w;
	World.m22 = 1.f - 2.f * H.x * H.x - 2.f * H.y * H.y;


	World.m03 = Position.x;
	World.m13 = Position.y;
	World.m23 = Position.z;
	World.m33 = 1.f;

	return World;
}


float3x3 Quaternion::GetMatrix(Quaternion H)
{
	float3x3 World(0.f);

	World.m00 = 1.f - 2.f * H.y * H.y - 2.f * H.z * H.z;
	World.m10 = 2.f * H.x * H.y - 2.f * H.z * H.w;
	World.m20 = 2.f * H.x * H.z + 2.f * H.y * H.w;
	World.m01 = 2.f * H.x * H.y + 2.f * H.z * H.w;
	World.m11 = 1.f - 2.f * H.x * H.x - 2.f * H.z * H.z;
	World.m21 = 2.f * H.y * H.z - 2.f * H.x * H.w;
	World.m02 = 2.f * H.x * H.z - 2.f * H.y * H.w;
	World.m12 = 2.f * H.y * H.z + 2.f * H.x * H.w;
	World.m22 = 1.f - 2.f * H.x * H.x - 2.f * H.y * H.y;

	return World;
}


void Quaternion::GetEulerAngles(float* fPhi, float* fTheta, float* fPsi)
{
	float ysqr = y * y;

	// roll (x-axis rotation)
	float t0 = +2.f * (w * x + y * z);
	float t1 = +1.f - 2.f * (x * x + ysqr);
	*fPhi = atan2f(t0, t1);

	// pitch (y-axis rotation)
	float t2 = +2.f * (w * y - z * x);
	t2 = t2 > 1.f ? 1.f : t2;
	t2 = t2 < -1.f ? -1.f : t2;
	*fTheta = -asinf(t2);

	// yaw (z-axis rotation)
	float t3 = +2.f * (w * z + x * y);
	float t4 = +1.f - 2.f * (ysqr + z * z);
	*fPsi = atan2f(t3, t4);
}


void Quaternion::ApplyEulerAngles(float fPhi, float fTheta, float fPsi)
{
	float t0 = cosf(fPsi * 0.5f);
	float t1 = sinf(fPsi * 0.5f);
	float t2 = cosf(fPhi * 0.5f);
	float t3 = sinf(fPhi * 0.5f);
	float t4 = cosf(-fTheta * 0.5f);
	float t5 = sinf(-fTheta * 0.5f);

	w = t0 * t2 * t4 + t1 * t3 * t5;
	x = t0 * t3 * t4 - t1 * t2 * t5;
	y = t0 * t2 * t5 + t1 * t3 * t4;
	z = t1 * t2 * t4 - t0 * t3 * t5;
}


DualQuaternion::DualQuaternion()
{
	q0 = 0.f;
	q1 = 0.f;
}


DualQuaternion::DualQuaternion(Quaternion& a, Quaternion& b)
{
	q0 = a;
	q1 = b;
}


DualQuaternion::DualQuaternion(Quaternion& q, float3& t)
{
	q0 = q;

	q1.w = -0.5f * (t.x * q.x + t.y * q.y + t.z * q.z);
	q1.x = 0.5f * (t.x * q.w + t.y * q.z - t.z * q.y);
	q1.y = 0.5f * (-t.x * q.z + t.y * q.w + t.z * q.x);
	q1.z = 0.5f * (t.x * q.y - t.y * q.x + t.z * q.w);
}


DualQuaternion DualQuaternion::operator=(DualQuaternion const& H)
{
	q0 = H.q0;
	q1 = H.q1;

	return *this;
}


DualQuaternion DualQuaternion::operator+=(DualQuaternion const& H)
{
	q0 += H.q0;
	q1 += H.q1;

	return *this;
}


DualQuaternion DualQuaternion::operator-=(DualQuaternion const& H)
{
	q0 -= H.q0;
	q1 -= H.q1;

	return *this;
}


DualQuaternion operator+(DualQuaternion const& H1, DualQuaternion const& H2)
{
	DualQuaternion H;

	H.q0 = H1.q0 + H2.q0;
	H.q1 = H1.q1 + H2.q1;

	return H;
}


DualQuaternion operator-(DualQuaternion const& H1, DualQuaternion const& H2)
{
	DualQuaternion H;

	H.q0 = H1.q0 - H2.q0;
	H.q1 = H1.q1 - H2.q1;

	return H;
}


DualQuaternion operator*(DualQuaternion const& H1, DualQuaternion const& H2)
{
	DualQuaternion H;

	H.q0 = H1.q0 * H2.q0;
	H.q1 = (H1.q0 * H2.q1) / MAX(1e-5f, H1.q0.length()) + H1.q1 * H2.q0;

	return H;
}
