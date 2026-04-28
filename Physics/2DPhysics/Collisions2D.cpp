#include <math.h>

#include "Bodies.h"
#include "Forces.h"
#include "2DPhysics.h"

enum Axis
{
	FACE_A_X,
	FACE_A_Y,
	FACE_B_X,
	FACE_B_Y
};

enum EdgeNumbers
{
	NO_EDGE = 0,
	EDGE1,
	EDGE2,
	EDGE3,
	EDGE4
};

// Local bridges to the engine's Maths API (no free abs/dot/transpose/col/unary-)
static inline float2 neg(const float2& v)                 { return float2(-v.x, -v.y); }
static inline float  dot(const float2& a, const float2& b){ return float2::dotproduct(a, b); }
static inline float2 xy(const float3& v)                  { return float2(v.x, v.y); }
static inline float2 vabs(const float2& v)                { return float2(fabsf(v.x), fabsf(v.y)); }
static inline float2x2 mabs(const float2x2& m)
{
	return float2x2(float2(fabsf(m.m00), fabsf(m.m01)),
	                float2(fabsf(m.m10), fabsf(m.m11)));
}
static inline float2x2 mtranspose(const float2x2& m)
{
	return float2x2(float2(m.m00, m.m10), float2(m.m01, m.m11));
}
static inline float2 col(const float2x2& m, int i)
{
	return i == 0 ? float2(m.m00, m.m10) : float2(m.m01, m.m11);
}

struct ClipVertex
{
	ClipVertex() { fp.value = 0; }
	float2 v;
	ContactManifold2D::FeaturePair fp;
};

static void Flip(ContactManifold2D::FeaturePair& fp)
{
	char temp = fp.e.inEdge1;
	fp.e.inEdge1 = fp.e.inEdge2;
	fp.e.inEdge2 = temp;

	temp = fp.e.outEdge1;
	fp.e.outEdge1 = fp.e.outEdge2;
	fp.e.outEdge2 = temp;
}

static int ClipSegmentToLine(ClipVertex vOut[2], ClipVertex vIn[2],
	const float2& normal, float offset, char clipEdge)
{
	int numOut = 0;

	float distance0 = dot(normal, vIn[0].v) - offset;
	float distance1 = dot(normal, vIn[1].v) - offset;

	if (distance0 <= 0.0f) vOut[numOut++] = vIn[0];
	if (distance1 <= 0.0f) vOut[numOut++] = vIn[1];

	if (distance0 * distance1 < 0.0f)
	{
		float interp = distance0 / (distance0 - distance1);
		vOut[numOut].v = vIn[0].v + (vIn[1].v - vIn[0].v) * interp;
		if (distance0 > 0.0f)
		{
			vOut[numOut].fp = vIn[0].fp;
			vOut[numOut].fp.e.inEdge1 = clipEdge;
			vOut[numOut].fp.e.inEdge2 = NO_EDGE;
		}
		else
		{
			vOut[numOut].fp = vIn[1].fp;
			vOut[numOut].fp.e.outEdge1 = clipEdge;
			vOut[numOut].fp.e.outEdge2 = NO_EDGE;
		}
		++numOut;
	}

	return numOut;
}

static void ComputeIncidentEdge(ClipVertex c[2], const float2& h, const float2& pos,
	const float2x2& Rot, const float2& normal)
{
	float2x2 RotT = mtranspose(Rot);
	float2 n = neg(RotT * normal);
	float2 nAbs = vabs(n);

	if (nAbs.x > nAbs.y)
	{
		if (sign(n.x) > 0)
		{
			c[0].v = float2(h.x, -h.y);
			c[0].fp.e.inEdge2 = EDGE3;
			c[0].fp.e.outEdge2 = EDGE4;

			c[1].v = float2(h.x, h.y);
			c[1].fp.e.inEdge2 = EDGE4;
			c[1].fp.e.outEdge2 = EDGE1;
		}
		else
		{
			c[0].v = float2(-h.x, h.y);
			c[0].fp.e.inEdge2 = EDGE1;
			c[0].fp.e.outEdge2 = EDGE2;

			c[1].v = float2(-h.x, -h.y);
			c[1].fp.e.inEdge2 = EDGE2;
			c[1].fp.e.outEdge2 = EDGE3;
		}
	}
	else
	{
		if (sign(n.y) > 0)
		{
			c[0].v = float2(h.x, h.y);
			c[0].fp.e.inEdge2 = EDGE4;
			c[0].fp.e.outEdge2 = EDGE1;

			c[1].v = float2(-h.x, h.y);
			c[1].fp.e.inEdge2 = EDGE1;
			c[1].fp.e.outEdge2 = EDGE2;
		}
		else
		{
			c[0].v = float2(-h.x, -h.y);
			c[0].fp.e.inEdge2 = EDGE2;
			c[0].fp.e.outEdge2 = EDGE3;

			c[1].v = float2(h.x, -h.y);
			c[1].fp.e.inEdge2 = EDGE3;
			c[1].fp.e.outEdge2 = EDGE4;
		}
	}

	float2x2 R = Rot;
	c[0].v = pos + R * c[0].v;
	c[1].v = pos + R * c[1].v;
}

static void FlipContacts(ContactManifold2D::Contact2D* contacts, int n)
{
	for (int i = 0; i < n; ++i)
	{
		contacts[i].normal = neg(contacts[i].normal);
		float2 tmp = contacts[i].rA;
		contacts[i].rA = contacts[i].rB;
		contacts[i].rB = tmp;
		Flip(contacts[i].feature);
	}
}

int ContactManifold2D::Collide(PhysicsBody2D& bodyA, PhysicsBody2D& bodyB, Contact2D* contacts, float2 wrapB)
{
	auto va = bodyA.getShapeView();
	auto vb = bodyB.getShapeView();

	using K = PhysicsBody2D::ShapeKind;

	// Static vs static: ignored.
	if (va.Kind == K::Static && vb.Kind == K::Static)
		return 0;

	// When we swap argument order in the dispatcher (to reuse a canonical
	// implementation), the "wrap bodyB to image near bodyA" offset flips sign.
	float2 wrapSwapped = neg(wrapB);

	switch (va.Kind)
	{
	case K::Box:
		if (vb.Kind == K::Box)    return CollideBoxBox(*va.Box, *vb.Box, contacts, wrapB);
		if (vb.Kind == K::Sphere) return CollideBoxSphere(*va.Box, *vb.Sphere, contacts, wrapB);
		/* Static */              return CollideBoxStatic(*va.Box, *vb.Static, contacts);

	case K::Sphere:
		if (vb.Kind == K::Box)
		{
			int n = CollideBoxSphere(*vb.Box, *va.Sphere, contacts, wrapSwapped);
			FlipContacts(contacts, n);
			return n;
		}
		if (vb.Kind == K::Sphere) return CollideSphereSphere(*va.Sphere, *vb.Sphere, contacts, wrapB);
		/* Static */              return CollideSphereStatic(*va.Sphere, *vb.Static, contacts);

	case K::Static:
		if (vb.Kind == K::Box)
		{
			int n = CollideBoxStatic(*vb.Box, *va.Static, contacts);
			FlipContacts(contacts, n);
			return n;
		}
		if (vb.Kind == K::Sphere)
		{
			int n = CollideSphereStatic(*vb.Sphere, *va.Static, contacts);
			FlipContacts(contacts, n);
			return n;
		}
		return 0;
	}
	return 0;
}

int ContactManifold2D::CollideBoxBox(RigidBox2D& bodyA, RigidBox2D& bodyB, Contact2D* contacts, float2 wrapB)
{
	float2 normal;

	// Setup
	float2 hA = bodyA.GetSize() * 0.5f;
	float2 hB = bodyB.GetSize() * 0.5f;

	float2 posA = xy(bodyA.m_Position);
	float2 posB = xy(bodyB.m_Position) + wrapB;  // image of B closest to A across any wrap seam

	float2x2 RotA = rotation(bodyA.m_Position.z);
	float2x2 RotB = rotation(bodyB.m_Position.z);

	float2x2 RotAT = mtranspose(RotA);
	float2x2 RotBT = mtranspose(RotB);

	float2 dp = posB - posA;
	float2 dA = RotAT * dp;
	float2 dB = RotBT * dp;

	float2x2 C = RotAT * RotB;
	float2x2 absC = mabs(C);
	float2x2 absCT = mtranspose(absC);

	// Box A faces
	float2 faceA = vabs(dA) - hA - absC * hB;
	if (faceA.x > 0.0f || faceA.y > 0.0f)
		return 0;

	// Box B faces
	float2 faceB = vabs(dB) - absCT * hA - hB;
	if (faceB.x > 0.0f || faceB.y > 0.0f)
		return 0;

	// Find best axis
	Axis axis;
	float separation;

	// Box A faces
	axis = FACE_A_X;
	separation = faceA.x;
	normal = dA.x > 0.0f ? col(RotA, 0) : neg(col(RotA, 0));

	const float relativeTol = 0.95f;
	const float absoluteTol = 0.01f;

	if (faceA.y > relativeTol * separation + absoluteTol * hA.y)
	{
		axis = FACE_A_Y;
		separation = faceA.y;
		normal = dA.y > 0.0f ? col(RotA, 1) : neg(col(RotA, 1));
	}

	// Box B faces
	if (faceB.x > relativeTol * separation + absoluteTol * hB.x)
	{
		axis = FACE_B_X;
		separation = faceB.x;
		normal = dB.x > 0.0f ? col(RotB, 0) : neg(col(RotB, 0));
	}

	if (faceB.y > relativeTol * separation + absoluteTol * hB.y)
	{
		axis = FACE_B_Y;
		separation = faceB.y;
		normal = dB.y > 0.0f ? col(RotB, 1) : neg(col(RotB, 1));
	}

	// Setup clipping plane data based on the separating axis
	float2 frontNormal, sideNormal;
	ClipVertex incidentEdge[2];
	float front, negSide, posSide;
	char negEdge, posEdge;

	// Compute the clipping lines and the line segment to be clipped.
	switch (axis)
	{
	case FACE_A_X:
	{
		frontNormal = normal;
		front = dot(posA, frontNormal) + hA.x;
		sideNormal = col(RotA, 1);
		float side = dot(posA, sideNormal);
		negSide = -side + hA.y;
		posSide = side + hA.y;
		negEdge = EDGE3;
		posEdge = EDGE1;
		ComputeIncidentEdge(incidentEdge, hB, posB, RotB, frontNormal);
	}
	break;

	case FACE_A_Y:
	{
		frontNormal = normal;
		front = dot(posA, frontNormal) + hA.y;
		sideNormal = col(RotA, 0);
		float side = dot(posA, sideNormal);
		negSide = -side + hA.x;
		posSide = side + hA.x;
		negEdge = EDGE2;
		posEdge = EDGE4;
		ComputeIncidentEdge(incidentEdge, hB, posB, RotB, frontNormal);
	}
	break;

	case FACE_B_X:
	{
		frontNormal = neg(normal);
		front = dot(posB, frontNormal) + hB.x;
		sideNormal = col(RotB, 1);
		float side = dot(posB, sideNormal);
		negSide = -side + hB.y;
		posSide = side + hB.y;
		negEdge = EDGE3;
		posEdge = EDGE1;
		ComputeIncidentEdge(incidentEdge, hA, posA, RotA, frontNormal);
	}
	break;

	case FACE_B_Y:
	{
		frontNormal = neg(normal);
		front = dot(posB, frontNormal) + hB.y;
		sideNormal = col(RotB, 0);
		float side = dot(posB, sideNormal);
		negSide = -side + hB.x;
		posSide = side + hB.x;
		negEdge = EDGE2;
		posEdge = EDGE4;
		ComputeIncidentEdge(incidentEdge, hA, posA, RotA, frontNormal);
	}
	break;
	}

	// clip other face with 5 box planes (1 face plane, 4 edge planes)
	ClipVertex clipPoints1[2];
	ClipVertex clipPoints2[2];
	int np;

	// Clip to box side 1
	np = ClipSegmentToLine(clipPoints1, incidentEdge, neg(sideNormal), negSide, negEdge);
	if (np < 2)
		return 0;

	// Clip to negative box side 1
	np = ClipSegmentToLine(clipPoints2, clipPoints1, sideNormal, posSide, posEdge);
	if (np < 2)
		return 0;

	// Now clipPoints2 contains the clipping points.
	// Due to roundoff, it is possible that clipping removes all points.
	int numContacts = 0;
	for (int i = 0; i < 2; ++i)
	{
		float sep = dot(frontNormal, clipPoints2[i].v) - front;

		if (sep <= 0)
		{
			contacts[numContacts].normal = neg(normal);

			// slide contact point onto reference face (easy to cull)
			contacts[numContacts].rA = mtranspose(RotA) * (clipPoints2[i].v - frontNormal * sep - posA);
			contacts[numContacts].rB = mtranspose(RotB) * (clipPoints2[i].v - posB);
			contacts[numContacts].feature = clipPoints2[i].fp;

			if (axis == FACE_B_X || axis == FACE_B_Y)
			{
				Flip(contacts[numContacts].feature);
				contacts[numContacts].rA = mtranspose(RotA) * (clipPoints2[i].v - posA);
				contacts[numContacts].rB = mtranspose(RotB) * (clipPoints2[i].v - frontNormal * sep - posB);
			}
			++numContacts;
		}
	}

	return numContacts;
}

int ContactManifold2D::CollideBoxSphere(RigidBox2D& bodyA, RigidSphere2D& bodyB, Contact2D* contacts, float2 wrapB)
{
	float2 posA = xy(bodyA.m_Position);
	float2 posB = xy(bodyB.m_Position) + wrapB;  // image of B closest to A across any wrap seam
	float2 h   = bodyA.GetSize() * 0.5f;
	float  rB  = bodyB.GetRadius();

	float2x2 RotA  = rotation(bodyA.m_Position.z);
	float2x2 RotAT = mtranspose(RotA);

	// Sphere center in box-local frame
	float2 local = RotAT * (posB - posA);

	// Closest point on box (local frame)
	float2 closest = float2(clamp(local.x, -h.x, h.x),
	                        clamp(local.y, -h.y, h.y));

	float2 diff   = local - closest;
	float  distSq = dot(diff, diff);

	if (distSq > rB * rB)
		return 0;

	float  dist        = sqrtf(distSq);
	float2 localNormal = dist > 1e-6f ? diff / dist : float2(1.0f, 0.0f);
	float2 worldNormal = RotA * localNormal;

	// Stored normal: from B (sphere) into A (box), matching CollideBoxBox.
	float2 storedNormal = neg(worldNormal);

	// rA: closest point on the box in box-local frame (already local).
	// rB: sphere's surface point toward the box, transformed to sphere-local
	// so Initialize's rotate(angleB, rB) reproduces the world offset.
	float2x2 RotBT = mtranspose(rotation(bodyB.m_Position.z));

	contacts[0].normal        = storedNormal;
	contacts[0].rA            = closest;
	contacts[0].rB            = RotBT * (storedNormal * rB);
	contacts[0].feature.value = 0;
	return 1;
}

int ContactManifold2D::CollideSphereSphere(RigidSphere2D& bodyA, RigidSphere2D& bodyB, Contact2D* contacts, float2 wrapB)
{
	float2 posA = xy(bodyA.m_Position);
	float2 posB = xy(bodyB.m_Position) + wrapB;  // image of B closest to A across any wrap seam
	float  rA   = bodyA.GetRadius();
	float  rB   = bodyB.GetRadius();

	float2 d     = posB - posA;
	float  distSq = dot(d, d);
	float  rSum  = rA + rB;

	if (distSq > rSum * rSum)
		return 0;

	float  dist   = sqrtf(distSq);
	float2 normal = dist > 1e-6f ? d / dist : float2(1.0f, 0.0f); // from A to B

	// Stored normal: from B to A. Store each sphere's surface point in its own
	// local frame so Initialize's rotate(angle, r) puts them back along the
	// contact normal regardless of the sphere's current angle.
	float2 storedNormal = neg(normal);
	float2x2 RotAT = mtranspose(rotation(bodyA.m_Position.z));
	float2x2 RotBT = mtranspose(rotation(bodyB.m_Position.z));

	contacts[0].normal        = storedNormal;
	contacts[0].rA            = RotAT * (normal * rA);        // A -> B
	contacts[0].rB            = RotBT * (storedNormal * rB);  // B -> A
	contacts[0].feature.value = 0;
	return 1;
}

int ContactManifold2D::CollideSphereStatic(RigidSphere2D& bodyA, RigidStatic2D& bodyB, Contact2D* contacts)
{
	float2 posA = xy(bodyA.m_Position);
	float2 posB = xy(bodyB.m_Position);
	float  rA   = bodyA.GetRadius();

	float d = bodyB.SampleSDF(posA);
	if (d > rA)
		return 0;

	float2 n = bodyB.SampleSDFNormal(posA); // outward from static -> from B to A
	float2 worldContact = posA - n * d;     // surface point of static closest to sphere

	// rA: sphere's surface point toward the terrain (opposite the outward normal),
	// stored in sphere-local so Initialize's rotate(angleA, rA) lands back on -n*rA.
	// rB: terrain-local offset to the contact point.
	float2x2 RotAT = mtranspose(rotation(bodyA.m_Position.z));
	float2x2 RotBT = mtranspose(rotation(bodyB.m_Position.z));

	contacts[0].normal        = n;
	contacts[0].rA            = RotAT * (n * (-rA));
	contacts[0].rB            = RotBT * (worldContact - posB);
	contacts[0].feature.value = 0;
	return 1;
}

int ContactManifold2D::CollideBoxStatic(RigidBox2D& bodyA, RigidStatic2D& bodyB, Contact2D* contacts)
{
	float2 posA = xy(bodyA.m_Position);
	float2 posB = xy(bodyB.m_Position);
	float2 h    = bodyA.GetSize() * 0.5f;

	float2x2 RotA  = rotation(bodyA.m_Position.z);
	float2x2 RotAT = mtranspose(RotA);

	// Sample SDF at the four box corners; keep the two most-penetrating negatives.
	const float2 localCorners[4] = {
		float2(-h.x, -h.y),
		float2( h.x, -h.y),
		float2( h.x,  h.y),
		float2(-h.x,  h.y),
	};

	int   bestIdx[2] = { -1, -1 };
	float bestD[2]   = { 0.0f, 0.0f };

	for (int i = 0; i < 4; ++i)
	{
		float2 worldCorner = posA + RotA * localCorners[i];
		float  d           = bodyB.SampleSDF(worldCorner);
		if (d >= 0.0f)
			continue;

		// Insert into a sorted (most-negative first) size-2 selection.
		if (bestIdx[0] < 0 || d < bestD[0])
		{
			bestIdx[1] = bestIdx[0]; bestD[1] = bestD[0];
			bestIdx[0] = i;          bestD[0] = d;
		}
		else if (bestIdx[1] < 0 || d < bestD[1])
		{
			bestIdx[1] = i; bestD[1] = d;
		}
	}

	int numContacts = 0;
	for (int k = 0; k < 2; ++k)
	{
		if (bestIdx[k] < 0)
			break;

		int    i           = bestIdx[k];
		float  d           = bestD[k];
		float2 worldCorner = posA + RotA * localCorners[i];
		float2 n           = bodyB.SampleSDFNormal(worldCorner);
		float2 surfacePt   = worldCorner - n * d; // d is negative, pushes outward

		contacts[numContacts].normal        = n;  // from B (static) to A
		contacts[numContacts].rA            = RotAT * (worldCorner - posA); // box-local
		contacts[numContacts].rB            = surfacePt - posB;
		contacts[numContacts].feature.value = 0;
		contacts[numContacts].feature.e.inEdge1  = (char)(i + 1);
		contacts[numContacts].feature.e.outEdge1 = (char)(i + 1);
		++numContacts;
	}

	return numContacts;
}
