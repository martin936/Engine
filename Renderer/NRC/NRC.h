#ifndef __NRC_H__
#define __NRC_H__


class CNRC
{
public:

	static void Init();

private:

	static void Reset();

	static void PrepareInputs();
	static void EncodeInputs();
	static void Inference();
	static void Merge();

	static void PathTraceTrainingSamples();
	static void EncodeTrainingInputs();
	static void ComputeNeuronGradients();
	static void ComputeWeightGradients();
	static void ReduceGradients();
	static void ADAM();
	static void TemporalStabilization();

	static unsigned ms_nTrainingEpoch;

	static BufferId	ms_InputBuffer;
	static BufferId	ms_EncodedInputBuffer;
	static BufferId ms_Weights;
	static BufferId ms_OutputBuffer;

	static BufferId ms_TransposedTrainingWeights;
	static BufferId ms_TrainingWeights;

	static BufferId ms_SamplesInput;
	static BufferId ms_TrainingInput;
	static BufferId ms_TrainingOutput;
	static BufferId ms_TrainingLayerOutputs;

	static BufferId ms_NeuronGradients_PerSample;
	static BufferId ms_WeightGradients_PerBlock;
	static BufferId ms_WeightGradients;

	static BufferId ms_WeightFirstMoment;
	static BufferId ms_WeightSecondMoment;
};


#endif

