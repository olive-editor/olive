#include "textanimationengine.h"

#include <cmath>
#include <QMap>
#include <functional>

#ifndef M_PI
#define M_PI     3.14159265358979323846
#endif



namespace olive {

namespace {

/// Template for all animation functions.
/// Each function depends on a value \p x that is in range [0,1]. When x=0, each
/// function returns 1 (i.e. animation not yet started); When x=1, each
/// function returns 0 (i.e. animation complete).
/// \param c1 and c2 are coefficient that may (or may not) affect the curve. In any
///     case, 1 is always a good default value.
typedef std::function<double ( double x, double c1, double c2)> animationFunction_t;

double Step( double x, double c1, double /*c2*/) {
  return  (x < 0.5*c1) ? 1. : 0.;
}

double Linear( double x, double /*c1*/, double /*c2*/) {
  return  (1.0 - x);
}

double EaseOutSine( double x, double /*c1*/, double /*c2*/) {
  return 1.0 - sin((x * M_PI) / 2.);
}

double EaseOutQuad( double x, double /*c1*/, double /*c2*/) {
  return (1.0 - x) * (1.0 - x);
}

/* a transition with a single overshoot */
double EaseOutBack( double x, double c1, double c2) {
  return - 2.7*c2*pow(x-1., 3) - 1.7*c1*pow(x-1., 2);
}

/* a transition with dumped sine envelope */
double EaseOutElastic( double x, double c1, double c2) {
  const double c4 = (2.0 * M_PI) / 3.0;
  double value;

  if (x < 0.01) { // almost x==0
    value = 0.;
  }
  else if ( x > 0.99) { // almost x==1
    value = 1.;
  }
  else {
    value = pow(2, -10 * c1 * x) * sin((x * 10 * c2 - 0.75) * c4) + 1;
  }

  return 1. - value;
}

/* a transition made up by 4 parabolic bounces */
double EaseOutBounce( double x, double c1, double /*c2*/) {

  double value;

  if (x <= 0.25) {
    value = -16.*x*x + 1.;
  } else if (x <= 0.5) {
    value = c1*( -16.*(x-3./8.)*(x-3./8.) + 1./4.);
  } else if (x <= 0.75) {
    value = c1*( -8.*(x-5./8.)*(x-5./8.) + 1./8.);
  } else {
    value = c1*( -4.*(x-7./8.)*(x-7./8.) + 1./16.);
  }

  return value;
}

const QMap< TextAnimation::Curve, animationFunction_t> ANIMATION_TABLE = {
  {TextAnimation::Step, Step},
  {TextAnimation::Linear, Linear},
  {TextAnimation::Ease_Sine, EaseOutSine},
  {TextAnimation::Ease_Quad, EaseOutQuad},
  {TextAnimation::Ease_Back, EaseOutBack},
  {TextAnimation::Ease_Elastic, EaseOutElastic},
  {TextAnimation::Ease_Bounce, EaseOutBounce},
};

} // namespace

TextAnimationEngine::TextAnimationEngine() :
  animators_(nullptr),
  text_size_(0),
  last_index_(-1)
{
}



void TextAnimationEngine::Calulate()
{
  ResetVectors();

  Q_ASSERT( animators_);

  foreach ( const TextAnimation::Descriptor & anim, *animators_) {
    CalculateAnimator( anim);
  }
}


void TextAnimationEngine::ResetVectors()
{
  horiz_offsets_.clear();
  vert_offsets_.clear();
  rotations_.clear();
  spacings_.clear();
  horiz_stretch_.clear();
  vert_stretch_.clear();
  transparency_.clear();

  // preset all vectors with the length of the full text
  horiz_offsets_.fill( 0.0, text_size_);
  vert_offsets_.fill( 0.0, text_size_);
  rotations_.fill( 0.0, text_size_);
  spacings_.fill( 0.0, text_size_);
  horiz_stretch_.fill( 0.0, text_size_);
  vert_stretch_.fill( 0.0, text_size_);
  transparency_.fill( 0, text_size_);
}

void TextAnimationEngine::CalculateAnimator( const TextAnimation::Descriptor &animator)
{
  int index = animator.character_from;
  last_index_ = CalculateEndIndex( animator);

  // avoid infinite loop if 'stride' is illegal
  int step = (animator.stride < 0) ? 1 : (1 + animator.stride);

  while (index <= last_index_) {

    // if 'last_to_first' is set, fill data in indexes from last to first
    int target_index = animator.last_to_first ?
          last_index_ + animator.character_from - index :
          index;

    double anim_value = CalculateAnimatorForChar( animator, index);

    switch( animator.feature) {
    case TextAnimation::PositionVertical:
      vert_offsets_[target_index] += anim_value;
      break;
    case TextAnimation::PositionHorizontal:
      horiz_offsets_[target_index] += anim_value;
      break;
    case TextAnimation::Rotation:
      rotations_[target_index] += anim_value;
      break;
    case TextAnimation::SpacingFactor:
      spacings_[target_index] += anim_value;
      break;
    case TextAnimation::StretchVertical:
      vert_stretch_[target_index] += anim_value;
      break;
    case TextAnimation::StretchHorizontal:
      horiz_stretch_[target_index] += anim_value;
      break;
    case TextAnimation::Transparency:
      transparency_[target_index] += (int)anim_value;
      transparency_[target_index] = qMin( transparency_[target_index], 255);
      transparency_[target_index] = qMax( transparency_[target_index], 0);
      break;
    default:
    case TextAnimation::None:
      break;
    }

    index += step;
  }
}

// the final index is 'animator.character_to', unless -1 is used (that means full text).
// If the text is empty, 0 is passed.
int TextAnimationEngine::CalculateEndIndex( const TextAnimation::Descriptor &animator) {
  int end_index = 0;

  if (text_size_ > 0) {
    if (animator.character_to == -1) {
      end_index = (text_size_-1);
    }
    else {
       end_index = qMin( (text_size_-1), animator.character_to);
    }
  }

  return end_index;
}

// based on the number of characters that must be animated, divide the range [0,1] in slices of length 'dt'.
// When 'overlap_in' and 'overlap_out' are 1, each character perform its transition in one of this slice.
// By lowering 'overlap_in', the transitions begin earlier (when overlap_in=0 they all start at the begin).
// By lowering 'overlap_out', the transitions end earlier (when overlap_out=1 they all finish at the end).
// The field 'progress' of parameter 'animator' identifies the evolution of the transition: when progress = 0,
// the transition is not started; when progress = 1, the transition is complete.
double TextAnimationEngine::CalculateAnimatorForChar( const TextAnimation::Descriptor &animator, int char_index)
{
  int num_chars = last_index_ - animator.character_from + 1;
  double animation_value = 0.0;

  Q_ASSERT( num_chars != 0);

  double dt = 1.0/(double)num_chars;
  const double i = (double)char_index - (double)animator.character_from;

  double low = i*dt*(animator.overlap_in);
  double hi = (i+1.)*dt*(animator.overlap_out) + (1. - (animator.overlap_out));

  if (animator.progress <= (low)) {
    animation_value = 1.0;
  } else if (animator.progress < hi) {
    double x = (animator.progress - low)/(hi-low);
    animation_value = ANIMATION_TABLE.value( animator.curve)( x, animator.c1, animator.c2);
  }
  else {
    animation_value = 0.0;
  }

  animation_value *= animator.value;

  return animation_value;
}

}  // olive

