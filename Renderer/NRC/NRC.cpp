#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RTX/RTX_Utils.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "NRC.h"

#include "Engine/Renderer/GameRenderPass.h"
#include "Engine/Renderer/Skybox/Skybox.h"
#include "Engine/Renderer/PostFX/ToneMapping/ToneMapping.h"


#define NUM_ADAM_STEPS	1

#define NUM_TRAINING_SAMPLES_X	192
#define NUM_TRAINING_SAMPLES_Y	256

BufferId	CNRC::ms_InputBuffer;
BufferId	CNRC::ms_EncodedInputBuffer;
BufferId	CNRC::ms_OutputBuffer;
BufferId	CNRC::ms_Weights;

BufferId	CNRC::ms_TransposedTrainingWeights;
BufferId	CNRC::ms_TrainingWeights;

BufferId	CNRC::ms_SamplesInput;
BufferId	CNRC::ms_TrainingInput;
BufferId	CNRC::ms_TrainingOutput;
BufferId	CNRC::ms_TrainingLayerOutputs;

BufferId	CNRC::ms_NeuronGradients_PerSample;
BufferId	CNRC::ms_WeightGradients_PerBlock;
BufferId	CNRC::ms_WeightGradients;

BufferId	CNRC::ms_WeightFirstMoment;
BufferId	CNRC::ms_WeightSecondMoment;

unsigned	CNRC::ms_nTrainingEpoch = 0u;


void CNRC::Init()
{
	unsigned width					= CDeviceManager::GetDeviceWidth();
	unsigned height					= CDeviceManager::GetDeviceWidth();

	unsigned tileWidth				= (width + 15u) / 16u;
	unsigned tileHeight				= (height + 15u) / 16u;
	unsigned numSamples				= tileWidth * tileHeight * 256;

	unsigned numTrainingSamples		= NUM_TRAINING_SAMPLES_X * NUM_TRAINING_SAMPLES_Y;
	unsigned numTrainingBlocks		= numTrainingSamples / 128u;

	ms_InputBuffer					= CResourceManager::CreateRwBuffer(numSamples * 16 * sizeof(short));
	ms_EncodedInputBuffer			= CResourceManager::CreateRwBuffer(numSamples * 64 * sizeof(short));
	ms_OutputBuffer					= CResourceManager::CreateRwBuffer(numSamples * 4 * sizeof(short));

	ms_Weights						= CResourceManager::CreateRwBuffer((64 * 64 * 5 + 64 * 3) * sizeof(short));
	ms_TrainingWeights				= CResourceManager::CreateRwBuffer((64 * 64 * 5 + 64 * 3) * sizeof(short));
	ms_TransposedTrainingWeights	= CResourceManager::CreateRwBuffer((64 * 64 * 5 + 64 * 3) * sizeof(short));

	ms_SamplesInput					= CResourceManager::CreateRwBuffer(numTrainingSamples * 16 * sizeof(short));
	ms_TrainingInput				= CResourceManager::CreateRwBuffer(numTrainingSamples * 64 * sizeof(short));
	ms_TrainingOutput				= CResourceManager::CreateRwBuffer(numTrainingSamples * 4 * sizeof(short));
	ms_TrainingLayerOutputs			= CResourceManager::CreateRwBuffer(numTrainingSamples * 64  * 6 * sizeof(short));

	ms_NeuronGradients_PerSample	= CResourceManager::CreateRwBuffer(numTrainingSamples * 64  * 6 * sizeof(short));
	ms_WeightGradients_PerBlock		= CResourceManager::CreateRwBuffer(numTrainingBlocks * 64 * 64 * 6 * sizeof(short));
	ms_WeightGradients				= CResourceManager::CreateRwBuffer((64 * 64 * 5 + 64 * 3) * sizeof(short));

	ms_WeightFirstMoment			= CResourceManager::CreateRwBuffer((64 * 64 * 5 + 64 * 3) * sizeof(short));
	ms_WeightSecondMoment			= CResourceManager::CreateRwBuffer((64 * 64 * 5 + 64 * 3) * sizeof(short));


	if (CRenderPass::BeginCompute(ERenderPassId::e_NRC_GI, "NRC GI"))
	{
		if (CRenderPass::BeginComputeSubPass("NRC Prepare Inputs"))
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetAlbedoTarget(),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetNormalTarget(),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetInfoTarget(),				CShader::e_ComputeShader);

			CRenderPass::SetNumTextures(4, 1, CShader::e_ComputeShader);
			CRenderPass::SetNumTextures(5, 1, CShader::e_ComputeShader);
			CRenderPass::SetNumTextures(6, 1, CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(7, ms_EncodedInputBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("NRC_PrepareInputs");

			CRenderPass::SetEntryPoint(PrepareInputs);

			CRenderPass::EndSubPass();
		}

		if (CRenderPass::BeginComputeSubPass("NRC Inference"))
		{
			CRenderPass::BindResourceToRead(0,	ms_EncodedInputBuffer,	CShader::e_ComputeShader,		CRenderPass::e_Buffer);
			CRenderPass::BindResourceToRead(1,	ms_Weights,				CShader::e_ComputeShader,		CRenderPass::e_Buffer);

			CRenderPass::BindResourceToWrite(2, ms_OutputBuffer,		CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("NRC_Inference");

			CRenderPass::SetEntryPoint(Inference);

			CRenderPass::EndSubPass();
		}

		if (CRenderPass::BeginComputeSubPass("NRC Merge GI"))
		{
			CRenderPass::BindResourceToRead(0,	ms_OutputBuffer,						CShader::e_ComputeShader,		CRenderPass::e_Buffer);
			CRenderPass::BindResourceToRead(1,	CDeferredRenderer::GetNormalTarget(),	CShader::e_ComputeShader);

			CRenderPass::SetNumTextures(2, 1, CShader::e_ComputeShader);
			CRenderPass::SetNumTextures(3, 1, CShader::e_ComputeShader);
			CRenderPass::SetNumTextures(4, 1, CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(5,	CDeferredRenderer::GetDiffuseTarget(),	CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("NRC_Merge");

			CRenderPass::SetEntryPoint(Merge);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}


	if (CRenderPass::BeginCompute(ERenderPassId::e_NRC_Training, "NRC Training"))
	{
		if (CRenderPass::BeginComputeSubPass("NRC Reset"))
		{
			CRenderPass::BindResourceToWrite(0, ms_TrainingWeights, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(1, ms_TransposedTrainingWeights, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(2, ms_WeightFirstMoment, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(3, ms_WeightSecondMoment, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("NRC_Reset");

			CRenderPass::SetEntryPoint(Reset);

			CRenderPass::End();
		}

		if (CRenderPass::BeginRayTracingSubPass("NRC Path-Trace Samples"))
		{
			CRenderPass::SetRTAccelerationStructureSlot(0);
			CRenderPass::SetNumBuffers	(1, 1,		CShader::e_AnyHitShader | CShader::e_ClosestHitShader);
			CRenderPass::SetNumTextures	(3, 1024,	CShader::e_AnyHitShader | CShader::e_ClosestHitShader);
			CRenderPass::SetNumSamplers	(4, 1,		CShader::e_AnyHitShader | CShader::e_ClosestHitShader | CShader::e_MissShader);
			CRenderPass::SetNumTextures	(5, 1,		CShader::e_MissShader);
			CRenderPass::SetNumTextures	(12, 1,		CShader::e_RayGenShader);
			CRenderPass::SetNumTextures	(13, 1,		CShader::e_RayGenShader);
			CRenderPass::SetNumTextures	(14, 1,		CShader::e_RayGenShader);

			CRenderPass::BindResourceToRead(6, CDeferredRenderer::GetDepthTarget(),		CShader::e_RayGenShader);
			CRenderPass::BindResourceToRead(7, CDeferredRenderer::GetNormalTarget(),	CShader::e_RayGenShader);
			CRenderPass::BindResourceToRead(8, CDeferredRenderer::GetAlbedoTarget(),	CShader::e_RayGenShader);
			CRenderPass::BindResourceToRead(9, CDeferredRenderer::GetInfoTarget(),		CShader::e_RayGenShader);

			CRenderPass::BindResourceToWrite(10, ms_TrainingInput,	CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(11, ms_TrainingOutput, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::CreateHitGroup("NRC_TrainingSamples", nullptr, nullptr, nullptr, nullptr);
			CRenderPass::CreateHitGroup(nullptr, nullptr, nullptr, nullptr, "NRC_TrainingSamples");
			CRenderPass::CreateHitGroup(nullptr, nullptr, nullptr, nullptr, "RT_Shadows");
			CRenderPass::CreateHitGroup(nullptr, nullptr, "NRC_TrainingSamples", "NRC_TrainingSamples", nullptr);
			CRenderPass::CreateHitGroup(nullptr, nullptr, "RT_Shadows", "RT_Shadows", nullptr);

			CRenderPass::SetEntryPoint(PathTraceTrainingSamples);

			CRenderPass::EndSubPass();
		}

		/*for (int step = 0; step < NUM_ADAM_STEPS; step++)
		{
			if (CRenderPass::BeginComputeSubPass("Compute Neuron Gradients"))
			{
				CRenderPass::BindResourceToRead(0,	ms_TrainingInput,				CShader::e_ComputeShader,		CRenderPass::e_Buffer);
				CRenderPass::BindResourceToRead(1,	ms_TrainingWeights,				CShader::e_ComputeShader,		CRenderPass::e_Buffer);
				CRenderPass::BindResourceToRead(2,	ms_TransposedTrainingWeights,	CShader::e_ComputeShader,		CRenderPass::e_Buffer);
				CRenderPass::BindResourceToRead(3,	ms_TrainingOutput,				CShader::e_ComputeShader,		CRenderPass::e_Buffer);

				CRenderPass::BindResourceToWrite(4, ms_TrainingLayerOutputs,		CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
				CRenderPass::BindResourceToWrite(5, ms_NeuronGradients_PerSample,	CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

				CRenderPass::BindProgram("NRC_ComputeNeuronGradients");

				CRenderPass::SetEntryPoint(ComputeNeuronGradients);

				CRenderPass::EndSubPass();
			}

			if (CRenderPass::BeginComputeSubPass("Compute Weight Gradients"))
			{
				CRenderPass::BindResourceToRead(0,  ms_TrainingLayerOutputs,		CShader::e_ComputeShader,		CRenderPass::e_Buffer);
				CRenderPass::BindResourceToRead(1,  ms_NeuronGradients_PerSample,	CShader::e_ComputeShader,		CRenderPass::e_Buffer);
		
				CRenderPass::BindResourceToWrite(2, ms_WeightGradients_PerBlock,	CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

				CRenderPass::BindProgram("NRC_ComputeWeightGradients");

				CRenderPass::SetEntryPoint(ComputeWeightGradients);

				CRenderPass::EndSubPass();
			}

			if (CRenderPass::BeginComputeSubPass("Reduce Gradients"))
			{
				CRenderPass::BindResourceToRead(0,	ms_WeightGradients_PerBlock,	CShader::e_ComputeShader,		CRenderPass::e_Buffer);		
				CRenderPass::BindResourceToWrite(1, ms_WeightGradients,				CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

				CRenderPass::BindProgram("NRC_ReduceGradients");

				CRenderPass::SetEntryPoint(ReduceGradients);

				CRenderPass::EndSubPass();
			}

			if (CRenderPass::BeginComputeSubPass("ADAM step"))
			{
				CRenderPass::BindResourceToRead(0,	ms_WeightGradients,				CShader::e_ComputeShader,		CRenderPass::e_Buffer);
		
				CRenderPass::BindResourceToWrite(1, ms_WeightFirstMoment,			CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
				CRenderPass::BindResourceToWrite(2, ms_WeightSecondMoment,			CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
				CRenderPass::BindResourceToWrite(3, ms_TrainingWeights,				CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
				CRenderPass::BindResourceToWrite(4, ms_TransposedTrainingWeights,	CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

				CRenderPass::BindProgram("NRC_ADAM");

				CRenderPass::SetEntryPoint(ADAM);

				CRenderPass::EndSubPass();
			}
		}*/

		if (CRenderPass::BeginComputeSubPass("Temporal Stabilization"))
		{
			CRenderPass::BindResourceToRead(0,		ms_TrainingWeights,				CShader::e_ComputeShader,		CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(1,		ms_Weights,						CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("NRC_TemporalStabilization");

			CRenderPass::SetEntryPoint(TemporalStabilization);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}


void CNRC::Reset()
{
	if (ms_nTrainingEpoch > 0)
		return;

	ms_nTrainingEpoch = 1;

	unsigned seed = std::rand();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &seed, sizeof(seed));

	CDeviceManager::Dispatch(1, 16, 6);
}



void CNRC::PrepareInputs()
{
	struct
	{
		unsigned width;
		unsigned height;

		unsigned tileWidth;
		unsigned tileHeight;
		unsigned temporalSeed;
	} constants;

	constants.width			= CDeviceManager::GetDeviceWidth();
	constants.height		= CDeviceManager::GetDeviceWidth();

	constants.tileWidth		= (constants.width + 15u) / 16u;
	constants.tileHeight	= (constants.height + 15u) / 16u;

	static unsigned seed = 0;

	constants.temporalSeed	= seed++;

	CTextureInterface::SetTexture(CRenderer::ms_pSobolSequence32->GetID(),	4,	CShader::e_ComputeShader);
	CTextureInterface::SetTexture(CRenderer::ms_pOwenScrambling32->GetID(), 5,	CShader::e_ComputeShader);
	CTextureInterface::SetTexture(CRenderer::ms_pOwenRanking32->GetID(),	6,	CShader::e_ComputeShader);

	CRenderer::SetViewProjConstantBuffer(8);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(constants));

	CDeviceManager::Dispatch(constants.tileWidth, constants.tileHeight, 1);
}


void CNRC::EncodeInputs()
{
	unsigned width		= CDeviceManager::GetDeviceWidth();
	unsigned height		= CDeviceManager::GetDeviceWidth();

	unsigned tileWidth	= (width + 15u) / 16u;
	unsigned tileHeight	= (height + 15u) / 16u;
	unsigned numSamples	= tileWidth * tileHeight * 256;

	CDeviceManager::Dispatch(numSamples, 1, 1);
}


void CNRC::Inference()
{
	unsigned width		= CDeviceManager::GetDeviceWidth();
	unsigned height		= CDeviceManager::GetDeviceWidth();

	unsigned tileWidth	= (width + 15u) / 16u;
	unsigned tileHeight	= (height + 15u) / 16u;
	unsigned numSamples	= tileWidth * tileHeight * 256;

	const unsigned numSamplesPerBlock = 128u;

	//CResourceManager::SetPushConstant(CShader::e_ComputeShader, &numSamples, sizeof(numSamples));

	CDeviceManager::Dispatch((numSamples + numSamplesPerBlock - 1u) / numSamplesPerBlock, 1, 1);
}


void CNRC::Merge()
{
	struct
	{
		unsigned width;
		unsigned height;

		unsigned tileWidth;
		unsigned tileHeight;

		float	 Exposure;
		unsigned temporalSeed;
	} constants;

	constants.width			= CDeviceManager::GetDeviceWidth();
	constants.height		= CDeviceManager::GetDeviceHeight();

	constants.tileWidth		= (constants.width + 15u) / 16u;
	constants.tileHeight	= (constants.height + 15u) / 16u;

	constants.Exposure		= 1.f / CToneMapping::GetExposure();

	static unsigned seed = 0;

	constants.temporalSeed	= seed++;

	CTextureInterface::SetTexture(CRenderer::ms_pSobolSequence32->GetID(),	2, CShader::e_ComputeShader);
	CTextureInterface::SetTexture(CRenderer::ms_pOwenScrambling32->GetID(), 3, CShader::e_ComputeShader);
	CTextureInterface::SetTexture(CRenderer::ms_pOwenRanking32->GetID(),	4, CShader::e_ComputeShader);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(constants));

	CDeviceManager::Dispatch(constants.tileWidth, constants.tileHeight, 1);
}


void CNRC::PathTraceTrainingSamples()
{
	return;

	CResourceManager::SetAccelerationStructure();

	CRenderer::SetViewProjConstantBuffer(15);
	CRTX::SetObjectBuffers(1);
	CMaterial::BindMaterialBuffer(2);
	CMaterial::BindMaterialTextures(3);
	CResourceManager::SetSampler(4, e_MinMagMip_Linear_UVW_Clamp);

	CTextureInterface::SetTexture(CSkybox::GetSkyboxTexture(),				5, CShader::e_MissShader);
	CTextureInterface::SetTexture(CRenderer::ms_pSobolSequence32->GetID(),	12, CShader::e_RayGenShader);
	CTextureInterface::SetTexture(CRenderer::ms_pOwenScrambling32->GetID(), 13, CShader::e_RayGenShader);
	CTextureInterface::SetTexture(CRenderer::ms_pOwenRanking32->GetID(),	14, CShader::e_RayGenShader);

	struct
	{
		float4  SunColor;
		float4  SunDir;
		float4	screenSpaceRatio;
	} constants;

	if (CShadowDir::GetSunShadowRenderer())
	{
		CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();

		constants.SunColor = desc.m_Color * desc.m_fIntensity;
		constants.SunDir = desc.m_Dir;
	}

	else
		constants.SunColor = 0.f;

	constants.SunColor.w = CSkybox::GetSkyLightIntensity();

	static unsigned int temporalSeed = 0u;

	constants.SunDir.w = *reinterpret_cast<float*>(&temporalSeed);

	temporalSeed++;

	constants.screenSpaceRatio.x = (1.f * CDeviceManager::GetDeviceWidth()) / NUM_TRAINING_SAMPLES_X;
	constants.screenSpaceRatio.y = (1.f * CDeviceManager::GetDeviceHeight()) / NUM_TRAINING_SAMPLES_Y;

	CResourceManager::SetConstantBuffer(16, &constants, sizeof(constants));

	CDeviceManager::RayTrace(NUM_TRAINING_SAMPLES_X, NUM_TRAINING_SAMPLES_Y, 1);
}


void CNRC::EncodeTrainingInputs()
{
	unsigned numSamples = NUM_TRAINING_SAMPLES_X * NUM_TRAINING_SAMPLES_Y;

	CDeviceManager::Dispatch(numSamples, 1, 1);
}


void CNRC::ComputeNeuronGradients()
{
	const unsigned sampleBlockSize	= 128;

	unsigned numSamples = NUM_TRAINING_SAMPLES_X * NUM_TRAINING_SAMPLES_Y;
	unsigned numBlocks	= (numSamples + sampleBlockSize - 1) / sampleBlockSize;

	CDeviceManager::Dispatch(numBlocks, 1, 1);
}


void CNRC::ComputeWeightGradients()
{
	const unsigned sampleBlockSize	= 128;

	unsigned numSamples = NUM_TRAINING_SAMPLES_X * NUM_TRAINING_SAMPLES_Y;
	unsigned numBlocks	= (numSamples + sampleBlockSize - 1) / sampleBlockSize;

	CDeviceManager::Dispatch(numBlocks, 16, 6);
}


void CNRC::ReduceGradients()
{
	const unsigned sampleBlockSize	= 128;

	unsigned numSamples = NUM_TRAINING_SAMPLES_X * NUM_TRAINING_SAMPLES_X;
	unsigned numBlocks	= (numSamples + sampleBlockSize - 1) / sampleBlockSize;

	CDeviceManager::Dispatch((numBlocks + 127) / 128, 16, 6);
}


void CNRC::ADAM()
{
	ms_nTrainingEpoch++;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &ms_nTrainingEpoch, sizeof(ms_nTrainingEpoch));

	CDeviceManager::Dispatch(1, 16, 6);
}


void CNRC::TemporalStabilization()
{
	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &ms_nTrainingEpoch, sizeof(ms_nTrainingEpoch));

	CDeviceManager::Dispatch(1, 16, 6);
}

