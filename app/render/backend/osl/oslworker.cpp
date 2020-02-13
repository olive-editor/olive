#include "oslworker.h"

#include <OpenImageIO/imagebuf.h>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "common/clamp.h"
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

  // If alpha is associated, disassociate for the color transform
  if (video_stream->premultiplied_alpha()) {
    ColorManager::DisassociateAlpha(frame);
  }

  // Convert frame to float for OCIO
  if (frame->format() != PixelFormat::PIX_FMT_RGBA32F) {
    frame = PixelService::ConvertPixelFormat(frame, frame->format());
  }

  // Perform color transform
  color_processor->ConvertFrame(frame);

  // Associate alpha
  if (video_stream->premultiplied_alpha()) {
    ColorManager::ReassociateAlpha(frame);
  } else {
    ColorManager::AssociateAlpha(frame);
  }

  table->Push(NodeParam::kTexture, QVariant::fromValue(frame));
}

void OSLWorker::RunNodeAccelerated(const Node *node, const TimeRange &range, const NodeValueDatabase &input_params, NodeValueTable *output_params)
{
  Q_UNUSED(node)
  Q_UNUSED(range)
  Q_UNUSED(input_params)
  Q_UNUSED(output_params)

  /*
  if (!node->IsAccelerated()) {
    return;
  }

  OSL::ShaderGroupRef group = shader_cache_->Get(node->id());

  if (!group) {
    return;
  }

  // Create the output textures
  QList<OpenGLTextureCache::ReferencePtr> dst_refs;
  dst_refs.append(texture_cache_.Get(ctx_, video_params_));
  GLuint iterative_input = 0;

  // If this node requires multiple iterations, get a texture for it too
  if (node->AcceleratedCodeIterations() > 1 && node->AcceleratedCodeIterativeInput()) {
    dst_refs.append(texture_cache_.Get(ctx_, video_params_));
  }

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
        OpenGLTextureCache::ReferencePtr texture = value.value<OpenGLTextureCache::ReferencePtr>();

        functions_->glActiveTexture(GL_TEXTURE0 + input_texture_count);

        GLuint tex_id = texture ? texture->texture()->texture() : 0;
        functions_->glBindTexture(GL_TEXTURE_2D, tex_id);

        // Set value to bound texture
        shader->setUniformValue(variable_location, input_texture_count);

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

        // If this texture binding is the iterative input, set it here
        if (input == node->AcceleratedCodeIterativeInput()) {
          iterative_input = input_texture_count;
        }

        OpenGLRenderFunctions::PrepareToDraw(functions_);

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

  // Some nodes use multiple iterations for optimization
  OpenGLTextureCache::ReferencePtr output_tex;

  for (int iteration=0;iteration<node->AcceleratedCodeIterations();iteration++) {
    // If this is not the first iteration, set the parameter that will receive the last iteration's texture
    OpenGLTextureCache::ReferencePtr source_tex = dst_refs.at((iteration+1)%dst_refs.size());
    OpenGLTextureCache::ReferencePtr destination_tex = dst_refs.at(iteration%dst_refs.size());

    // Set iteration number
    shading_system_->ReParameter(*group.get(),
                                 "layer1",
                                 "ove_iteration",
                                 OIIO::TypeDesc::INT,
                                 &iteration);

    if (iteration > 0) {
      functions_->glActiveTexture(GL_TEXTURE0 + iterative_input);
      functions_->glBindTexture(GL_TEXTURE_2D, source_tex->texture()->texture());
    }

    destination_tex->texture(); // DESTINATION TEXTURE

    static OIIO::array_view<OSL::ustring> outputs = {OSL::ustring("Cout")};

    OIIO::ImageBufAlgo::parallel_image_options popt;
#if OPENIMAGEIO_VERSION > 10902
    popt.minitems = 4096;
    popt.splitdir = OIIO::Split_Tile;
    popt.recursive = true;
#endif

    OIIO::ImageBuf buffer;

    shade_image(*shading_system_,
                *group.get(),
                nullptr,
                buffer,
                outputs,
                OSL::ShadePixelCenters,
                OIIO::ROI(),
                popt);

    // Update output reference to the last texture we wrote to
    output_tex = destination_tex;
  }

  output_params->Push(NodeParam::kTexture, QVariant::fromValue(output_tex));
  */
}

void OSLWorker::TextureToBuffer(const QVariant &tex_in, QByteArray &buffer)
{
  FramePtr frame = tex_in.value<FramePtr>();

  memcpy(buffer.data(), frame->data(), static_cast<size_t>(buffer.size()));
}
