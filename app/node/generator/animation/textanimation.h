#ifndef ANIMATION_H
#define ANIMATION_H

#include <QMap>

namespace olive {

namespace TextAnimation {

/// The feature of the text that is being animated
enum Feature{
  // invalid value
  None,
  // vertical offset for each character
  PositionVertical,
  // horizontal offset for each character
  PositionHorizontal,
  // rotation of each character around its bottom left corner
  Rotation,
  // space between letters
  SpacingFactor
};

const QMap< Feature, QString> FEATURE_TABLE = {{None, "NONE"},
                                               {PositionVertical, "POSITION_VER"},
                                               {PositionHorizontal, "POSITION_HOR"},
                                               {Rotation, "ROTATION"},
                                               {SpacingFactor,  "SPACING"}
                                              };
const QMap< QString, Feature> FEATURE_TABLE_REV = {{"NONE", None},
                                                   {"POSITION_VER", PositionVertical},
                                                   {"POSITION_HOR", PositionHorizontal},
                                                   {"ROTATION", Rotation},
                                                   {"SPACING", SpacingFactor}
                                                  };

/// shape of animation curve
enum Curve {
  Step,
  Linear,
  Ease_Sine,
  Ease_Quad,
  Ease_Back,
  Ease_Elastic,
  Ease_Bounce
};

const QMap< Curve, QString> CURVE_TABLE = {{Step, "STEP"},
                                           {Linear, "LINEAR"},
                                           {Ease_Sine, "EASE_SINE"},
                                           {Ease_Quad, "EASE_QUAD"},
                                           {Ease_Back, "EASE_BACK"},
                                           {Ease_Elastic, "EASE_ELASTIC"},
                                           {Ease_Bounce, "EASE_BOUNCE"}
                                          };

const QMap< QString, Curve> CURVE_TABLE_REV = { { "STEP", Step},
                                                { "LINEAR", Linear },
                                                { "EASE_SINE", Ease_Sine },
                                                { "EASE_QUAD", Ease_Quad },
                                                { "EASE_BACK", Ease_Back },
                                                { "EASE_ELASTIC", Ease_Elastic },
                                                { "EASE_BOUNCE", Ease_Bounce}
                                              };

/// full descriptor of an animation
struct Descriptor {
  TextAnimation::Feature feature;
  // first character to be animated. Use 0 for the first character
  int character_from;
  // last character to be animated. Use -1 to indicate the end of the text
  int character_to;
  // range [0,1]. The lower this value, the more all characters start
  // their animation together
  double overlap_in;
  // range [0,1]. The lower this value, the more all characters end
  // their animation together
  double overlap_out;
  TextAnimation::Curve curve;
  // coefficient that modifies the curve. May or may not have effect for each curve.
  // Value 1 should always be a good default.
  double c1;
  // coefficient that modifies the curve. May or may not have effect for each curve.
  // Value 1 should always be a good default.
  double c2;
  // value of the feature at the begin of the transition
  double value;
  // Evolution of the transition in range [0,1]. When 0, the animated value is equal to 'value';
  // When 1, the animated value is equal to 0.
  double alpha;
};

}  // TextAnimation

}  // olive

#endif // ANIMATION_H
