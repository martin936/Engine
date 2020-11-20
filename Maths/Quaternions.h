#ifndef __QUATERNIONS_H__
#define __QUATERNIONS_H__


class Quaternion
{
public:

	Quaternion(float a, float b, float c, float d);
	Quaternion(float a);
	Quaternion();

	float	w;
	float	x;
	float	y;
	float	z;

	static float4x4 GetMatrix(Quaternion H, float3 Position);
	static float3x3 GetMatrix(Quaternion H);

	void GetEulerAngles(float* fPhi, float* fTheta, float* fPsi);
	void ApplyEulerAngles(float fPhi, float fTheta, float fPsi);

	void Normalize();

	inline float length() const
	{
		return sqrt(x*x + y*y + z*z + w*w);
	}

	Quaternion operator=(Quaternion const& vec);
	Quaternion operator=(float value);
	Quaternion operator+=(Quaternion const& vec);
	Quaternion operator-=(Quaternion const& vec);
};

Quaternion operator+(Quaternion const& H1, Quaternion const& H2);
Quaternion operator-(Quaternion const& H1, Quaternion const& H2);
Quaternion operator*(Quaternion const& H1, Quaternion const& H2);
Quaternion operator*(Quaternion const& H1, float val);
Quaternion operator/(Quaternion const& H1, float val);


_declspec(align(32)) class DualQuaternion
{
public:

	DualQuaternion(Quaternion& q1, Quaternion& q2);
	DualQuaternion(Quaternion& q, float3& t);
	DualQuaternion();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	Quaternion q0;
	Quaternion q1;

	DualQuaternion operator=(DualQuaternion const& vec);
	DualQuaternion operator=(float value);
	DualQuaternion operator+=(DualQuaternion const& vec);
	DualQuaternion operator-=(DualQuaternion const& vec);
};

DualQuaternion operator+(DualQuaternion const& H1, DualQuaternion const& H2);
DualQuaternion operator-(DualQuaternion const& H1, DualQuaternion const& H2);
DualQuaternion operator*(DualQuaternion const& H1, DualQuaternion const& H2);


#endif
