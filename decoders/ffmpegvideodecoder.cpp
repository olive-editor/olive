#include "ffmpegvideodecoder.h"

#include <QDebug>

FFmpegVideoDecoder::FFmpegVideoDecoder()
{

}

FramePtr FFmpegVideoDecoder::Retrieve(const rational &timecode, const rational &length)
{
  // TODO index frames

  int receive_ret;

  // Allocate new frame
  AVFrame* frame = av_frame_alloc();
  AVPacket* pkt = av_packet_alloc();

  while ((receive_ret = avcodec_receive_frame(codec_ctx_, frame)) == AVERROR(EAGAIN)) {
    int read_ret = 0;
    do {

      // Make sure any previous packet buffers are cleared before reading new ones
      av_packet_unref(pkt);

      // Read next packet from file
      read_ret = av_read_frame(fmt_ctx_, pkt);

    } while (read_ret >= 0 && pkt->stream_index != stream()->file_index);

    if (read_ret >= 0) {
      int send_ret = avcodec_send_packet(codec_ctx_, pkt);
      if (send_ret < 0) {
        qCritical() << "Failed to send packet to decoder." << send_ret;
        return nullptr;
      }
    } else {
      if (read_ret == AVERROR_EOF) {
        int send_ret = avcodec_send_packet(codec_ctx_, nullptr);
        if (send_ret < 0) {
          qCritical() << "Failed to send packet to decoder." << send_ret;
          return nullptr;
        }
      } else {
        qCritical() << "Could not read frame." << read_ret;
        return nullptr;
      }
    }
  }
  if (receive_ret < 0) {
    if (receive_ret != AVERROR_EOF) {
      qCritical() << "Failed to receive packet from decoder." << receive_ret;
    }
  }

  // Free AVPacket
  av_packet_free(&pkt);

  // AVFrame is wrapped in Frame object and shared ptr to automatically clear it
  return std::make_shared<Frame>(frame);
}
