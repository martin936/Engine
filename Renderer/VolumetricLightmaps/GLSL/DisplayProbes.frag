// shadertype=glsl

#version 450


layout(location = 0) out vec4 Color;


void main() 
{
	if (length(gl_PointCoord.xy) > 1.f)
		discard;

	Color = vec4(0.5f, 1.f, 0.5f, 1.f);
}
