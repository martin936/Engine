#ifndef __DEVICE_MANAGER_H__
#define __DEVICE_MANAGER_H__

#include "Engine/Engine.h"
#include "Engine/Device/Shaders.h"
#include "Engine/Renderer/Textures/Textures.h"
#include <vector>


class CDeviceManager
{
public:

#ifdef __VULKAN__
	static PFN_vkGetBufferDeviceAddressKHR						vkGetBufferDeviceAddressKHR;
	static PFN_vkCreateAccelerationStructureKHR					vkCreateAccelerationStructureKHR;
	static PFN_vkDestroyAccelerationStructureKHR				vkDestroyAccelerationStructureKHR;
	static PFN_vkGetAccelerationStructureBuildSizesKHR			vkGetAccelerationStructureBuildSizesKHR;
	static PFN_vkGetAccelerationStructureDeviceAddressKHR		vkGetAccelerationStructureDeviceAddressKHR;
	static PFN_vkCmdBuildAccelerationStructuresKHR				vkCmdBuildAccelerationStructuresKHR;
	static PFN_vkBuildAccelerationStructuresKHR					vkBuildAccelerationStructuresKHR;
	static PFN_vkCmdWriteAccelerationStructuresPropertiesKHR	vkCmdWriteAccelerationStructuresPropertiesKHR;
	static PFN_vkCmdCopyAccelerationStructureKHR				vkCmdCopyAccelerationStructureKHR;
	static PFN_vkCmdTraceRaysKHR								vkCmdTraceRaysKHR;
	static PFN_vkGetRayTracingShaderGroupHandlesKHR				vkGetRayTracingShaderGroupHandlesKHR;
	static PFN_vkCreateRayTracingPipelinesKHR					vkCreateRayTracingPipelinesKHR;
#endif

	struct SStream
	{
		unsigned int m_nSlot;
		unsigned int m_nBufferId;
		unsigned int m_nOffset;
	};

	static void CreateDevice();
	static void DestroyDevice();

	static void DrawInstanced(int nVertexOffset, int nVertexCount, int nInstanceOffset, int nInstanceCount);
	static void DrawInstancedIndexed(int nIndexOffset, int nIndexCount, int nVertexOffset, int nInstanceOffset, int nInstanceCount);

	static void Dispatch(unsigned int nThreadsX, unsigned int nThreadsY, unsigned int nThreadsZ);
	static void DispatchIndirect(unsigned int argsBuffer, size_t offset);

	static void RayTrace(unsigned int nSizeX, unsigned int nSizeY, unsigned int nSizeZ);

	static void ClearDepthStencil(float Z = 1.f, unsigned int stencil = 0, unsigned int nSlice = 0);
	static void ClearDepthStencil(std::vector<unsigned int>& slices, float Z = 1.f, unsigned int stencil = 0);

	static void ClearDepth(float Z = 1.f, unsigned int nSlice = 0);
	static void ClearDepth(std::vector<unsigned int>& slices, float Z = 1.f);

	static void ClearStencil(unsigned int stencil = 0, unsigned int nSlice = 0);
	static void ClearStencil(std::vector<unsigned int>& slices, unsigned int stencil = 0);

	static void SetStreams(std::vector<SStream>& streams);

	static void BindIndexBuffer(unsigned int bufferId);

	static void SetViewport(unsigned int xstart, unsigned int ystart, unsigned int xend, unsigned int yend);
	static unsigned int GetViewportWidth();
	static unsigned int GetViewportHeight();
	static void ResetViewport();

	static void FlipScreen();

	static void PrepareToFlip(void* cmdBuffer = nullptr);
	static void PrepareToDraw();

	static void InitFrame();

	static void FlushGPU();

	static unsigned int GetDeviceWidth() { return ms_nWidth; }
	static unsigned int GetDeviceHeight() { return ms_nHeight; }

#ifdef __VULKAN__
	static VkInstance			GetInstance()			{ return ms_pInstance; }
	static VkDevice				GetDevice()				{ return ms_pDevice; }
	static VkPhysicalDevice		GetPhysicalDevice()		{ return ms_pPhysicalDevice; }
	static VkSurfaceKHR			GetSurface()			{ return ms_pSurface; }

	static uint32_t				GetGraphicsQueueFamilyIndex()	{ return ms_GraphicsQueueFamilyIndex;	}

	static VkImageView			GetFramebufferImageView(int frameIndex)
	{
		return ms_SwapchainImageViews[frameIndex];
	}

	static VkImageView			GetCurrentFramebufferImageView()
	{
		return ms_SwapchainImageViews[ms_FrameIndex];
	}

	static ETextureFormat		GetFramebufferFormat();
#endif

	static unsigned int GetFrameIndex()
	{
		return ms_FrameIndex;
	}

	static const unsigned int		ms_FrameCount = 2;

private:

	static unsigned int						ms_FrameIndex;

	thread_local static ProgramHandle		ms_ActiveProgramId;

	static unsigned int				ms_nWidth;
	static unsigned int				ms_nHeight;

#ifdef __VULKAN__
	static VkInstance				ms_pInstance;
	static VkPhysicalDevice			ms_pPhysicalDevice;
	static VkDevice					ms_pDevice;
	static VkQueue					ms_pPresentQueue; 
	static VkSurfaceKHR				ms_pSurface;

	static VkSwapchainKHR			ms_pSwapchain;
	static std::vector<VkImage>		ms_SwapchainImages;
	static std::vector<VkImageView> ms_SwapchainImageViews;
	static VkFormat					ms_SwapchainImageFormat;
	static VkExtent2D				ms_SwapchainImageExtent;

	static uint32_t					ms_GraphicsQueueFamilyIndex;

	static bool CreateSwapchain(VkPhysicalDevice physicalDevice);
	static bool	CreateLogicalDevice(VkInstance instance, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice& device);
	static bool CreateImageViews();
#endif
};


#endif