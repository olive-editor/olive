#ifndef VOIDEFFECT_H
#define VOIDEFFECT_H

/* VoidEffect is a placeholder used when Olive is unable to find an effect
 * requested by a loaded project. It displays a missing effect so the user knows
 * an effect is missing, and stores the XML project data verbatim so that it
 * isn't lost if the user saves over the project.
 */

#include "project/effect.h"

class VoidEffect : public Effect {
public:
    VoidEffect(Clip* c, const QString& n);
    void load(QXmlStreamReader &stream) override;
    void save(QXmlStreamWriter &stream) override;
private:
    QByteArray bytes;
};

#endif // VOIDEFFECT_H
