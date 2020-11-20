#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Particles/Particles.h"
#include "Engine/Physics/Particles/Particles.h"


ProgramHandle CParticleRenderer::ms_nProgramID = 0;
ProgramHandle CParticleRenderer::ms_nBlurPID = 0;
ProgramHandle CParticleRenderer::ms_nMergePID = 0;

CParticleRenderer::SBufferIDs CParticleRenderer::ms_nBufferIDs[MAX_PARTICLE_SYSTEMS];

unsigned int CParticleRenderer::ms_nParticleSystemCount = 0;
unsigned int CParticleRenderer::ms_nConstantBufferID = 0;
unsigned int CParticleRenderer::ms_nBlurConstantBufferID = 0;
unsigned int CParticleRenderer::ms_nCommandListID = 0;

SRenderTarget* CParticleRenderer::ms_pNormalBlurTarget = NULL;
SRenderTarget* CParticleRenderer::ms_pNormalTarget = NULL;
SRenderTarget* CParticleRenderer::ms_pDepthBlurTarget = NULL;
SRenderTarget* CParticleRenderer::ms_pDepthTarget = NULL;

//CFramebuffer* CParticleRenderer::ms_pFramebuffer = NULL;




void CParticleRenderer::Init()
{
	/*int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_nProgramID = CShader::LoadProgram(SHADER_PATH("Particles"), "Particles", "Particles");
	ms_nBlurPID = CShader::LoadProgram(SHADER_PATH("Particles"), "Blur", "Blur");
	ms_nMergePID = CShader::LoadProgram(SHADER_PATH("Particles"), "Merge", "Merge");
	//ms_nBlurDepthPID = CShader::LoadProgram(SHADER_PATH("Particles"), "Blur", "BlurDepth");

	ms_nParticleSystemCount = 0;

	ms_nConstantBufferID = CDeviceManager::CreateConstantBuffer(NULL, 16 * sizeof(float));

	ms_nBlurConstantBufferID = CDeviceManager::CreateConstantBuffer(NULL, 4 * sizeof(float));

	ms_nCommandListID = CDeviceManager::CreateCommandList();

	ms_pNormalBlurTarget	= CFramebuffer::CreateRenderTarget(nWidth, nHeight, e_R8G8B8A8);
	ms_pNormalTarget		= CFramebuffer::CreateRenderTarget(nWidth, nHeight, e_R8G8B8A8);
	ms_pDepthBlurTarget		= CFramebuffer::CreateRenderTarget(nWidth, nHeight, e_R32_FLOAT);
	ms_pDepthTarget			= CFramebuffer::CreateRenderTarget(nWidth, nHeight, e_R32_FLOAT);

	ms_pFramebuffer = new CFramebuffer;
	ms_pFramebuffer->SetDrawBuffers(2);*/
}


void CParticleRenderer::Terminate()
{
	CShader::DeleteProgram(ms_nProgramID);

	//delete ms_pNormalBlurTarget;
	//delete ms_pNormalTarget;
	//delete ms_pDepthBlurTarget;
	//delete ms_pDepthTarget;
}

/*
void CParticleRenderer::Process(SRenderTarget* pTarget, DepthStencil* pDepthStencil)
{
	ms_pFramebuffer->SetActive();

	CShader::BindProgram(ms_nProgramID);

	float fViewProj[16];

	float4x4 mViewProj = CRenderer::GetCurrentCamera()->GetViewProjMatrix();
	mViewProj.transpose();

	memcpy(fViewProj, &mViewProj.m00, 16 * sizeof(float));

	CDeviceManager::FillConstantBuffer(ms_nConstantBufferID, fViewProj, 16 * sizeof(float));
	CDeviceManager::BindConstantBuffer(ms_nConstantBufferID, ms_nProgramID, 0);

	ms_pFramebuffer->BindRenderTarget(0, ms_pNormalTarget);
	ms_pFramebuffer->BindRenderTarget(1, ms_pDepthTarget);
	ms_pFramebuffer->BindDepthStencil(CRenderer::GetDepthStencil());
	CRenderer::ClearColor(1.f, 1.f, 1.f, 0.f);
	CRenderer::ClearColorBits();
	CRenderer::ClearColor(0.f, 0.f, 0.f, 0.f);

	CRenderStates::SetBlendingState(e_Opaque_AdditiveAlpha);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetRasterizerState(e_Solid);
	CRenderStates::SetDepthStencil(e_LessEqual, false);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);

	CDeviceManager::BindCommandList(ms_nCommandListID);

	for (unsigned int i = 0; i < ms_nParticleSystemCount; i++)
	{
		if (ms_nBufferIDs[i].m_nParticleCount > 0)
		{
			CDeviceManager::BindVertexBuffer(ms_nBufferIDs[i].m_nVertexBufferID, 0, 4, CStream::e_Float, sizeof(CParticles::SParticleVertex));

			CDeviceManager::Draw(e_PointList, 0, ms_nBufferIDs[i].m_nParticleCount);
		}
	}

	CDeviceManager::BindCommandList(0);

	CShader::BindProgram(ms_nBlurPID);

	float	registers[4];
	
	registers[0] = 1.f;
	registers[1] = 0.f;
	registers[2] = 1.f / ms_pNormalBlurTarget->m_nWidth;
	registers[3] = 1.f / ms_pNormalBlurTarget->m_nHeight;

	CDeviceManager::FillConstantBuffer(ms_nBlurConstantBufferID, registers, 4 * sizeof(float));
	CDeviceManager::BindConstantBuffer(ms_nBlurConstantBufferID, ms_nBlurPID, 0);

	ms_pFramebuffer->BindRenderTarget(0, ms_pNormalBlurTarget);
	ms_pFramebuffer->BindRenderTarget(1, ms_pDepthBlurTarget);

	CTexture::SetTexture(ms_nBlurPID, ms_pNormalTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CTexture::SetTexture(ms_nBlurPID, ms_pDepthTarget->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetBlendingState(e_Opaque);

	//glEnable(GL_STENCIL_TEST);
	//glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_EQUAL, 1, 0xff);
	//glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);

	CRenderer::RenderQuadScreen();

	registers[0] = 0.f;
	registers[1] = 1.f;

	CDeviceManager::FillConstantBuffer(ms_nBlurConstantBufferID, registers, 4 * sizeof(float));
	CDeviceManager::BindConstantBuffer(ms_nBlurConstantBufferID, ms_nBlurPID, 0);
	CRenderStates::SetDepthStencil(e_Zero, false);

	ms_pFramebuffer->BindRenderTarget(0, ms_pNormalTarget);
	ms_pFramebuffer->BindRenderTarget(1, ms_pDepthTarget);

	CTexture::SetTexture(ms_nBlurPID, ms_pNormalBlurTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CTexture::SetTexture(ms_nBlurPID, ms_pDepthBlurTarget->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CRenderer::RenderQuadScreen();

	//glDisable(GL_STENCIL_TEST);

	CShader::BindProgram(ms_nMergePID);

	CRenderStates::SetBlendingState(e_AlphaBlend);

	//CRenderer::GetInstance()->GetDeferredFramebuffer()->SetActive();

	CTexture::SetTexture(ms_nMergePID, ms_pNormalTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CTexture::SetTexture(ms_nMergePID, ms_pDepthTarget->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CRenderStates::SetWriteMask(e_Depth | e_Red | e_Green | e_Blue | e_Alpha);
	CRenderStates::SetDepthStencil(e_LessEqual, false);

	CRenderer::RenderQuadScreen();

	ms_nParticleSystemCount = 0;
}
*/

void CParticleRenderer::AddSystem(unsigned int nVertexBufferID, unsigned int nParticleCount)
{
	ms_nBufferIDs[ms_nParticleSystemCount].m_nVertexBufferID = nVertexBufferID;
	ms_nBufferIDs[ms_nParticleSystemCount].m_nParticleCount = nParticleCount;

	ms_nParticleSystemCount++;
}
