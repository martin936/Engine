// shadertype=glsl

#version 450

layout (location = 0) in vec3 position;

out struct PS_INPUT
{
	vec2 Texcoords;
} interp;

 
void main(void)
{
  interp.Texcoords = ( position.xy + vec2( 1.f, 1.f ) ) * 0.5f;
  gl_Position = vec4( position, 1.f );
}
