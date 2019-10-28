#include "openclbackend.h"

OpenCLBackend::OpenCLBackend()
{

}

bool OpenCLBackend::Init()
{
  // Get platform and device information
  cl_platform_id platform_id = nullptr;
  cl_uint ret_num_devices;
  cl_uint ret_num_platforms;
  cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
  ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_GPU, 1,
                        &device_id_, &ret_num_devices);

  if (ret != CL_SUCCESS) {
    qWarning() << "Failed to find compatible OpenCL device";
    return false;
  }

  // Create an OpenCL context
  context_ = clCreateContext(nullptr, 1, &device_id_, nullptr, nullptr, &ret);

  // Create a command queue
  //command_queue_ = clCreateCommandQueue(context_, device_id_, 0, &ret);

  return true;
}

void OpenCLBackend::GenerateFrame(const rational &time)
{
  Q_UNUSED(time)
  /*
  // Copy the lists A and B to their respective memory buffers
  cl_int ret = clEnqueueWriteBuffer(command_queue_, a_mem_obj, CL_TRUE, 0,
                             BITMAP_SZ, source_bmp, 0, NULL, NULL);

  // Set the arguments of the kernel
  ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
  ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&c_mem_obj);

  // Execute the OpenCL kernel on the list
  size_t global_item_size = BITMAP_SZ; // Process the entire lists
  size_t local_item_size = 30; // Divide work items into groups of 64
  ret = clEnqueueNDRangeKernel(command_queue_, kernel, 1, NULL,
                               &global_item_size, &local_item_size, 0, NULL, NULL);

  if (ret != CL_SUCCESS) {
    fprintf(stderr, "Failed to run\n");
    exit(1);
  }

  // Read the memory buffer C on the device to the local variable C
  ret = clEnqueueReadBuffer(command_queue_, c_mem_obj, CL_TRUE, 0,
                            BITMAP_SZ, dest_bmp, 0, NULL, NULL);

  // Clean up
  ret = clFlush(command_queue_);
  ret = clFinish(command_queue_);
  */
}

void OpenCLBackend::Close()
{
  Decompile();

  cl_int ret;

  /*ret = clReleaseKernel(kernel);
  ret = clReleaseMemObject(a_mem_obj);
  ret = clReleaseMemObject(c_mem_obj);*/
  ret = clReleaseCommandQueue(command_queue_);
  ret = clReleaseContext(context_);
}

void OpenCLBackend::Decompile()
{
  clReleaseProgram(program_);
}

void OpenCLBackend::Compile()
{
  /*
  cl_int ret;

  // Create memory buffers on the device for each vector
  cl_mem a_mem_obj = clCreateBuffer(context_, CL_MEM_READ_ONLY, BITMAP_SZ, nullptr, &ret);
  cl_mem c_mem_obj = clCreateBuffer(context_, CL_MEM_WRITE_ONLY, BITMAP_SZ, nullptr, &ret);

  // Create a program from the kernel source
  program_ = clCreateProgramWithSource(context_,
                                       1,
                                       (const char **)&source_str,
                                       (const size_t *)&source_size,
                                       &ret);

  // Build the program
  ret = clBuildProgram(program_, 1, &device_id_, nullptr, nullptr, nullptr);

  if (ret != CL_SUCCESS) {
    // Decompile failed, the user will probably want to know why
    size_t error_len = 0;

    clGetProgramBuildInfo(program_, device_id_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &error_len);
    char* err = new char[error_len];
    clGetProgramBuildInfo(program_, device_id_, CL_PROGRAM_BUILD_LOG, error_len, err, nullptr);

    SetError(err);
    //fprintf(stderr, "%s\n", err);

    delete [] err;
  }

  // Create the OpenCL kernel
  cl_kernel kernel = clCreateKernel(program_, "vector_add", &ret);
  */
}

void OpenCLBackend::GenerateCode()
{
  if (viewer_node() == nullptr) {
    return;
  }


}
