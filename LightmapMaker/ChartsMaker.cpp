#include "LightmapMaker.h"
#include "Engine/Maths/Maths.h"
#include "Engine/Device/DeviceManager.h"


float CLightmapMaker::ms_fMaxNormalAngle = 0.2f;

unsigned int CMeshCharts::ms_nBufferID = 0;
unsigned int CMeshCharts::ms_nBufferSize = 0;


CMeshCharts::CMeshCharts(CMesh* pMesh, unsigned int nPacketID)
{
	m_pMesh			= pMesh;
	m_nMeshPacketID = nPacketID;

	ExtractFaces();
	ConnectFaces();
}


void CMeshCharts::CreateBuffer()
{
	unsigned int numCharts = static_cast<unsigned int>(m_NumFaces.size());
	unsigned int numFaces = static_cast<unsigned int>(m_Faces.size());

	std::vector<float4> colors(numCharts);

	for (unsigned int i = 0; i < numCharts; i++)
	{
		float3 color = float3(Randf(0.f, 1.f), Randf(0.f, 1.f), Randf(0.f, 1.f));

		colors[i] = float4(color, 1.f);
	}

	float4* colorData = new float4[numFaces];

	for (unsigned int i = 0; i < numFaces; i++)
	{
		if (m_Faces[i].m_nChartID < 0)
			colorData[i] = 0;

		else
			colorData[i] = colors[m_Faces[i].m_nChartID];
	}

	ms_nBufferSize = numFaces;

	//ms_nBufferID = CDeviceManager::CreateStorageBuffer(colorData, numFaces * sizeof(float4));
}


void CMeshCharts::BindBuffer()
{
	//CDeviceManager::BindStorageBuffer(ms_nBufferID, 0, sizeof(float4), 0, ms_nBufferSize);
}


void CMeshCharts::ExtractFaces()
{
	unsigned int numFaces = m_pMesh->GetNumTrianglesInPacket(m_nMeshPacketID);

	m_Faces.clear();
	m_Faces.reserve(numFaces);

	SFace face = {};

	for (unsigned int i = 0; i < numFaces; i++)
	{
		face.m_nChartID				= -1;
		face.m_nNumConnectedFaces	= 0;
		face.m_Barycenter			= 0.f;
		face.m_nDistanceToSeed		= 1e8f;

		m_pMesh->GetTriangleIndices(m_nMeshPacketID, i, face.m_nVertexID);

		for (unsigned int j = 0; j < 3; j++)
			face.m_Barycenter += m_pMesh->GetVertexPosition(face.m_nVertexID[j]);

		face.m_Barycenter *= (1.f / 3.f);

		float3 e1	= m_pMesh->GetVertexPosition(face.m_nVertexID[1]) - m_pMesh->GetVertexPosition(face.m_nVertexID[0]);
		float3 e2	= m_pMesh->GetVertexPosition(face.m_nVertexID[2]) - m_pMesh->GetVertexPosition(face.m_nVertexID[0]);
		float3 n	= m_pMesh->GetVertexNormal(face.m_nVertexID[0]);

		float3 a	= float3::cross(e1, e2);

		face.m_fPerimeter = e1.length() + e2.length() + (e2 - e1).length();

		face.m_Normal	= float3::normalize(a);
		face.m_fArea	= a.length() * 0.5f;

		if (float3::dotproduct(face.m_Normal, n) < 0.f)
			face.m_Normal *= -1.f;

		m_Faces.push_back(face);
	}
}


void CMeshCharts::ConnectFaces()
{
	unsigned int numFaces = static_cast<unsigned int>(m_Faces.size());

	SFace currentFace;

	for (unsigned int i = 0; i < numFaces; i++)
	{
		currentFace = m_Faces[i];

		for (unsigned int j = 0; j < numFaces; j++)
		{
			if (currentFace.m_nNumConnectedFaces == 3)
				break;

			if (i == j || m_Faces[j].m_nNumConnectedFaces == 3)
				continue;

			bool shouldContinue = false;

			for (unsigned int k = 0; k < currentFace.m_nNumConnectedFaces; k++)
				if (currentFace.m_nConnectedFacesID[k] == j)
					shouldContinue = true;

			if (shouldContinue)
				continue;

			unsigned int numSharedVertices = 0;

			for (unsigned int k = 0; k < 3; k++)
			{
				for (unsigned int l = 0; l < 3; l++)
					if (currentFace.m_nVertexID[l] == m_Faces[j].m_nVertexID[k])
						numSharedVertices++;
			}

			if (numSharedVertices == 2)
			{
				currentFace.m_nConnectedFacesID[currentFace.m_nNumConnectedFaces++] = j;
				m_Faces[j].m_nConnectedFacesID[m_Faces[j].m_nNumConnectedFaces++] = i;
			}
		}

		m_Faces[i] = currentFace;
	}
}


void CMeshCharts::Segment()
{
	InitSeeds();

	/*do
	{
		GrowCharts();
		UpdateSeeds();
		
	} while (!HasConverged());*/
}


void CMeshCharts::InitSeeds()
{
	unsigned int numFacesLeftToAssign = static_cast<unsigned int>(m_Faces.size());

	m_Seeds.clear();
	m_SeedsHistory.clear();

	m_ChartArea.clear();
	m_ChartNormal.clear();

	m_NumFaces.clear();

	unsigned int	currentFace = 0;
	unsigned int	nextFace;
	std::vector<unsigned int> history;

	while (numFacesLeftToAssign > 0)
	{
		history.clear();

		currentFace = PickNextSeed(currentFace);
		numFacesLeftToAssign--;
		history.push_back(currentFace);

		do
		{
			do
			{
				nextFace = SelectBestCandidate(currentFace);

				if (history.size() == 0)
					break;

				if (nextFace == 0xffffffff)
				{
					if (currentFace == history.back())
						history.pop_back();

					if (history.size() == 0)
						break;

					currentFace = history.back();					
				}

			} while (nextFace == 0xffffffff);

			if (nextFace != 0xffffffff)
			{
				AddFace(currentFace, nextFace);
				numFacesLeftToAssign--;
				history.push_back(nextFace);
				currentFace = nextFace;
			}

			else
				break;

		} while (1);
	}
}


void CMeshCharts::GrowCharts()
{
	unsigned int numFaces = static_cast<unsigned int>(m_Faces.size());

	for (unsigned int i = 0; i < numFaces; i++)
		m_Faces[i].m_nChartID = -1;

	unsigned int numCharts = static_cast<unsigned int>(m_Seeds.size());
	unsigned int numGrowingCharts = numCharts;

	std::vector<unsigned int> currentFace = m_Seeds;
	std::vector<std::vector<unsigned int>> history;
	std::vector<bool> isGrowing;
	isGrowing.resize(numCharts);
	history.resize(numCharts);

	for (unsigned int i = 0; i < numCharts; i++)
	{
		history[i].clear();
		m_Faces[currentFace[i]].m_nChartID = i;
		m_ChartArea[i] = m_Faces[currentFace[i]].m_fArea;
		isGrowing[i] = true;
		history[i].push_back(currentFace[i]);
	}

	while (numGrowingCharts > 0)
	{
		for (unsigned int i = 0; i < numCharts; i++)
		{
			if (!isGrowing[i])
				continue;

			unsigned int nextFace;
			
			do
			{
				nextFace = SelectBestCandidate(currentFace[i]);

				if (history[i].size() == 0)
					break;

				if (nextFace == 0xffffffff)
				{
					if (currentFace[i] == history[i].back())
						history[i].pop_back();

					if (history[i].size() == 0)
						break;

					currentFace[i] = history[i].back();
				}

			} while (nextFace == 0xffffffff);

			if (nextFace != 0xffffffff)
			{
				AddFace(currentFace[i], nextFace);
				history[i].push_back(nextFace);
				currentFace[i] = nextFace;
			}

			else
			{
				isGrowing[i] = false;
				numGrowingCharts--;
			}
		}
	}
}


void CMeshCharts::UpdateSeeds()
{
	ComputeChartNormals();

	ComputeSeeds();
}


unsigned int CMeshCharts::SelectBestCandidate(unsigned int currentFace)
{
	unsigned int	nBestFace	= 0xffffffff;
	unsigned int	nChartID	= m_Faces[currentFace].m_nChartID;
	float			faceCost	= 1e8f;

	for (unsigned int i = 0; i < m_Faces[currentFace].m_nNumConnectedFaces; i++)
	{
		unsigned int faceID = m_Faces[currentFace].m_nConnectedFacesID[i];

		if (m_Faces[faceID].m_nChartID >= 0)
			continue;

		for (unsigned int j = 0; j < m_Faces[faceID].m_nNumConnectedFaces; j++)
		{
			unsigned int f = m_Faces[faceID].m_nConnectedFacesID[j];

			if (m_Faces[f].m_nChartID == m_Faces[currentFace].m_nChartID)
			{
				float dist = (m_Faces[faceID].m_Barycenter - m_Faces[f].m_Barycenter).length();
				m_Faces[faceID].m_nDistanceToSeed = MIN(m_Faces[faceID].m_nDistanceToSeed, m_Faces[f].m_nDistanceToSeed + dist);
			}
		}

		float c = Cost(faceID, nChartID);

		if (c < faceCost && IsCandidateValid(currentFace, faceID))
		{
			faceCost = c;
			nBestFace = faceID;
		}
	}

	return nBestFace;
}


unsigned int CMeshCharts::SelectWorstCandidate(unsigned int currentFace)
{
	unsigned int	nWorstFace	= 0xffffffff;
	unsigned int	nChartID	= m_Faces[currentFace].m_nChartID;
	float			faceCost	= 0.f;

	for (unsigned int i = 0; i < m_Faces[currentFace].m_nNumConnectedFaces; i++)
	{
		unsigned int faceID = m_Faces[currentFace].m_nConnectedFacesID[i];

		if (m_Faces[faceID].m_nChartID >= 0)
			continue;

		float c = Cost(faceID, nChartID);

		if (c > faceCost)
		{
			faceCost = c;
			nWorstFace = faceID;
		}
	}

	return nWorstFace;
}


unsigned int CMeshCharts::PickNextSeed(unsigned int lastFace)
{
	unsigned int numFaces = static_cast<unsigned int>(m_Faces.size());
	unsigned int numCharts = static_cast<unsigned int>(m_Seeds.size());
	unsigned int nextSeed = 0;

	if (m_Seeds.size() == 0)
		nextSeed = Randi(0, numFaces);

	else
	{
		float fMaxDist = 0.f;

		for (unsigned int i = 0; i < numFaces; i++)
		{
			if (m_Faces[i].m_nChartID >= 0)
				continue;

			float dist = 0.f;

			for (unsigned int j = 0; j < numCharts; j++)
				dist += (m_Faces[i].m_Barycenter - m_Faces[m_Seeds[j]].m_Barycenter).length();

			if (dist > fMaxDist)
			{
				fMaxDist = dist;
				nextSeed = i;
			}
		}

		/*nextSeed = SelectWorstCandidate(lastFace);

		if (nextSeed == 0xffffffff)
		{
			for (unsigned int i = 0; i < numFaces; i++)
			{
				if (m_Faces[i].m_nChartID == -1)
				{
					nextSeed = i;
					break;
				}
			}
		}*/
	}

	m_Faces[nextSeed].m_nChartID = static_cast<unsigned int>(m_Seeds.size());
	m_Faces[nextSeed].m_nDistanceToSeed = 0.f;

	m_ChartNormal.push_back(m_Faces[nextSeed].m_Normal);
	m_ChartArea.push_back(m_Faces[nextSeed].m_fArea);

	m_NumFaces.push_back(1);

	m_Seeds.push_back(nextSeed);

	return nextSeed;
}


void CMeshCharts::AddFace(unsigned int currentFace, unsigned int nextFace)
{
	m_Faces[nextFace].m_nChartID = m_Faces[currentFace].m_nChartID;
	m_Faces[nextFace].m_nDistanceToSeed = 1e8f;

	for (unsigned int j = 0; j < m_Faces[nextFace].m_nNumConnectedFaces; j++)
	{
		unsigned int f = m_Faces[nextFace].m_nConnectedFacesID[j];

		if (m_Faces[f].m_nChartID == m_Faces[currentFace].m_nChartID)
		{
			float dist = (m_Faces[nextFace].m_Barycenter - m_Faces[f].m_Barycenter).length();
			m_Faces[nextFace].m_nDistanceToSeed = MIN(m_Faces[nextFace].m_nDistanceToSeed, m_Faces[f].m_nDistanceToSeed + dist);
		}
	}

	m_ChartArea[m_Faces[nextFace].m_nChartID] += m_Faces[nextFace].m_fArea;
	m_NumFaces[m_Faces[nextFace].m_nChartID]++;
}


bool CMeshCharts::IsCandidateValid(unsigned int currentFace, unsigned int nextFace)
{
	if (nextFace == 0xffffffff)
		return false;

	unsigned int chartID = m_Faces[currentFace].m_nChartID;

	//return float3::dotproduct(m_Faces[nextFace].m_Normal, m_ChartNormal[chartID]) > CLightmapMaker::ms_fMaxNormalAngle;
	return Cost(nextFace, chartID) < 2.f;
}


float CMeshCharts::Cost(unsigned int faceID, unsigned int chartID)
{
	static const float alpha	= 0.7f;
	static const float beta		= 0.5f;

	float F = 1.f - float3::dotproduct(m_Faces[faceID].m_Normal, m_ChartNormal[chartID]);
	F *= F;

	float C = 3.1415926f * m_Faces[faceID].m_nDistanceToSeed * m_Faces[faceID].m_nDistanceToSeed / (m_ChartArea[chartID] + m_Faces[faceID].m_fArea);

	float l_in = 0.f;
	float l_out = 0.f;

	for (unsigned int i = 0; i < m_Faces[faceID].m_nNumConnectedFaces; i++)
	{
		unsigned int index = m_Faces[faceID].m_nConnectedFacesID[i];

		float d = EdgeLength(faceID, index);

		if (m_Faces[index].m_nChartID == chartID)
			l_in += d;
	}

	float P = MAX(0.f, m_Faces[faceID].m_fPerimeter - l_in) / l_in;

	return F * powf(C, alpha) * powf(P, beta);
}


float CMeshCharts::EdgeLength(unsigned int face1, unsigned int face2)
{
	unsigned int vertexID[2];
	unsigned int numVertex = 0;

	for (unsigned int i = 0; i < 3; i++)
	{
		for (unsigned int j = 0; j < 3; j++)
			if (m_Faces[face1].m_nVertexID[i] == m_Faces[face2].m_nVertexID[j])
			{
				vertexID[numVertex] = m_Faces[face1].m_nVertexID[i];
				numVertex++;
			}
	}

	float3 v1 = m_pMesh->GetVertexPosition(vertexID[0]);
	float3 v2 = m_pMesh->GetVertexPosition(vertexID[1]);

	return (v1 - v2).length();
}


bool CMeshCharts::HasConverged()
{
	int numIterations			= static_cast<int>(m_SeedsHistory.size());
	unsigned int numSeeds		= static_cast<unsigned int>(m_Seeds.size());
	bool		 foundMatch		= false;

	if (numIterations > 1)
	{
		for (int i = numIterations - 1; i >= MAX(0, numIterations - 10); i--)
		{
			foundMatch = true;

			for (unsigned int j = 0; j < numSeeds; j++)
			{
				if (m_Seeds[j] != m_SeedsHistory[i][j])
				{
					foundMatch = false;
					break;
				}
			}

			if (foundMatch)
				break;
		}
	}

	m_SeedsHistory.push_back(m_Seeds);

	return foundMatch || m_SeedsHistory.size() > 20;
}


void CMeshCharts::ComputeChartNormals()
{
	unsigned int numCharts = static_cast<unsigned int>(m_Seeds.size());
	unsigned int numFaces = static_cast<unsigned int>(m_Faces.size());

	for (unsigned int i = 0; i < numCharts; i++)
		m_ChartNormal[i] = 0.f;

	for (unsigned int i = 0; i < numFaces; i++)
	{
		int chartID = m_Faces[i].m_nChartID;

		if (chartID < 0)
			continue;

		m_ChartNormal[chartID] += m_Faces[i].m_fArea * m_Faces[i].m_Normal;
	}

	for (unsigned int i = 0; i < numCharts; i++)
		m_ChartNormal[i].normalize();

	/*std::vector<float3> n0(numCharts);

	std::vector<float>  phi(numCharts);
	std::vector<float>  theta(numCharts);

	std::vector<float>  nextPhi(numCharts);
	std::vector<float>  nextTheta(numCharts);

	std::vector<float>  dphi(numCharts);
	std::vector<float>  dtheta(numCharts);

	std::vector<float>  gamma(numCharts);

	std::vector<float>	sum(numCharts);
	std::vector<float>	testSum(numCharts);

	std::vector<bool>   isLower(numCharts);
	
	bool done = false;
	unsigned int numIter = 0;

	for (unsigned int i = 0; i < numCharts; i++)
	{
		n0[i]		= m_ChartNormal[i];
		phi[i]		= atan2(n0[i].y, n0[i].x);
		theta[i]	= acosf(n0[i].z);

		n0[i] = float3(sinf(theta[i]) * cosf(phi[i]), sinf(theta[i]) * sinf(phi[i]), cosf(theta[i]));

		gamma[i] = 1.f;
	}

	ComputeSum(n0, sum);

	while (!done && numIter < 1000)
	{
		for (unsigned int i = 0; i < numCharts; i++)
		{
			float t = theta[i] + 1e-3f;
			float p = phi[i];

			n0[i] = float3(sinf(t) * cosf(p), sinf(t) * sinf(p), cosf(t));
		}

		ComputeSum(n0, dtheta);

		for (unsigned int i = 0; i < numCharts; i++)
		{
			float t = theta[i];
			float p = phi[i] + 1e-3f;

			n0[i] = float3(sinf(t) * cosf(p), sinf(t) * sinf(p), cosf(t));
		}

		ComputeSum(n0, dphi);

		for (unsigned int i = 0; i < numCharts; i++)
		{
			dtheta[i]	= (dtheta[i] - sum[i]) * 1e3f;
			dphi[i]		= (dphi[i] - sum[i]) * 1e3f;

			isLower[i] = false;
		}

		bool keepGoing = true;

		while (keepGoing)
		{
			for (unsigned int i = 0; i < numCharts; i++)
			{
				nextTheta[i] = theta[i] - gamma[i] * dtheta[i];
				nextPhi[i] = phi[i] - gamma[i] * dphi[i];

				n0[i] = float3(sinf(nextTheta[i]) * cosf(nextPhi[i]), sinf(nextTheta[i]) * sinf(nextPhi[i]), cosf(nextTheta[i]));
			}

			ComputeSum(n0, testSum);

			keepGoing = false;

			for (unsigned int i = 0; i < numCharts; i++)
			{
				if (dtheta[i] < 1e-2f && dphi[i] < 1e-2f)
					continue;

				if (!isLower[i] && testSum[i] < sum[i])
				{
					isLower[i] = true;
					gamma[i] *= 1.2f;
				}

				else if (testSum[i] >= sum[i])
				{
					keepGoing = true;
					isLower[i] = false;
					gamma[i] *= (1.f / 1.5f);
				}
			}
		}

		theta = nextTheta;
		phi = nextPhi;
		sum = testSum;

		done = true;

		for (unsigned int i = 0; i < numCharts; i++)
		{
			if (dtheta[i] > 1e-4f || dphi[i] > 1e-4f)
				done = false;
		}

		numIter++;
	}

	for (unsigned int i = 0; i < numCharts; i++)
		m_ChartNormal[i] = n0[i];*/
}


void CMeshCharts::ComputeSum(std::vector<float3>& normal, std::vector<float>& sum)
{
	unsigned int numCharts	= static_cast<unsigned int>(sum.size());
	unsigned int numFaces = static_cast<unsigned int>(m_Faces.size());

	for (unsigned int i = 0; i < numCharts; i++)
		sum[i] = 0.f;

	for (unsigned int i = 0; i < numFaces; i++)
	{
		int chartID = m_Faces[i].m_nChartID;

		if (chartID < 0)
			continue;

		float F = 1.f - float3::dotproduct(m_Faces[i].m_Normal, normal[chartID]);
		F *= F;

		sum[chartID] += m_Faces[i].m_fArea * F;
	}

	for (unsigned int i = 0; i < numCharts; i++)
		sum[i] /= m_ChartArea[i];
}


void CMeshCharts::ComputeSeeds()
{
	unsigned int numCharts = static_cast<unsigned int>(m_Seeds.size());
	unsigned int numFaces = static_cast<unsigned int>(m_Faces.size());

	std::vector<unsigned int>	bestFace[10];
	std::vector<float>			bestFit[10];	

	std::vector<float>          closestPos(numCharts);

	for (unsigned int j = 0; j < 10; j++)
	{
		bestFit[j].resize(numCharts);
		bestFace[j].resize(numCharts);

		for (unsigned int i = 0; i < numCharts; i++)
			bestFit[j][i] = 1e8f;
	}

	for (unsigned int i = 0; i < numFaces; i++)
	{
		int chartID = m_Faces[i].m_nChartID;

		if (chartID < 0)
			continue;

		float F = fabs(1.f - float3::dotproduct(m_Faces[i].m_Normal, m_ChartNormal[chartID]));

		for (unsigned int j = 0; j < 10; j++)
		{
			if (F < bestFit[j][chartID])
			{
				bestFit[j][chartID] = F;
				bestFace[j][chartID] = i;
			}
		}
	}

	for (unsigned int i = 0; i < numCharts; i++)
	{
		closestPos[i] = 1e8f;

		for (unsigned int j = 0; j < MIN(10, m_NumFaces[i]); j++)
		{
			if (m_Faces[bestFace[j][i]].m_nDistanceToSeed < closestPos[i])
			{
				closestPos[i] = m_Faces[bestFace[j][i]].m_nDistanceToSeed;
				bestFace[0][i] = bestFace[j][i];
			}
		}

		m_Seeds[i] = bestFace[0][i];
	}
}
