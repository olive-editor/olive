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

#include "vsthost.h"

// adapted from http://teragonaudio.com/article/How-to-make-your-own-VST-host.html

#include <QPushButton>
#include <QDialog>
#include <QMessageBox>
#include <QFile>
#include <QXmlStreamWriter>
#include <QWindow>

#include "rendering/audio.h"
#include "ui/mainwindow.h"
#include "global/global.h"
#include "global/debug.h"

// Load libraries for retrieving the native window handle. Used for VST plugins that have a separate window
// dedicated to controls.
#if defined(Q_OS_WIN)
#include <Windows.h>
#elif defined(Q_OS_MACOS)
#include <CoreFoundation/CoreFoundation.h>
class NSWindow;
#elif defined(Q_OS_LINUX)
#include <X11/X.h>
#endif

#define BLOCK_SIZE 512

struct VSTRect {
  int16_t top;
  int16_t left;
  int16_t bottom;
  int16_t right;
};

#define effGetChunk 23
#define effSetChunk 24

// C callbacks
extern "C" {
// Main host callback
intptr_t hostCallback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) {
  Q_UNUSED(value)

  switch(opcode) {
  case audioMasterAutomate:
    effect->setParameter(effect, index, opt);
    break;
  case audioMasterVersion:
    return 2400;
  case audioMasterIdle:
    effect->dispatcher(effect, effEditIdle, 0, 0, nullptr, 0);
    break;
  case audioMasterWantMidi:
    // no midi support, return 0
    break;
  case audioMasterGetSampleRate:
    return current_audio_freq();
  case audioMasterGetBlockSize:
    return BLOCK_SIZE;
  case audioMasterGetCurrentProcessLevel:
    // process level happens to be 0
    break;
  case audioMasterGetProductString:
    strcpy(static_cast<char*>(ptr), "OLIVETEAM");
    break;
  case audioMasterBeginEdit:
    // we don't really care about this
    // but we are aware of it
    break;
  case audioMasterEndEdit: // change made
    olive::Global->set_modified(true);
    break;
  default:
    qInfo() << "Plugin requested unhandled opcode" << opcode;
  }
  return 0;
}
}

// Plugin's entry point
typedef AEffect *(*vstPluginFuncPtr)(audioMasterCallback host);
// Plugin's getParameter() method
typedef float (*getParameterFuncPtr)(AEffect *effect, int32_t index);
// Plugin's setParameter() method
typedef void (*setParameterFuncPtr)(AEffect *effect, int32_t index, float value);
// Plugin's processEvents() method
typedef int32_t (*processEventsFuncPtr)(VstEvents *events);
// Plugin's process() method
typedef void (*processFuncPtr)(AEffect *effect, float **inputs, float **outputs, int32_t sampleFrames);

void VSTHost::loadPlugin() {

  QString dll_fn = file_field->GetFileAt(0);

  if (dll_fn.isEmpty()) {
    return;
  }

  // Try to load the plugin
  modulePtr.setFileName(dll_fn);
  if (!modulePtr.load()) {

    // Show an error if the plugin fails to load

    qCritical() << "Failed to load VST plugin" << dll_fn << "-" << modulePtr.errorString();
    QMessageBox::critical(olive::MainWindow,
                          tr("Error loading VST plugin"),
                          tr("Failed to load VST plugin \"%1\": %2").arg(dll_fn, modulePtr.errorString()));
    return;

  }

  // Try to find the VST entry point (first using VSTPluginMain() )
  vstPluginFuncPtr mainEntryPoint = reinterpret_cast<vstPluginFuncPtr>(modulePtr.resolve("VSTPluginMain"));

  if (mainEntryPoint == nullptr) {
    // If there's no VSTPluginMain(), the plugin may use main() instead
    mainEntryPoint = reinterpret_cast<vstPluginFuncPtr>(modulePtr.resolve("main"));
  }

  if (mainEntryPoint == nullptr) {
    QMessageBox::critical(olive::MainWindow,
                          tr("Error loading VST plugin"),
                          tr("Failed to locate entry point for dynamic library."));
    modulePtr.unload();
    return;
  }

  // Instantiate the plugin
  plugin = mainEntryPoint(hostCallback);

}

void VSTHost::freePlugin() {
  if (plugin != nullptr) {
    stopPlugin();
    data_cache.clear();
    modulePtr.unload();
    plugin = nullptr;
  }
}

bool VSTHost::configurePluginCallbacks() {
  // Check plugin's magic number
  // If incorrect, then the file either was not loaded properly, is not a
  // real VST plugin, or is otherwise corrupt.
  if(plugin->magic != kEffectMagic) {
    qCritical() << "Plugin's magic number is bad";
    QMessageBox::critical(olive::MainWindow, tr("VST Error"), tr("Plugin's magic number is invalid"));
    return false;
  }

  // Create dispatcher handle
  dispatcher = reinterpret_cast<dispatcherFuncPtr>(plugin->dispatcher);

  // Set up plugin callback functions
  plugin->getParameter = reinterpret_cast<getParameterFuncPtr>(plugin->getParameter);
  plugin->processReplacing = reinterpret_cast<processFuncPtr>(plugin->processReplacing);
  plugin->setParameter = reinterpret_cast<setParameterFuncPtr>(plugin->setParameter);

  return true;
}

void VSTHost::startPlugin() {
  dispatcher(plugin, effOpen, 0, 0, nullptr, 0.0f);

  // Set some default properties
  dispatcher(plugin, effSetSampleRate, 0, 0, nullptr, current_audio_freq());
  dispatcher(plugin, effSetBlockSize, 0, BLOCK_SIZE, nullptr, 0.0f);

  resumePlugin();
}

void VSTHost::stopPlugin() {
  suspendPlugin();

  dispatcher(plugin, effClose, 0, 0, nullptr, 0);
}

void VSTHost::resumePlugin() {
  dispatcher(plugin, effMainsChanged, 0, 1, nullptr, 0.0f);
}

void VSTHost::suspendPlugin() {
  dispatcher(plugin, effMainsChanged, 0, 0, nullptr, 0.0f);
}

bool VSTHost::canPluginDo(char *canDoString) {
  return (dispatcher(plugin, effCanDo, 0, 0, static_cast<void*>(canDoString), 0.0f) > 0);
}

void VSTHost::CreateDialogIfNull()
{
  if (dialog == nullptr) {
    dialog = new QDialog(olive::MainWindow);
    dialog->setWindowTitle(tr("VST Plugin"));
    dialog->setAttribute(Qt::WA_NativeWindow, true);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
    connect(dialog, SIGNAL(finished(int)), this, SLOT(uncheck_show_button()));
  }
}

void VSTHost::send_data_cache_to_plugin()
{
  dispatcher(plugin, effSetChunk, 0, int32_t(data_cache.size()), static_cast<void*>(data_cache.data()), 0);
}

VSTHost::VSTHost(Clip* c, const EffectMeta *em) :
  Effect(c, em),
  plugin(nullptr),
  dialog(nullptr),
  input_cache(BLOCK_SIZE),
  output_cache(BLOCK_SIZE)
{
  plugin = nullptr;

  EffectRow* file_row = new EffectRow(this, tr("Plugin"), true, false);
  file_field = new FileField(file_row, "filename");
  connect(file_field, SIGNAL(Changed()), this, SLOT(change_plugin()), Qt::QueuedConnection);

  EffectRow* interface_row = new EffectRow(this, tr("Interface"), false, false);

  show_interface_btn = new ButtonField(interface_row, tr("Show"));
  show_interface_btn->SetCheckable(true);
  show_interface_btn->SetEnabled(false);
  connect(show_interface_btn, SIGNAL(Toggled(bool)), this, SLOT(show_interface(bool)));
}

VSTHost::~VSTHost() {
  freePlugin();
}

void VSTHost::process_audio(double timecode_start,
                            double timecode_end,
                            float **samples,
                            int nb_samples,
                            int channel_count,
                            int type) {
  if (plugin != nullptr) {

    // Make copy of audio
    input_cache.Create(channel_count);
    output_cache.Create(channel_count);

    for (int i=0;i<nb_samples;i+=BLOCK_SIZE) {
      int sample_size = qMin(BLOCK_SIZE, nb_samples - i);

      // Copy samples to input cache
      for (int j=0;j<channel_count;j++) {
        memcpy(input_cache.data()[j], &samples[j][i], nb_samples * sizeof(float));
      }

      // send to VST
      plugin->processReplacing(plugin, input_cache.data(), output_cache.data(), sample_size);

      // Copy output cache back to samples
      for (int j=0;j<channel_count;j++) {
        memcpy(&samples[j][i], output_cache.data()[j], nb_samples * sizeof(float));
      }
    }
  }
}

void VSTHost::custom_load(QXmlStreamReader &stream) {
  if (stream.name() == "plugindata") {
    stream.readNext();
    data_cache = QByteArray::fromBase64(stream.text().toUtf8());
    if (plugin != nullptr) {
      send_data_cache_to_plugin();
    }
  }
}

void VSTHost::save(QXmlStreamWriter &stream) {
  Effect::save(stream);
  if (plugin != nullptr) {
    char* p = nullptr;
    int32_t length = int32_t(dispatcher(plugin, effGetChunk, 0, 0, &p, 0));
    data_cache = QByteArray(p, length);
  }
  if (data_cache.size() > 0) {
    stream.writeTextElement("plugindata", data_cache.toBase64());
  }
}

void VSTHost::show_interface(bool show) {
  CreateDialogIfNull();
  dialog->setVisible(show);

  if (show) {
#if defined(Q_OS_WIN)
    dispatcher(plugin, effEditOpen, 0, 0, reinterpret_cast<HWND>(dialog->windowHandle()->winId()), 0);
#elif defined(Q_OS_MACOS)
    dispatcher(plugin, effEditOpen, 0, 0, reinterpret_cast<NSWindow*>(dialog->windowHandle()->winId()), 0);
#elif defined(Q_OS_LINUX) || defined(__HAIKU__)
    dispatcher(plugin, effEditOpen, 0, 0, reinterpret_cast<void*>(dialog->windowHandle()->winId()), 0);
#endif
  } else {
    dispatcher(plugin, effEditClose, 0, 0, nullptr, 0);
  }
}

void VSTHost::uncheck_show_button() {
  show_interface_btn->SetChecked(false);
}

void VSTHost::change_plugin() {
  freePlugin();
  loadPlugin();
  if (plugin != nullptr) {
    if (configurePluginCallbacks()) {
      startPlugin();

      VSTRect* eRect = nullptr;
      plugin->dispatcher(plugin, effEditGetRect, 0, 0, &eRect, 0);

      if (!data_cache.isEmpty()) {
        send_data_cache_to_plugin();
      }

      CreateDialogIfNull();
      dialog->setFixedSize(eRect->right - eRect->left, eRect->bottom - eRect->top);

    } else {

      modulePtr.unload();
      plugin = nullptr;

    }
  }
  show_interface_btn->SetEnabled(plugin != nullptr);
}

SampleCache::SampleCache(int block_size) :
  block_size_(block_size),
  channel_count_(0),
  array_(nullptr)
{
}

SampleCache::~SampleCache()
{
  destroy();
}

void SampleCache::Create(int channels)
{
  if (channel_count_ != channels) {

    if (channel_count_ > 0) {
      destroy();
    }

    channel_count_ = channels;

    array_ = new float* [channel_count_];
    for (int i=0;i<channel_count_;i++) {
      array_[i] = new float[block_size_];
    }
  }
}

void SampleCache::SetZero()
{
  for (int i=0;i<channel_count_;i++) {
    memset(array_[i], 0, block_size_ * sizeof(float));
  }
}

float **SampleCache::data()
{
  return array_;
}

void SampleCache::destroy()
{
  for (int i=0;i<channel_count_;i++) {
    delete [] array_[i];
  }

  delete [] array_;
  array_ = nullptr;
}
