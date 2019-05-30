#include "nodemedia.h"

#include <QDebug>

#include "nodes/nodegraph.h"
#include "decoders/ffmpegvideodecoder.h"
#include "decoders/pixelformatconverter.h"
#include "global/config.h"
#include "global/global.h"

NodeMedia::NodeMedia(NodeGraph* c) :
  Node(c),
  media_(nullptr),
  img_buffer_(c->memory_cache()),
  tex_buffer_(c->memory_cache()),
  decoder_(nullptr)
{
  matrix_input_ = new NodeIO(this, "matrix", tr("Matrix"), true, false);
  matrix_input_->AddAcceptedNodeInput(olive::nodes::kMatrix);

  texture_output_ = new NodeIO(this, "texture", tr("Texture"), true, false);
  texture_output_->SetOutputDataType(olive::nodes::kTexture);
}

QString NodeMedia::name()
{
  return tr("Media");
}

QString NodeMedia::id()
{
  return "org.olivevideoeditor.Olive.media";
}

void NodeMedia::Process(const rational &time)
{
  QOpenGLFunctions* f = ParentGraph()->GLContext()->functions();

  tex_buffer_.buffer()->BindBuffer();

  f->glClearColor(0.0, 0.5, 0.0, 0.0);
  f->glClear(GL_COLOR_BUFFER_BIT);

  tex_buffer_.buffer()->ReleaseBuffer();

  FramePtr frame = decoder_->Retrieve(time);

  olive::pix_fmt_conv->AVFrameToPipeline(frame->data(),
                                         frame->linesize(),
                                         frame->width(),
                                         frame->height(),
                                         static_cast<AVPixelFormat>(frame->format()),
                                         img_buffer_.buffer()->data(),
                                         olive::Global->effective_bit_depth());

  const olive::PixelFormatInfo& pix_fmt_info = olive::pixel_formats.at(olive::Global->effective_bit_depth());

  tex_buffer_.buffer()->BindTexture();

  f->glTexSubImage2D(GL_TEXTURE_2D,
                     0,
                     0,
                     0,
                     frame->width(),
                     frame->height(),
                     pix_fmt_info.pixel_format,
                     pix_fmt_info.pixel_type,
                     img_buffer_.buffer()->data()
                     );

  tex_buffer_.buffer()->ReleaseTexture();

  qDebug() << "Uploaded to texture" << tex_buffer_.buffer()->texture();

  texture_output_->SetValue(tex_buffer_.buffer()->texture());
}

void NodeMedia::SetMedia(FootageStream *f)
{
  // TODO method of finding the decoder we need

  decoder_ = std::make_shared<FFmpegVideoDecoder>();

  media_ = f;

  decoder_->set_stream(media_);

  img_buffer_.SetSize(f->video_width, f->video_height);
}

NodeIO *NodeMedia::matrix_input()
{
  return matrix_input_;
}

NodeIO *NodeMedia::texture_output()
{
  return texture_output_;
}


