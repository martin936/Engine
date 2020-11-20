#include "Engine/Engine.h"
#include "../RenderTarget.h"


CFramebuffer::SViewport CFramebuffer::ms_ActiveViewport = { 0, 0, 0, 0 };
unsigned int CFramebuffer::ms_nActiveFBO = INVALIDHANDLE;

CFramebuffer* CFramebuffer::ms_pCurrent = NULL;

int CFramebuffer::ms_nDrawBufferCount = 0;


CFramebuffer::CFramebuffer(void)
{
	glGenFramebuffers(1, &m_nId);

	m_nTargets = 0;

	for (int i = 0; i < 8; i++)
	{
		m_pTargets[i] = NULL;
		m_bCreated[i] = false;
	}

	m_pDepthStencil = NULL;
	m_pDepthStencilArray = NULL;
	m_nDepthStencilSlice = 0;
	m_bDSCreated = false;

	m_pBackupDepthStencil = NULL;
}


CFramebuffer::~CFramebuffer()
{
	if (m_pBackupDepthStencil != NULL)
	{
		glDeleteTextures(1, &(m_pBackupDepthStencil->m_nTextureId));
		delete m_pBackupDepthStencil;
	}

	if (m_pDepthStencil != NULL && m_bDSCreated)
	{
		glDeleteTextures(1, &(m_pDepthStencil->m_nTextureId));
		delete m_pDepthStencil;
	}

	for (int i = 0; i < 8; i++)
	{
		if (m_pTargets[i] != NULL && m_bCreated[i])
		{
			delete m_pTargets[i];
			m_pTargets[i] = NULL;
			m_bCreated[i] = false;
		}
	}

	glDeleteFramebuffers(1, &m_nId);
}



void CFramebuffer::Init()
{
	ms_pCurrent = new CFramebuffer;
}



void CFramebuffer::Terminate()
{
	delete ms_pCurrent;
	ms_pCurrent = NULL;
}


void CFramebuffer::InitFrame()
{
	ms_pCurrent->SetActive();
}


DepthStencil* CFramebuffer::CreateDepthStencil(int width, int height)
{

	DepthStencil* buffer = new DepthStencil;

	buffer->width = width;
	buffer->height = height;
	buffer->format = e_R24_DEPTH_G8_STENCIL;

	glGenTextures(1, &buffer->m_nTextureId);
	glBindTexture(GL_TEXTURE_2D, buffer->m_nTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);


	return buffer;
}


DepthStencilArray* CFramebuffer::CreateDepthStencilArray(int width, int height, int arraySize)
{

	DepthStencilArray* buffer = new DepthStencilArray;

	buffer->width = width;
	buffer->height = height;
	buffer->arraySize = arraySize;
	buffer->format = e_R24_DEPTH_G8_STENCIL;

	glGenTextures(1, &buffer->m_nTextureId);
	glBindTexture(GL_TEXTURE_2D_ARRAY, buffer->m_nTextureId);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH24_STENCIL8, width, height, arraySize);

	return buffer;
}


DepthStencil::~DepthStencil()
{
	glDeleteTextures(1, &m_nTextureId);
}


DepthStencilArray::~DepthStencilArray()
{
	glDeleteTextures(1, &m_nTextureId);
}


DepthStencil* CFramebuffer::CreateDepthStencil(int width, int height, ETextureFormat format, bool bShadowMap)
{

	DepthStencil* buffer = new DepthStencil;

	buffer->width = width;
	buffer->height = height;
	buffer->format = format;

	SetActive();

	glGenTextures(1, &buffer->m_nTextureId);
	glBindTexture(GL_TEXTURE_2D, buffer->m_nTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (bShadowMap)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, buffer->m_nTextureId, 0);

	m_pDepthStencil = buffer;
	m_bDSCreated = true;

	return buffer;
}


DepthStencil* CFramebuffer::CreateCubeMapDepthStencil(int width, int height, ETextureFormat format, bool bShadowMap)
{

	DepthStencil* buffer = new DepthStencil;

	buffer->width = width;
	buffer->height = height;
	buffer->format = format;

	glBindFramebuffer(GL_FRAMEBUFFER, m_nId);

	glGenTextures(1, &buffer->m_nTextureId);
	glBindTexture(GL_TEXTURE_CUBE_MAP, buffer->m_nTextureId);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (bShadowMap)
	{
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}

	for (int i = 0; i < 6; i++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, m_nId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, buffer->m_nTextureId, 0);

	m_pDepthStencil = buffer;
	m_bDSCreated = true;

	return buffer;
}


void CFramebuffer::BindCubeMapDepthStencil(DepthStencil* depthstencil, int nFace)
{

	//if (depthstencil != m_pDepthStencil)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + nFace, depthstencil->m_nTextureId, 0);

		/*if (m_bDSCreated)
		{
		m_pBackupDepthStencil = m_pDepthStencil;
		m_bDSCreated = false;
		}
		else if (depthstencil == m_pBackupDepthStencil)
		{
		m_pBackupDepthStencil = NULL;
		m_bDSCreated = true;
		}*/

		ms_pCurrent->m_pDepthStencil = depthstencil;
	}
}



void CFramebuffer::BindDepthStencil(DepthStencil* depthstencil)
{
	if (depthstencil != ms_pCurrent->m_pDepthStencil)
	{
		if (depthstencil != NULL)
		{
			SetViewport(0, 0, depthstencil->width, depthstencil->height);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthstencil->m_nTextureId, 0);
		}

		else
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);

		ms_pCurrent->m_pDepthStencil = depthstencil;
		ms_pCurrent->m_pDepthStencilArray = NULL;
	}

	else if (depthstencil == NULL && ms_pCurrent->m_pDepthStencilArray != NULL)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);
		ms_pCurrent->m_pDepthStencilArray = NULL;
	}
}


void CFramebuffer::BindDepthStencilArray(DepthStencilArray* depthstencil, int nSlice)
{
	if (depthstencil != ms_pCurrent->m_pDepthStencilArray || nSlice != ms_pCurrent->m_nDepthStencilSlice)
	{
		if (depthstencil != NULL)
		{
			SetViewport(0, 0, depthstencil->width, depthstencil->height);
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthstencil->m_nTextureId, 0, nSlice);
		}

		else
			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);

		ms_pCurrent->m_pDepthStencilArray = depthstencil;
		ms_pCurrent->m_nDepthStencilSlice = nSlice;
		ms_pCurrent->m_pDepthStencil = NULL;
	}
}


void CFramebuffer::SetBackbuffer()
{
	if (ms_nActiveFBO != 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		ms_nActiveFBO = 0;
	}
}


void CFramebuffer::RestoreDepthStencil()
{
	m_pDepthStencil = m_pBackupDepthStencil;
	m_bDSCreated = true;
	m_pBackupDepthStencil = NULL;

	SetActive();
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_pDepthStencil->m_nTextureId, 0);
}



void CFramebuffer::SetDrawBuffers(unsigned int nDrawBuffers)
{
	ms_nDrawBufferCount = nDrawBuffers;

	if (nDrawBuffers > 0)
	{
		GLenum* drawbuffers = new GLenum[nDrawBuffers];

		for (unsigned int i = 0; i < nDrawBuffers; i++)
			drawbuffers[i] = GL_COLOR_ATTACHMENT0 + i;

		glDrawBuffers(nDrawBuffers, drawbuffers);

		delete[] drawbuffers;
	}

	else
		glDrawBuffers(0, GL_NONE);
}


SRenderTarget* CFramebuffer::CreateRenderTarget(int width, int height, ETextureFormat format)
{

	SRenderTarget* RTarget = new SRenderTarget;

	RTarget->m_nWidth = width;
	RTarget->m_nHeight = height;
	RTarget->m_eFormat = format;

	RTarget->m_pTexture = new CTexture(width, height, format);

	RTarget->m_nTextureId = RTarget->m_pTexture->m_nID;

	return RTarget;
}


SRenderTarget::SRenderTarget()
{
	m_pTexture = NULL;
	m_nWidth = 0;
	m_nHeight = 0;
	m_nTextureId = 0;
	m_eFormat = e_R8G8B8A8;
}


SRenderTarget::SRenderTarget(CTexture* pTexture)
{
	m_pTexture = pTexture;
	m_nWidth = pTexture->m_nWidth;
	m_nHeight = pTexture->m_nWidth;
	m_nTextureId = pTexture->m_nID;
	m_eFormat = pTexture->m_eFormat;
}


SRenderTarget::SRenderTarget(int nWidth, int nHeight, ETextureFormat eFormat, CTexture::ETextureType eType, int nDepth, int nArrayDim, bool bGenMipMaps)
{
	m_pTexture = new CTexture(nWidth, nHeight, eFormat, eType, nDepth, nArrayDim, bGenMipMaps);
	m_nWidth = m_pTexture->m_nWidth;
	m_nHeight = m_pTexture->m_nHeight;
	m_nTextureId = m_pTexture->m_nID;
	m_eFormat = m_pTexture->m_eFormat;
}


SRenderTarget::~SRenderTarget()
{
	if (m_pTexture != NULL)
		delete m_pTexture;
}


SRenderTarget* CFramebuffer::CreateRenderTarget(int slot, int width, int height, ETextureFormat format)
{

	SRenderTarget* RTarget = new SRenderTarget;

	RTarget->m_nWidth = width;
	RTarget->m_nHeight = height;
	RTarget->m_eFormat = format;

	SetActive();

	RTarget->m_pTexture = new CTexture(width, height, format);

	RTarget->m_nTextureId = RTarget->m_pTexture->m_nID;

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, RTarget->m_nTextureId, 0);

	m_pTargets[slot] = RTarget;

	m_nTargets++;

	return RTarget;
}



void CFramebuffer::BindRenderTarget(unsigned int slot, SRenderTarget* target, int nLayer, int nLevel)
{
	if (slot < 8)
	{
		if (target != nullptr || nLevel > 0)
		{
			if (target->m_pTexture->GetType() == CTexture::eTextureArray)
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, target->m_nTextureId, nLevel, nLayer);

			else
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, target->m_nTextureId, nLevel);

			ms_pCurrent->m_pTargets[slot] = target;

			SetViewport(0, 0, max(1, target->m_nWidth >> nLevel), max(1, target->m_nHeight >> nLevel));

			if (slot >= ms_pCurrent->m_nTargets)
				ms_pCurrent->m_nTargets++;
		}
		
		else
		{
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, 0, 0);
			ms_pCurrent->m_pTargets[slot] = target;
		}
	}
}


void CFramebuffer::BindCubeMap(unsigned int slot, SRenderTarget* target, int nFace, int nLayer, int nLevel)
{
	if (slot < 8)
	{
		if (target != nullptr)
		{
			if (target->m_pTexture->GetType() == CTexture::eCubeMapArray)
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, target->m_nTextureId, nLevel, nLayer * 6 + nFace);

			else
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, GL_TEXTURE_CUBE_MAP_POSITIVE_X + nFace, target->m_nTextureId, nLevel);

			ms_pCurrent->m_pTargets[slot] = target;

			SetViewport(0, 0, target->m_nWidth, target->m_nHeight);

			if (slot >= ms_pCurrent->m_nTargets)
				ms_pCurrent->m_nTargets++;
		}

		else
		{
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, 0, 0);
			ms_pCurrent->m_pTargets[slot] = target;
		}
	}
}


void CFramebuffer::SetActive()
{
	if (ms_nActiveFBO != m_nId)
	{
		ms_nActiveFBO = m_nId;

		glBindFramebuffer(GL_FRAMEBUFFER, m_nId);

		if (m_pTargets[0] != NULL)
			SetViewport(0, 0, m_pTargets[0]->m_nWidth, m_pTargets[0]->m_nHeight);
		else if (m_pDepthStencil != NULL)
			SetViewport(0, 0, m_pDepthStencil->width, m_pDepthStencil->height);
	}
}


void CFramebuffer::SetViewport(int xstart, int ystart, int xend, int yend)
{
	if (xstart != ms_ActiveViewport.m_xstart || ystart != ms_ActiveViewport.m_ystart || xend != ms_ActiveViewport.m_xend || yend != ms_ActiveViewport.m_yend)
	{
		glViewport(xstart, ystart, xend, yend);

		ms_ActiveViewport.m_xstart = xstart;
		ms_ActiveViewport.m_ystart = ystart;
		ms_ActiveViewport.m_xend = xend;
		ms_ActiveViewport.m_yend = yend;
	}
}
