// shadertype=glsl

#version 450

layout(location = 0) in vec3 position;

layout(location = 0) out vec2 texcoord;

 
void main(void)
{
  texcoord = position.xy * 0.5f + 0.5f;
  gl_Position = vec4( position, 1.f );
}
