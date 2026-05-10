#ifndef __TEXTURED_SPRITE_RENDERER_H__
#define __TEXTURED_SPRITE_RENDERER_H__


// Render pass that consumes packets from CPacketBuilder::BuildTexturedQuad2D
// and draws them as alpha-blended textured 2D quads on top of the tone-mapped
// target. The bound texture is currently the font atlas (the text renderer is
// the only consumer); future textured-sprite users can sit alongside it on the
// e_RenderType_TexturedSprites list.
class CTexturedSpriteRenderer
{
public:

	static void Init();

	static int UpdateShader(class Packet* packet, void* p_pShaderData);
};


#endif
