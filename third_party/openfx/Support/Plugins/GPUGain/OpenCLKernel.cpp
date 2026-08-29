

#ifdef _WIN64
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include <map>
#include <stdio.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

const char *KernelSourceBuffers = "\n" \
"__kernel void GainAdjustKernelBuffers(                                 \n" \
"   int p_Width,                                                        \n" \
"   int p_Height,                                                       \n" \
"   float p_GainR,                                                      \n" \
"   float p_GainG,                                                      \n" \
"   float p_GainB,                                                      \n" \
"   float p_GainA,                                                      \n" \
"   __global const float* p_Input,                                      \n" \
"   __global float* p_Output)                                           \n" \
"{                                                                      \n" \
"   const int x = get_global_id(0);                                     \n" \
"   const int y = get_global_id(1);                                     \n" \
"                                                                       \n" \
"   if ((x < p_Width) && (y < p_Height))                                \n" \
"   {                                                                   \n" \
"       const int index = ((y * p_Width) + x) * 4;                      \n" \
"                                                                       \n" \
"       p_Output[index + 0] = p_Input[index + 0] * p_GainR;             \n" \
"       p_Output[index + 1] = p_Input[index + 1] * p_GainG;             \n" \
"       p_Output[index + 2] = p_Input[index + 2] * p_GainB;             \n" \
"       p_Output[index + 3] = p_Input[index + 3] * p_GainA;             \n" \
"   }                                                                   \n" \
"}                                                                      \n" \
"\n";

const char *KernelSourceImages = "\n" \
"__constant sampler_t imageSampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;  \n" \
"                                                                                                           \n" \
"__kernel void GainAdjustKernelImages(                                                                      \n" \
"   int p_Width,                                                                                            \n" \
"   int p_Height,                                                                                           \n" \
"   float p_GainR,                                                                                          \n" \
"   float p_GainG,                                                                                          \n" \
"   float p_GainB,                                                                                          \n" \
"   float p_GainA,                                                                                          \n" \
"   __read_only  image2d_t p_Input,                                                                         \n" \
"   __write_only image2d_t p_Output)                                                                        \n" \
"{                                                                                                          \n" \
"   const int x = get_global_id(0);                                                                         \n" \
"   const int y = get_global_id(1);                                                                         \n" \
"                                                                                                           \n" \
"   if ((x < p_Width) && (y < p_Height))                                                                    \n" \
"   {                                                                                                       \n" \
"       int2 coord = (int2)(x, y);                                                                          \n" \
"       float4 out = read_imagef(p_Input, imageSampler, coord);                                             \n" \
"       out *= (float4)(p_GainR, p_GainG, p_GainB, p_GainA);                                                \n" \
"       write_imagef(p_Output, coord, out);                                                                 \n" \
"   }                                                                                                       \n" \
"}                                                                                                          \n" \
"\n";

void CheckError(cl_int p_Error, const char* p_Msg)
{
    if (p_Error != CL_SUCCESS)
    {
        fprintf(stderr, "%s [%d]\n", p_Msg, p_Error);
    }
}

class Locker
{
public:
    Locker()
    {
#ifdef _WIN64
        InitializeCriticalSection(&mutex);
#else
        pthread_mutex_init(&mutex, NULL);
#endif
    }

    ~Locker()
    {
#ifdef _WIN64
        DeleteCriticalSection(&mutex);
#else
        pthread_mutex_destroy(&mutex);
#endif
    }

    void Lock()
    {
#ifdef _WIN64
        EnterCriticalSection(&mutex);
#else
        pthread_mutex_lock(&mutex);
#endif
    }

    void Unlock()
    {
#ifdef _WIN64
        LeaveCriticalSection(&mutex);
#else
        pthread_mutex_unlock(&mutex);
#endif
    }

private:
#ifdef _WIN64
    CRITICAL_SECTION mutex;
#else
    pthread_mutex_t mutex;
#endif
};

void RunOpenCLKernelBuffers(void* p_CmdQ, int p_Width, int p_Height, float* p_Gain, const float* p_Input, float* p_Output)
{
    cl_int error;

    cl_command_queue cmdQ = static_cast<cl_command_queue>(p_CmdQ);

    // store device id and kernel per command queue (required for multi-GPU systems)
    static std::map<cl_command_queue, cl_device_id> deviceIdMap;
    static std::map<cl_command_queue, cl_kernel> kernelMap;

    static Locker locker; // simple lock to control access to the above maps from multiple threads

    locker.Lock();

    // find the device id corresponding to the command queue
    cl_device_id deviceId = NULL;
    if (deviceIdMap.find(cmdQ) == deviceIdMap.end())
    {
        error = clGetCommandQueueInfo(cmdQ, CL_QUEUE_DEVICE, sizeof(cl_device_id), &deviceId, NULL);
        CheckError(error, "Unable to get the device");

        deviceIdMap[cmdQ] = deviceId;
    }
    else
    {
        deviceId = deviceIdMap[cmdQ];
    }

    // find the program kernel corresponding to the command queue
    cl_kernel kernel = NULL;
    if (kernelMap.find(cmdQ) == kernelMap.end())
    {
        cl_context clContext = NULL;
        error = clGetCommandQueueInfo(cmdQ, CL_QUEUE_CONTEXT, sizeof(cl_context), &clContext, NULL);
        CheckError(error, "Unable to get the context");

        cl_program program = clCreateProgramWithSource(clContext, 1, (const char **)&KernelSourceBuffers, NULL, &error);
        CheckError(error, "Unable to create program");

        error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
        CheckError(error, "Unable to build program");

        kernel = clCreateKernel(program, "GainAdjustKernelBuffers", &error);
        CheckError(error, "Unable to create kernel");

        kernelMap[cmdQ] = kernel;
    }
    else
    {
        kernel = kernelMap[cmdQ];
    }

    locker.Unlock();

    int count = 0;
    error  = clSetKernelArg(kernel, count++, sizeof(int), &p_Width);
    error |= clSetKernelArg(kernel, count++, sizeof(int), &p_Height);
    error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Gain[0]);
    error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Gain[1]);
    error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Gain[2]);
    error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Gain[3]);
    error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &p_Input);
    error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &p_Output);
    CheckError(error, "Unable to set kernel arguments");

    size_t localWorkSize[2], globalWorkSize[2];
    clGetKernelWorkGroupInfo(kernel, deviceId, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), localWorkSize, NULL);
    localWorkSize[1] = 1;
    globalWorkSize[0] = ((p_Width + localWorkSize[0] - 1) / localWorkSize[0]) * localWorkSize[0];
    globalWorkSize[1] = p_Height;

    clEnqueueNDRangeKernel(cmdQ, kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
}

void RunOpenCLKernelImages(void* p_CmdQ, int p_Width, int p_Height, float* p_Gain, const float* p_Input, float* p_Output)
{
    cl_int error;

    cl_command_queue cmdQ = static_cast<cl_command_queue>(p_CmdQ);

    // store device id and kernel per command queue (required for multi-GPU systems)
    static std::map<cl_command_queue, cl_device_id> deviceIdMap;
    static std::map<cl_command_queue, cl_kernel> kernelMap;

    static Locker locker; // simple lock to control access to the above maps from multiple threads

    locker.Lock();

    // find the device id corresponding to the command queue
    cl_device_id deviceId = NULL;
    if (deviceIdMap.find(cmdQ) == deviceIdMap.end())
    {
        error = clGetCommandQueueInfo(cmdQ, CL_QUEUE_DEVICE, sizeof(cl_device_id), &deviceId, NULL);
        CheckError(error, "Unable to get the device");

        deviceIdMap[cmdQ] = deviceId;
    }
    else
    {
        deviceId = deviceIdMap[cmdQ];
    }

    // find the program kernel corresponding to the command queue
    cl_kernel kernel = NULL;
    if (kernelMap.find(cmdQ) == kernelMap.end())
    {
        cl_context clContext = NULL;
        error = clGetCommandQueueInfo(cmdQ, CL_QUEUE_CONTEXT, sizeof(cl_context), &clContext, NULL);
        CheckError(error, "Unable to get the context");

        cl_program program = clCreateProgramWithSource(clContext, 1, (const char **)&KernelSourceImages, NULL, &error);
        CheckError(error, "Unable to create program");

        error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
        CheckError(error, "Unable to build program");

        kernel = clCreateKernel(program, "GainAdjustKernelImages", &error);
        CheckError(error, "Unable to create kernel");

        kernelMap[cmdQ] = kernel;
    }
    else
    {
        kernel = kernelMap[cmdQ];
    }

    locker.Unlock();

    int count = 0;
    error = clSetKernelArg(kernel, count++, sizeof(int), &p_Width);
    error |= clSetKernelArg(kernel, count++, sizeof(int), &p_Height);
    error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Gain[0]);
    error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Gain[1]);
    error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Gain[2]);
    error |= clSetKernelArg(kernel, count++, sizeof(float), &p_Gain[3]);
    error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &p_Input);
    error |= clSetKernelArg(kernel, count++, sizeof(cl_mem), &p_Output);
    CheckError(error, "Unable to set kernel arguments");

    size_t localWorkSize[2], globalWorkSize[2];
    clGetKernelWorkGroupInfo(kernel, deviceId, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), localWorkSize, NULL);
    localWorkSize[1] = 1;
    globalWorkSize[0] = ((p_Width + localWorkSize[0] - 1) / localWorkSize[0]) * localWorkSize[0];
    globalWorkSize[1] = p_Height;

    clEnqueueNDRangeKernel(cmdQ, kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
}
