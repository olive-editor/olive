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

#ifndef VSTHOSTWIN_H
#define VSTHOSTWIN_H

#ifndef NOVST

#include "project/effect.h"

#include "io/crossplatformlib.h"

#include "include/vestige.h"

// Plugin's dispatcher function
typedef intptr_t (*dispatcherFuncPtr)(AEffect *effect, int32_t opCode, int32_t index, int32_t value, void *ptr, float opt);

#include <QDialog>

class VSTHost : public Effect {
  Q_OBJECT
public:
  VSTHost(Clip* c, const EffectMeta* em);
  ~VSTHost();
  void process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int channel_count);

  void custom_load(QXmlStreamReader& stream);
  void save(QXmlStreamWriter& stream);
private slots:
  void show_interface(bool show);
  void uncheck_show_button();
  void change_plugin();
private:
  FileField* file_field;
  ButtonField* show_interface_btn;

  void loadPlugin();
  void freePlugin();
  dispatcherFuncPtr dispatcher;
  AEffect* plugin;
  bool configurePluginCallbacks();
  void startPlugin();
  void stopPlugin();
  void resumePlugin();
  void suspendPlugin();
  bool canPluginDo(char *canDoString);
  void processAudio(long numFrames);
  float** inputs;
  float** outputs;
  QDialog* dialog;
  QByteArray data_cache;

#if defined(__APPLE__)
  CFBundleRef bundle;
#else
  ModulePtr modulePtr;
#endif
};

#endif

#endif // VSTHOSTWIN_H
