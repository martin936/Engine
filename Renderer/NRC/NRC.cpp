#include "Engine/Renderer/Renderer.h"
#include "NRC.h"

#include "Engine/Renderer/GameRenderPass.h"


#define NUM_ADAM_STEPS	4


void CNRC::Init()
{
	unsigned width		= CDeviceManager::GetDeviceWidth();
	unsigned height		= CDeviceManager::GetDeviceWidth();

	const unsigned sampleBlockSize	= 128;
	const unsigned sampleWidth		= 64;

	unsigned numSamples = width * height;
	unsigned numBlocks	= (numSamples + sampleBlockSize - 1) / sampleBlockSize;

	numSamples = numBlocks * sampleBlockSize;

	ms_InputBuffer		= CResourceManager::CreateRwBuffer(numSamples * sampleWidth * sizeof(float));
	ms_OutputBuffer		= CResourceManager::CreateRwBuffer(numSamples * sampleWidth * sizeof(float));

	ms_Weights			= CResourceManager::CreateRwBuffer((64 * 64 * 5 + 64 * 3) * sizeof(float));

	if (CRenderPass::BeginCompute(ERenderPassId::e_NRC_Inference, "NRC Inference"))
	{
		CRenderPass::BindResourceToRead(0,	ms_InputBuffer,		CShader::e_ComputeShader,		CRenderPass::e_Buffer);
		CRenderPass::BindResourceToRead(1,	ms_Weights,			CShader::e_ComputeShader,		CRenderPass::e_Buffer);

		CRenderPass::BindResourceToWrite(2, ms_OutputBuffer,	CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

		CRenderPass::BindProgram("NRC_Inference");

		CRenderPass::SetEntryPoint(Inference);

		CRenderPass::End();
	}


	if (CRenderPass::BeginCompute(ERenderPassId::e_NRC_Training, "NRC Training"))
	{
		for (int step = 0; step < NUM_ADAM_STEPS; step++)
		{
			if (CRenderPass::BeginComputeSubPass("Compute Neuron Gradients"))
			{
				CRenderPass::BindResourceToRead(0,	ms_TrainingInput,				CShader::e_ComputeShader,		CRenderPass::e_Buffer);
				CRenderPass::BindResourceToRead(1,	ms_TrainingWeights,				CShader::e_ComputeShader,		CRenderPass::e_Buffer);
				CRenderPass::BindResourceToRead(2,	ms_TransposedTrainingWeights,	CShader::e_ComputeShader,		CRenderPass::e_Buffer);
				CRenderPass::BindResourceToRead(4,	ms_TrainingOutput,				CShader::e_ComputeShader,		CRenderPass::e_Buffer);

				CRenderPass::BindResourceToWrite(5, ms_TrainingLayerOutputs,		CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
				CRenderPass::BindResourceToWrite(6, ms_NeuronGradients_PerSample,	CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

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
		}

		if (CRenderPass::BeginComputeSubPass("Temporal Stabilization"))
		{


			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}


void CNRC::Inference()
{
	unsigned width		= CDeviceManager::GetDeviceWidth();
	unsigned height		= CDeviceManager::GetDeviceWidth();

	const unsigned sampleBlockSize	= 128;

	unsigned numSamples = width * height;
	unsigned numBlocks	= (numSamples + sampleBlockSize - 1) / sampleBlockSize;

	CDeviceManager::Dispatch(numBlocks, 1, 1);
}
