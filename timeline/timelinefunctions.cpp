#include "timelinefunctions.h"



void olive::timeline::RelinkClips(QVector<Clip *> &pre_clips, QVector<ClipPtr> &post_clips)
{
  // Loop through each "pre" clip
  for (int i=0;i<pre_clips.size();i++) {

    Clip* pre = pre_clips.at(i);

    // Loop through each "pre" clip's linked array
    for (int j=0;j<pre->linked.size();j++) {

      // Loop again through the "pre" clips to determine which are linked to each other
      for (int l=0;l<pre_clips.size();l++) {

        // If this "pre" is linked to this "pre"
        if (pre->linked.at(j) == pre_clips.at(l)) {

          // Check if we have an equivalent in the post_clips, and link it if so
          if (post_clips.at(l) != nullptr) {
            post_clips.at(i)->linked.append(post_clips.at(l).get());
          }

        }
      }
    }
  }
}
