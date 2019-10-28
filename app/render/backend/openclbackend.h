#ifndef OPENCLBACKEND_H
#define OPENCLBACKEND_H

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "renderbackend.h"

class OpenCLBackend : public RenderBackend
{
public:
  OpenCLBackend();

  virtual bool Init() override;

  virtual void GenerateFrame(const rational& time) override;

  virtual void Close() override;

  void Compile();

protected:
  virtual void Decompile() override;

private:
  void GenerateCode();

  cl_context context_;

  cl_program program_;

  cl_command_queue command_queue_;

  cl_device_id device_id_;
};

#endif // OPENCLBACKEND_H
