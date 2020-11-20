#include "Maths.h"

bool Maths::SegTriangleIntersection(TVector Pos, TVector NextPos, TVector4 Vertex[3], float* pDist)
{
	TVector dx;
	TVector Normal;
	TVector Edges[2];
	TVector p, t;
	float det, invdet, u, v;

	Subi(dx, NextPos, Pos);
	Normalize(dx);

	Subi(Edges[0], Vertex[1], Vertex[0]);
	Subi(Edges[1], Vertex[2], Vertex[0]);
	CrossProduct(Normal, Edges[0], Edges[1]);
	Normalize(Normal);

	Subi(p, Pos, Vertex[0]);
	Subi(t, NextPos, Vertex[0]);

	if (DotProduct(p, Normal) * DotProduct(t, Normal) > 0.f)
		return false;

	CrossProduct(p, dx, Edges[1]);
	det = DotProduct(p, Edges[0]);

	if (fabs(det) < 1e-5f)
		return false;

	invdet = 1.f / det;

	Subi(t, Pos, Vertex[0]);
	u = invdet * DotProduct(t, p);

	if (u < 0.f || u > 1.f)
		return false;

	CrossProduct(p, t, Edges[0]);
	v = invdet * DotProduct(dx, p);

	if (v < 0.f || u + v > 1.f)
		return false;

	v = invdet * DotProduct(p, Edges[1]);

	if (v < 1e-5f)
		return false;

	if (pDist != NULL)
	{
		*pDist = -(DotProduct(t, Normal) / DotProduct(dx, Normal));
	}

	return true;
}

