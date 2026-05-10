// shadertype=glsl

#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(location = 0) in vec2 vTexcoord;
layout(location = 1) in vec4 vColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform texture2D Sprite;
layout(binding = 1) uniform sampler   SpriteSampler;

void main()
{
	// R8 coverage atlas (font glyph alpha). Fold the sampled coverage into
	// the vertex colour's alpha so the alpha-blended pass paints in vColor.rgb.
	float coverage = texture(sampler2D(Sprite, SpriteSampler), vTexcoord).r;

	outColor = vec4(vColor.rgb, vColor.a * coverage);
}
