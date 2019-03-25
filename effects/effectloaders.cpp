/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "effectloaders.h"

#include <QDir>
#include <QXmlStreamReader>
#include <QDebug>

#include "effects/effect.h"
#include "effects/transition.h"
#include "global/path.h"
#include "panels/panels.h"
#include "panels/effectcontrols.h"
#include "global/config.h"

QMutex olive::effects_loaded;

void load_internal_effects() {
  if (!olive::CurrentRuntimeConfig.shaders_are_enabled) {
    qWarning() << "Shaders are disabled, some effects may be nonfunctional";
  }

  EffectMeta em;

  // load internal effects
  em.path = ":/internalshaders";

  em.type = EFFECT_TYPE_EFFECT;
  em.subtype = EFFECT_TYPE_AUDIO;

  em.name = "Volume";
  em.internal = EFFECT_INTERNAL_VOLUME;
  olive::effects.append(em);

  em.name = "Pan";
  em.internal = EFFECT_INTERNAL_PAN;
  olive::effects.append(em);

  em.name = "VST Plugin 2.x";
  em.internal = EFFECT_INTERNAL_VST;
  olive::effects.append(em);

  em.name = "Tone";
  em.internal = EFFECT_INTERNAL_TONE;
  olive::effects.append(em);

  em.name = "Noise";
  em.internal = EFFECT_INTERNAL_NOISE;
  olive::effects.append(em);

  em.name = "Fill Left/Right";
  em.internal = EFFECT_INTERNAL_FILLLEFTRIGHT;
  olive::effects.append(em);

  em.subtype = EFFECT_TYPE_VIDEO;

  em.name = "Transform";
  em.category = "Distort";
  em.internal = EFFECT_INTERNAL_TRANSFORM;
  olive::effects.append(em);

  em.name = "Corner Pin";
  em.internal = EFFECT_INTERNAL_CORNERPIN;
  olive::effects.append(em);

  /*em.name = "Mask";
  em.internal = EFFECT_INTERNAL_MASK;
  olive::effects.append(em);*/

  em.name = "Shake";
  em.internal = EFFECT_INTERNAL_SHAKE;
  olive::effects.append(em);

  em.name = "Text";
  em.category = "Render";
  em.internal = EFFECT_INTERNAL_TEXT;
  olive::effects.append(em);

  em.name = "Rich Text";
  em.category = "Render";
  em.internal = EFFECT_INTERNAL_RICHTEXT;
  olive::effects.append(em);

  em.name = "Timecode";
  em.internal = EFFECT_INTERNAL_TIMECODE;
  olive::effects.append(em);

  em.name = "Solid";
  em.internal = EFFECT_INTERNAL_SOLID;
  olive::effects.append(em);

  // internal transitions
  em.type = EFFECT_TYPE_TRANSITION;
  em.category = "";

  em.name = "Cross Dissolve";
  em.internal = TRANSITION_INTERNAL_CROSSDISSOLVE;
  olive::effects.append(em);

  em.subtype = EFFECT_TYPE_AUDIO;

  em.name = "Linear Fade";
  em.internal = TRANSITION_INTERNAL_LINEARFADE;
  olive::effects.append(em);

  em.name = "Exponential Fade";
  em.internal = TRANSITION_INTERNAL_EXPONENTIALFADE;
  olive::effects.append(em);

  em.name = "Logarithmic Fade";
  em.internal = TRANSITION_INTERNAL_LOGARITHMICFADE;
  olive::effects.append(em);
}

void load_shader_effects_worker(const QString& effects_path) {
  QDir effects_dir(effects_path);
  if (effects_dir.exists()) {
    QList<QString> entries = effects_dir.entryList(QStringList("*.xml"), QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    for (int i=0;i<entries.size();i++) {
      QString entry_path = effects_dir.filePath(entries.at(i));
      if (QFileInfo(entry_path).isDir()) {
        load_shader_effects_worker(entry_path);
      } else {
        QFile file(effects_path + "/" + entries.at(i));
        if (!file.open(QIODevice::ReadOnly)) {
          qCritical() << "Could not open" << entries.at(i);
          return;
        }

        QXmlStreamReader reader(&file);
        while (!reader.atEnd()) {
          if (reader.name() == "effect") {
            QString effect_name = "";
            QString effect_cat = "";
            const QXmlStreamAttributes attr = reader.attributes();
            for (int j=0;j<attr.size();j++) {
              if (attr.at(j).name() == "name") {
                effect_name = attr.at(j).value().toString();
              } else if (attr.at(j).name() == "category") {
                effect_cat = attr.at(j).value().toString();
              }
            }
            if (!effect_name.isEmpty()) {
              EffectMeta em;
              em.type = EFFECT_TYPE_EFFECT;
              em.subtype = EFFECT_TYPE_VIDEO;
              em.name = effect_name;
              em.category = effect_cat;
              em.filename = file.fileName();
              em.path = effects_path;
              em.internal = -1;
              olive::effects.append(em);
            } else {
              qCritical() << "Invalid effect found in" << entries.at(i);
            }
            break;
          }
          reader.readNext();
        }

        file.close();
      }
    }
  }
}

void load_shader_effects() {
  QList<QString> effects_paths = get_effects_paths();

  for (int h=0;h<effects_paths.size();h++) {
    const QString& effects_path = effects_paths.at(h);
    load_shader_effects_worker(effects_path);
  }
}

void EffectInit::StartLoading() {
  EffectInit* init_thread = new EffectInit();
  QObject::connect(init_thread, SIGNAL(finished()), init_thread, SLOT(deleteLater()));
  init_thread->start();
}

EffectInit::EffectInit() {
  olive::effects_loaded.lock();
}

void EffectInit::run() {
  qInfo() << "Initializing effects...";
  load_internal_effects();
  load_shader_effects();
  olive::effects_loaded.unlock();
  qInfo() << "Finished initializing effects";
}
