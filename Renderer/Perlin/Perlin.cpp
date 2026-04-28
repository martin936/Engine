#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Perlin.h"

static int perm[] = 
{
        151,160,137,91,90,15,
        131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
        190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
        88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
        77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
        102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
        135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
        5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
        223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
        129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
        251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
        49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
        151
};


CTexture* CPerlinGenerator::CreateTileablePerlin3D(int pixels, int tiles)
{
	float tiling = 0.4f;

	int tileX = (int)(pixels * tiling);
	int tileY = (int)(pixels * tiling);
	int tileZ = (int)(pixels * tiling);

	int extWidth = pixels + tileX;
	int extHeight = pixels + tileY;
	int extDepth = pixels + tileZ;
	float* values = new float[extWidth * extHeight * extDepth];

	for (int z = 0; z < extDepth; z++) 
	{
		int zOffset = z * extWidth * extHeight;
		for (int y = 0; y < extHeight; y++) 
		{
			int yOffset = y * extWidth;
			for (int x = 0; x < extWidth; x++) 
			{
				values[x + yOffset + zOffset] = Noise(1.f * x / tiles, 1.f * y / tiles, 1.f * z / tiles) * 0.5f + 0.5f;
			}
		}
	}

	//TILE X
	for (int z = 0; z < extDepth; z++) 
	{
		int zOffset = z * extWidth * extHeight;
		for (int y = 0; y < extHeight; y++) 
		{
			int yOffset = y * extWidth;
			for (int x = 0; x < tileX; x++) 
			{
				int offs = x + yOffset + zOffset;
				float colorA = values[offs];
				float colorB = values[x + pixels + y * extWidth + z * extWidth * extHeight];
				values[offs] = Lerp((float)x / (float)tileX, colorB, colorA);
			}
		}
	}

	//TILE Y
	for (int z = 0; z < extDepth; z++) 
	{
		int zOffset = z * extWidth * extHeight;
		for (int y = 0; y < tileY; y++) 
		{
			int yOffset = y * extWidth;
			for (int x = 0; x < extWidth; x++) 
			{
				int offs = x + yOffset + zOffset;
				float colorA = values[offs];
				float colorB = values[x + (y + pixels) * extWidth + z * extWidth * extHeight];
				values[offs] = Lerp((float)y / (float)tileY, colorB, colorA);
			}
		}
	}

	//TILE Z
	for (int z = 0; z < tileZ; z++) 
	{
		int zOffset = z * extWidth * extHeight;
		for (int y = 0; y < extHeight; y++) 
		{
			int yOffset = y * extWidth;
			for (int x = 0; x < extWidth; x++) 
			{
				int offs = x + yOffset + zOffset;
				float colorA = values[offs];
				float colorB = values[x + y * extWidth + (z + pixels) * extWidth * extHeight];
				values[offs] = Lerp((float)z / (float)tileZ, colorB, colorA);
			}
		}
	}

	//PACK ORIGINAL RES
	float* tiledValues = new float[pixels * pixels * pixels];
	for (int z = 0; z < pixels; z++) 
	{
		int zOffset = z * pixels * pixels;
		for (int y = 0; y < pixels; y++) 
		{
			int yOffset = y * pixels;
			for (int x = 0; x < pixels; x++) 
			{
				tiledValues[x + yOffset + zOffset] = values[x + y * extWidth + z * extWidth * extHeight];
			}
		}
	}

	delete[] values;

	uint8_t* compressedValues = new uint8_t[pixels * pixels * pixels];
	for (int i = 0; i < pixels * pixels * pixels; i++)
		compressedValues[i] = (uint8_t)floor(tiledValues[i] * 256);

	delete[] tiledValues;

	CTexture* pTex = new CTexture(pixels, pixels, pixels, ETextureFormat::e_R8, eTexture3D, compressedValues);

	delete[] compressedValues;

	return pTex;
}

float CPerlinGenerator::Noise(float x, float y, float z)
{
    int X = (int)floor(x) & 0xff;
    int Y = (int)floor(y) & 0xff;
    int Z = (int)floor(z) & 0xff;
    x -= floor(x);
    y -= floor(y);
    z -= floor(z);
    float u = Fade(x);
    float v = Fade(y);
    float w = Fade(z);
    int A = (perm[X] + Y) & 0xff;
    int B = (perm[X + 1] + Y) & 0xff;
    int AA = (perm[A] + Z) & 0xff;
    int BA = (perm[B] + Z) & 0xff;
    int AB = (perm[A + 1] + Z) & 0xff;
    int BB = (perm[B + 1] + Z) & 0xff;
    return Lerp(w, Lerp(v, Lerp(u, Grad(perm[AA], x, y, z), Grad(perm[BA], x - 1, y, z)),
        Lerp(u, Grad(perm[AB], x, y - 1, z), Grad(perm[BB], x - 1, y - 1, z))),
        Lerp(v, Lerp(u, Grad(perm[AA + 1], x, y, z - 1), Grad(perm[BA + 1], x - 1, y, z - 1)),
            Lerp(u, Grad(perm[AB + 1], x, y - 1, z - 1), Grad(perm[BB + 1], x - 1, y - 1, z - 1))));
}

float CPerlinGenerator::Fade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float CPerlinGenerator::Lerp(float t, float a, float b)
{
    return a + t * (b - a);
}

float CPerlinGenerator::Grad(int hash, float x)
{
    return (hash & 1) == 0 ? x : -x;
}

float CPerlinGenerator::Grad(int hash, float x, float y)
{
    return ((hash & 1) == 0 ? x : -x) + ((hash & 2) == 0 ? y : -y);
}

float CPerlinGenerator::Grad(int hash, float x, float y, float z)
{
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}
