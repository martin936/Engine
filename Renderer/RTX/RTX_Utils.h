#ifndef __RTX_UTILS_H__
#define __RTX_UTILS_H__

#include "Engine/PolygonalMesh/PolygonalMesh.h"


class CRTX_BLAS
{
	friend class CRTX;
public:

	CRTX_BLAS(CMesh& parentMesh);
	~CRTX_BLAS();

	void AddToCurrentBatch();

	static void BuildCurrentBatch();

	struct SObjectDesc
	{
		uint32_t	m_MaterialID;
		uint32_t	m_VertexOffset;
		uint32_t	m_TexcoordOffset;
		uint32_t	m_Stride;

		uint64_t	m_VertexBufferAddr;
		uint64_t	m_IndexBufferAddr;
	};

private:

	struct BuildAccelerationStructure
	{
		VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
		VkAccelerationStructureBuildRangeInfoKHR*	rangeInfo;
		VkAccelerationStructureBuildSizesInfoKHR	sizeInfo;
		BufferId									asBufferId;
		VkAccelerationStructureKHR					accStruct;
	};

	BufferId					m_ObjectsBuffer;
	std::vector<SObjectDesc>	m_ObjectData;

	std::vector<Packet>			m_Packets;

	static void CreateBLAS(std::vector<uint32_t>& indices, std::vector<BuildAccelerationStructure>& buildAs, VkDeviceAddress scratchAddress, VkQueryPool& queryPool);
	static void CompactBLAS(std::vector<uint32_t>& indices, std::vector<BuildAccelerationStructure>& buildAs, VkQueryPool queryPool);

	static std::vector<CRTX_BLAS*> ms_CurrentBatch;

	BuildAccelerationStructure	m_AccStruct;

	std::vector<VkAccelerationStructureGeometryKHR>			m_asGeometry;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR>	m_asBuildOffsetInfo;
};


class CRTX
{
public:

	static void AddInstance(CRTX_BLAS* blas, float3x4 WorldMatrix);

	static void CreateTLAS();

	static void SetObjectBuffers(unsigned int nSlot);

	static VkPhysicalDeviceRayTracingPipelinePropertiesKHR ms_Properties;

	static VkAccelerationStructureKHR& GetTLAS()
	{
		return ms_tlasAccStruct;
	}

private:

	struct SInstance
	{
		CRTX_BLAS*	blas;
		float3x4	worldMatrix;
	};

	static std::vector<SInstance>		ms_Instances;
	static std::vector<BufferId>		ms_ObjectBuffers;

	static BufferId						ms_tlasBufferId;
	static VkAccelerationStructureKHR	ms_tlasAccStruct;
};


#endif
