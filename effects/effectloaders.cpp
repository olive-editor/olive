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

#include "nodes/oldeffectnode.h"
#include "effects/transition.h"
#include "global/path.h"
#include "panels/panels.h"
#include "panels/effectcontrols.h"
#include "global/config.h"

#include "effects/internal/transformeffect.h"
#include "effects/internal/texteffect.h"
#include "effects/internal/timecodeeffect.h"
#include "effects/internal/solideffect.h"
#include "effects/internal/audionoiseeffect.h"
#include "effects/internal/toneeffect.h"
#include "effects/internal/volumeeffect.h"
#include "effects/internal/paneffect.h"
#include "effects/internal/shakeeffect.h"
#include "effects/internal/cornerpineffect.h"
#include "effects/internal/vsthost.h"
#include "effects/internal/fillleftrighteffect.h"
#include "effects/internal/richtexteffect.h"

#include "effects/internal/crossdissolvetransition.h"
#include "effects/internal/linearfadetransition.h"
#include "effects/internal/logarithmicfadetransition.h"
#include "effects/internal/exponentialfadetransition.h"

#include "nodes/nodes/nodemedia.h"
#include "nodes/nodes/nodetexturepassthru.h"
#include "nodes/nodes/nodeshader.h"

QMutex olive::effects_loaded;

void load_internal_effects() {
  if (!olive::runtime_config.shaders_are_enabled) {
    qWarning() << "Shaders are disabled, some effects may be nonfunctional";
  }

  olive::node_library.resize(kInvalidNode);
  olive::node_library.fill(nullptr);

  olive::node_library[kTransformEffect] = std::make_shared<TransformEffect>(nullptr);
  olive::node_library[kTextInput] = std::make_shared<TextEffect>(nullptr);
  olive::node_library[kSolidInput] = std::make_shared<SolidEffect>(nullptr);
  olive::node_library[kNoiseInput] = std::make_shared<AudioNoiseEffect>(nullptr);
  olive::node_library[kVolumeEffect] = std::make_shared<VolumeEffect>(nullptr);
  olive::node_library[kPanEffect] = std::make_shared<PanEffect>(nullptr);
  olive::node_library[kToneInput] = std::make_shared<ToneEffect>(nullptr);
  olive::node_library[kShakeEffect] = std::make_shared<ShakeEffect>(nullptr);
  olive::node_library[kTimecodeEffect] = std::make_shared<TimecodeEffect>(nullptr);
  olive::node_library[kFillLeftRightEffect] = std::make_shared<FillLeftRightEffect>(nullptr);
  olive::node_library[kVstEffect] = std::make_shared<VSTHost>(nullptr);
  olive::node_library[kCornerPinEffect] = std::make_shared<CornerPinEffect>(nullptr);
  olive::node_library[kRichTextInput] = std::make_shared<RichTextEffect>(nullptr);
  //olive::node_library[kMediaInput] = std::make_shared<NodeMedia>(nullptr);
  //olive::node_library[kImageOutput] = std::make_shared<NodeTexturePassthru>(nullptr);
  olive::node_library[kCrossDissolveTransition] = std::make_shared<CrossDissolveTransition>(nullptr);
  olive::node_library[kLinearFadeTransition] = std::make_shared<LinearFadeTransition>(nullptr);
  olive::node_library[kExponentialFadeTransition] = std::make_shared<ExponentialFadeTransition>(nullptr);
  olive::node_library[kLogarithmicFadeTransition] = std::make_shared<LogarithmicFadeTransition>(nullptr);
}

void load_shader_effects_worker(const QString& effects_path) {
  QDir effects_dir(effects_path);
  if (effects_dir.exists()) {

    QList<QString> entries = effects_dir.entryList({"*.xml"},
                                                   QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    for (int i=0;i<entries.size();i++) {
      QString entry_path = effects_dir.filePath(entries.at(i));
      if (QFileInfo(entry_path).isDir()) {
        load_shader_effects_worker(entry_path);
      } else {
        QString file_url = QDir(effects_path).filePath(entries.at(i));

        QFile file(file_url);

        if (!file.open(QIODevice::ReadOnly)) {
          qCritical() << "Could not open" << entries.at(i);
          return;
        }

        QXmlStreamReader reader(&file);
        while (!reader.atEnd()) {
          if (reader.name() == "effect") {
            QString effect_name;
            QString effect_cat;
            QString effect_id;
            const QXmlStreamAttributes attr = reader.attributes();
            for (int j=0;j<attr.size();j++) {
              if (attr.at(j).name() == "name") {
                effect_name = attr.at(j).value().toString();
              } else if (attr.at(j).name() == "category") {
                effect_cat = attr.at(j).value().toString();
              } else if (attr.at(j).name() == "id") {
                effect_id = attr.at(j).value().toString();
              }
            }
            if (!effect_name.isEmpty() && !effect_id.isEmpty()) {
              olive::node_library.append(std::make_shared<NodeShader>(nullptr,
                                                                      effect_name,
                                                                      effect_id,
                                                                      effect_cat,
                                                                      file_url));
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
