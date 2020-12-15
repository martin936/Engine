#extension GL_EXT_samplerless_texture_functions : require

#define SDF_H

#define CB_DECL(x)		layout(binding = x, std140) uniform cb ## x
#define DECLARE_CB(x)	CB_DECL(x)


DECLARE_CB(SDF_CB_SLOT)
{
	vec4	m_SDFCenter[64];
	vec4	m_SDFSize[64];

	int		m_NumSDFs;
};


layout(binding = SDF_TEX_SLOT)				uniform texture3D		SDFTex[];

#ifdef VOLUME_ALBEDO_TEX_SLOT
layout(binding = VOLUME_ALBEDO_TEX_SLOT)	uniform texture3D		VolumeAlbedo[];
#endif

vec3 SDFGradient(in sampler sampLinear, in vec3 pos)
{
	ivec3 size = textureSize(SDFTex[0], 0).xyz;

	vec3 cellSize = m_SDFSize[0].xyz / size;

	vec3 dx = 0.0125f * cellSize;

	vec3 p = (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f;

	vec3 grad;
	grad.x = textureLod(sampler3D(SDFTex[0], sampLinear), p + vec3(dx.x, 0, 0), 0).r - textureLod(sampler3D(SDFTex[0], sampLinear), p - vec3(dx.x, 0, 0), 0).r;
	grad.y = textureLod(sampler3D(SDFTex[0], sampLinear), p + vec3(0, dx.y, 0), 0).r - textureLod(sampler3D(SDFTex[0], sampLinear), p - vec3(0, dx.y, 0), 0).r;
	grad.z = textureLod(sampler3D(SDFTex[0], sampLinear), p + vec3(0, 0, dx.z), 0).r - textureLod(sampler3D(SDFTex[0], sampLinear), p - vec3(0, 0, dx.z), 0).r;

	grad /= 2.f * dx;

	return grad;
}


float SDFNearestExteriorPoint(in sampler sampLinear, in vec3 pos, in float margin, out vec3 newPos)
{
	newPos = pos;

	vec3 p = (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f;

	precise float d = textureLod(sampler3D(SDFTex[0], sampLinear), p, 0).r;

	if (d > margin)
		return d;

	precise vec3 grad = normalize(SDFGradient(sampLinear, newPos));

	if (isnan(dot(grad, 1.f.xxx)) || isinf(dot(grad, 1.f.xxx)))
		return d;

	while (d < margin)
	{
		newPos += (abs(d) + 0.02f) * grad;

		p = (newPos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f;

		if ((p.x * (1.f - p.x) < 0.f) || (p.y * (1.f - p.y) < 0.f) || (p.z * (1.f - p.z) < 0.f))
			return d;

		d = textureLod(sampler3D(SDFTex[0], sampLinear), p, 0).r;
	}

	return d;
}


float SampleSDFVisibilityTarget(in sampler sampLinear, in vec3 origin, in vec3 target)
{
	vec3 pos = origin;

	float d = textureLod(sampler3D(SDFTex[0], sampLinear), (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f, 0).r;

	if (d < 0.01f)
	{
		pos += 0.001f * normalize(target - pos);
		d = textureLod(sampler3D(SDFTex[0], sampLinear), ((pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz) + vec3(0.5), 0.0).x;
	}

	vec3 dir = target - pos;
	float dmax = length(dir);
	dir /= max(1e-6f, dmax);	

	float t = 0.f;

	precise float dmin = 1.f;
	float ph = 1e20f;

	while (d > 0.01f)
	{
		pos += d * dir;
		t += d;

		vec3 p = (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f;

		if (t > dmax || (p.x * (1.f - p.x) < 0.f) || (p.y * (1.f - p.y) < 0.f) || (p.z * (1.f - p.z) < 0.f))
			return smoothstep(0.001f, 1.f, dmin);

		d = textureLod(sampler3D(SDFTex[0], sampLinear), p, 0).r;

		float h = d - 0.01f;

		float y = h * h / (2.f * ph);
		float a = sqrt(h * h - y * y);
		ph = h;

        dmin = min(dmin, (16.f * a) / max(0.f, t - y));
	}

	return 0.001f;
}



float SampleSDFVisibilityDir(in sampler sampLinear, in vec3 origin, in vec3 dir)
{
	float t = 0.f;
	float dx = 0.25f * length(m_SDFSize[0].xyz / textureSize(SDFTex[0], 0).xyz);

	vec3 pos = origin + dx * dir;

	float d = textureLod(sampler3D(SDFTex[0], sampLinear), (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f, 0).r;

	float dmin = 1.f;
	float ph = 1e20f;

	while (d > 0.01f)
	{
		pos += d * dir;
		t += d;

		vec3 p = (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f;

		if ((p.x * (1.f - p.x) < 0.f) || (p.y * (1.f - p.y) < 0.f) || (p.z * (1.f - p.z) < 0.f))
			return smoothstep(0.f, 1.f, dmin);

		d = textureLod(sampler3D(SDFTex[0], sampLinear), p, 0).r;

		float h = d - 0.01f;

		float y = h * h / (2.f * ph);
		float a = sqrt(h * h - y * y);
		ph = h;

        dmin = min(dmin, (1.0 * a) / max(0.f, t - y));
	}

	return 0.f;
}


bool RayMarchSDF(in sampler sampLinear, in vec3 origin, in vec3 dir, out vec3 pos)
{
	pos = origin;

	float d = textureLod(sampler3D(SDFTex[0], sampLinear), (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f, 0).r;

	while (d > 0.01f)
	{
		pos += d * dir * 0.5f;

		vec3 p = (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f;

		if ((p.x * (1.f - p.x) < 0.f) || (p.y * (1.f - p.y) < 0.f) || (p.z * (1.f - p.z) < 0.f))
			return false;

		d = textureLod(sampler3D(SDFTex[0], sampLinear), p, 0).r;
	}

	return true;
}


bool RayMarchSDF(in sampler sampLinear, in vec3 origin, in vec3 dir, out float rayLength)
{
	vec3 pos = origin;
	rayLength = 0.f;

	float d = textureLod(sampler3D(SDFTex[0], sampLinear), (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f, 0).r;

	while (d > 0.01f)
	{
		pos += d * dir * 0.5f;
		rayLength += d * 0.5f;

		vec3 p = (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f;

		if ((p.x * (1.f - p.x) < 0.f) || (p.y * (1.f - p.y) < 0.f) || (p.z * (1.f - p.z) < 0.f))
			return false;

		d = textureLod(sampler3D(SDFTex[0], sampLinear), p, 0).r;
	}

	return true;
}



bool SDFNearestExteriorPointInBox(in sampler sampLinear, in vec3 pos, in vec3 box, in float margin, out vec3 newPos)
{
	newPos = pos;

	vec3 p = (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f;

	float d = textureLod(sampler3D(SDFTex[0], sampLinear), p, 0).r;

	if (d > margin)
		return true;

	int i = 0;

	while (d < margin)
	{
		if (i++ > 20)
			return false;

		vec3 grad = normalize(SDFGradient(sampLinear, newPos));

		if (isnan(dot(grad, 1.f.xxx)) || isinf(dot(grad, 1.f.xxx)))
			return false;

		newPos += (abs(d) + 0.01f) * grad;

		if ((newPos.x < pos.x - box.x) || (newPos.x > pos.x + box.x) || 
			(newPos.y < pos.y - box.y) || (newPos.y > pos.y + box.y) || 
			(newPos.z < pos.z - box.z) || (newPos.z > pos.z + box.z))
			return false;

		p = (newPos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f;

		if ((p.x * (1.f - p.x) < 0.f) || (p.y * (1.f - p.y) < 0.f) || (p.z * (1.f - p.z) < 0.f))
			return false;

		d = textureLod(sampler3D(SDFTex[0], sampLinear), p, 0).r;
	}

	return true;
}



#ifdef VOLUME_ALBEDO_TEX_SLOT
vec4 GetVolumeAlbedo(in sampler sampLinear, in vec3 pos)
{
	vec3 p = (pos - m_SDFCenter[0].xyz) / m_SDFSize[0].xyz + 0.5f;

	return textureLod(sampler3D(VolumeAlbedo[0], sampLinear), p, 0.f);
}
#endif
