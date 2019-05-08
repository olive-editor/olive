#include "projectfunctions.h"

#include "projectmodel.h"
#include "global/config.h"

MediaPtr olive::project::CreateFolder(QString name) {

  MediaPtr item = std::make_shared<Media>();

  item->set_folder();
  item->set_name(name);

  return item;

}

SequencePtr olive::project::CreateSequenceFromMedia(QVector<olive::timeline::MediaImportData> &media_list)
{
  SequencePtr s = std::make_shared<Sequence>();

  s->set_name(olive::project_model.GetNextSequenceName());

  // Retrieve default Sequence settings from Config
  s->set_width(olive::config.default_sequence_width);
  s->set_height(olive::config.default_sequence_height);
  s->set_frame_rate(olive::config.default_sequence_framerate);
  s->set_audio_frequency(olive::config.default_sequence_audio_frequency);
  s->set_audio_layout(olive::config.default_sequence_audio_channel_layout);

  bool got_video_values = false;
  bool got_audio_values = false;
  for (int i=0;i<media_list.size();i++) {
    Media* media = media_list.at(i).media();
    switch (media->get_type()) {
    case MEDIA_TYPE_FOOTAGE:
    {
      Footage* m = media->to_footage();
      if (m->ready) {
        if (!got_video_values) {
          for (int j=0;j<m->video_tracks.size();j++) {
            const FootageStream& ms = m->video_tracks.at(j);
            s->set_width(ms.video_width);
            s->set_height(ms.video_height);
            if (!qFuzzyCompare(ms.video_frame_rate, 0.0)) {
              s->set_frame_rate(ms.video_frame_rate * m->speed);

              if (ms.video_interlacing != VIDEO_PROGRESSIVE) s->set_frame_rate(s->frame_rate() * 2);

              // only break with a decent frame rate, otherwise there may be a better candidate
              got_video_values = true;
              break;
            }
          }
        }
        if (!got_audio_values && m->audio_tracks.size() > 0) {
          const FootageStream& ms = m->audio_tracks.at(0);
          s->set_audio_frequency(ms.audio_frequency);
          got_audio_values = true;
        }
      }
    }
      break;
    case MEDIA_TYPE_SEQUENCE:
    {
      // Clone all attributes of the original sequence (seq) into the new one (s)
      Sequence* seq = media->to_sequence().get();

      s->set_width(seq->width());
      s->set_height(seq->height());
      s->set_frame_rate(seq->frame_rate());
      s->set_audio_frequency(seq->audio_frequency());
      s->set_audio_layout(seq->audio_layout());

      got_video_values = true;
      got_audio_values = true;
    }
      break;
    }
    if (got_video_values && got_audio_values) break;
  }

  return s;
}
