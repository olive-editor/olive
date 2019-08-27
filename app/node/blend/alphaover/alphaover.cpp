#include "alphaover.h"

#include "render/rendertexture.h"

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

QVariant AlphaOverBlend::Value(NodeOutput *param, const rational &time)
{
  Q_UNUSED(param)
  Q_UNUSED(time)

  return 0;
}
