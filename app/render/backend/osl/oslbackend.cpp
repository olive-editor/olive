#include "oslbackend.h"

#include <QEventLoop>
#include <QThread>

#include "oslworker.h"

OSLBackend::OSLBackend(QObject *parent) :
  VideoRenderBackend(parent)
{
}

OSLBackend::~OSLBackend()
{
  Close();
}

bool OSLBackend::InitInternal()
{
  if (!VideoRenderBackend::InitInternal()) {
    return false;
  }

  // Initiate one thread per CPU core
  for (int i=0;i<threads().size();i++) {
    // Create one processor object for each thread
    OSLWorker* processor = new OSLWorker(frame_cache());
    processor->SetParameters(params());
    processors_.append(processor);
  }

  return true;
}

bool OSLBackend::CompileInternal()
{
  return true;
}

void OSLBackend::DecompileInternal()
{
}
