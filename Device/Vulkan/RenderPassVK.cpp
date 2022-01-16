#include "../RenderPass.h"
#include "../PipelineManager.h"
#include "../ResourceManager.h"
#include "../CommandListManager.h"
#include "Engine/Renderer/Textures/TextureInterface.h"

void ConvertResourceState(unsigned int eState, unsigned int& layout, unsigned int& stageFlags, unsigned int& accessFlags)
{
	layout = VK_IMAGE_LAYOUT_UNDEFINED;
	stageFlags = 0;
	accessFlags = 0;

	if (eState & CRenderPass::e_PixelShaderResource)
	{
		layout |= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessFlags |= VK_ACCESS_SHADER_READ_BIT;
	}

	if (eState & CRenderPass::e_NonPixelShaderResource)
	{
		layout |= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		stageFlags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		accessFlags |= VK_ACCESS_SHADER_READ_BIT;
	}

	if (eState & CRenderPass::e_ShaderResource)
	{
		layout |= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		accessFlags |= VK_ACCESS_SHADER_READ_BIT;
	}

	if (eState & CRenderPass::e_RenderTarget)
	{
		layout |= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessFlags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	if (eState & CRenderPass::e_DepthStencil_Write)
	{
		layout |= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessFlags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	if (eState & CRenderPass::e_DepthStencil_Read)
	{
		layout |= VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		accessFlags |= VK_ACCESS_SHADER_READ_BIT;
	}

	if (eState & CRenderPass::e_UnorderedAccess)
	{
		layout |= VK_IMAGE_LAYOUT_GENERAL;
		stageFlags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		accessFlags |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	}

	if (eState & CRenderPass::e_CopyDest)
	{
		layout |= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		accessFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	if (eState & CRenderPass::e_CopySrc)
	{
		layout |= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		accessFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;
	}
}


extern VkFormat ConvertFormat(ETextureFormat format);


CRenderPass::~CRenderPass()
{
	int numFramebuffers = static_cast<int>(m_pFramebuffer.size());

	for (int i = 0; i < numFramebuffers; i++)
		vkDestroyFramebuffer(CDeviceManager::GetDevice(), (VkFramebuffer)m_pFramebuffer[i], nullptr);

	vkDestroyRenderPass(CDeviceManager::GetDevice(), (VkRenderPass)m_pDeviceRenderPass, nullptr);

	if (m_pEntryPointParam != nullptr)
		delete m_pEntryPointParam;

	m_SubPasses.clear();
}



void CRenderPass::CreateFramebuffer()
{
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(m_nPipelineStateID);

	if (m_nWritenResourceID.size() == 0)
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format	= CDeviceManager::GetFramebufferFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass		= 0;
		dependency.dstSubpass		= 0;
		dependency.srcStageMask		= 0;
		dependency.dstStageMask		= 0;
		dependency.srcAccessMask	= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		dependency.dstAccessMask	= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		if (m_bEnableMemoryBarriers)
		{
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;
		}

		VkResult res = vkCreateRenderPass(CDeviceManager::GetDevice(), &renderPassInfo, nullptr, (VkRenderPass*)&m_pDeviceRenderPass);
		ASSERT(res == VK_SUCCESS);

		m_pFramebuffer.resize(CDeviceManager::ms_FrameCount);

		for (size_t i = 0; i < CDeviceManager::ms_FrameCount; i++) 
		{
			VkImageView attachments[] = { CDeviceManager::GetFramebufferImageView((int)i) };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass		= (VkRenderPass)m_pDeviceRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments	= attachments;
			framebufferInfo.width			= CDeviceManager::GetDeviceWidth();
			framebufferInfo.height			= CDeviceManager::GetDeviceHeight();
			framebufferInfo.layers			= 1;

			res = vkCreateFramebuffer(CDeviceManager::GetDevice(), &framebufferInfo, nullptr, (VkFramebuffer*)&m_pFramebuffer[i]);
			ASSERT(res == VK_SUCCESS);
		}
	}

	else
	{
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

		framebufferInfo.layers = 1;

		int numTargets = static_cast<int>(m_nWritenResourceID.size());
		std::vector<VkImageView> attachments(numTargets);

		int numAttachments = 0;
		VkAttachmentDescription colorAttachment[9]{};
		VkAttachmentReference	colorAttachmentRef[8]{};
		VkAttachmentReference	depthstencilAttachmentRef{};

		for (int i = 0; i < numTargets; i++)
		{
			if (m_nWritenResourceID[i].m_eType == e_RenderTarget)
			{
				ETextureType eType = CTextureInterface::GetTextureType(m_nWritenResourceID[i].m_nResourceID);

				framebufferInfo.layers = CTextureInterface::GetTextureArraySize(m_nWritenResourceID[i].m_nResourceID);
				framebufferInfo.layers *= (eType == eCubeMap || eType == eCubeMapArray) ? 6 : 1;

				colorAttachment[numAttachments].format = ConvertFormat(CTextureInterface::GetTextureFormat(m_nWritenResourceID[i].m_nResourceID));
				colorAttachment[numAttachments].samples = (VkSampleCountFlagBits)(1 << (CTextureInterface::GetTextureSampleCount(m_nWritenResourceID[i].m_nResourceID) - 1));
				colorAttachment[numAttachments].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				colorAttachment[numAttachments].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachment[numAttachments].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				colorAttachment[numAttachments].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachment[numAttachments].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorAttachment[numAttachments].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				colorAttachmentRef[numAttachments].attachment = m_nWritenResourceID[i].m_nSlot;
				colorAttachmentRef[numAttachments].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				framebufferInfo.width = CTextureInterface::GetTextureWidth(m_nWritenResourceID[0].m_nResourceID, m_nWritenResourceID[0].m_nLevel);
				framebufferInfo.height = CTextureInterface::GetTextureHeight(m_nWritenResourceID[0].m_nResourceID, m_nWritenResourceID[0].m_nLevel);

				attachments[m_nWritenResourceID[i].m_nSlot] = CTextureInterface::GetTexture(m_nWritenResourceID[i].m_nResourceID)->GetImageView(m_nWritenResourceID[i].m_nSlice, m_nWritenResourceID[i].m_nLevel);
				numAttachments++;
			}
		}

		if (m_nDepthStencilID != INVALIDHANDLE)
		{
			for (int i = 0; i < numTargets; i++)
			{
				if (m_nWritenResourceID[i].m_eType == e_DepthStencil_Write)
				{
					ETextureType eType = CTextureInterface::GetTextureType(m_nWritenResourceID[i].m_nResourceID);

					framebufferInfo.layers = CTextureInterface::GetTextureArraySize(m_nWritenResourceID[i].m_nResourceID);
					framebufferInfo.layers *= (eType == eCubeMap || eType == eCubeMapArray) ? 6 : 1;

					colorAttachment[numAttachments].format = ConvertFormat(CTextureInterface::GetTextureFormat(m_nWritenResourceID[i].m_nResourceID));
					colorAttachment[numAttachments].samples = (VkSampleCountFlagBits)(1 << (CTextureInterface::GetTextureSampleCount(m_nWritenResourceID[i].m_nResourceID) - 1));
					colorAttachment[numAttachments].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
					colorAttachment[numAttachments].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					colorAttachment[numAttachments].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
					colorAttachment[numAttachments].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
					colorAttachment[numAttachments].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					colorAttachment[numAttachments].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					depthstencilAttachmentRef.attachment = numAttachments;
					depthstencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

					framebufferInfo.width = CTextureInterface::GetTextureWidth(m_nWritenResourceID[0].m_nResourceID, m_nWritenResourceID[0].m_nLevel);
					framebufferInfo.height = CTextureInterface::GetTextureHeight(m_nWritenResourceID[0].m_nResourceID, m_nWritenResourceID[0].m_nLevel);

					attachments[numAttachments] = CTextureInterface::GetTexture(m_nWritenResourceID[i].m_nResourceID)->GetImageView(m_nWritenResourceID[i].m_nSlice, m_nWritenResourceID[i].m_nLevel);
					numAttachments++;
					break;
				}
			}
		}

		framebufferInfo.attachmentCount = numAttachments;
		framebufferInfo.pAttachments = attachments.data();

		VkSubpassDependency dependency{};
		dependency.srcSubpass = 0;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = 0;
		dependency.dstStageMask = 0;
		dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments = colorAttachmentRef;

		if (m_nDepthStencilID != INVALIDHANDLE)
		{
			subpass.colorAttachmentCount	= numAttachments - 1;
			subpass.pDepthStencilAttachment = &depthstencilAttachmentRef;
		}

		else
			subpass.colorAttachmentCount = numAttachments;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = numAttachments;
		renderPassInfo.pAttachments = colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		if (m_bEnableMemoryBarriers)
		{
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;
		}

		VkResult res = vkCreateRenderPass(CDeviceManager::GetDevice(), &renderPassInfo, nullptr, (VkRenderPass*)&m_pDeviceRenderPass);
		ASSERT(res == VK_SUCCESS);

		framebufferInfo.renderPass = (VkRenderPass)m_pDeviceRenderPass;

		VkFramebuffer framebuffer;
		res = vkCreateFramebuffer(CDeviceManager::GetDevice(), &framebufferInfo, nullptr, &framebuffer);
		ASSERT(res == VK_SUCCESS);

		m_pFramebuffer.push_back(framebuffer);
	}
}



void CRenderPass::BeginRenderPass()
{
	VkRenderPassBeginInfo beginInfo{};
	beginInfo.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass	= (VkRenderPass)m_pDeviceRenderPass;

	if (m_pFramebuffer.size() > 1)
		beginInfo.framebuffer	= (VkFramebuffer)m_pFramebuffer[CDeviceManager::GetFrameIndex()];
	else
		beginInfo.framebuffer	= (VkFramebuffer)m_pFramebuffer[0];

	unsigned numWrittenResources = static_cast<unsigned>(m_nWritenResourceID.size());

	VkExtent2D extent{ CDeviceManager::GetDeviceWidth(), CDeviceManager::GetDeviceHeight()};

	for (unsigned i = 0; i < numWrittenResources; i++)
	{
		if (m_nWritenResourceID[i].m_eType == e_RenderTarget || m_nWritenResourceID[i].m_eType == e_DepthStencil_Write)
		{
			extent.width	= CTextureInterface::GetTextureWidth(m_nWritenResourceID[i].m_nResourceID, m_nWritenResourceID[i].m_nLevel);
			extent.height	= CTextureInterface::GetTextureHeight(m_nWritenResourceID[i].m_nResourceID, m_nWritenResourceID[i].m_nLevel);
			break;
		}
	}

	CDeviceManager::SetViewport(0, 0, extent.width, extent.height);

	beginInfo.renderArea.offset = { 0, 0 };
	beginInfo.renderArea.extent = extent;

	VkCommandBuffer cmd = (VkCommandBuffer)CCommandListManager::GetCurrentThreadCommandListPtr();

	vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

	m_bIsGraphicsRenderPassRunning = true;
}


void CRenderPass::EndRenderPass()
{
	VkCommandBuffer cmd = (VkCommandBuffer)CCommandListManager::GetCurrentThreadCommandListPtr();

	vkCmdEndRenderPass(cmd);

	m_bIsGraphicsRenderPassRunning = false;
}



void CFrameBlueprint::TransitionBarrier(unsigned int nResourceID, unsigned int eCurrentState, unsigned int eNextState, EBarrierFlags eFlags, CRenderPass::EResourceType eType)
{
	unsigned int nID = ms_pCurrentRenderPass->m_nSortedID;

	unsigned int nextLayout, currLayout;
	unsigned int nextStageFlags, currStageFlags;
	unsigned int nextAccessFlags, currAccessFlags;

	ConvertResourceState(eCurrentState, currLayout, currStageFlags, currAccessFlags);
	ConvertResourceState(eNextState, nextLayout, nextStageFlags, nextAccessFlags);

	SResourceBarrier barrier;
	barrier.m_eType			= e_Barrier_ResourceTransition;
	barrier.m_eFlags		= eFlags;
	barrier.m_eResourceType	= eType;
	barrier.m_ResourceTransition.m_nResourceID		= nResourceID;
	barrier.m_ResourceTransition.m_nLevel			= -1;
	barrier.m_ResourceTransition.m_nSlice			= -1;
	barrier.m_ResourceTransition.m_nCurrentState	= currLayout;
	barrier.m_ResourceTransition.m_nNextState		= nextLayout;
	barrier.m_ResourceTransition.m_nBeforeStage		= currStageFlags;
	barrier.m_ResourceTransition.m_nAfterStage		= nextStageFlags;
	barrier.m_ResourceTransition.m_nSrcAccess		= currAccessFlags;
	barrier.m_ResourceTransition.m_nDstAccess		= nextAccessFlags;

	ms_BarrierCache[nID].push_back(barrier);
}


void CFrameBlueprint::FlushBarriers(unsigned int nRenderPassID)
{
	UINT nID = nRenderPassID == INVALIDHANDLE ? static_cast<UINT>(ms_BarrierCache.size() - 1) : nRenderPassID;

	UINT nNumBarriers = static_cast<UINT>(ms_BarrierCache[nID].size());

	std::vector<VkImageMemoryBarrier>	pTexBarriers;
	std::vector<VkBufferMemoryBarrier>	pBuffBarrier;
	std::vector<VkMemoryBarrier>		pMemBarrier;

	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();

	for (UINT i = 0; i < nNumBarriers; i++)
	{
		VkImageMemoryBarrier	imBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		VkBufferMemoryBarrier	bufBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
		VkMemoryBarrier			memBarrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };

		switch (ms_BarrierCache[nID][i].m_eType)
		{
		case e_Barrier_ResourceTransition:
			if (ms_BarrierCache[nID][i].m_eResourceType == CRenderPass::e_Texture)
			{
				unsigned int texID = ms_BarrierCache[nID][i].m_ResourceTransition.m_nResourceID;
				ETextureFormat format = CTextureInterface::GetTextureFormat(texID);

				imBarrier.oldLayout = (VkImageLayout)ms_BarrierCache[nID][i].m_ResourceTransition.m_nCurrentState;
				imBarrier.newLayout = (VkImageLayout)ms_BarrierCache[nID][i].m_ResourceTransition.m_nNextState;
				imBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				if (format == e_R24_DEPTH || format == e_R32_DEPTH)
					imBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

				else if (format == e_R24_DEPTH_G8_STENCIL || format == e_R32_DEPTH_G8_STENCIL)
					imBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

				else
					imBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

				ETextureType eType = CTextureInterface::GetTextureType(texID);

				imBarrier.image = CTextureInterface::GetTexture(texID)->GetImage();

				if (ms_BarrierCache[nID][i].m_ResourceTransition.m_nLevel >= 0)
				{
					imBarrier.subresourceRange.baseMipLevel = ms_BarrierCache[nID][i].m_ResourceTransition.m_nLevel;
					imBarrier.subresourceRange.levelCount = 1;
				}
				else
				{
					imBarrier.subresourceRange.baseMipLevel = 0;
					imBarrier.subresourceRange.levelCount = CTextureInterface::GetTextureMipCount(texID);
				}

				if (ms_BarrierCache[nID][i].m_ResourceTransition.m_nSlice >= 0)
				{
					imBarrier.subresourceRange.baseArrayLayer = ms_BarrierCache[nID][i].m_ResourceTransition.m_nSlice;
					imBarrier.subresourceRange.layerCount = 1;
				}

				else
				{
					imBarrier.subresourceRange.baseArrayLayer = 0;
					imBarrier.subresourceRange.layerCount = CTextureInterface::GetTextureArraySize(texID);
				}

				imBarrier.subresourceRange.baseArrayLayer *= (eType == eCubeMap || eType == eCubeMapArray) ? 6 : 1;
				imBarrier.subresourceRange.layerCount *= (eType == eCubeMap || eType == eCubeMapArray) ? 6 : 1;

				imBarrier.srcAccessMask = ms_BarrierCache[nID][i].m_ResourceTransition.m_nSrcAccess;
				imBarrier.dstAccessMask = ms_BarrierCache[nID][i].m_ResourceTransition.m_nDstAccess;

				pTexBarriers.push_back(imBarrier);
			}

			else if (ms_BarrierCache[nID][i].m_eResourceType == CRenderPass::e_Buffer)
			{
				BufferId buffer = ms_BarrierCache[nID][i].m_ResourceTransition.m_nResourceID;

				bufBarrier.buffer = (VkBuffer)CResourceManager::GetBufferHandle(buffer);
				bufBarrier.offset = (VkDeviceSize)CResourceManager::GetBufferOffset(buffer);
				bufBarrier.size = (VkDeviceSize)CResourceManager::GetBufferSize(buffer);
				bufBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				bufBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				bufBarrier.srcAccessMask = ms_BarrierCache[nID][i].m_ResourceTransition.m_nSrcAccess;
				bufBarrier.dstAccessMask = ms_BarrierCache[nID][i].m_ResourceTransition.m_nDstAccess;

				pBuffBarrier.push_back(bufBarrier);
			}
			break;

		case e_Barrier_UAV:
			//if (ms_BarrierCache[nID][i].m_eResourceType == CRenderPass::e_Texture)
			//{
			//	unsigned int texID = ms_BarrierCache[nID][i].m_UAV.m_nResourceID;
			//	ETextureFormat format = CTextureInterface::GetTextureFormat(texID);
			//
			//	imBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			//	imBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			//	imBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			//	imBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			//
			//	if (format == e_R24_DEPTH || format == e_R32_DEPTH)
			//		imBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			//
			//	else if (format == e_R24_DEPTH_G8_STENCIL || format == e_R32_DEPTH_G8_STENCIL)
			//		imBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			//
			//	else
			//		imBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			//
			//	imBarrier.image = CTextureInterface::GetTexture(texID)->GetImage();
			//	imBarrier.subresourceRange.baseMipLevel = 0;
			//	imBarrier.subresourceRange.levelCount = CTextureInterface::GetTextureMipCount(texID);
			//	imBarrier.subresourceRange.baseArrayLayer = 0;
			//	imBarrier.subresourceRange.layerCount = CTextureInterface::GetTextureArraySize(texID);
			//
			//	imBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			//	imBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			//
			//	pTexBarriers.push_back(imBarrier);
			//}
			//
			//else if (ms_BarrierCache[nID][i].m_eResourceType == CRenderPass::e_Buffer)
			//{
			//	BufferId buffer = ms_BarrierCache[nID][i].m_UAV.m_nResourceID;
			//
			//	bufBarrier.buffer = (VkBuffer)CResourceManager::GetBufferHandle(buffer);
			//	bufBarrier.offset = (VkDeviceSize)CResourceManager::GetBufferOffset(buffer);
			//	bufBarrier.size = (VkDeviceSize)CResourceManager::GetBufferSize(buffer);
			//	bufBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			//	bufBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			//
			//	bufBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			//	bufBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			//
			//	pBuffBarrier.push_back(bufBarrier);
			//}
			//break;

		case e_Barrier_Memory:
			memBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
			memBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
			pMemBarrier.push_back(memBarrier);
		}		
	}

	if (nNumBarriers > 0)
	{
		VkCommandBuffer cmdBuffer = (VkCommandBuffer)CCommandListManager::GetCurrentThreadCommandListPtr();

		//bool bRestartRenderPass = pRenderPass && pRenderPass->m_bIsGraphicsRenderPassRunning;

		//if (bRestartRenderPass)
		//	pRenderPass->EndRenderPass();

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, static_cast<uint32_t>(pMemBarrier.size()), pMemBarrier.data(), static_cast<uint32_t>(pBuffBarrier.size()), pBuffBarrier.data(), static_cast<uint32_t>(pTexBarriers.size()), pTexBarriers.data());
	
		//if (bRestartRenderPass)
		//	pRenderPass->BeginRenderPass();
	}

	ms_BarrierCache[nID].clear();
}



struct SSubResource
{
	int m_nLevel;
	int m_nSlice;
	CRenderPass::EResourceAccessType m_nCurrentState;
	bool m_bResolved;
};



void CFrameBlueprint::GetTransitions(std::vector<SSubResourceTransition>& transitions, SResourceUsage& usage, unsigned int index)
{
	transitions.clear();

	if (usage.m_eType == CRenderPass::e_Texture)
	{
		int numLevels = CTextureInterface::GetTextureMipCount(usage.m_nResourceID);
		int numSlices = CTextureInterface::GetTextureArraySize(usage.m_nResourceID);

		int numSubresources = static_cast<int>(usage.m_nLevel[index].size());

		std::vector<SSubResource> subresources;

		std::vector<SSubResourceTransition> tmpTransitions;
		std::vector<SSubResourceTransition> reduceSlicesTransitions;
		std::vector<SSubResourceTransition> reduceLevelsTransitions;

		for (int i = 0; i < numSubresources; i++)
		{
			if (usage.m_nLevel[index][i] >= 0)
			{
				if (usage.m_nSlice[index][i] >= 0)
					subresources.push_back({ usage.m_nLevel[index][i], usage.m_nSlice[index][i], usage.m_nUsage[index][i], false });

				else
				{
					for (int j = 0; j < numSlices; j++)
						subresources.push_back({ usage.m_nLevel[index][i], j, usage.m_nUsage[index][i], false });
				}
			}

			else
			{
				for (int j = 0; j < numLevels; j++)
				{
					if (usage.m_nSlice[index][i] >= 0)
						subresources.push_back({ j, usage.m_nSlice[index][i], usage.m_nUsage[index][i], false });

					else
					{
						for (int k = 0; k < numSlices; k++)
							subresources.push_back({ j, k, usage.m_nUsage[index][i], false });
					}
				}
			}
		}

		numSubresources = static_cast<int>(subresources.size());

		for (int i = 0; i < numSubresources; i++)
		{
			int start = index > 0 ? index - 1 : static_cast<int>(usage.m_nLevel.size()) - 1;

			for (int j = start; j >= 0 && !subresources[i].m_bResolved; j--)
			{
				int n = static_cast<int>(usage.m_nLevel[j].size());

				for (int k = 0; k < n; k++)
				{
					if ((usage.m_nLevel[j][k] < 0 || usage.m_nLevel[j][k] == subresources[i].m_nLevel) && (usage.m_nSlice[j][k] < 0 || usage.m_nSlice[j][k] == subresources[i].m_nSlice))
					{
						if ((usage.m_nUsage[j][k] != subresources[i].m_nCurrentState) || (usage.m_nUsage[j][k] == CRenderPass::e_UnorderedAccess))
						{
							SSubResourceTransition transition;
							transition.m_nLevel			= subresources[i].m_nLevel;
							transition.m_nSlice			= subresources[i].m_nSlice;
							transition.m_nCurrentState	= usage.m_nUsage[j][k];
							transition.m_nNextState		= subresources[i].m_nCurrentState;
							transition.m_nRenderPassID	= usage.m_nRenderPassID[j];
							tmpTransitions.push_back(transition);
						}

						subresources[i].m_bResolved = true;
						break;
					}
				}
			}
		}

		/*bool reduceSlices = false;
		int level = subresources[0].m_nLevel;


		for (int i = 0; i < numSubresources; i++)
		{
			
		}*/
		transitions = tmpTransitions;
	}

	else
	{
		int start = index > 0 ? index - 1 : static_cast<int>(usage.m_nLevel.size()) - 1;

		if ((usage.m_nUsage[start][0] != usage.m_nUsage[index][0]) || (usage.m_nUsage[start][0] == CRenderPass::e_UnorderedAccess))
		{
			SSubResourceTransition transition;
			transition.m_nLevel = -1;
			transition.m_nSlice = -1;
			transition.m_nCurrentState = usage.m_nUsage[start][0];
			transition.m_nNextState = usage.m_nUsage[index][0];
			transition.m_nRenderPassID = usage.m_nRenderPassID[start];
			transitions.push_back(transition);
		}
	}
}



void CFrameBlueprint::BakeFrame()
{
	ms_EventList.clear();
	ms_ResourceUsage.clear();

	ms_BarrierCache.clear();
	ms_TransitionsToFirstState.clear();

	UINT numRenderPasses = static_cast<UINT>(CRenderPass::ms_pSortedRenderPasses.size());

	for (UINT i = 0; i < numRenderPasses; i++)
	{
		CRenderPass* pass = CRenderPass::ms_pSortedRenderPasses[i];

		UINT numResourceToRead = static_cast<UINT>(pass->m_nReadResourceID.size());
		UINT numResourceToWrite = static_cast<UINT>(pass->m_nWritenResourceID.size());

		for (UINT j = 0; j < numResourceToRead; j++)
		{
			UINT index = GetResourceIndex(pass->m_nReadResourceID[j].m_nResourceID, pass->m_nReadResourceID[j].m_eType);

			CRenderPass::EResourceAccessType accessType;

			if (pass->m_nReadResourceID[j].m_nShaderStages == CShader::EShaderType::e_FragmentShader)
				accessType = CRenderPass::e_PixelShaderResource;

			else if (pass->m_nReadResourceID[j].m_nShaderStages & CShader::EShaderType::e_FragmentShader)
				accessType = CRenderPass::e_ShaderResource;

			else
				accessType = CRenderPass::e_NonPixelShaderResource;

			if (ms_ResourceUsage[index].m_nRenderPassID.size() > 0 && ms_ResourceUsage[index].m_nRenderPassID.back() == i)
			{
				ms_ResourceUsage[index].m_nLevel.back().push_back(pass->m_nReadResourceID[j].m_nLevel);
				ms_ResourceUsage[index].m_nSlice.back().push_back(pass->m_nReadResourceID[j].m_nSlice);
				ms_ResourceUsage[index].m_nUsage.back().push_back(accessType);
			}

			else
			{
				ms_ResourceUsage[index].m_nRenderPassID.push_back(i);
				ms_ResourceUsage[index].m_nLevel.push_back(std::vector<int>());
				ms_ResourceUsage[index].m_nSlice.push_back(std::vector<int>());
				ms_ResourceUsage[index].m_nUsage.push_back(std::vector<CRenderPass::EResourceAccessType>());

				ms_ResourceUsage[index].m_nLevel.back().push_back(pass->m_nReadResourceID[j].m_nLevel);
				ms_ResourceUsage[index].m_nSlice.back().push_back(pass->m_nReadResourceID[j].m_nSlice);
				ms_ResourceUsage[index].m_nUsage.back().push_back(accessType);
			}			
		}

		for (UINT j = 0; j < numResourceToWrite; j++)
		{
			UINT index = GetResourceIndex(pass->m_nWritenResourceID[j].m_nResourceID, pass->m_nWritenResourceID[j].m_eResourceType);

			if (ms_ResourceUsage[index].m_nRenderPassID.size() > 0 && ms_ResourceUsage[index].m_nRenderPassID.back() == i)
			{
				ms_ResourceUsage[index].m_nLevel.back().push_back(pass->m_nWritenResourceID[j].m_nLevel);
				ms_ResourceUsage[index].m_nSlice.back().push_back(pass->m_nWritenResourceID[j].m_nSlice);
				ms_ResourceUsage[index].m_nUsage.back().push_back(pass->m_nWritenResourceID[j].m_eType);
			}

			else
			{
				ms_ResourceUsage[index].m_nRenderPassID.push_back(i);
				ms_ResourceUsage[index].m_nLevel.push_back(std::vector<int>());
				ms_ResourceUsage[index].m_nSlice.push_back(std::vector<int>());
				ms_ResourceUsage[index].m_nUsage.push_back(std::vector<CRenderPass::EResourceAccessType>());

				ms_ResourceUsage[index].m_nLevel.back().push_back(pass->m_nWritenResourceID[j].m_nLevel);
				ms_ResourceUsage[index].m_nSlice.back().push_back(pass->m_nWritenResourceID[j].m_nSlice);
				ms_ResourceUsage[index].m_nUsage.back().push_back(pass->m_nWritenResourceID[j].m_eType);
			}
		}
	}

	UINT numResources = static_cast<UINT>(ms_ResourceUsage.size());

	ms_EventList.resize(numRenderPasses);
	ms_BarrierCache.resize(numRenderPasses + 1);
	ms_BarrierCache.back().clear();

	for (UINT i = 0; i < numRenderPasses; i++)
	{
		ms_BarrierCache[i].clear();
		ms_EventList[i].clear();
	}

	std::vector<SResourceBarrier>	 pEndFrameEvents;

	for (UINT i = 0; i < numResources; i++)
	{
		UINT numStates = static_cast<UINT>(ms_ResourceUsage[i].m_nUsage.size());

		for (UINT j = 1; j < numStates; j++)
		{
			std::vector<SSubResourceTransition> transitions;

			GetTransitions(transitions, ms_ResourceUsage[i], j);

			int numTransitions = static_cast<int>(transitions.size());

			for(int k = 0; k < numTransitions; k++)
			{
				if (transitions[k].m_nCurrentState == CRenderPass::e_UnorderedAccess && transitions[k].m_nNextState == CRenderPass::e_UnorderedAccess)
				{
					SResourceBarrier barrier;

					barrier.m_nRenderPassID							= transitions[k].m_nRenderPassID;
					barrier.m_eResourceType							= ms_ResourceUsage[i].m_eType;
					barrier.m_bExecuteBeforeDrawCall				= false;
					barrier.m_eType									= e_Barrier_UAV;
					barrier.m_eFlags								= e_Immediate;
					barrier.m_UAV.m_nResourceID						= ms_ResourceUsage[i].m_nResourceID;
					barrier.m_UAV.m_nLevel							= transitions[k].m_nLevel;
					barrier.m_UAV.m_nSlice							= transitions[k].m_nSlice;
					ms_EventList[barrier.m_nRenderPassID].push_back(barrier);
				}

				else
				{
					SResourceBarrier barrier;

					unsigned int nextLayout, currLayout;
					unsigned int nextStageFlags, currStageFlags;
					unsigned int nextAccessFlags, currAccessFlags;

					ConvertResourceState(transitions[k].m_nCurrentState, currLayout, currStageFlags, currAccessFlags);
					ConvertResourceState(transitions[k].m_nNextState, nextLayout, nextStageFlags, nextAccessFlags);

					barrier.m_nRenderPassID							= transitions[k].m_nRenderPassID;
					barrier.m_bExecuteBeforeDrawCall				= false;
					barrier.m_eType									= e_Barrier_ResourceTransition;
					barrier.m_eResourceType							= ms_ResourceUsage[i].m_eType;
					barrier.m_eFlags								= e_Immediate;
					barrier.m_ResourceTransition.m_nResourceID		= ms_ResourceUsage[i].m_nResourceID;
					barrier.m_ResourceTransition.m_nCurrentState	= currLayout;
					barrier.m_ResourceTransition.m_nNextState		= nextLayout;
					barrier.m_ResourceTransition.m_nBeforeStage		= currStageFlags;
					barrier.m_ResourceTransition.m_nAfterStage		= nextStageFlags;
					barrier.m_ResourceTransition.m_nSrcAccess		= currAccessFlags;
					barrier.m_ResourceTransition.m_nDstAccess		= nextAccessFlags;
					barrier.m_ResourceTransition.m_nLevel			= transitions[k].m_nLevel;
					barrier.m_ResourceTransition.m_nSlice			= transitions[k].m_nSlice;
					ms_EventList[barrier.m_nRenderPassID].push_back(barrier);
				}
			}
		}

		if (numStates > 1)
		{
			std::vector<SSubResourceTransition> transitions;

			GetTransitions(transitions, ms_ResourceUsage[i], 0);

			int numTransitions = static_cast<int>(transitions.size());

			for (int k = 0; k < numTransitions; k++)
			{
				if (transitions[k].m_nCurrentState == CRenderPass::e_UnorderedAccess && transitions[k].m_nNextState == CRenderPass::e_UnorderedAccess)
				{
					SResourceBarrier barrier;

					barrier.m_nRenderPassID				= ms_ResourceUsage[i].m_nRenderPassID.back();
					barrier.m_eResourceType				= ms_ResourceUsage[i].m_eType;
					barrier.m_bExecuteBeforeDrawCall	= false;
					barrier.m_eType						= e_Barrier_UAV;
					barrier.m_eFlags					= e_Immediate;
					barrier.m_UAV.m_nResourceID			= ms_ResourceUsage[i].m_nResourceID;
					barrier.m_UAV.m_nLevel				= transitions[k].m_nLevel;
					barrier.m_UAV.m_nSlice				= transitions[k].m_nSlice;
					ms_EventList[barrier.m_nRenderPassID].push_back(barrier);
				}

				else
				{
					SResourceBarrier barrier;

					unsigned int nextLayout, currLayout;
					unsigned int nextStageFlags, currStageFlags;
					unsigned int nextAccessFlags, currAccessFlags;

					ConvertResourceState(transitions[k].m_nCurrentState, currLayout, currStageFlags, currAccessFlags);
					ConvertResourceState(transitions[k].m_nNextState, nextLayout, nextStageFlags, nextAccessFlags);

					barrier.m_nRenderPassID							= ms_ResourceUsage[i].m_nRenderPassID.back();
					barrier.m_bExecuteBeforeDrawCall				= false;
					barrier.m_eType									= e_Barrier_ResourceTransition;
					barrier.m_eResourceType							= ms_ResourceUsage[i].m_eType;
					barrier.m_eFlags								= e_Immediate;
					barrier.m_ResourceTransition.m_nResourceID		= ms_ResourceUsage[i].m_nResourceID;
					barrier.m_ResourceTransition.m_nCurrentState	= currLayout;
					barrier.m_ResourceTransition.m_nNextState		= nextLayout;
					barrier.m_ResourceTransition.m_nBeforeStage		= currStageFlags;
					barrier.m_ResourceTransition.m_nAfterStage		= nextStageFlags;
					barrier.m_ResourceTransition.m_nSrcAccess		= currAccessFlags;
					barrier.m_ResourceTransition.m_nDstAccess		= nextAccessFlags;
					barrier.m_ResourceTransition.m_nLevel			= transitions[k].m_nLevel;
					barrier.m_ResourceTransition.m_nSlice			= transitions[k].m_nSlice;
					ms_EventList[barrier.m_nRenderPassID].push_back(barrier);
				}
			}
		}

		if (ms_ResourceUsage[i].m_eType == CRenderPass::e_Texture)
		{
			int numSubResources = static_cast<int>(ms_ResourceUsage[i].m_nUsage[0].size());

			unsigned int nextLayout, currLayout;
			unsigned int nextStageFlags, currStageFlags;
			unsigned int nextAccessFlags, currAccessFlags;

			for (int k = 0; k < numSubResources; k++)
			{
				if (CTextureInterface::GetCurrentState(ms_ResourceUsage[i].m_nResourceID) != ms_ResourceUsage[i].m_nUsage[0][k])
				{
					ConvertResourceState(CTextureInterface::GetCurrentState(ms_ResourceUsage[i].m_nResourceID), currLayout, currStageFlags, currAccessFlags);
					ConvertResourceState(ms_ResourceUsage[i].m_nUsage[0][k], nextLayout, nextStageFlags, nextAccessFlags);

					SResourceBarrier barrier;
					barrier.m_eType									= e_Barrier_ResourceTransition;
					barrier.m_eFlags								= e_Immediate;
					barrier.m_eResourceType							= ms_ResourceUsage[i].m_eType;
					barrier.m_ResourceTransition.m_nResourceID		= ms_ResourceUsage[i].m_nResourceID;
					barrier.m_ResourceTransition.m_nLevel			= ms_ResourceUsage[i].m_nLevel[0][k];
					barrier.m_ResourceTransition.m_nSlice			= ms_ResourceUsage[i].m_nSlice[0][k];
					barrier.m_ResourceTransition.m_nCurrentState	= currLayout;
					barrier.m_ResourceTransition.m_nNextState		= nextLayout;
					barrier.m_ResourceTransition.m_nBeforeStage		= currStageFlags;
					barrier.m_ResourceTransition.m_nAfterStage		= nextStageFlags;
					barrier.m_ResourceTransition.m_nSrcAccess		= currAccessFlags;
					barrier.m_ResourceTransition.m_nDstAccess		= nextAccessFlags;
					ms_TransitionsToFirstState.push_back(barrier);
				}
			}

			CTextureInterface::SetCurrentState(ms_ResourceUsage[i].m_nResourceID, ms_ResourceUsage[i].m_nUsage[0][0]);
		}
	}

	ms_EventList.push_back(pEndFrameEvents);
}

