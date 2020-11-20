#version 450
#extension GL_EXT_samplerless_texture_functions : require
#extension GL_GOOGLE_include_directive : require

#include "../../Lights/GLSL/Lighting.glsl"

layout(location = 0) in vec2 Texcoords;


layout (binding = 5, std140) uniform cb5
{
	mat4	m_View;
	mat4	m_Proj;
	mat4	m_ViewProj;
	mat4	m_InvView;
	mat4	m_InvViewProj;

	mat4	m_LastView;
	mat4	m_LastProj;
	mat4	m_LastViewProj;
	mat4	m_LastInvView;
	mat4	m_LastInvViewProj;

	vec4	m_Eye;
};


layout(push_constant) uniform pc0
{
	vec3 Center;
	float Near;

	vec3 Size;
	float Far;
};


layout(binding = 0) uniform texture2DArray	LightFieldDepthMaps;
layout(binding = 1) uniform texture2DArray	LightFieldLowDepthMaps;
layout(binding = 2) uniform utexture2DArray	LightFieldGBuffer;
layout(binding = 3) uniform texture2D		NormalMap;
layout(binding = 4) uniform texture2D		DepthMap;


layout(location = 0) out uvec4 Output;


#define TraceResult int
#define TRACE_RESULT_MISS    0
#define TRACE_RESULT_HIT     1
#define TRACE_RESULT_UNKNOWN 2

const float rayBumpEpsilon  = 0.001; // meters
const float minThickness	= 0.03; // meters
const float maxThickness	= 0.50; // meters


vec2 octEncode(in vec3 v) 
{
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    vec2 result = v.xy * (1.0 / l1norm);

    if (v.z < 0.0)
        result = (1.0 - abs(result.yx)) * signNotZero(result.xy);
    
    return result;
}


/** Returns a unit vector. Argument o is an octahedral vector packed via octEncode,
    on the [-1, +1] square*/
vec3 octDecode(vec2 o) 
{
    vec3 v = vec3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));

    if (v.z < 0.0)
        v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
    
    return normalize(v);
}


ivec3 GetProbeCoords(texture2DArray depthMaps, int probeIndex)
{
	ivec2 size = textureSize(depthMaps, 0).xy >> 7;

	int index = probeIndex;
	ivec3 coords;
	coords.z = index / (size.x * size.y);
	coords.y = (index - coords.z * size.x * size.y) / size.x;
	coords.x = index - coords.z * size.x * size.y - coords.y * size.x;

	return coords;
}


vec3 GetProbePos(texture2DArray depthMaps, ivec3 coords)
{
	uvec3 size = textureSize(depthMaps, 0).xyz >> ivec3(7, 7, 0);

	return Center + vec3((coords.x + 0.5f) / size.x - 0.5f, (coords.y + 0.5f) / size.y - 0.5f, (coords.z + 0.5f) / size.z - 0.5f) * Size;
}


vec3 probeLocation(texture2DArray depthMaps, in int index) 
{
    return GetProbePos(depthMaps, GetProbeCoords(depthMaps, index));
}


int nearestProbeIndices(texture2DArray depthMaps, vec3 X) 
{
	ivec3 numProbes = textureSize(depthMaps, 0).xyz >> ivec3(7, 7, 0);

	vec3 coords = clamp((X - Center) / Size + 0.5f, 0.f.xxx, 1.f.xxx);
	vec3 floatProbeCoords = coords * numProbes - 0.5f.xxx;
    vec3 baseProbeCoords = clamp(floor(floatProbeCoords), 0.f.xxx, numProbes - 1);

    float minDist = 10.0f;
    int nearestIndex = -1;

    for (int i = 0; i < 8; ++i) 
	{
        vec3 newProbeCoords = min(baseProbeCoords + vec3(i & 1, (i >> 1) & 1, (i >> 2) & 1), numProbes - 1);
        float d = length(newProbeCoords - floatProbeCoords);

        if (d < minDist) 
		{
            minDist = d;
            nearestIndex = i;
        }       
    }

    return nearestIndex;
}


int relativeProbeIndex(texture2DArray depthMaps, in int baseProbeIndex, in int relativeIndex) 
{
	ivec3 size = textureSize(depthMaps, 0).xyz >> ivec3(7, 7, 0);

    // Guaranteed to be a power of 2
    int numProbes = size.x * size.y * size.z;

    ivec3 offset = ivec3(relativeIndex & 1, (relativeIndex >> 1) & 1, (relativeIndex >> 2) & 1);
    ivec3 stride = ivec3(1, size.x, size.x * size.y);

    return clamp(baseProbeIndex + int(dot(offset, stride)), 0, numProbes - 1);
}


/** Two-element sort: maybe swaps a and b so that a' = min(a, b), b' = max(a, b). */
void minSwap(inout float a, inout float b) 
{
    float temp = min(a, b);
    b = max(a, b);
    a = temp;
}


/** Sort the three values in v from least to 
    greatest using an exchange network (i.e., no branches) */
void sort(inout vec3 v) 
{
    minSwap(v[0], v[1]);
    minSwap(v[1], v[2]);
    minSwap(v[0], v[1]);
}


void computeRaySegments (in vec3 origin, in vec3 directionFrac, in float tMin, in float tMax, out float boundaryTs[5]) 
{
    boundaryTs[0] = tMin;
    
    // Time values for intersection with x = 0, y = 0, and z = 0 planes, sorted
    // in increasing order
    vec3 t = origin * -directionFrac;
    sort(t);

    // Copy the values into the interval boundaries.
    // This loop expands at compile time and eliminates the
    // relative indexing, so it is just three conditional move operations
    for (int i = 0; i < 3; ++i) 
	{
        boundaryTs[i + 1] = clamp(t[i], tMin, tMax);
    }

    boundaryTs[4] = tMax;
}


/** Returns the distance along v from the origin to the intersection 
    with ray R (which it is assumed to intersect) */
float distanceToIntersection(in vec3 pos, in vec3 R, in vec3 v) 
{
    float numer;
    float denom = v.y * R.z - v.z * R.y;

    if (abs(denom) > 0.1) 
	{
        numer = pos.y * R.z - pos.z * R.y;
    } 
	
	else 
	{
        // We're in the yz plane; use another one
        numer = pos.x * R.y - pos.y * R.x;
        denom = v.x * R.y - v.y * R.x;
    }

    return numer / denom;
}



bool lowResolutionTraceOneSegment(in texture2DArray depthMaps, in vec3 probeSpacePos, in vec3 probeSpaceRay, in int probeIndex, inout vec2 texCoord, in vec2 segmentEndTexCoord, inout vec2 endHighResTexCoord) 
{        
    vec2 lowResSize    = 16.f.xx;
    vec2 lowResInvSize = (1.f / 16.f).xx;

	ivec3 probeCoords = GetProbeCoords(depthMaps, probeIndex);

    // Convert the texels to pixel coordinates:
    vec2 P0 = texCoord           * lowResSize;
    vec2 P1 = segmentEndTexCoord * lowResSize;

    // If the line is degenerate, make it cover at least one pixel
    // to avoid handling zero-pixel extent as a special case later
    P1 += vec2((dot(P0 - P1, P0 - P1) < 0.0001) ? 0.01 : 0.0);
    // In pixel coordinates
    vec2 delta = P1 - P0;

    // Permute so that the primary iteration is in x to reduce
    // large branches later
    bool permute = false;
    if (abs(delta.x) < abs(delta.y)) 
	{ 
        // This is a more-vertical line
        permute = true;
        delta = delta.yx; 
		P0 = P0.yx; 
		P1 = P1.yx; 
    }

    float   stepDir = sign(delta.x);
    float   invdx = stepDir / delta.x;
    vec2 dP = vec2(stepDir, delta.y * invdx);
    
    vec3 initialDirectionFromProbe = octDecode(texCoord * 2.0 - 1.0);
    float prevRadialDistMaxEstimate = max(0.0, distanceToIntersection(probeSpacePos, probeSpaceRay, initialDirectionFromProbe));
    // Slide P from P0 to P1
    float  end = P1.x * stepDir;
    
    float absInvdPY = 1.0 / abs(dP.y);

    // Don't ever move farther from texCoord than this distance, in texture space,
    // because you'll move past the end of the segment and into a different projection
    float maxTexCoordDistance = dot(segmentEndTexCoord - texCoord, segmentEndTexCoord - texCoord);

    for (vec2 P = P0; ((P.x * sign(delta.x)) <= end); ) 
	{        
        vec2 hitPixel = permute ? P.yx : P;
        
        float sceneRadialDistMin = texelFetch(depthMaps, ivec3(probeCoords.xy * lowResSize + hitPixel, probeCoords.z), 0).r;
		sceneRadialDistMin = 2.f * Near * Far / (Far + Near + (2.f * sceneRadialDistMin - 1.f) * (Far - Near));

        // Distance along each axis to the edge of the low-res texel
        vec2 intersectionPixelDistance = (sign(delta) * 0.5 + 0.5) - sign(delta) * fract(P);

        // abs(dP.x) is 1.0, so we skip that division
        // If we are parallel to the minor axis, the second parameter will be inf, which is fine
        float rayDistanceToNextPixelEdge = min(intersectionPixelDistance.x, intersectionPixelDistance.y * absInvdPY);

        // The exit coordinate for the ray (this may be *past* the end of the segment, but the 
        // callr will handle that)
        endHighResTexCoord = (P + dP * rayDistanceToNextPixelEdge) * lowResInvSize;
        endHighResTexCoord = permute ? endHighResTexCoord.yx : endHighResTexCoord;

        if (dot(endHighResTexCoord - texCoord, endHighResTexCoord - texCoord) > maxTexCoordDistance) 
		{
            // Clamp the ray to the segment, because if we cross a segment boundary in oct space
            // then we bend the ray in probe and world space.
            endHighResTexCoord = segmentEndTexCoord;
        }

        // Find the 3D point *on the trace ray* that corresponds to the tex coord.
        // This is the intersection of the ray out of the probe origin with the trace ray.
        vec3 directionFromProbe = octDecode(endHighResTexCoord * 2.0 - 1.0);
        float distanceFromProbeToRay = max(0.0, distanceToIntersection(probeSpacePos, probeSpaceRay, directionFromProbe));

        float maxRadialRayDistance = max(distanceFromProbeToRay, prevRadialDistMaxEstimate);
        prevRadialDistMaxEstimate = distanceFromProbeToRay;
        
        if (sceneRadialDistMin < maxRadialRayDistance) 
		{
            // A conservative hit.
            //
            //  -  endHighResTexCoord is already where the ray would have LEFT the texel
            //     that created the hit.
            //
            //  -  texCoord should be where the ray entered the texel
            texCoord = (permute ? P.yx : P) * lowResInvSize;
            return true;
        }

        // Ensure that we step just past the boundary, so that we're slightly inside the next
        // texel, rather than at the boundary and randomly rounding one way or the other.
        const float epsilon = 0.001; // pixels
        P += dP * (rayDistanceToNextPixelEdge + epsilon);
    } // for each pixel on ray

    // If exited the loop, then we went *past* the end of the segment, so back up to it (in practice, this is ignored
    // by the caller because it indicates a miss for the whole segment)
    texCoord = segmentEndTexCoord;

    return false;
}


float maxComponent(vec2 x)
{
	return max(x.x, x.y);
}



TraceResult highResolutionTraceOneRaySegment (in texture2DArray depthMaps, in utexture2DArray gbuffer, in vec3 probeSpacePos, in vec3 probeSpaceRay, in vec2 startTexCoord, in vec2 endTexCoord, in int probeIndex, inout float tMin, inout float tMax, inout vec2 hitProbeTexCoord) 
{      
    vec2 texCoordDelta			= endTexCoord - startTexCoord;
    float texCoordDistance		= length(texCoordDelta);
    vec2 texCoordDirection		= texCoordDelta * (1.0 / texCoordDistance);

	ivec3 probeCoords = GetProbeCoords(depthMaps, probeIndex);

    float texCoordStep = (1.f / 128.f) * (texCoordDistance / maxComponent(abs(texCoordDelta)));
    
    vec3 directionFromProbeBefore = octDecode(startTexCoord * 2.0 - 1.0);
    float distanceFromProbeToRayBefore = max(0.0, distanceToIntersection(probeSpacePos, probeSpaceRay, directionFromProbeBefore));

    // Special case for singularity of probe on ray
    if (false) 
	{
        float cosTheta = dot(directionFromProbeBefore, probeSpaceRay);
        if (abs(cosTheta) > 0.9999) 
		{        
            // Check if the ray is going in the same direction as a ray from the probe through the start texel
            if (cosTheta > 0) 
			{
                // If so, return a hit
                float distanceFromProbeToSurface = texelFetch(depthMaps, ivec3((probeCoords.xy + startTexCoord) * 128, probeCoords.z), 0).r;
				distanceFromProbeToSurface = 2.f * Near * Far / (Near + Far + (2.f * distanceFromProbeToSurface - 1.f) * (Far - Near));

                tMax = length(probeSpacePos - directionFromProbeBefore * distanceFromProbeToSurface);
                hitProbeTexCoord = startTexCoord;
                return TRACE_RESULT_HIT;
            } 
			
			else 
			{
                // If it is going in the opposite direction, we're never going to find anything useful, so return false
                return TRACE_RESULT_UNKNOWN;
            }
        }
    }

    for (float d = 0.0f; d <= texCoordDistance; d += texCoordStep) 
	{
        vec2 texCoord = (texCoordDirection * min(d + texCoordStep * 0.5, texCoordDistance)) + startTexCoord;

        // Fetch the probe data
        float distanceFromProbeToSurface = texelFetch(depthMaps, ivec3((probeCoords.xy + texCoord) * 128, probeCoords.z), 0).r;
		distanceFromProbeToSurface = 2.f * Near * Far / (Near + Far + (2.f * distanceFromProbeToSurface - 1.f) * (Far - Near));

        // Find the corresponding point in probe space. This defines a line through the 
        // probe origin
        vec3 directionFromProbe = octDecode(texCoord * 2.0 - 1.0);
        
        vec2 texCoordAfter = (texCoordDirection * min(d + texCoordStep, texCoordDistance)) + startTexCoord;
        vec3 directionFromProbeAfter = octDecode(texCoordAfter * 2.0 - 1.0);
        float distanceFromProbeToRayAfter = max(0.0, distanceToIntersection(probeSpacePos, probeSpaceRay, directionFromProbeAfter));
        float maxDistFromProbeToRay = max(distanceFromProbeToRayBefore, distanceFromProbeToRayAfter);

        if (maxDistFromProbeToRay >= distanceFromProbeToSurface) 
		{
            // At least a one-sided hit; see if the ray actually passed through the surface, or was behind it

            float minDistFromProbeToRay = min(distanceFromProbeToRayBefore, distanceFromProbeToRayAfter);

            // Find the 3D point *on the trace ray* that corresponds to the tex coord.
            // This is the intersection of the ray out of the probe origin with the trace ray.
            float distanceFromProbeToRay = (minDistFromProbeToRay + maxDistFromProbeToRay) * 0.5;

            // Use probe information
            vec3 probeSpaceHitPoint = distanceFromProbeToSurface * directionFromProbe;
            float distAlongRay = dot(probeSpaceHitPoint - probeSpacePos, probeSpaceRay);

            // Read the normal for use in detecting backfaces
			uint n = texelFetch(gbuffer, ivec3((probeCoords.xy + texCoord) * 128, probeCoords.z), 0).x;
            vec3 normal = DecodeOct(vec2(n & 0xff, n >> 8) * (1.f / 255.f));

            // Only extrude towards and away from the view ray, not perpendicular to it
            // Don't allow extrusion TOWARDS the viewer, only away
            float surfaceThickness = minThickness
                + (maxThickness - minThickness) * 

                // Alignment of probe and view ray
                max(dot(probeSpaceRay, directionFromProbe), 0.0) * 

                // Alignment of probe and normal (glancing surfaces are assumed to be thicker because they extend into the pixel)
                (2 - abs(dot(probeSpaceRay, normal))) *

                // Scale with distance along the ray
                clamp(distAlongRay * 0.1, 0.05, 1.0);


            if ((minDistFromProbeToRay < distanceFromProbeToSurface + surfaceThickness) && (dot(normal, probeSpaceRay) < 0)) {
                // Two-sided hit
                // Use the probe's measure of the point instead of the ray distance, since
                // the probe is more accurate (floating point precision vs. ray march iteration/oct resolution)
                tMax = distAlongRay;
                hitProbeTexCoord = texCoord;
                
                return TRACE_RESULT_HIT;
            } 
			
			else 
			{
                // "Unknown" case. The ray passed completely behind a surface. This should trigger moving to another
                // probe and is distinguished from "I successfully traced to infinity"
                
                // Back up conservatively so that we don't set tMin too large
                vec3 probeSpaceHitPointBefore = distanceFromProbeToRayBefore * directionFromProbeBefore;
                float distAlongRayBefore = dot(probeSpaceHitPointBefore - probeSpacePos, probeSpaceRay);
                
                // Max in order to disallow backing up along the ray (say if beginning of this texel is before tMin from probe switch)
                // distAlongRayBefore in order to prevent overstepping
                // min because sometimes distAlongRayBefore > distAlongRay
                tMin = max(tMin, min(distAlongRay,distAlongRayBefore));

                return TRACE_RESULT_UNKNOWN;
            }
        }
        distanceFromProbeToRayBefore = distanceFromProbeToRayAfter;
    } // ray march

    return TRACE_RESULT_MISS;
}





TraceResult traceOneRaySegment(texture2DArray depthMaps, texture2DArray lowDepthMaps, utexture2DArray gbufferMaps, vec3 probeSpacePos, vec3 probeSpaceRay, in float t0, in float t1, in int probeIndex, inout float tMin, inout float tMax, inout vec2 hitProbeTexCoord) 
{    
    // Euclidean probe-space line segment, composed of two points on the probeSpaceRay
    vec3 probeSpaceStartPoint = probeSpacePos + probeSpaceRay * (t0 + rayBumpEpsilon);
    vec3 probeSpaceEndPoint   = probeSpacePos + probeSpaceRay * (t1 - rayBumpEpsilon);

    // If the original ray origin is really close to the probe origin, then probeSpaceStartPoint will be close to zero
    // and we get NaN when we normalize it. One common case where this can happen is when the camera is at the probe
    // center. (The end point is also potentially problematic, but the chances of the end landing exactly on a probe 
    // are relatively low.) We only need the *direction* to the start point, and using probeSpaceRay.direction
    // is safe in that case.
    if (dot(probeSpaceStartPoint, probeSpaceStartPoint) < 0.001) 
	{
        probeSpaceStartPoint = probeSpaceRay;
    }

    // Corresponding octahedral ([-1, +1]^2) space line segment.
    // Because the points are in probe space, we don't have to subtract off the probe's origin
    vec2 startOctCoord         = octEncode(normalize(probeSpaceStartPoint));
    vec2 endOctCoord           = octEncode(normalize(probeSpaceEndPoint));

    // Texture coordinates on [0, 1]
    vec2 texCoord              = startOctCoord * 0.5 + 0.5;
    vec2 segmentEndTexCoord    = endOctCoord   * 0.5 + 0.5;

    while (true) 
	{
        vec2 endTexCoord;

        // Trace low resolution, min probe until we:
        // - reach the end of the segment (return "miss" from the whole function)
        // - "hit" the surface (invoke high-resolution refinement, and then iterate if *that* misses)
            
        // If lowResolutionTraceOneSegment conservatively "hits", it will set texCoord and endTexCoord to be the high-resolution texture coordinates.
        // of the intersection between the low-resolution texel that was hit and the ray segment.
        vec2 originalStartCoord = texCoord;
        if (! lowResolutionTraceOneSegment(lowDepthMaps, probeSpacePos, probeSpaceRay, probeIndex, texCoord, segmentEndTexCoord, endTexCoord)) 
		{
            // The whole trace failed to hit anything           
            return TRACE_RESULT_MISS;
        } 
		
		else 
		{

            // The low-resolution trace already guaranted that endTexCoord is no farther along the ray than segmentEndTexCoord if this point is reached,
            // so we don't need to clamp to the segment length
            TraceResult result = highResolutionTraceOneRaySegment(depthMaps, gbufferMaps, probeSpacePos, probeSpaceRay, texCoord, endTexCoord, probeIndex, tMin, tMax, hitProbeTexCoord);

            if (result != TRACE_RESULT_MISS) 
			{
                // High-resolution hit or went behind something, which must be the result for the whole segment trace
                return result;
            } 
        } // else...continue the outer loop; we conservatively refined and didn't actually find a hit

        // Recompute each time around the loop to avoid increasing the peak register count
        vec2 texCoordRayDirection = normalize(segmentEndTexCoord - texCoord);

        if (dot(texCoordRayDirection, segmentEndTexCoord - endTexCoord) <= (1.f / 128.f)) 
		{
            // The high resolution trace reached the end of the segment; we've failed to find a hit
            return TRACE_RESULT_MISS;
        } 
		
		else 
		{
            // We made it to the end of the low-resolution texel using the high-resolution trace, so that's
            // the starting point for the next low-resolution trace. Bump the ray to guarantee that we advance
            // instead of getting stuck back on the low-res texel we just verified...but, if that fails on the 
            // very first texel, we'll want to restart the high-res trace exactly where we left off, so
            // don't bump by an entire high-res texel
            texCoord = endTexCoord + texCoordRayDirection * (0.1 / 128.f);
        }
    } // while low-resolution trace

    // Reached the end of the segment
    return TRACE_RESULT_MISS;
}



TraceResult traceOneProbeOct(texture2DArray depthMaps, texture2DArray lowDepthMaps, utexture2DArray gbufferMaps, in int index, in vec3 pos, in vec3 ray, inout float tMin, inout float tMax, inout vec2 hitProbeTexCoord) 
{
    // How short of a ray segment is not worth tracing?
    const float degenerateEpsilon = 0.001; // meters
    
    vec3 probeOrigin = probeLocation(depthMaps, index);
    
    
    vec3 probeSpacePos	= pos - probeOrigin;

    // Maximum of 5 boundary points when projecting ray onto octahedral map; 
    // ray origin, ray end, intersection with each of the XYZ planes.
    float boundaryTs[5];
    computeRaySegments(pos, 1.f.xxx / ray, tMin, tMax, boundaryTs);
    
    // for each open interval (t[i], t[i + 1]) that is not degenerate
    for (int i = 0; i < 4; ++i) 
	{
        if (abs(boundaryTs[i] - boundaryTs[i + 1]) >= degenerateEpsilon) 
		{
            TraceResult result = traceOneRaySegment(depthMaps, lowDepthMaps, gbufferMaps, probeSpacePos, ray, boundaryTs[i], boundaryTs[i + 1], index, tMin, tMax, hitProbeTexCoord);
            
            switch (result) 
			{
            case TRACE_RESULT_HIT:
                // Hit!            
                return TRACE_RESULT_HIT;

            case TRACE_RESULT_UNKNOWN:
                // Failed to find anything conclusive
                return TRACE_RESULT_UNKNOWN;
            } // switch
        } // if 
    } // For each segment

    return TRACE_RESULT_MISS;
}


int nextCycleIndex(int cycleIndex) 
{
    return (cycleIndex + 3) & 7;
}


bool RayTrace(texture2DArray depthMaps, texture2DArray lowDepthMaps, utexture2DArray gbufferMaps, in vec3 pos, in vec3 ray, out float rayLength, out vec2 hitProbeTexCoord, out int hitProbeIndex)
{
	hitProbeIndex = -1;

	int baseIndex = nearestProbeIndices(depthMaps, pos);
	int i = 0;
    int probesLeft = 8;
    float tMin = 0.0f;

    while (probesLeft > 0) 
	{
        TraceResult result = traceOneProbeOct(depthMaps, lowDepthMaps, gbufferMaps, relativeProbeIndex(depthMaps, baseIndex, i), pos, ray, tMin, rayLength, hitProbeTexCoord);

        if (result == TRACE_RESULT_UNKNOWN) 
		{
            i = nextCycleIndex(i);
            --probesLeft;
        } 
		
		else 
		{
            if (result == TRACE_RESULT_HIT)
                hitProbeIndex = relativeProbeIndex(depthMaps, baseIndex, i);
            
            // Found the hit point
            break;
        }
    }

	/*if (hitProbeIndex == -1) 
	{
        // No probe found a solution, so force some backup plan 
        vec3 ignore;
        hitProbeIndex = nearestProbeIndex(lightFieldSurface, worldSpaceRay.origin, ignore);
        hitProbeTexCoord = octEncode(worldSpaceRay.direction) * 0.5 + 0.5;

        float probeDistance = texelFetch(depthMaps, ivec3(ivec2(hitProbeTexCoord * lightFieldSurface.distanceProbeGrid.size.xy), hitProbeIndex), 0).r;
        if (probeDistance < 10000) 
		{
            Point3 hitLocation = probeLocation(lightFieldSurface, hitProbeIndex) + worldSpaceRay.direction * probeDistance;
            tMax = length(worldSpaceRay.origin - hitLocation);
            return true;
        }
    }*/

	return (hitProbeIndex != -1);
}



uvec2 packRayTo2x32(in vec3 ray, in float rayLength)
{
	uvec2 res;
	uvec2 uv = uvec2((octEncode(ray) * 0.5f + 0.5f) * ((1 << 24) - 1));

	uint l = uint(rayLength * ((1 << 16) - 1));

	res.r = (uv.r << 8U) | (uv.g >> 16U);
	res.g = (uv.g << 16U) | (l & 0xffff);

	return res;
}


vec3 DecodeNormal(in vec3 e) 
{
	e = e * 2.f - 1.f;
	
	vec2 temp = vec2(e.x + e.y, e.x - e.y) * 0.5;
	vec3 v = vec3(temp, 1.0 - abs(temp.x) - abs(temp.y));

	return normalize(v) * sign(e.z);
}


void main(void)
{
	float depth = texelFetch(DepthMap, ivec2(gl_FragCoord.xy), 0).r;

	if (depth == 0.f)
		discard;

	vec4 pos = m_InvViewProj * vec4(Texcoords.xy * vec2(2.f, 2.f) - vec2(1.f, 1.f), depth, 1.f);
	pos /= pos.w;

	vec4 normalTex = texelFetch(NormalMap, ivec2(gl_FragCoord.xy), 0);

	vec3 normal = DecodeNormal(normalTex.rga);

	vec3 view	= normalize(m_Eye.xyz - pos.xyz);

	vec3 ray = reflect(view, normal);
	pos.xyz += rayBumpEpsilon * ray;

	vec2 hitPoint;
	int hitProbeIndex;
	float rayLength;

	if (RayTrace(LightFieldDepthMaps, LightFieldLowDepthMaps, LightFieldGBuffer, pos.xyz, ray, rayLength, hitPoint, hitProbeIndex))
	{
		ivec3 hitProbe = GetProbeCoords(LightFieldDepthMaps, hitProbeIndex);

		Output.rg = packRayTo2x32(ray, rayLength);
		uvec4 gbuffer = texelFetch(LightFieldGBuffer, ivec3(hitProbe.xy * 128, hitProbe.z), 0);
		Output.ba = uvec2((gbuffer.r << 16U) | (gbuffer.g & 0xffff), (gbuffer.b << 16U) | (gbuffer.a & 0xffff));
	}

	else
		Output = uvec4(0);
}