#include "oslbackend.h"

#include <OSL/oslcomp.h>
#include <QEventLoop>
#include <QThread>

#include "oslworker.h"

OSLBackend::OSLBackend(QObject *parent) :
  VideoRenderBackend(parent),
  shading_system_(nullptr),
  renderer_(nullptr)
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

  renderer_ = new OSL::SimpleRenderer();

  shading_system_ = new OSL::ShadingSystem(renderer_);

  // Initiate one thread per CPU core
  for (int i=0;i<threads().size();i++) {
    // Create one processor object for each thread
    OSLWorker* processor = new OSLWorker(frame_cache(),
                                         shading_system_,
                                         &shader_cache_,
                                         &color_cache_);
    processor->SetParameters(params());
    processors_.append(processor);
  }

  return true;
}

void OSLBackend::CloseInternal()
{
  VideoRenderBackend::CloseInternal();

  shader_cache_.Clear();

  delete shading_system_;
  shading_system_ = nullptr;

  delete renderer_;
  renderer_= nullptr;
}

bool OSLBackend::CompileInternal()
{
  OSL::ShadingSystem* system = shading_system_;

  OSL::OSLCompiler compiler;

  QList<Node*> deps = viewer_node()->GetDependencies();
  foreach (Node* n, deps) {
    if (!shader_cache_.Has(n->id())) {
      if (n->IsAccelerated()) {

        std::string oso_buffer;

        QString function_name = n->id().replace('.', '_');

        if (!compiler.compile_buffer(n->AcceleratedCodeFragment().toStdString(),
                                     oso_buffer,
                                     std::vector<std::string>(),
                                     "",
                                     function_name.toStdString())) {
          qWarning() << "Failed to compile" << n->id();
          return false;
        }

        system->LoadMemoryCompiledShader(function_name.toStdString(), oso_buffer);

        OSL::ShaderGroupRef ref = system->ShaderGroupBegin(n->id().toStdString());
        system->Shader("shader", function_name.toStdString(), "layer1");
        system->ShaderGroupEnd();

        shader_cache_.Add(n->id(), ref);

      } else {
        // Insert null
        shader_cache_.Add(n->id(), nullptr);
      }
    }
  }

  return true;
}

void OSLBackend::DecompileInternal()
{
}
