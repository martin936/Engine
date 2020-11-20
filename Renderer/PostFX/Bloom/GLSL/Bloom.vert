// shadertype=glsl

#version 450

layout(location = 0) in vec3 Position;

layout(location = 0) out vec2 Texc0;

 
void main(void)
{
	Texc0 = (Position.xy + 1.f.xx) * 0.5f;
	gl_Position = vec4( Position, 1.f );
}
