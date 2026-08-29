

__global__ void GainAdjustKernel(int p_Width, int p_Height, float p_GainR, float p_GainG, float p_GainB, float p_GainA, const float* p_Input, float* p_Output)
{
   const int x = blockIdx.x * blockDim.x + threadIdx.x;
   const int y = blockIdx.y * blockDim.y + threadIdx.y;

   if ((x < p_Width) && (y < p_Height))
   {
       const int index = ((y * p_Width) + x) * 4;

       p_Output[index + 0] = p_Input[index + 0] * p_GainR;
       p_Output[index + 1] = p_Input[index + 1] * p_GainG;
       p_Output[index + 2] = p_Input[index + 2] * p_GainB;
       p_Output[index + 3] = p_Input[index + 3] * p_GainA;
   }
}

void RunCudaKernel(void* p_Stream, int p_Width, int p_Height, float* p_Gain, const float* p_Input, float* p_Output)
{
    dim3 threads(128, 1, 1);
    dim3 blocks(((p_Width + threads.x - 1) / threads.x), p_Height, 1);
    cudaStream_t stream = static_cast<cudaStream_t>(p_Stream);

    GainAdjustKernel<<<blocks, threads, 0, stream>>>(p_Width, p_Height, p_Gain[0], p_Gain[1], p_Gain[2], p_Gain[3], p_Input, p_Output);
}
