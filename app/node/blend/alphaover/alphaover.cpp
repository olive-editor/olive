#include "alphaover.h"

AlphaOverBlend::AlphaOverBlend()
{

}

void AlphaOverBlend::Process(const rational &time)
{
  Q_UNUSED(time)

  // FIXME: Write Alpha Over Formula

  // Note that alpha will always be premultiplied by this point

  //GLuint base_tex = base_input_->get_value(time).value<GLuint>();
}
