#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "SHProbes.h"


std::vector<CSHNetwork*> CSHNetwork::ms_pNetworks;


CSHNetwork::CSHNetwork(EType eType, float3& Center, float3& Dim, int nNumX, int nNumY, int nNumZ)
{
	float3 Pos;
	float3 dx = float3(Dim.x / MAX(nNumX - 1, 1), Dim.y / MAX(nNumY - 1, 1), Dim.z / MAX(nNumZ - 1, 1));
	int nIndex = 0;

	for (int z = 0; z < nNumZ; z++)
		for (int y = 0; y < nNumY; y++)
			for (int x = 0; x < nNumX; x++)
			{
				Pos = Center + float3(dx.x * (x - (nNumX - 1) * 0.5f), dx.y * (y - (nNumY - 1) * 0.5f), dx.z * (z - (nNumZ - 1) * 0.5f));

				nIndex = z * nNumX * nNumY + y * nNumX + x;

				m_pSHProbes.push_back(new CSHProbe(nIndex, Pos));
			}
}


CSHNetwork::~CSHNetwork()
{
	std::vector<CSHProbe*>::iterator it;

	for (it = m_pSHProbes.begin(); it < m_pSHProbes.end(); it++)
	{
		delete (*it);
	}

	m_pSHProbes.clear();
}


void CSHNetwork::Update()
{
	std::vector<CSHProbe*>::iterator it;

	for (it = m_pSHProbes.begin(); it < m_pSHProbes.end(); it++)
	{
		(*it)->Update();
	}
}


void CSHNetwork::Init()
{

}


void CSHNetwork::Terminate()
{
	std::vector<CSHNetwork*>::iterator it;

	for (it = ms_pNetworks.begin(); it < ms_pNetworks.end(); it++)
	{
		delete (*it);
	}

	ms_pNetworks.clear();
}


void CSHNetwork::UpdateAll()
{
	std::vector<CSHNetwork*>::iterator it;

	for (it = ms_pNetworks.begin(); it < ms_pNetworks.end(); it++)
	{
		(*it)->Update();
	}
}


void CSHNetwork::Add(EType eType, float3& Center, float3& Dim, int nNumX, int nNumY, int nNumZ)
{
	ms_pNetworks.push_back(new CSHNetwork(eType, Center, Dim, nNumX, nNumY, nNumZ));
}
