#include "alphaover.h"

#include "render/rendertypes.h"

AlphaOverBlend::AlphaOverBlend()
{

}

QString AlphaOverBlend::Name()
{
  return tr("Alpha Over");
}

QString AlphaOverBlend::id()
{
  return "org.olivevideoeditor.Olive.alphaoverblend";
}

QString AlphaOverBlend::Description()
{
  return tr("A blending node that composites one texture over another using its alpha channel.");
}

void AlphaOverBlend::Process(const rational &time)
{
  Q_UNUSED(time)

  // FIXME: Write Alpha Over Formula

  // Note that alpha will always be premultiplied by this point



  //GLuint base_tex = base_input_->get_value(time).value<GLuint>();

  // FIXME: Does nothing
  texture_output()->set_value(blend_input()->get_value(time).value<RenderTexture>());
}
