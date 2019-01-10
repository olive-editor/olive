#include "vsthostwin.h"

// adapted from http://teragonaudio.com/article/How-to-make-your-own-VST-host.html

#include <Windows.h>

#include <QPushButton>
#include <QDialog>
#include <QMessageBox>
#include <QFile>
#include <QXmlStreamWriter>

#include "playback/audio.h"
#include "mainwindow.h"
#include "debug.h"

#define BLOCK_SIZE 512
#define CHANNEL_COUNT 2

// C callbacks
extern "C" {
	// Main host callback
	VstIntPtr VSTCALLBACK hostCallback(AEffect *effect, int opcode, int index, long long value, void *ptr, float opt) {
		switch(opcode) {
		case audioMasterVersion:
			return 2400;
		case audioMasterIdle:
			effect->dispatcher(effect, effEditIdle, 0, 0, 0, 0);
			break;
		case 6: // audioMasterWantMidi
			return 0;
		case audioMasterGetCurrentProcessLevel:
			return 0;
		// Handle other opcodes here... there will be lots of them
		case audioMasterEndEdit: // change made
			mainWindow->setWindowModified(true);
			break;
		default:
			dout << "[INFO] Plugin requested unhandled opcode" << opcode;
			break;
		}
	}
}

// Plugin's entry point
typedef AEffect *(*vstPluginFuncPtr)(audioMasterCallback host);
// Plugin's getParameter() method
typedef float (*getParameterFuncPtr)(AEffect *effect, VstInt32 index);
// Plugin's setParameter() method
typedef void (*setParameterFuncPtr)(AEffect *effect, VstInt32 index, float value);
// Plugin's processEvents() method
typedef VstInt32 (*processEventsFuncPtr)(VstEvents *events);
// Plugin's process() method
typedef void (*processFuncPtr)(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleFrames);

void VSTHostWin::loadPlugin() {
	QString dll_fn = file_field->get_filename(0, true);
	LPCWSTR dll_fn_w = reinterpret_cast<const wchar_t*>(dll_fn.utf16());

	modulePtr = LoadLibrary(dll_fn_w);
	if(modulePtr == NULL) {
		DWORD dll_err = GetLastError();
		dout << "[ERROR] Failed to load VST" << dll_fn_w << "-" << dll_err;
		QString msg_err = "Failed to load VST plugin \"" + dll_fn + "\": " + QString::number(dll_err);
		if (dll_err == 193) {
#ifdef _WIN64
			msg_err += "\n\nNOTE: You can't load 32-bit VST plugins into a 64-bit build of Olive. Please find a 64-bit version of this plugin or switch to a 32-bit build of Olive.";
#elif _WIN32
			msg_err += "\n\nNOTE: You can't load 64-bit VST plugins into a 32-bit build of Olive. Please find a 32-bit version of this plugin or switch to a 64-bit build of Olive.";
#endif
		}
		QMessageBox::critical(mainWindow, "Error loading VST plugin", msg_err);
		return;
	}

	vstPluginFuncPtr mainEntryPoint =
	(vstPluginFuncPtr)GetProcAddress(modulePtr, "VSTPluginMain");
	// Instantiate the plugin
	plugin = mainEntryPoint(hostCallback);
}

void VSTHostWin::freePlugin() {
	if (plugin != NULL) {
		stopPlugin();
		FreeLibrary(modulePtr);
		plugin = NULL;
	}
}

bool VSTHostWin::configurePluginCallbacks() {
	// Check plugin's magic number
	// If incorrect, then the file either was not loaded properly, is not a
	// real VST plugin, or is otherwise corrupt.
	if(plugin->magic != kEffectMagic) {
		dout << "[ERROR] Plugin's magic number is bad";
		QMessageBox::critical(mainWindow, "VST Error", "Plugin's magic number is invalid");
		return false;
	}

	// Create dispatcher handle
	dispatcher = (dispatcherFuncPtr)(plugin->dispatcher);

	// Set up plugin callback functions
	plugin->getParameter = (getParameterFuncPtr)plugin->getParameter;
	plugin->processReplacing = (processFuncPtr)plugin->processReplacing;
	plugin->setParameter = (setParameterFuncPtr)plugin->setParameter;

	return true;
}

void VSTHostWin::startPlugin() {
	dispatcher(plugin, effOpen, 0, 0, NULL, 0.0f);

	// Set some default properties
	dispatcher(plugin, effSetSampleRate, 0, 0, NULL, current_audio_freq());
	dispatcher(plugin, effSetBlockSize, 0, BLOCK_SIZE, NULL, 0.0f);

	resumePlugin();
}

void VSTHostWin::stopPlugin() {
	suspendPlugin();

	dispatcher(plugin, effClose, 0, 0, NULL, 0);
}

void VSTHostWin::resumePlugin() {
	dispatcher(plugin, effMainsChanged, 0, 1, NULL, 0.0f);
}

void VSTHostWin::suspendPlugin() {
	dispatcher(plugin, effMainsChanged, 0, 0, NULL, 0.0f);
}

bool VSTHostWin::canPluginDo(char *canDoString) {
	return (dispatcher(plugin, effCanDo, 0, 0, (void*)canDoString, 0.0f) > 0);
}

void VSTHostWin::initializeIO() {
	// inputs and outputs are assumed to be float** and are declared elsewhere,
	// most likely the are fields owned by this class. numChannels and blocksize
	// are also fields, both should be size_t (or unsigned int, if you prefer).
	inputs = new float* [CHANNEL_COUNT];
	outputs = new float* [CHANNEL_COUNT];
	for(int channel = 0; channel < CHANNEL_COUNT; channel++) {
		inputs[channel] = new float[BLOCK_SIZE];
		outputs[channel] = new float[BLOCK_SIZE];
	}
}

void VSTHostWin::processAudio(long numFrames) {
	// Always reset the output array before processing.
	for (int i=0;i<CHANNEL_COUNT;i++) {
		memset(outputs[i], 0, BLOCK_SIZE*sizeof(float));
	}

	plugin->processReplacing(plugin, inputs, outputs, numFrames);
}

VSTHostWin::VSTHostWin(Clip* c, const EffectMeta *em) : Effect(c, em) {
	plugin = NULL;

	initializeIO();

	file_field = add_row("Plugin", true, false)->add_field(EFFECT_FIELD_FILE, "filename");
	connect(file_field, SIGNAL(changed()), this, SLOT(change_plugin()));

	EffectRow* interface_row = add_row("Interface", false, false);
	show_interface_btn = new QPushButton("Show");
	show_interface_btn->setCheckable(true);
	show_interface_btn->setEnabled(false);
	connect(show_interface_btn, SIGNAL(toggled(bool)), this, SLOT(show_interface(bool)));
	interface_row->add_widget(show_interface_btn);

	dialog = new QDialog(mainWindow);
	dialog->setWindowTitle("VST Plugin");
	dialog->setAttribute(Qt::WA_NativeWindow, true);
	dialog->setWindowFlags(dialog->windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
	connect(dialog, SIGNAL(finished(int)), this, SLOT(uncheck_show_button()));
}

VSTHostWin::~VSTHostWin() {
	freePlugin();
}

void VSTHostWin::process_audio(double timecode_start, double timecode_end, quint8* samples, int nb_bytes, int) {
	if (plugin != NULL) {
		int interval = BLOCK_SIZE*4;
		for (int i=0;i<nb_bytes;i+=interval) {
			int process_size = qMin(interval, nb_bytes - i);
			int lim = i + process_size;

			// convert to float
			for (int j=i;j<lim;j+=4) {
				qint16 left_sample = (qint16) (((samples[j+1] & 0xFF) << 8) | (samples[j] & 0xFF));
				qint16 right_sample = (qint16) (((samples[j+3] & 0xFF) << 8) | (samples[j+2] & 0xFF));

				int index = (j-i)>>2;
				inputs[0][index] = float(left_sample) / float(INT16_MAX);
				inputs[1][index] = float(right_sample) / float(INT16_MAX);
			}

			// send to VST
			processAudio(process_size>>2);

			// convert back to int16
			for (int j=i;j<lim;j+=4) {
				int index = (j-i)>>2;

				qint16 left_sample = qRound(outputs[0][index] * INT16_MAX);
				qint16 right_sample = qRound(outputs[1][index] * INT16_MAX);

				samples[j+3] = (quint8) (right_sample >> 8);
				samples[j+2] = (quint8) right_sample;
				samples[j+1] = (quint8) (left_sample >> 8);
				samples[j] = (quint8) left_sample;
			}
		}
	}
}

void VSTHostWin::custom_load(QXmlStreamReader &stream) {
	if (stream.name() == "plugindata") {
		stream.readNext();
		QByteArray b = QByteArray::fromBase64(stream.text().toUtf8());
		const char* data = b.constData();
		if (plugin != NULL) {
			dispatcher(plugin, effSetChunk, 0, (VstInt32) b.size(), (void*) b.constData(), 0);
		}
	}
}

void VSTHostWin::save(QXmlStreamWriter &stream) {
	Effect::save(stream);
	if (plugin != NULL) {
		char* p = NULL;
		VstInt32 length = dispatcher(plugin, effGetChunk, 0, 0, &p, 0);
		QByteArray b(p, length);
		stream.writeTextElement("plugindata", b.toBase64());
	}
}

void VSTHostWin::show_interface(bool show) {
	dialog->setVisible(show);
}

void VSTHostWin::uncheck_show_button() {
	show_interface_btn->setChecked(false);
}

void VSTHostWin::change_plugin() {
	freePlugin();
	loadPlugin();
	if (plugin != NULL) {
		if (configurePluginCallbacks()) {
			startPlugin();
			dispatcher(plugin, effEditOpen, 0, 0, reinterpret_cast<HWND>(dialog->winId()), 0);
			ERect* eRect = NULL;
			plugin->dispatcher(plugin, effEditGetRect, 0, 0, &eRect, 0);
			dialog->setFixedWidth(eRect->right);
			dialog->setFixedHeight(eRect->bottom);
		} else {
			FreeLibrary(modulePtr);
			plugin = NULL;
		}
	}
	show_interface_btn->setEnabled(plugin != NULL);
}
