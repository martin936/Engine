#include "Engine/Engine.h"
#include "../DeviceManager.h"
#include "../CommandListManager.h"
#include "../ResourceManager.h"
#include "../RenderThreads.h"
#include "../PipelineManager.h"
#include "Engine/Renderer/Window/Window.h"
#include "Engine/Renderer/RTX/RTX_Utils.h"
#include "Engine/Timer/Timer.h"
#include <set>

unsigned int CDeviceManager::ms_FrameIndex = 0;

thread_local static ProgramHandle		ms_ActiveProgramId;
thread_local int	gs_ActiveViewport[4] = { 0, 0, 0, 0 };

VkInstance				CDeviceManager::ms_pInstance		= VK_NULL_HANDLE;
VkPhysicalDevice		CDeviceManager::ms_pPhysicalDevice	= VK_NULL_HANDLE;
VkDevice				CDeviceManager::ms_pDevice			= VK_NULL_HANDLE;
VkQueue					CDeviceManager::ms_pPresentQueue	= VK_NULL_HANDLE;
VkSurfaceKHR			CDeviceManager::ms_pSurface			= VK_NULL_HANDLE;
VkSwapchainKHR			CDeviceManager::ms_pSwapchain		= VK_NULL_HANDLE;

unsigned int			CDeviceManager::ms_nWidth;
unsigned int			CDeviceManager::ms_nHeight;

std::vector<VkImage>		CDeviceManager::ms_SwapchainImages;
std::vector<VkImageView>	CDeviceManager::ms_SwapchainImageViews;
VkFormat					CDeviceManager::ms_SwapchainImageFormat;
VkExtent2D					CDeviceManager::ms_SwapchainImageExtent;

uint32_t				CDeviceManager::ms_GraphicsQueueFamilyIndex;


VkSemaphore		g_imageAvailableSemaphore[CDeviceManager::ms_FrameCount];
VkSemaphore		g_renderingFinishedSemaphore[CDeviceManager::ms_FrameCount];
VkFence			g_inFlightFences[CDeviceManager::ms_FrameCount];

bool			g_FirstFrame[CDeviceManager::ms_FrameCount] = { true };


PFN_vkGetBufferDeviceAddressKHR						CDeviceManager::vkGetBufferDeviceAddressKHR;
PFN_vkCreateAccelerationStructureKHR				CDeviceManager::vkCreateAccelerationStructureKHR;
PFN_vkDestroyAccelerationStructureKHR				CDeviceManager::vkDestroyAccelerationStructureKHR;
PFN_vkGetAccelerationStructureBuildSizesKHR			CDeviceManager::vkGetAccelerationStructureBuildSizesKHR;
PFN_vkGetAccelerationStructureDeviceAddressKHR		CDeviceManager::vkGetAccelerationStructureDeviceAddressKHR;
PFN_vkCmdBuildAccelerationStructuresKHR				CDeviceManager::vkCmdBuildAccelerationStructuresKHR;
PFN_vkBuildAccelerationStructuresKHR				CDeviceManager::vkBuildAccelerationStructuresKHR;
PFN_vkCmdWriteAccelerationStructuresPropertiesKHR	CDeviceManager::vkCmdWriteAccelerationStructuresPropertiesKHR;
PFN_vkCmdCopyAccelerationStructureKHR				CDeviceManager::vkCmdCopyAccelerationStructureKHR;
PFN_vkCmdTraceRaysKHR								CDeviceManager::vkCmdTraceRaysKHR;
PFN_vkGetRayTracingShaderGroupHandlesKHR			CDeviceManager::vkGetRayTracingShaderGroupHandlesKHR;
PFN_vkCreateRayTracingPipelinesKHR					CDeviceManager::vkCreateRayTracingPipelinesKHR;

VkDebugUtilsMessengerEXT gs_debugMessenger;

bool gs_ValidationSupported = false;
bool gs_bExtensionsSupported = false;

#ifdef _WIN32
extern HINSTANCE g_hInstance;
#endif

extern ETextureFormat ConvertFormat(VkFormat format);

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};


#define BUFFER_OFFSET(i) ((char *)NULL + (i))

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr) 
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else 
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) 
	{
		func(instance, debugMessenger, pAllocator);
	}
}


void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

void setupDebugMessenger() 
{
	if (!gs_ValidationSupported) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(CDeviceManager::GetInstance(), &createInfo, nullptr, &gs_debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}


struct QueueFamilyIndices 
{
	uint32_t graphicsFamily;
	uint32_t presentFamily;

	QueueFamilyIndices() { graphicsFamily = -1; presentFamily = -1; }

	bool IsComplete()
	{
		return graphicsFamily != 0xffffffff && presentFamily != 0xffffffff;
	}
};


struct SwapChainSupportDetails 
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};


bool checkValidationLayerSupport() 
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}


bool checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*>& deviceExtensions)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}


SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details;

	VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
	ASSERT(res == VK_SUCCESS);

	uint32_t formatCount;
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	ASSERT(res == VK_SUCCESS);

	if (formatCount != 0) 
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) 
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}


QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0U;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport)
			indices.presentFamily = i;		

		i++;
	}

	return indices;
}


VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
{
	for (const auto& availableFormat : availableFormats) 
	{
		if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	return availableFormats[0];
}


VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
{
	for (const auto& availablePresentMode : availablePresentModes) 
	{
		if (availablePresentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
			return availablePresentMode;
	}

	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
{
	if (capabilities.currentExtent.width != UINT32_MAX)
		return capabilities.currentExtent;
	
	else 
	{
		VkExtent2D actualExtent = { static_cast<uint32_t>(CDeviceManager::GetDeviceWidth()), static_cast<uint32_t>(CDeviceManager::GetDeviceHeight()) };

		actualExtent.width = MAX(capabilities.minImageExtent.width, MIN(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = MAX(capabilities.minImageExtent.height, MIN(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}


bool CreateInstance(VkInstance& instance)
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Fighting Potatoes";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Untitled Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

#ifdef _WIN32
	std::vector<const char*> instanceExtensions;

	uint32_t extensionCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

	for (const auto& extension : availableExtensions)
		instanceExtensions.push_back(extension.extensionName);

	const char** extensions = instanceExtensions.data();
#else
	unsigned int extensionCount = 0U;
	const char** extensions;

	extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
#endif

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = extensions;

//#if _DEBUG
	gs_ValidationSupported = checkValidationLayerSupport();
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (gs_ValidationSupported)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		//populateDebugMessengerCreateInfo(debugCreateInfo);
		//createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else 
//#endif
	{
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	ASSERT(result == VK_SUCCESS);

	return (result == VK_SUCCESS);
}


bool SelectPhysicalDevice(VkInstance instance, VkPhysicalDevice& physicalDevice)
{
	unsigned int deviceCount = 0U;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		return false;

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
		physicalDevice = devices[0];

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	printf("Found GPU : %s\n", deviceProperties.deviceName);

	return true;
}


ETextureFormat	CDeviceManager::GetFramebufferFormat()
{
	return ConvertFormat(ms_SwapchainImageFormat);
}


bool CDeviceManager::CreateLogicalDevice(VkInstance instance, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice& device)
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily };

	ms_GraphicsQueueFamilyIndex = indices.graphicsFamily;

	for (uint32_t queueFamily : uniqueQueueFamilies) 
	{
		unsigned int numQueues = (queueFamily == indices.graphicsFamily ? CCommandListManager::EQueueType::e_NumQueues : 1);
		float* queuePriority = new float[numQueues];
		
		for (unsigned int i = 0; i < numQueues; i++)
			queuePriority[i] = 1.f;

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = numQueues;
		queueCreateInfo.pQueuePriorities = queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy						= VK_TRUE;
	deviceFeatures.tessellationShader						= VK_TRUE;
	deviceFeatures.geometryShader							= VK_TRUE;
	deviceFeatures.shaderUniformBufferArrayDynamicIndexing	= VK_TRUE;
	deviceFeatures.shaderSampledImageArrayDynamicIndexing	= VK_TRUE;
	deviceFeatures.fragmentStoresAndAtomics					= VK_TRUE;
	deviceFeatures.depthClamp								= VK_TRUE;
	deviceFeatures.fillModeNonSolid							= VK_TRUE;
	deviceFeatures.wideLines								= VK_TRUE;
	deviceFeatures.imageCubeArray							= VK_TRUE;
	deviceFeatures.shaderStorageImageWriteWithoutFormat		= VK_TRUE;
	deviceFeatures.shaderInt64								= VK_TRUE;

	uint32_t count;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(count);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, availableExtensions.data());

	std::vector<const char*> deviceExtensions;

	for (auto extension : availableExtensions)
		deviceExtensions.push_back(extension.extensionName);
	
	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	deviceExtensions.push_back(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME);
	deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
	deviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
	//deviceExtensions.push_back("VK_KHR_buffer_device_address");

	VkPhysicalDeviceVulkan13Features vk13Features = {};
	vk13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vk13Features.dynamicRendering		= VK_TRUE;
	vk13Features.maintenance4			= VK_TRUE;

	VkPhysicalDeviceVulkan12Features vk12Features = {};
	vk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vk12Features.pNext = &vk13Features;
	vk12Features.bufferDeviceAddress	= VK_TRUE;
	vk12Features.hostQueryReset			= VK_TRUE;
	vk12Features.descriptorIndexing		= VK_TRUE;
	vk12Features.runtimeDescriptorArray = VK_TRUE;

	VkPhysicalDeviceRobustness2FeaturesEXT robustness{};
	robustness.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
	robustness.pNext = &vk12Features;
	robustness.nullDescriptor = VK_TRUE;

	VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT interlocked{};
	interlocked.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
	interlocked.pNext = &robustness;
	interlocked.fragmentShaderPixelInterlock = VK_TRUE;
	interlocked.fragmentShaderSampleInterlock = VK_TRUE;
	interlocked.fragmentShaderShadingRateInterlock = VK_FALSE;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR vkPhysicalDeviceAccelerationStructureFeatures = {};
	vkPhysicalDeviceAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	vkPhysicalDeviceAccelerationStructureFeatures.pNext = &interlocked;
	vkPhysicalDeviceAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
	vkPhysicalDeviceAccelerationStructureFeatures.accelerationStructureCaptureReplay = VK_FALSE;
	vkPhysicalDeviceAccelerationStructureFeatures.accelerationStructureHostCommands = VK_FALSE;
	vkPhysicalDeviceAccelerationStructureFeatures.accelerationStructureIndirectBuild = VK_FALSE;
	vkPhysicalDeviceAccelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR vkPhysicalDeviceRayTracingPipelineFeatures = {};
	vkPhysicalDeviceRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	vkPhysicalDeviceRayTracingPipelineFeatures.pNext = &vkPhysicalDeviceAccelerationStructureFeatures;
	vkPhysicalDeviceRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
	vkPhysicalDeviceRayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE;
	vkPhysicalDeviceRayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE;
	vkPhysicalDeviceRayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect = VK_FALSE;
	vkPhysicalDeviceRayTracingPipelineFeatures.rayTraversalPrimitiveCulling = VK_FALSE;

	VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.features = deviceFeatures;
	deviceFeatures2.pNext = &vkPhysicalDeviceRayTracingPipelineFeatures;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &deviceFeatures2;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	VkResult res = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
	ASSERT(res == VK_SUCCESS);

	for (size_t i = 0; i < queueCreateInfos.size(); i++)
	{
		delete[] queueCreateInfos[i].pQueuePriorities;
	}

	vkCmdBuildAccelerationStructuresKHR				= reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
	vkBuildAccelerationStructuresKHR				= reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
	vkCreateAccelerationStructureKHR				= reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
	vkDestroyAccelerationStructureKHR				= reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
	vkGetAccelerationStructureBuildSizesKHR			= reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
	vkGetAccelerationStructureDeviceAddressKHR		= reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
	vkCmdTraceRaysKHR								= reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
	vkGetRayTracingShaderGroupHandlesKHR			= reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
	vkCreateRayTracingPipelinesKHR					= reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
	vkCmdWriteAccelerationStructuresPropertiesKHR	= reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR>(vkGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureKHR"));
	vkCmdCopyAccelerationStructureKHR				= reinterpret_cast<PFN_vkCmdCopyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureKHR"));

	CRTX::ms_Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	CRTX::ms_Properties.pNext = nullptr;

	VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	prop2.pNext = &CRTX::ms_Properties;
	vkGetPhysicalDeviceProperties2(physicalDevice, &prop2);

	return (res == VK_SUCCESS);
}


bool CDeviceManager::CreateSwapchain(VkPhysicalDevice physicalDevice)
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, ms_pSurface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	ms_nWidth = extent.width;
	ms_nHeight = extent.height;

	uint32_t imageCount = ms_FrameCount;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = ms_pSurface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, ms_pSurface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	ms_SwapchainImageFormat = surfaceFormat.format;
	ms_SwapchainImageExtent = extent;

	VkResult res = vkCreateSwapchainKHR(ms_pDevice, &createInfo, nullptr, &ms_pSwapchain);
	ASSERT(res == VK_SUCCESS);

	vkGetSwapchainImagesKHR(ms_pDevice, ms_pSwapchain, &imageCount, nullptr);
	ms_SwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(ms_pDevice, ms_pSwapchain, &imageCount, ms_SwapchainImages.data());

	CreateImageViews();

	return (res == VK_SUCCESS);
}


bool CDeviceManager::CreateImageViews()
{
	ms_SwapchainImageViews.resize(ms_SwapchainImages.size());

	for (size_t i = 0; i < ms_SwapchainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = ms_SwapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = ms_SwapchainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(ms_pDevice, &createInfo, nullptr, &ms_SwapchainImageViews[i]) != VK_SUCCESS)
			return false;
	}

	return true;
}


void createSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	for (unsigned int i = 0; i < CDeviceManager::ms_FrameCount; i++)
	{
		VkResult res = vkCreateSemaphore(CDeviceManager::GetDevice(), &semaphoreInfo, nullptr, &g_imageAvailableSemaphore[i]);
		ASSERT(res == VK_SUCCESS);

		res = vkCreateSemaphore(CDeviceManager::GetDevice(), &semaphoreInfo, nullptr, &g_renderingFinishedSemaphore[i]);
		ASSERT(res == VK_SUCCESS);

		res = vkCreateFence(CDeviceManager::GetDevice(), &fenceInfo, nullptr, &g_inFlightFences[i]);
		ASSERT(res == VK_SUCCESS);

		g_FirstFrame[i] = true;
	}
}



void CDeviceManager::CreateDevice()
{
	if (!CreateInstance(ms_pInstance))
	{
		ASSERT_MSG(0, "ERROR: could not init Vulkan");
		return;
	}

#if _DEBUG
	//setupDebugMessenger();
#endif

	ms_pPhysicalDevice = VK_NULL_HANDLE;

	if (!SelectPhysicalDevice(ms_pInstance, ms_pPhysicalDevice))
	{
		ASSERT_MSG(0, "Error : could not find a GPU with Vulkan support");
		return;
	}

#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR surfCreateInfo{};
	surfCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfCreateInfo.hinstance = g_hInstance;
	surfCreateInfo.hwnd = (HWND)CWindow::GetMainWindow()->GetHandle();

	VkResult res = vkCreateWin32SurfaceKHR(CDeviceManager::GetInstance(), &surfCreateInfo, nullptr, &ms_pSurface);
#else
	VkResult res = glfwCreateWindowSurface(CDeviceManager::GetInstance(), (GLFWwindow*)(CWindow::GetMainWindow()->GetHandle()), nullptr, &ms_pSurface);
#endif
	ASSERT_MSG(res == VK_SUCCESS, "ERROR : Failed to create window surface");

	if (!CreateLogicalDevice(ms_pInstance, ms_pPhysicalDevice, ms_pSurface, ms_pDevice))
	{
		ASSERT_MSG(0, "Error : failed to create device");
		return;
	}

	if (!CreateSwapchain(ms_pPhysicalDevice))
	{
		ASSERT_MSG(0, "Error : failed to create swapchain");
		return;
	}

	createSyncObjects();

	CResourceManager::Init();
	CRenderWorkerThread::Init(1);
	CPipelineManager::Init();
	CFrameBlueprint::Init();

	VkCommandBuffer cmdBuffer = (VkCommandBuffer)CCommandListManager::BeginOneTimeCommandList();

	for (size_t i = 0; i < ms_SwapchainImages.size(); i++)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = ms_SwapchainImages[i];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	CCommandListManager::EndOneTimeCommandList(cmdBuffer);
}


void CDeviceManager::FlushGPU()
{
	vkDeviceWaitIdle(ms_pDevice);
}


void CDeviceManager::DestroyDevice()
{
	CPipelineManager::Terminate();
	CRenderWorkerThread::Terminate();
	CResourceManager::Terminate();	
	CCommandListManager::Terminate();

	for (unsigned int i = 0; i < CDeviceManager::ms_FrameCount; i++)
	{
		vkDestroySemaphore(ms_pDevice, g_imageAvailableSemaphore[i], nullptr);
		vkDestroySemaphore(ms_pDevice, g_renderingFinishedSemaphore[i], nullptr);
		vkDestroyFence(ms_pDevice, g_inFlightFences[i], nullptr);
	}

	for (auto imageView : ms_SwapchainImageViews)
		vkDestroyImageView(ms_pDevice, imageView, nullptr);

/*#if _DEBUG
	if (gs_ValidationSupported)
		DestroyDebugUtilsMessengerEXT(ms_pInstance, gs_debugMessenger, nullptr);
#endif*/

	vkDestroySwapchainKHR(ms_pDevice, ms_pSwapchain, nullptr);
	vkDestroySurfaceKHR(ms_pInstance, ms_pSurface, nullptr);
	vkDestroyDevice(ms_pDevice, nullptr);
	vkDestroyInstance(ms_pInstance, nullptr);
}



void CDeviceManager::InitFrame()
{
	VkResult res;

	if (!g_FirstFrame[ms_FrameIndex])
	{
		res = vkWaitForFences(ms_pDevice, 1, &g_inFlightFences[ms_FrameIndex], VK_TRUE, UINT64_MAX);
		ASSERT(res == VK_SUCCESS);
	}

	g_FirstFrame[ms_FrameIndex] = false;
	res = vkResetFences(ms_pDevice, 1, &g_inFlightFences[ms_FrameIndex]);
	ASSERT(res == VK_SUCCESS);

	uint32_t index;
	res = vkAcquireNextImageKHR(CDeviceManager::GetDevice(), ms_pSwapchain, UINT64_MAX, g_imageAvailableSemaphore[ms_FrameIndex], VK_NULL_HANDLE, &index);
	ASSERT(res == VK_SUCCESS);
}



void CDeviceManager::DrawInstanced(int nVertexOffset, int nVertexCount, int nInstanceOffset, int nInstanceCount)
{
	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorSet descriptorSet = (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()][pipeline->m_nCurrentVersion]);
	std::vector<unsigned int>& dynamicOffsets = pipeline->m_nDynamicOffsets;

	if (descriptorSet != nullptr)
		vkCmdBindDescriptorSets(pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipelineLayout)pipeline->m_pRootSignature, 0, 1, &descriptorSet, (uint32_t)dynamicOffsets.size(), dynamicOffsets.data());

	vkCmdDraw(pCmdBuffer, nVertexCount, nInstanceCount, nVertexOffset, nInstanceOffset);

	pipeline->m_bVersionNumUpToDate = false;
}



void CDeviceManager::RayTrace(unsigned int nSizeX, unsigned int nSizeY, unsigned int nSizeZ)
{
	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorSet descriptorSet = (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()][pipeline->m_nCurrentVersion]);
	std::vector<unsigned int>& dynamicOffsets = pipeline->m_nDynamicOffsets;

	if (descriptorSet != nullptr)
		vkCmdBindDescriptorSets(pCmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, (VkPipelineLayout)pipeline->m_pRootSignature, 0, 1, &descriptorSet, (uint32_t)dynamicOffsets.size(), dynamicOffsets.data());

	CDeviceManager::vkCmdTraceRaysKHR(pCmdBuffer, &pipeline->m_rgenRegion, &pipeline->m_missRegion, &pipeline->m_hitRegion, &pipeline->m_callRegion, nSizeX, nSizeY, nSizeZ);

	pipeline->m_bVersionNumUpToDate = false;
}


void CDeviceManager::SetStreams(unsigned int numStreams, SStream* pStreams)
{
	VkBuffer		buffers[e_MaxVertexElementUsage];
	VkDeviceSize	offsets[e_MaxVertexElementUsage];

	for (unsigned i = 0; i < numStreams; i++)
	{
		buffers[pStreams[i].m_nSlot] = reinterpret_cast<VkBuffer>(CResourceManager::GetBufferHandle(pStreams[i].m_nBufferId));
		offsets[pStreams[i].m_nSlot] = CResourceManager::GetBufferOffset(pStreams[i].m_nBufferId) + pStreams[i].m_nOffset;
	}

	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	vkCmdBindVertexBuffers(pCmdBuffer, 0, numStreams, buffers, offsets);
}


void CDeviceManager::BindIndexBuffer(unsigned int bufferId)
{
	VkBuffer buffer		= reinterpret_cast<VkBuffer>(CResourceManager::GetBufferHandle(bufferId));
	VkDeviceSize offset = CResourceManager::GetBufferOffset(bufferId);

	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	vkCmdBindIndexBuffer(pCmdBuffer, buffer, offset, VK_INDEX_TYPE_UINT32);
}


void CDeviceManager::DrawInstancedIndexed(int nIndexOffset, int nIndexCount, int nVertexOffset, int nInstanceOffset, int nInstanceCount)
{
	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorSet descriptorSet = (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()][pipeline->m_nCurrentVersion]);
	std::vector<unsigned int>& dynamicOffsets = pipeline->m_nDynamicOffsets;

	if (descriptorSet != nullptr)
		vkCmdBindDescriptorSets(pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipelineLayout)pipeline->m_pRootSignature, 0, 1, &descriptorSet, (uint32_t)dynamicOffsets.size(), dynamicOffsets.data());

	vkCmdDrawIndexed(pCmdBuffer, nIndexCount, nInstanceCount, nIndexOffset, nVertexOffset, nInstanceOffset);

	pipeline->m_bVersionNumUpToDate = false;
}


void CDeviceManager::Dispatch(unsigned int nThreadsX, unsigned int nThreadsY, unsigned int nThreadsZ)
{
	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorSet descriptorSet = (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()][pipeline->m_nCurrentVersion]);
	std::vector<unsigned int>& dynamicOffsets = pipeline->m_nDynamicOffsets;

	if (descriptorSet != nullptr)
		vkCmdBindDescriptorSets(pCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipelineLayout)pipeline->m_pRootSignature, 0, 1, &descriptorSet, (uint32_t)dynamicOffsets.size(), dynamicOffsets.data());

	vkCmdDispatch(pCmdBuffer, nThreadsX, nThreadsY, nThreadsZ);

	pipeline->m_bVersionNumUpToDate = false;
}


void CDeviceManager::DispatchIndirect(BufferId argsBuffer, size_t offset)
{
	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorSet descriptorSet = (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()][pipeline->m_nCurrentVersion]);
	std::vector<unsigned int>& dynamicOffsets = pipeline->m_nDynamicOffsets;

	if (descriptorSet != nullptr)
		vkCmdBindDescriptorSets(pCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipelineLayout)pipeline->m_pRootSignature, 0, 1, &descriptorSet, (uint32_t)dynamicOffsets.size(), dynamicOffsets.data());

	vkCmdDispatchIndirect(pCmdBuffer, (VkBuffer)CResourceManager::GetBufferHandle(argsBuffer), offset);

	pipeline->m_bVersionNumUpToDate = false;
}


void CDeviceManager::ClearDepth(float Z, unsigned int nSlice)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();

	ASSERT(pRenderPass->m_nDepthStencilID != INVALIDHANDLE);

	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	VkClearAttachment attachment{};
	attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	attachment.clearValue.depthStencil = { Z, 0 };
	
	VkClearRect rect{};
	rect.rect = { {0, 0}, {CTextureInterface::GetTextureWidth(pRenderPass->m_nDepthStencilID),CTextureInterface::GetTextureHeight(pRenderPass->m_nDepthStencilID)} };
	rect.baseArrayLayer = nSlice;
	rect.layerCount = 1;

	vkCmdClearAttachments(pCmdBuffer, 1, &attachment, 1, &rect);
}


void CDeviceManager::ClearDepth(std::vector<unsigned int>& slices, float Z)
{
	if (slices.size() == 0)
		return;

	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();

	ASSERT(pRenderPass->m_nDepthStencilID != INVALIDHANDLE);

	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	VkClearAttachment attachment{};
	attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	attachment.clearValue.depthStencil = { Z, 0 };
	
	unsigned int numSlices = static_cast<unsigned int>(slices.size());

	std::vector<VkClearRect> rect(numSlices);

	for (unsigned int i = 0; i < numSlices; i++)
	{
		rect[i].rect = { {0, 0}, {CTextureInterface::GetTextureWidth(pRenderPass->m_nDepthStencilID),CTextureInterface::GetTextureHeight(pRenderPass->m_nDepthStencilID)} };
		rect[i].baseArrayLayer = slices[i];
		rect[i].layerCount = 1;
	}

	vkCmdClearAttachments(pCmdBuffer, 1, &attachment, numSlices, rect.data());
}




void CDeviceManager::ClearStencil(unsigned int stencil, unsigned int nSlice)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();

	ASSERT(pRenderPass->m_nDepthStencilID != INVALIDHANDLE);

	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	VkClearAttachment attachment{};
	attachment.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
	attachment.clearValue.depthStencil = { 1.f, stencil };
	
	VkClearRect rect{};
	rect.rect = { {0, 0}, {CTextureInterface::GetTextureWidth(pRenderPass->m_nDepthStencilID),CTextureInterface::GetTextureHeight(pRenderPass->m_nDepthStencilID)} };
	rect.baseArrayLayer = nSlice;
	rect.layerCount = 1;

	vkCmdClearAttachments(pCmdBuffer, 1, &attachment, 1, &rect);
}


void CDeviceManager::ClearStencil(std::vector<unsigned int>& slices, unsigned int stencil)
{
	if (slices.size() == 0)
		return;

	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();

	ASSERT(pRenderPass->m_nDepthStencilID != INVALIDHANDLE);

	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	VkClearAttachment attachment{};
	attachment.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
	attachment.clearValue.depthStencil = { 1.f, stencil };
	
	unsigned int numSlices = static_cast<unsigned int>(slices.size());

	std::vector<VkClearRect> rect(numSlices);

	for (unsigned int i = 0; i < numSlices; i++)
	{
		rect[i].rect = { {0, 0}, {CTextureInterface::GetTextureWidth(pRenderPass->m_nDepthStencilID),CTextureInterface::GetTextureHeight(pRenderPass->m_nDepthStencilID)} };
		rect[i].baseArrayLayer = slices[i];
		rect[i].layerCount = 1;
	}

	vkCmdClearAttachments(pCmdBuffer, 1, &attachment, numSlices, rect.data());
}




void CDeviceManager::ClearDepthStencil(float Z, unsigned int stencil, unsigned int nSlice)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();

	ASSERT(pRenderPass->m_nDepthStencilID != INVALIDHANDLE);

	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	VkClearAttachment attachment{};
	attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	attachment.clearValue.depthStencil = { Z, stencil };
	
	VkClearRect rect{};
	rect.rect = { {0, 0}, {CTextureInterface::GetTextureWidth(pRenderPass->m_nDepthStencilID),CTextureInterface::GetTextureHeight(pRenderPass->m_nDepthStencilID)} };
	rect.baseArrayLayer = nSlice;
	rect.layerCount = 1;

	vkCmdClearAttachments(pCmdBuffer, 1, &attachment, 1, &rect);
}


void CDeviceManager::ClearDepthStencil(std::vector<unsigned int>& slices, float Z, unsigned int stencil)
{
	if (slices.size() == 0)
		return;

	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();

	ASSERT(pRenderPass->m_nDepthStencilID != INVALIDHANDLE);

	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	VkClearAttachment attachment{};
	attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	attachment.clearValue.depthStencil = { Z, stencil };
	
	unsigned int numSlices = static_cast<unsigned int>(slices.size());

	std::vector<VkClearRect> rect(numSlices);

	for (unsigned int i = 0; i < numSlices; i++)
	{
		rect[i].rect = { {0, 0}, {CTextureInterface::GetTextureWidth(pRenderPass->m_nDepthStencilID),CTextureInterface::GetTextureHeight(pRenderPass->m_nDepthStencilID)} };
		rect[i].baseArrayLayer = slices[i];
		rect[i].layerCount = 1;
	}

	vkCmdClearAttachments(pCmdBuffer, 1, &attachment, numSlices, rect.data());
}



void CDeviceManager::PrepareToFlip(void* cmdBuffer)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = ms_SwapchainImages[ms_FrameIndex];
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = 0;

	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(cmdBuffer);
	
	if (pCmdBuffer == nullptr)
		pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	vkCmdPipelineBarrier(pCmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	CTimerManager::GetGPUTimer("GPU Frame")->Stop();
}



void CDeviceManager::PrepareToDraw()
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = ms_SwapchainImages[ms_FrameIndex];
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

	VkCommandBuffer pCmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	vkCmdPipelineBarrier(pCmdBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	CTimerManager::ResetGPUTimers();

	CTimerManager::GetGPUTimer("GPU Frame")->Start();
}



void CDeviceManager::FlipScreen()
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	VkSemaphore waitSemaphores[] = { g_imageAvailableSemaphore[ms_FrameIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	VkSemaphore signalSemaphores[] = { g_renderingFinishedSemaphore[ms_FrameIndex] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VkResult res = vkQueueSubmit((VkQueue)CCommandListManager::GetCommandQueuePtr(CCommandListManager::e_Queue_Direct), 1, &submitInfo, g_inFlightFences[ms_FrameIndex]);
	ASSERT(res == VK_SUCCESS);

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { ms_pSwapchain };
	presentInfo.swapchainCount	= 1;
	presentInfo.pSwapchains		= swapChains;
	presentInfo.pImageIndices	= &ms_FrameIndex;
	presentInfo.pResults		= nullptr;

	vkQueuePresentKHR((VkQueue)CCommandListManager::GetCommandQueuePtr(CCommandListManager::e_Queue_Direct), &presentInfo);

	ms_FrameIndex = (ms_FrameIndex + 1) % ms_FrameCount;
}


void CDeviceManager::ResetViewport()
{
	gs_ActiveViewport[0] = gs_ActiveViewport[1] = gs_ActiveViewport[2] = gs_ActiveViewport[3] = 0;
}


void CDeviceManager::SetViewport(unsigned int xstart, unsigned int ystart, unsigned int xend, unsigned int yend)
{
	if (xstart != gs_ActiveViewport[0] || ystart != gs_ActiveViewport[1] || xend != gs_ActiveViewport[2] || yend != gs_ActiveViewport[3])
	{
		VkCommandBuffer cmd = (VkCommandBuffer)CCommandListManager::GetCurrentThreadCommandListPtr();

		VkViewport viewport = { 1.f * xstart, 1.f * yend, 1.f * xend - xstart, 1.f * ystart - yend, 0.f, 1.f };

		VkRect2D scissor = { {(int)xstart, (int)ystart}, {xend - xstart, yend - ystart} };

		vkCmdSetScissor(cmd, 0, 1, &scissor);
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		gs_ActiveViewport[0] = xstart;
		gs_ActiveViewport[1] = ystart;
		gs_ActiveViewport[2] = xend;
		gs_ActiveViewport[3] = yend;
	}
}


unsigned int CDeviceManager::GetViewportWidth()
{
	return gs_ActiveViewport[2] - gs_ActiveViewport[0];
}


unsigned int CDeviceManager::GetViewportHeight()
{
	return gs_ActiveViewport[3] - gs_ActiveViewport[1];
}

