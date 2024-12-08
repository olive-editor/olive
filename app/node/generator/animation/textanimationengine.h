#ifndef TEXTANIMATIONENGINE_H
#define TEXTANIMATIONENGINE_H

#include "textanimation.h"
#include <QVector>

namespace olive {


/// This class calculates the values of each character feature
/// (offset, rotation, spacing) for a given moment of the transition.
/// Method \a calculate must be called after setting all animators. After that,
/// output functions can be called.
/// If there are several animation, each one is calculated in sequence to generate
/// the final vectors of offset and rotation for each character.
class TextAnimationEngine
{
public:
  TextAnimationEngine();

  /// to be called before 'calculate' to indicate the number
  /// of characters of the full document.
  void SetTextSize( int text_size) {
    text_size_ = text_size;
  }

  /// set the list of animators to be applied to text document.
  void SetAnimators( const QList<TextAnimation::Descriptor> * animators) {
    animators_ = animators;
  }

  /// perform all calculation for the animation of text.
  void Calulate();

  /// valid after calling \a calculate
  const QVector<double> & HorizontalOffset() const {
    return horiz_offsets_;
  }

  /// valid after calling \a calculate
  const QVector<double> & VerticalOffset() const {
    return vert_offsets_;
  }

  /// valid after calling \a calculate
  const QVector<double> & Rotation() const {
    return rotations_;
  }

  /// valid after calling \a calculate
  const QVector<double> & Spacing() const {
    return spacings_;
  }

  /// valid after calling \a calculate
  const QVector<double> & HorizontalStretch() const {
    return horiz_stretch_;
  }

  /// valid after calling \a calculate
  const QVector<double> & VerticalStretch() const {
    return vert_stretch_;
  }

  /// valid after calling \a calculate
  const QVector<int> & Transparency() const {
    return transparency_;
  }

private:
  void ResetVectors();
  void CalculateAnimator( const TextAnimation::Descriptor & animator);
  int CalculateEndIndex(const TextAnimation::Descriptor &animator);
  double CalculateAnimatorForChar( const TextAnimation::Descriptor &animator, int char_index);

private:
  const QList<TextAnimation::Descriptor> * animators_;
  int text_size_;
  int last_index_;

  QVector<double> horiz_offsets_;
  QVector<double> vert_offsets_;
  QVector<double> rotations_;
  QVector<double> spacings_;
  QVector<double> horiz_stretch_;
  QVector<double> vert_stretch_;
  QVector<int> transparency_;

};

}  // olive

#endif // TEXTANIMATIONENGINE_H
