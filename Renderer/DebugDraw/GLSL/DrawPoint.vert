// shadertype=glsl

#version 450

layout(location = 0) in vec3 p;
layout(location = 7) in vec4 col;

layout( location = 0 ) out vec4 color;

layout(std140) uniform cb0
{
	mat4	ViewProj;
};
 
void main(void)
{
	color = col;
	gl_Position = ViewProj * vec4(p, 1.f);
}
