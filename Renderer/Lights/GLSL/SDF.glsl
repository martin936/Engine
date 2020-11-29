#extension GL_EXT_samplerless_texture_functions : require


float SampleSDFVisibility(in texture3D SDF, in sampler sampLinear, in vec3 pos, in vec3 dir, in float dmax)
{
	float t = 0.f;

	float d = textureLod(sampler3D(SDF, sampLinear), (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f, 0).r;

	if (d < 0.f)
	{
		t = 0.05f;
		pos += t * dir;
		d = textureLod(sampler3D(SDF, sampLinear), (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f, 0).r;
	}

	float dmin = 1.f;

	while (d > 0.f)
	{
		pos += (d + 0.02f) * dir;
		t += d + 0.02f;

		vec3 p = (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f;

		if (t > dmax || (p.x * (1.f - p.x) < 0.f) || (p.y * (1.f - p.y) < 0.f) || (p.z * (1.f - p.z) < 0.f))
			return dmin * dmin;

		d = textureLod(sampler3D(SDF, sampLinear), p, 0).r;
		dmin = min(dmin, 2.f * d / t);
	}

	return 0.f;
}

