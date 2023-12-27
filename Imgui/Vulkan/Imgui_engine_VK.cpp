#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "Engine/Device/ResourceManager.h"
#include "Engine/Renderer/Window/Window.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/GameRenderPass.h"
#include "Engine/Imgui/imgui.h"
#include "Engine/Imgui/Vulkan/imgui_impl_vulkan.h"
#include "Engine/Imgui/imgui_impl_win32.h"
#include "../Imgui_engine.h"


static void check_vk_result(VkResult err)
{
	ASSERT(err == VK_SUCCESS);
}


void Imgui_RenderPass();


void CImGui_Impl::Init()
{
	if (CRenderPass::BeginGraphics(ERenderPassId::e_Imgui, "Imgui"))
	{
		CRenderPass::BindResourceToWrite(0, INVALIDHANDLE, CRenderPass::e_RenderTarget);
		CRenderPass::SetNumTextures(0, 1);
		CRenderPass::SetNumSamplers(1, 1);

		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

		CRenderPass::BindProgram("Imgui", "Imgui");

		CRenderPass::SetBlendState(true, false, EBlendFunc::e_BlendFunc_SrcAlpha, EBlendFunc::e_BlendFunc_InvSrcAlpha, EBlendOp::e_BlendOp_Add, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_Zero, EBlendOp::e_BlendOp_Add);

		CRenderPass::SetEntryPoint(Imgui_RenderPass);

		CRenderPass::End();
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init((HWND)CWindow::GetMainWindow()->GetHandle());

	// Setup Platform/Renderer bindings
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = CDeviceManager::GetInstance();
	init_info.PhysicalDevice = CDeviceManager::GetPhysicalDevice();
	init_info.Device = CDeviceManager::GetDevice();
	init_info.QueueFamily = CDeviceManager::GetGraphicsQueueFamilyIndex();
	init_info.Queue = (VkQueue)CCommandListManager::GetCommandQueuePtr(CCommandListManager::e_Queue_Direct);
	init_info.PipelineCache = nullptr;
	init_info.DescriptorPool = (VkDescriptorPool)CResourceManager::GetLocalSRVDecriptorHeap(0);
	init_info.Allocator = nullptr;
	init_info.MinImageCount = CDeviceManager::ms_FrameCount;
	init_info.ImageCount = CDeviceManager::ms_FrameCount;
	init_info.CheckVkResultFn = check_vk_result;

	ImGui_ImplVulkan_Init(&init_info);

	// Use any command queue
	VkCommandPool command_pool = reinterpret_cast<VkCommandPool>(CCommandListManager::GetLoadingCommandListAllocator());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = command_pool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	VkResult err = vkAllocateCommandBuffers(CDeviceManager::GetDevice(), &allocInfo, &command_buffer);
	ASSERT(err == VK_SUCCESS);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	err = vkBeginCommandBuffer(command_buffer, &begin_info);
	ASSERT(err == VK_SUCCESS);

	ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

	VkSubmitInfo end_info = {};
	end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	end_info.commandBufferCount = 1;
	end_info.pCommandBuffers = &command_buffer;
	err = vkEndCommandBuffer(command_buffer);
	ASSERT(err == VK_SUCCESS);
	err = vkQueueSubmit((VkQueue)CCommandListManager::GetCommandQueuePtr(CCommandListManager::e_Queue_Direct), 1, &end_info, VK_NULL_HANDLE);
	ASSERT(err == VK_SUCCESS);

	err = vkDeviceWaitIdle(CDeviceManager::GetDevice());
	ASSERT(err == VK_SUCCESS);
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	vkFreeCommandBuffers(CDeviceManager::GetDevice(), command_pool, 1, &command_buffer);

	NewFrame();
}



void Imgui_RenderPass()
{
	CResourceManager::SetTexture(0, ImGui_ImplVulkan_GetFontImageView());
	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Wrap);

	CRenderer::DrawPackets(e_RenderType_ImGui);
}



void CImGui_Impl::Terminate()
{
	ImGui_ImplWin32_Shutdown();
	ImGui_ImplVulkan_Shutdown();
	ImGui::DestroyContext();
}


void CImGui_Impl::NewFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();
}

