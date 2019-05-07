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

#include <QDialog>
#include <QLibrary>

#include "nodes/oldeffectnode.h"
#include "include/vestige.h"

// Plugin's dispatcher function
typedef intptr_t (*dispatcherFuncPtr)(AEffect *effect, int32_t opCode, int32_t index, int32_t value, void *ptr, float opt);

class SampleCache {
public:
  SampleCache(int block_size);
  ~SampleCache();

  void Create(int channels);
  void SetZero();

  float** data();
private:
  int channel_count_;
  int block_size_;
  float** array_;
  void destroy();
};

class VSTHost : public OldEffectNode {
  Q_OBJECT
public:
  VSTHost(Clip* c);
  virtual ~VSTHost() override;

  virtual QString name() override;
  virtual QString id() override;
  virtual QString description() override;
  virtual EffectType type() override;
  virtual olive::TrackType subtype() override;
  virtual OldEffectNodePtr Create(Clip *c) override;

  virtual void process_audio(double timecode_start,
                             double timecode_end,
                             float **samples,
                             int nb_samples,
                             int channel_count,
                             int type) override;

  virtual void custom_load(QXmlStreamReader& stream) override;
  virtual void save(QXmlStreamWriter& stream) override;
private slots:
  void show_interface(bool show);
  void uncheck_show_button();
  void change_plugin();
private:
  FileInput* file_field;
  ButtonWidget* show_interface_btn;

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
  void CreateDialogIfNull();
  QDialog* dialog;
  QByteArray data_cache;
  SampleCache input_cache;
  SampleCache output_cache;

  void send_data_cache_to_plugin();

  QLibrary modulePtr;
};

#endif // VSTHOSTWIN_H
