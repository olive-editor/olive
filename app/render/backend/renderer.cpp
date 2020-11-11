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

#include <QFloat16>

#include "common/ocioutils.h"
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

void Renderer::BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, Texture *destination, bool flipped)
{
  qDebug() << "BlitColorManaged is a partial stub";

  QVariant shader = CreateNativeShader(ShaderCode(FileFunctions::ReadFileAsString(":/shaders/default.frag"), FileFunctions::ReadFileAsString(":/shaders/default.vert")));

  ShaderJob job;
  job.InsertValue(QStringLiteral("ove_maintex"), ShaderValue(QVariant::fromValue(source), NodeParam::kTexture));

  if (flipped) {
    QMatrix4x4 mat;
    mat.scale(1, -1, 1);
    job.SetMatrix(mat);
  }

  BlitToTexture(shader, job, destination);

  DestroyNativeShader(shader);
}

void Renderer::BlitColorManaged(ColorProcessorPtr color_processor, TexturePtr source, VideoParams params, bool flipped)
{
  qDebug() << "BlitColorManaged is a partial stub";

  /*ColorContext color_ctx;

  if (color_cache_.contains(color_processor->id())) {
    color_ctx = color_cache_.value(color_processor->id());
  } else {
    // Create shader description
    const char* ocio_func_name = "OCIODisplay";
    OCIO::GpuShaderDescRcPtr shader_desc = OCIO::GpuShaderDesc::CreateShaderDesc();
    shader_desc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_2);
    shader_desc->setFunctionName(ocio_func_name);
    shader_desc->setResourcePrefix("ocio_");

    // Generate shader
    color_processor->GetProcessor()->getDefaultGPUProcessor()->extractGpuShaderInfo(shader_desc);

    qDebug() << "Shader:" << shader_desc->getShaderText();
  }*/

  QVariant shader = CreateNativeShader(ShaderCode(FileFunctions::ReadFileAsString(":/shaders/default.frag"), FileFunctions::ReadFileAsString(":/shaders/default.vert")));

  ShaderJob job;
  job.InsertValue(QStringLiteral("ove_maintex"), ShaderValue(QVariant::fromValue(source), NodeParam::kTexture));

  if (flipped) {
    QMatrix4x4 mat;
    mat.scale(1, -1, 1);
    job.SetMatrix(mat);
  }

  Blit(shader, job, params);

  DestroyNativeShader(shader);
}

OLIVE_NAMESPACE_EXIT
