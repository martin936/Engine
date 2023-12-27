#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "Engine/PolygonalMesh/PolygonalMesh.h"
#include "Engine/Renderer/Packets/Packet.h"
#include "RTX_Utils.h"


std::vector<CRTX_BLAS*>			CRTX_BLAS::ms_CurrentBatch;
std::vector<CRTX::SInstance>	CRTX::ms_Instances;
std::vector<BufferId>			CRTX::ms_ObjectBuffers;

BufferId						CRTX::ms_tlasBufferId;
VkAccelerationStructureKHR		CRTX::ms_tlasAccStruct;

VkPhysicalDeviceRayTracingPipelinePropertiesKHR		CRTX::ms_Properties;


extern void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

extern unsigned int				g_VertexStreamOffsetNoSkin[];


CRTX_BLAS::CRTX_BLAS(CMesh& mesh)
{
	std::vector<Packet>& packets = ((PacketList *)mesh.m_pPacketList)->m_pPackets;
	m_Packets = packets;

	m_asGeometry.clear();
	m_asBuildOffsetInfo.clear();

	unsigned int numPackets = static_cast<unsigned int>(packets.size());

	m_ObjectData.clear();
	m_ObjectData.reserve(numPackets);

	for (unsigned int i = 0; i < numPackets; i++)
	{
		VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
		triangles.sType						= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		triangles.vertexFormat				= VK_FORMAT_R32G32B32_SFLOAT;
		triangles.vertexData.deviceAddress	= (VkDeviceAddress)CResourceManager::GetBufferDeviceAddress(packets[i].m_VertexBuffer);
		triangles.vertexStride				= packets[i].m_nStride;

		triangles.indexType					= VK_INDEX_TYPE_UINT32;
		triangles.indexData.deviceAddress	= (VkDeviceAddress)CResourceManager::GetBufferDeviceAddress(packets[i].m_IndexBuffer);
		triangles.maxVertex					= packets[i].m_nNumVertex;

		VkAccelerationStructureGeometryKHR asGeom{};
		asGeom.sType						= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		asGeom.geometryType					= VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		asGeom.flags						= VK_GEOMETRY_OPAQUE_BIT_KHR;
		asGeom.geometry.triangles			= triangles;

		VkAccelerationStructureBuildRangeInfoKHR offset{};
		offset.firstVertex					= packets[i].m_nFirstVertex;
		offset.primitiveCount				= packets[i].m_nNumIndex / 3;
		offset.primitiveOffset				= packets[i].m_nFirstIndex / 3;
		offset.transformOffset				= 0;

		SObjectDesc obj;
		obj.m_VertexBufferAddr				= triangles.vertexData.deviceAddress;
		obj.m_IndexBufferAddr				= triangles.indexData.deviceAddress;
		obj.m_MaterialID					= packets[i].m_pMaterial->GetID();
		obj.m_VertexOffset					= g_VertexStreamOffsetNoSkin[e_POSITION] / sizeof(float);
		obj.m_TexcoordOffset				= g_VertexStreamOffsetNoSkin[e_TEXCOORD] / sizeof(float);
		obj.m_Stride						= packets[i].m_nStride / sizeof(float);

		m_ObjectData.emplace_back(obj);
		m_asGeometry.emplace_back(asGeom);
		m_asBuildOffsetInfo.emplace_back(offset);
	}

	m_ObjectsBuffer = CResourceManager::CreateBuffer(m_ObjectData.size() * sizeof(SObjectDesc), m_ObjectData.data());

	AddToCurrentBatch();
}


CRTX_BLAS::~CRTX_BLAS()
{
	CResourceManager::DestroyBuffer(m_AccStruct.asBufferId);
	CDeviceManager::vkDestroyAccelerationStructureKHR(CDeviceManager::GetDevice(), m_AccStruct.accStruct, nullptr);
}


void CRTX_BLAS::AddToCurrentBatch()
{
	ms_CurrentBatch.push_back(this);
}


void CRTX_BLAS::BuildCurrentBatch()
{
	uint32_t numBlas = static_cast<uint32_t>(ms_CurrentBatch.size());

	VkDeviceSize asTotalSize = 0;
	VkDeviceSize maxScratchSize = 0;

	std::vector<BuildAccelerationStructure> buildAs(numBlas);

	for (uint32_t i = 0; i < numBlas; i++)
	{
		buildAs[i].buildInfo.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildAs[i].buildInfo.type			= VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildAs[i].buildInfo.mode			= VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildAs[i].buildInfo.flags			= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		buildAs[i].buildInfo.geometryCount	= static_cast<uint32_t>(ms_CurrentBatch[i]->m_asGeometry.size());
		buildAs[i].buildInfo.pGeometries	= ms_CurrentBatch[i]->m_asGeometry.data();

		buildAs[i].rangeInfo				= ms_CurrentBatch[i]->m_asBuildOffsetInfo.data();
		buildAs[i].sizeInfo.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		std::vector<uint32_t> maxPrimCount(ms_CurrentBatch[i]->m_asBuildOffsetInfo.size());
		for (size_t tt = 0; tt < ms_CurrentBatch[i]->m_asBuildOffsetInfo.size(); tt++)
			maxPrimCount[tt] = ms_CurrentBatch[i]->m_asBuildOffsetInfo[tt].primitiveCount;

		CDeviceManager::vkGetAccelerationStructureBuildSizesKHR(CDeviceManager::GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildAs[i].buildInfo, maxPrimCount.data(), &buildAs[i].sizeInfo);

		asTotalSize += buildAs[i].sizeInfo.accelerationStructureSize;
		maxScratchSize += MAX(maxScratchSize, buildAs[i].sizeInfo.buildScratchSize);
	}

	VkBuffer scratchBuffer;
	VkDeviceMemory scratchMemory;

	createBuffer(maxScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, scratchBuffer, scratchMemory);

	VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, scratchBuffer };
	VkDeviceAddress           scratchAddress = vkGetBufferDeviceAddress(CDeviceManager::GetDevice(), &bufferInfo);

	VkQueryPool queryPool{ VK_NULL_HANDLE };

	VkQueryPoolCreateInfo qpci{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
	qpci.queryCount = numBlas;
	qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
	vkCreateQueryPool(CDeviceManager::GetDevice(), &qpci, nullptr, &queryPool);

	// Batching creation/compaction of BLAS to allow staying in restricted amount of memory
	std::vector<uint32_t> indices;  // Indices of the BLAS to create
	VkDeviceSize          batchSize{ 0 };
	VkDeviceSize          batchLimit{ 256'000'000 };  // 256 MB
	for (uint32_t idx = 0; idx < numBlas; idx++)
	{
		indices.push_back(idx);
		batchSize += buildAs[idx].sizeInfo.accelerationStructureSize;
		// Over the limit or last BLAS element
		if (batchSize >= batchLimit || idx == numBlas - 1)
		{			
			CreateBLAS(indices, buildAs, scratchAddress, queryPool);

			/*if (queryPool)
			{
				CompactBLAS(indices, buildAs, queryPool);
			}*/
			// Reset

			batchSize = 0;
			indices.clear();
		}
	}

	for (uint32_t i = 0; i < numBlas; i++)
		ms_CurrentBatch[i]->m_AccStruct = buildAs[i];

	vkDestroyQueryPool(CDeviceManager::GetDevice(), queryPool, nullptr);
	vkFreeMemory(CDeviceManager::GetDevice(), scratchMemory, nullptr);
	vkDestroyBuffer(CDeviceManager::GetDevice(), scratchBuffer, nullptr);
}


void CRTX_BLAS::CreateBLAS(std::vector<uint32_t>& indices, std::vector<BuildAccelerationStructure>& buildAs, VkDeviceAddress scratchAddress, VkQueryPool& queryPool)
{
	VkCommandBuffer cmdBuf = reinterpret_cast<VkCommandBuffer>(CCommandListManager::BeginOneTimeCommandList());

	if (queryPool)
		vkResetQueryPool(CDeviceManager::GetDevice(), queryPool, 0, static_cast<uint32_t>(indices.size()));

	uint32_t queryCnt{0};

	for (const uint32_t& idx : indices)
	{
		VkAccelerationStructureCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		createInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;

		buildAs[idx].asBufferId = CResourceManager::CreateAccelerationStructureBuffer(createInfo.size);

		createInfo.buffer = reinterpret_cast<VkBuffer>(CResourceManager::GetBufferHandle(buildAs[idx].asBufferId));		

		CDeviceManager::vkCreateAccelerationStructureKHR(CDeviceManager::GetDevice(), &createInfo, nullptr, &buildAs[idx].accStruct);

		buildAs[idx].buildInfo.dstAccelerationStructure		= buildAs[idx].accStruct;
		buildAs[idx].buildInfo.scratchData.deviceAddress	= scratchAddress;

		CDeviceManager::vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildAs[idx].buildInfo, &buildAs[idx].rangeInfo);

		VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

		/*if (queryPool)
		{
			// Add a query to find the 'real' amount of memory needed, use for compaction
			pvkCmdWriteAccelerationStructuresPropertiesKHR(cmdBuf, 1, &buildAs[idx].buildInfo.dstAccelerationStructure, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCnt++);
		}*/
	}

	CCommandListManager::EndOneTimeCommandList(cmdBuf);
}


void CRTX_BLAS::CompactBLAS(std::vector<uint32_t>& indices, std::vector<BuildAccelerationStructure>& buildAs, VkQueryPool queryPool)
{
	struct CleanUpInfos
	{
		VkAccelerationStructureKHR	as;
		BufferId					buffer;
	};

	uint32_t                    queryCtn{ 0 };
	std::vector<CleanUpInfos>	cleanupAS;  // previous AS to destroy

	VkCommandBuffer cmdBuf = reinterpret_cast<VkCommandBuffer>(CCommandListManager::BeginOneTimeCommandList());

	// Get the compacted size result back
	std::vector<VkDeviceSize> compactSizes(static_cast<uint32_t>(indices.size()));
	vkGetQueryPoolResults(CDeviceManager::GetDevice(), queryPool, 0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize),
		compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

	for (uint32_t idx : indices)
	{
		cleanupAS.push_back({ buildAs[idx].accStruct, buildAs[idx].asBufferId });       // previous AS to destroy
		buildAs[idx].sizeInfo.accelerationStructureSize = compactSizes[queryCtn++];  // new reduced size

		// Creating a compact version of the AS
		VkAccelerationStructureCreateInfoKHR asCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		asCreateInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

		buildAs[idx].asBufferId = CResourceManager::CreateAccelerationStructureBuffer(asCreateInfo.size);

		asCreateInfo.buffer = reinterpret_cast<VkBuffer>(CResourceManager::GetBufferHandle(buildAs[idx].asBufferId));

		CDeviceManager::vkCreateAccelerationStructureKHR(CDeviceManager::GetDevice(), &asCreateInfo, nullptr, &buildAs[idx].accStruct);

		// Copy the original BLAS to a compact version
		VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
		copyInfo.src = buildAs[idx].buildInfo.dstAccelerationStructure;
		copyInfo.dst = buildAs[idx].accStruct;
		copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
		CDeviceManager::vkCmdCopyAccelerationStructureKHR(cmdBuf, &copyInfo);
	}

	CCommandListManager::EndOneTimeCommandList(cmdBuf);

	for (CleanUpInfos infos : cleanupAS)
	{
		CResourceManager::DestroyBuffer(infos.buffer);
		CDeviceManager::vkDestroyAccelerationStructureKHR(CDeviceManager::GetDevice(), infos.as, nullptr);
	}	
}


void CRTX::AddInstance(CRTX_BLAS* blas, float3x4 WorldMatrix)
{
	ms_Instances.push_back({blas, WorldMatrix});
}


void CRTX::SetObjectBuffers(unsigned int nSlot)
{
	/*struct
	{
		uint64_t VertexBuffers;
		uint64_t IndexBuffers;
	} registers;

	CRTX_BLAS& blas = *ms_Instances[0].blas;

	unsigned int numPackets = static_cast<unsigned int>(blas.m_Packets.size());

	//for (unsigned int i = 0; i < numPackets; i++)
	//{
		registers.VertexBuffers	= (VkDeviceAddress)CResourceManager::GetBufferDeviceAddress(blas.m_Packets[0].m_VertexBuffer);
		registers.IndexBuffers	= (VkDeviceAddress)CResourceManager::GetBufferDeviceAddress(blas.m_Packets[0].m_IndexBuffer);
	//}

	CResourceManager::SetConstantBuffer(nSlot, &registers, sizeof(registers));

	CResourceManager::SetBuffer(7, blas.m_Packets[0].m_IndexBuffer);*/

	CResourceManager::SetBuffer(nSlot, ms_ObjectBuffers[0]);
}


void CRTX::CreateTLAS()
{
	std::vector<VkAccelerationStructureInstanceKHR> tlas;
	tlas.reserve(ms_Instances.size());

	uint32_t index = 0;

	ms_ObjectBuffers.clear();

	for (const SInstance& inst : ms_Instances)
	{
		VkAccelerationStructureInstanceKHR rayInst{};
		memcpy(&rayInst.transform, &inst.worldMatrix.m00, 12 * sizeof(float));
		rayInst.instanceCustomIndex						= index;
		rayInst.accelerationStructureReference			= (uint64_t)CResourceManager::GetBufferDeviceAddress(inst.blas->m_AccStruct.asBufferId);
		rayInst.flags									= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		rayInst.mask									= 0xFF;       //  Only be hit if rayMask & instance.mask != 0
		rayInst.instanceShaderBindingTableRecordOffset	= 0;  // We will use the same hit group for all objects
		tlas.emplace_back(rayInst);

		ms_ObjectBuffers.push_back(inst.blas->m_ObjectsBuffer);

		index++;
	}

	uint32_t countInstance = static_cast<uint32_t>(ms_Instances.size());

	BufferId instanceBuffer = CResourceManager::CreateAccelerationStructureInstanceBuffer(tlas.size() * sizeof(VkAccelerationStructureInstanceKHR), tlas.data());

	VkDeviceAddress instBufferAddr = (VkDeviceAddress)CResourceManager::GetBufferDeviceAddress(instanceBuffer);

	// Wraps a device pointer to the above uploaded instances.
	VkAccelerationStructureGeometryInstancesDataKHR instancesVk{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
	instancesVk.data.deviceAddress = instBufferAddr;

	// Put the above into a VkAccelerationStructureGeometryKHR. We need to put the instances struct in a union and label it as instance data.
	VkAccelerationStructureGeometryKHR topASGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	topASGeometry.geometry.instances = instancesVk;

	// Find sizes
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &topASGeometry;
	buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

	VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	CDeviceManager::vkGetAccelerationStructureBuildSizesKHR(CDeviceManager::GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &countInstance, &sizeInfo);

	VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	createInfo.size = sizeInfo.accelerationStructureSize;

	ms_tlasBufferId = CResourceManager::CreateAccelerationStructureBuffer(createInfo.size);

	createInfo.buffer = reinterpret_cast<VkBuffer>(CResourceManager::GetBufferHandle(ms_tlasBufferId));

	CDeviceManager::vkCreateAccelerationStructureKHR(CDeviceManager::GetDevice(), &createInfo, nullptr, &ms_tlasAccStruct);

	VkBuffer scratchBuffer;
	VkDeviceMemory scratchMemory;

	createBuffer(sizeInfo.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, scratchBuffer, scratchMemory);

	VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, scratchBuffer };
	VkDeviceAddress           scratchAddress = vkGetBufferDeviceAddress(CDeviceManager::GetDevice(), &bufferInfo);

	// Update build information
	buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
	buildInfo.dstAccelerationStructure = ms_tlasAccStruct;
	buildInfo.scratchData.deviceAddress = scratchAddress;

	// Build Offsets info: n instances
	VkAccelerationStructureBuildRangeInfoKHR        buildOffsetInfo{ countInstance, 0, 0, 0 };
	const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

	VkCommandBuffer cmdBuf = reinterpret_cast<VkCommandBuffer>(CCommandListManager::BeginOneTimeCommandList());

	// Build the TLAS
	CDeviceManager::vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfo, &pBuildOffsetInfo);

	CCommandListManager::EndOneTimeCommandList(cmdBuf);

	vkFreeMemory(CDeviceManager::GetDevice(), scratchMemory, nullptr);
	vkDestroyBuffer(CDeviceManager::GetDevice(), scratchBuffer, nullptr);
}

