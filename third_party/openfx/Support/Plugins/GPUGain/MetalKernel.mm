

#import <Metal/Metal.h>

#include <unordered_map>
#include <mutex>

const char* kernelSource =  \
"#include <metal_stdlib>\n" \
"using namespace metal; \n" \
"kernel void GainAdjustKernel(constant int& p_Width [[buffer (11)]], constant int& p_Height [[buffer (12)]], constant float& p_GainR [[buffer (13)]],    \n" \
"                             constant float& p_GainG [[buffer (14)]], constant float& p_GainB [[buffer (15)]], constant float& p_GainA [[buffer (16)]], \n" \
"                             const device float* p_Input [[buffer (0)]], device float* p_Output [[buffer (8)]], uint2 id [[ thread_position_in_grid ]]) \n" \
"{                                                                                                                                                       \n" \
"   if ((id.x < p_Width) && (id.y < p_Height))                                                                                                           \n" \
"   {                                                                                                                                                    \n" \
"       const int index = ((id.y * p_Width) + id.x) * 4;                                                                                                 \n" \
"       p_Output[index + 0] = p_Input[index + 0] * p_GainR;                                                                                              \n" \
"       p_Output[index + 1] = p_Input[index + 1] * p_GainG;                                                                                              \n" \
"       p_Output[index + 2] = p_Input[index + 2] * p_GainB;                                                                                              \n" \
"       p_Output[index + 3] = p_Input[index + 3] * p_GainA;                                                                                              \n" \
"   }                                                                                                                                                    \n" \
"}                                                                                                                                                       \n";

std::mutex s_PipelineQueueMutex;
typedef std::unordered_map<id<MTLCommandQueue>, id<MTLComputePipelineState>> PipelineQueueMap;
PipelineQueueMap s_PipelineQueueMap;

void RunMetalKernel(void* p_CmdQ, int p_Width, int p_Height, float* p_Gain, const float* p_Input, float* p_Output)
{
    const char* kernelName = "GainAdjustKernel";

    id<MTLCommandQueue>            queue = static_cast<id<MTLCommandQueue> >(p_CmdQ);
    id<MTLDevice>                  device = queue.device;
    id<MTLLibrary>                 metalLibrary;     // Metal library
    id<MTLFunction>                kernelFunction;   // Compute kernel
    id<MTLComputePipelineState>    pipelineState;    // Metal pipeline
    NSError* err;

    std::unique_lock<std::mutex> lock(s_PipelineQueueMutex);

    const auto it = s_PipelineQueueMap.find(queue);
    if (it == s_PipelineQueueMap.end())
    {
        id<MTLLibrary>                 metalLibrary;     // Metal library
        id<MTLFunction>                kernelFunction;   // Compute kernel
        NSError* err;

        MTLCompileOptions* options = [MTLCompileOptions new];
        options.fastMathEnabled = YES;
        if (!(metalLibrary    = [device newLibraryWithSource:@(kernelSource) options:options error:&err]))
        {
            fprintf(stderr, "Failed to load metal library, %s\n", err.localizedDescription.UTF8String);
            return;
        }
        [options release];
        if (!(kernelFunction  = [metalLibrary newFunctionWithName:[NSString stringWithUTF8String:kernelName]/* constantValues : constantValues */]))
        {
            fprintf(stderr, "Failed to retrieve kernel\n");
            [metalLibrary release];
            return;
        }
        if (!(pipelineState   = [device newComputePipelineStateWithFunction:kernelFunction error:&err]))
        {
            fprintf(stderr, "Unable to compile, %s\n", err.localizedDescription.UTF8String);
            [metalLibrary release];
            [kernelFunction release];
            return;
        }

        s_PipelineQueueMap[queue] = pipelineState;

        //Release resources
        [metalLibrary release];
        [kernelFunction release];
    }
    else
    {
        pipelineState = it->second;
    }

    id<MTLBuffer> srcDeviceBuf = reinterpret_cast<id<MTLBuffer> >(const_cast<float *>(p_Input));
    id<MTLBuffer> dstDeviceBuf = reinterpret_cast<id<MTLBuffer> >(p_Output);

    id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
    commandBuffer.label = [NSString stringWithFormat:@"GainAdjustKernel"];

    id<MTLComputeCommandEncoder> computeEncoder = [commandBuffer computeCommandEncoder];
    [computeEncoder setComputePipelineState:pipelineState];

    int exeWidth = [pipelineState threadExecutionWidth];
    MTLSize threadGroupCount = MTLSizeMake(exeWidth, 1, 1);
    MTLSize threadGroups     = MTLSizeMake((p_Width + exeWidth - 1)/exeWidth, p_Height, 1);

    [computeEncoder setBuffer:srcDeviceBuf offset: 0 atIndex: 0];
    [computeEncoder setBuffer:dstDeviceBuf offset: 0 atIndex: 8];
    [computeEncoder setBytes:&p_Width length:sizeof(int) atIndex:11];
    [computeEncoder setBytes:&p_Height length:sizeof(int) atIndex:12];
    [computeEncoder setBytes:&p_Gain[0] length:sizeof(float) atIndex:13];
    [computeEncoder setBytes:&p_Gain[1] length:sizeof(float) atIndex:14];
    [computeEncoder setBytes:&p_Gain[2] length:sizeof(float) atIndex:15];
    [computeEncoder setBytes:&p_Gain[3] length:sizeof(float) atIndex:16];

    [computeEncoder dispatchThreadgroups:threadGroups threadsPerThreadgroup: threadGroupCount];

    [computeEncoder endEncoding];
    [commandBuffer commit];
}
