#include "oslworker.h"

#include <OpenImageIO/imagebuf.h>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "common/clamp.h"
#include "common/define.h"
#include "core.h"
#include "node/block/transition/transition.h"
#include "node/node.h"
#include "render/colormanager.h"
#include "render/pixelservice.h"

OSLWorker::OSLWorker(VideoRenderFrameCache* frame_cache,
                     OSL::ShadingSystem* shading_system,
                     OSLShaderCache* shader_cache,
                     ColorProcessorCache* color_cache,
                     QObject* parent) :
  VideoRenderWorker(frame_cache, parent),
  shading_system_(shading_system),
  shader_cache_(shader_cache),
  color_cache_(color_cache)
{
}

void OSLWorker::FrameToValue(StreamPtr stream, FramePtr frame, NodeValueTable *table)
{
  // Ensure stream is video or image type
  if (stream->type() != Stream::kVideo && stream->type() != Stream::kImage) {
    return;
  }

  ImageStreamPtr video_stream = std::static_pointer_cast<ImageStream>(stream);

  // Set up OCIO context
  ColorProcessorPtr color_processor = color_cache_->Get(video_stream->colorspace());

  if (!color_processor) {
    // FIXME: We match with the colorspace string, but this won't change if the user sets a new config with a colorspace with the same string
    color_processor = ColorProcessor::Create(video_stream->footage()->project()->color_manager()->GetConfig(),
                                             video_stream->colorspace(),
                                             OCIO::ROLE_SCENE_LINEAR);
    color_cache_->Add(video_stream->colorspace(), color_processor);
  }

  // Convert frame to float for OCIO
  if (frame->format() != PixelFormat::PIX_FMT_RGBA32F) {
    frame = PixelService::ConvertPixelFormat(frame, PixelFormat::PIX_FMT_RGBA32F);
  }

  // If alpha is associated, disassociate for the color transform
  if (video_stream->premultiplied_alpha()) {
    ColorManager::DisassociateAlpha(frame);
  }

  // Perform color transform
  color_processor->ConvertFrame(frame);

  // Associate alpha
  if (video_stream->premultiplied_alpha()) {
    ColorManager::ReassociateAlpha(frame);
  } else {
    ColorManager::AssociateAlpha(frame);
  }

  OIIOImageBufRef buf = std::make_shared<OIIO::ImageBuf>(OIIO::ImageSpec(frame->width(), frame->height(), kRGBAChannels, OIIO::TypeDesc::FLOAT));

  memcpy(buf->localpixels(), frame->data(), frame->allocated_size());

  table->Push(NodeParam::kTexture, QVariant::fromValue(buf));
}

void OSLWorker::RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable *output_params)
{
  if (!node->IsAccelerated()) {
    return;
  }

  OSL::ShaderGroupRef group = shader_cache_->Get(node->id());

  if (!group) {
    return;
  }

  int input_texture_count = 0;

  OIIOImageBufRef iteration_buf = nullptr;
  int iteration_id = -1;

  foreach (NodeParam* param, node->parameters()) {
    if (param->type() == NodeParam::kInput) {
      // This variable is used in the shader, let's set it to our value

      NodeInput* input = static_cast<NodeInput*>(param);

      // Get value from database at this input
      const NodeValueTable& input_data = input_params[input];

      QVariant value = node->InputValueFromTable(input, input_data);

      switch (input->data_type()) {
      case NodeInput::kInt:
      {
        int val = value.toInt();
        shading_system_->ReParameter(*group.get(), "layer1", param->id().toStdString(), OIIO::TypeDesc::INT, &val);
        break;
      }
      case NodeInput::kFloat:
      {
        double val = value.toDouble();
        shading_system_->ReParameter(*group.get(), "layer1", param->id().toStdString(), OIIO::TypeDesc::DOUBLE, &val);
        break;
      }
      case NodeInput::kVec2:
      {
        QVector2D val = value.value<QVector2D>();
        shading_system_->ReParameter(*group.get(), "layer1", param->id().toStdString(), OIIO::TypeDesc(OIIO::TypeDesc::FLOAT, OIIO::TypeDesc::VEC2), &val);
        break;
      }
      case NodeInput::kVec3:
      {
        QVector3D val = value.value<QVector3D>();
        shading_system_->ReParameter(*group.get(), "layer1", param->id().toStdString(), OIIO::TypeDesc(OIIO::TypeDesc::FLOAT, OIIO::TypeDesc::VEC3), &val);
        break;
      }
      case NodeInput::kVec4:
      {
        QVector4D val = value.value<QVector4D>();
        shading_system_->ReParameter(*group.get(), "layer1", param->id().toStdString(), OIIO::TypeDesc(OIIO::TypeDesc::FLOAT, OIIO::TypeDesc::VEC4), &val);
        break;
      }
      case NodeInput::kMatrix:
      {
        QMatrix4x4 val = value.value<QMatrix4x4>();
        shading_system_->ReParameter(*group.get(), "layer1", param->id().toStdString(), OIIO::TypeDesc(OIIO::TypeDesc::FLOAT, OIIO::TypeDesc::MATRIX44), &val);
        break;
      }
      case NodeInput::kColor:
      {
        QVector4D val = value.value<QVector4D>();
        shading_system_->ReParameter(*group.get(), "layer1", param->id().toStdString(), OIIO::TypeDesc(OIIO::TypeDesc::FLOAT, OIIO::TypeDesc::VEC4, OIIO::TypeDesc::COLOR), &val);
        break;
      }
      case NodeInput::kBoolean:
      {
        bool val = value.toBool();
        shading_system_->ReParameter(*group.get(), "layer1", param->id().toStdString(), OIIO::TypeDesc::CHAR, &val);
        break;
      }
      case NodeInput::kFootage:
      case NodeInput::kTexture:
      case NodeInput::kBuffer:
      {
        OIIOImageBufRef buf = value.value<OIIOImageBufRef>();

        if (buf) {

          QString tex_fn = ImageBufToTexture(*buf.get(), input_texture_count);

          if (!tex_fn.isEmpty()) {
            QByteArray tex_fn_bytes = tex_fn.toUtf8();
            const char* tex_fn_c_str = tex_fn_bytes.constData();

            if (!shading_system_->ReParameter(*group.get(), "layer1", param->id().toStdString(), OIIO::TypeDesc::STRING, &tex_fn_c_str)) {
              qDebug() << "Failed to set string parameter";
            }
          }

        } else {

          shading_system_->ReParameter(*group.get(), "layer1", param->id().toStdString(), OIIO::TypeDesc::STRING, nullptr);

        }

        if (node->AcceleratedCodeIterativeInput() == input) {
          iteration_buf = buf;

          iteration_id = input_texture_count;
        }






        /*
        // Set enable flag if shader wants it
        int enable_param_location = shader->uniformLocation(QStringLiteral("%1_enabled").arg(input->id()));
        if (enable_param_location > -1) {
          shader->setUniformValue(enable_param_location,
                                  tex_id > 0);
        }

        if (tex_id > 0) {
          // Set texture resolution if shader wants it
          int res_param_location = shader->uniformLocation(QStringLiteral("%1_resolution").arg(input->id()));
          if (res_param_location > -1) {
            shader->setUniformValue(res_param_location,
                                    static_cast<GLfloat>(texture->texture()->width()),
                                    static_cast<GLfloat>(texture->texture()->height()));
          }
        }



        OpenGLRenderFunctions::PrepareToDraw(functions_);
        */

        input_texture_count++;
        break;
      }
      case NodeInput::kSamples:
      case NodeInput::kText:
      case NodeInput::kRational:
      case NodeInput::kFont:
      case NodeInput::kFile:
      case NodeInput::kDecimal:
      case NodeInput::kWholeNumber:
      case NodeInput::kNumber:
      case NodeInput::kString:
      case NodeInput::kVector:
      case NodeInput::kNone:
      case NodeInput::kAny:
        break;
      }
    }
  }

  // Provide some standard args
  {
    QVector2D resolution(static_cast<float>(video_params().width()), static_cast<float>(video_params().height()));
    shading_system_->ReParameter(*group.get(),
                                 "layer1",
                                 "ove_resolution",
                                 OIIO::TypeDesc(OIIO::TypeDesc::FLOAT, OIIO::TypeDesc::VEC2),
                                 &resolution);
  }


  if (node->IsBlock() && static_cast<const Block*>(node)->type() == Block::kTransition) {
    const TransitionBlock* transition_node = static_cast<const TransitionBlock*>(node);

    // Provides total transition progress from 0.0 (start) - 1.0 (end)
    double p = transition_node->GetTotalProgress(range.in());
    shading_system_->ReParameter(*group.get(),
                                 "layer1",
                                 "ove_tprog_all",
                                 OIIO::TypeDesc::DOUBLE,
                                 &p);

    // Provides progress of out section from 1.0 (start) - 0.0 (end)
    p = transition_node->GetOutProgress(range.in());
    shading_system_->ReParameter(*group.get(),
                                 "layer1",
                                 "ove_tprog_out",
                                 OIIO::TypeDesc::DOUBLE,
                                 &p);

    // Provides progress of in section from 0.0 (start) - 1.0 (end)
    p = transition_node->GetInProgress(range.in());
    shading_system_->ReParameter(*group.get(),
                                 "layer1",
                                 "ove_tprog_in",
                                 OIIO::TypeDesc::DOUBLE,
                                 &p);
  }

  OIIOImageBufRef dest_buf = nullptr;

  // Some nodes use multiple iterations for optimization
  for (int iteration=0;iteration<node->AcceleratedCodeIterations();iteration++) {
    // Set iteration number
    shading_system_->ReParameter(*group.get(),
                                 "layer1",
                                 "ove_iteration",
                                 OIIO::TypeDesc::INT,
                                 &iteration);

    if (iteration > 0) {
      // Convert destination buffer into texture
      QString s = ImageBufToTexture(*dest_buf.get(), iteration_id);

      QByteArray s_bytes = s.toUtf8();
      const char* s_c_str = s_bytes.constData();

      // Set texture as iterative input
      shading_system_->ReParameter(*group.get(), "layer1", node->AcceleratedCodeIterativeInput()->id().toStdString(), OIIO::TypeDesc::STRING, &s_c_str);

      // Swap destination buffer and iteration buffer
      std::swap(iteration_buf, dest_buf);
    }

    // Ensure dest_buf exists
    if (!dest_buf) {
      dest_buf = std::make_shared<OIIO::ImageBuf>(OIIO::ImageSpec(video_params().width(),
                                                                  video_params().height(),
                                                                  kRGBAChannels,
                                                                  PixelService::GetPixelFormatInfo(video_params().format()).oiio_desc));
    }

    static OSL::ustring outputs[] = {OSL::ustring("Cout")};

    OIIO::ImageBufAlgo::parallel_image_options popt;
#if OPENIMAGEIO_VERSION > 10902
    popt.minitems = 4096;
    popt.splitdir = OIIO::Split_Tile;
    popt.recursive = true;
#endif

    shade_image(*shading_system_,
                *group.get(),
                nullptr,
                *dest_buf.get(),
                outputs,
                OSL::ShadePixelCenters,
                OIIO::ROI(),
                popt);
  }

  output_params->Push(NodeParam::kTexture, QVariant::fromValue(dest_buf));
}

void OSLWorker::TextureToBuffer(const QVariant &tex_in, QByteArray &buffer)
{
  OIIOImageBufRef frame = tex_in.value<OIIOImageBufRef>();

  memcpy(buffer.data(), frame->localpixels(), static_cast<size_t>(buffer.size()));
}

QString OSLWorker::ImageBufToTexture(const OpenImageIO_v2_1::ImageBuf &buf, int tex_no)
{
  OIIO::ImageSpec config;
  config.attribute("maketx:filtername", "lanczos3");

  QString tex_fn = QStringLiteral("C:/Users/Matt/Desktop/temp-%1-%2.tx").arg(QString::number(reinterpret_cast<quintptr>(this)), QString::number(tex_no));

  stringstream s;

  if (!OIIO::ImageBufAlgo::make_texture(OIIO::ImageBufAlgo::MakeTextureMode::MakeTxTexture,
                                       buf,
                                       tex_fn.toStdString(),
                                       config,
                                       &s)) {
    qCritical() << "Failed to make_texture:" << s.str().c_str();

    return QString();
  }

  return tex_fn;
}
