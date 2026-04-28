#ifndef __PERLIN_H__
#define __PERLIN_H__

class CPerlinGenerator
{
public:

	static CTexture* CreateTileablePerlin3D(int pixels, int tiles);

private:

	static float Noise(float x, float y, float z);

	static float Fade(float t);

	static float Lerp(float t, float a, float b);

	static float Grad(int hash, float x);

	static float Grad(int hash, float x, float y);

	static float Grad(int hash, float x, float y, float z);
};

#endif
