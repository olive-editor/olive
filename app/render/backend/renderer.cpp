/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "renderer.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include <QFloat16>

#include "render/colormanager.h"

OLIVE_NAMESPACE_ENTER

Renderer::Renderer(QObject *parent) :
  QObject(parent)
{

}

Renderer::TexturePtr Renderer::CreateTexture(const VideoParams &param, void *data, int linesize)
{
  QVariant v = CreateNativeTexture(param, data, linesize);

  if (v.isNull()) {
    return nullptr;
  }

  return std::make_shared<Texture>(this, v, param);
}

// copied from source code to OCIODisplay
/*const int OCIO_LUT3D_EDGE_SIZE = 64;

const int OCIO_LUT3D_PIXEL_COUNT = OCIO_LUT3D_EDGE_SIZE*OCIO_LUT3D_EDGE_SIZE*OCIO_LUT3D_EDGE_SIZE;
const int OCIO_LUT3D_ENTRY_COUNT = 3 * OCIO_LUT3D_PIXEL_COUNT;
const int OCIO_LUT3D_ENTRY_COUNT_WITH_ALPHA = 4 * OCIO_LUT3D_PIXEL_COUNT;
const int OCIO_LUT2D_EDGE_SIZE = 512;*/

void Renderer::BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, Texture *destination)
{
  /*ColorContext color_ctx;

  if (color_cache_.contains(color_processor->id())) {
    color_ctx = color_cache_.value(color_processor->id());
  } else {
    // Generate OCIO color context

    // Generate OCIO shader descriptor
    const char* ocio_func_name = "OCIODisplay";
    OCIO::GpuShaderDesc shader_desc;
    shader_desc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_0);
    shader_desc.setFunctionName(ocio_func_name);
    shader_desc.setLut3DEdgeLen(OCIO_LUT3D_EDGE_SIZE);

    // Generate LUT
    QVector<float> lut_data(OCIO_LUT3D_ENTRY_COUNT);
    color_processor->GetProcessor()->getGpuLut3D(lut_data.data(), shader_desc);

    // Convert to half float RGBA
    QVector<qfloat16> texture_ready_lut_data(OCIO_LUT3D_ENTRY_COUNT_WITH_ALPHA);
    for (int i=0; i<OCIO_LUT3D_PIXEL_COUNT; i++) {
      texture_ready_lut_data[i*kRGBAChannels+0] = lut_data[i*kRGBChannels+0];
      texture_ready_lut_data[i*kRGBAChannels+1] = lut_data[i*kRGBChannels+1];
      texture_ready_lut_data[i*kRGBAChannels+2] = lut_data[i*kRGBChannels+2];
      texture_ready_lut_data[i*kRGBAChannels+3] = 1.0f;
    }

    // Create LUT texture
    color_ctx.lut = CreateTexture(VideoParams(OCIO_LUT2D_EDGE_SIZE, OCIO_LUT2D_EDGE_SIZE, PixelFormat::PIX_FMT_RGBA32F),
                                  texture_ready_lut_data.data());

    // Create shader
    QString frag_code;

    frag_code.append(QStringLiteral("sampler2D texture;\n"
                                    "sampler2D lut;\n"
                                    "\n"
                                    "vec3 LUTLookup(sampler2D lut3d, vec3 in_coord)\n"
                                    "{\n"
                                    "  return texture2D(lut3d, vec2());\n"
                                    "}\n"
                                    "\n"));

    // Correct code for our GLSL set up
    QString ocio_code = color_processor->GetProcessor()->getGpuShaderText(shader_desc);
    ocio_code.replace(QStringLiteral("texture3D"), QStringLiteral("texture2D"));
    ocio_code.replace(QStringLiteral("sampler3D"), QStringLiteral("sampler2D"));



    qDebug() << frag_code;

    //qDebug() << "FIXME: GPU doesn't handle associated alpha yet";
  }*/

  qDebug() << "BlitColorManaged is a partial stub";

  QVariant shader = CreateNativeShader(ShaderCode(FileFunctions::ReadFileAsString(":/shaders/default.frag"), FileFunctions::ReadFileAsString(":/shaders/default.vert")));

  ShaderJob job;
  job.InsertValue(QStringLiteral("ove_maintex"), ShaderValue(QVariant::fromValue(source), NodeParam::kTexture));

  BlitToTexture(shader, job, destination);
}

void Renderer::BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, VideoParams params)
{
  qDebug() << "BlitColorManaged is a partial stub";

  QVariant shader = CreateNativeShader(ShaderCode(FileFunctions::ReadFileAsString(":/shaders/default.frag"), FileFunctions::ReadFileAsString(":/shaders/default.vert")));

  ShaderJob job;
  job.InsertValue(QStringLiteral("ove_maintex"), ShaderValue(QVariant::fromValue(source), NodeParam::kTexture));

  Blit(shader, job, params);
}

OLIVE_NAMESPACE_EXIT
