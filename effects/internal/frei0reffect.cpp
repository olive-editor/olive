#include "frei0reffect.h"

#include <QMessageBox>

#include <Windows.h>

typedef f0r_instance_t (*f0rConstructFunc)(unsigned int width, unsigned int height);
typedef int (*f0rInitFunc) ();
typedef void (*f0rDeinitFunc) ();
typedef void (*f0rUpdateFunc) (f0r_instance_t instance,
						double time, const uint32_t* inframe, uint32_t* outframe);
typedef void (*f0rDestructFunc)(f0r_instance_t instance);

Frei0rEffect::Frei0rEffect(Clip *c, const EffectMeta *em) : Effect(c, em) {
	enable_image = true;

	add_row("Heck");


	// Windows DLL loading routine
	QString dll_fn = "E:/invert0r.dll";
	LPCWSTR dll_fn_w = reinterpret_cast<const wchar_t*>(dll_fn.utf16());

	modulePtr = LoadLibrary(dll_fn_w);
	if(modulePtr == nullptr) {
		DWORD dll_err = GetLastError();
		qCritical() << "Failed to load VST" << dll_fn_w << "-" << dll_err;
		QString msg_err = tr("Failed to load VST plugin \"%1\": %2").arg(dll_fn, QString::number(dll_err));
		if (dll_err == 193) {
#ifdef _WIN64
			msg_err += "\n\n" + tr("NOTE: You can't load 32-bit VST plugins into a 64-bit build of Olive. Please find a 64-bit version of this plugin or switch to a 32-bit build of Olive.");
#elif _WIN32
			msg_err += "\n\n" + tr("NOTE: You can't load 64-bit VST plugins into a 32-bit build of Olive. Please find a 32-bit version of this plugin or switch to a 64-bit build of Olive.");
#endif
		}
		QMessageBox::critical(nullptr, tr("Error loading VST plugin"), msg_err);
		return;
	}

	f0rConstructFunc construct = reinterpret_cast<f0rConstructFunc>(GetProcAddress(modulePtr, "f0r_construct"));

	f0rInitFunc init = reinterpret_cast<f0rInitFunc>(GetProcAddress(modulePtr, "f0r_init"));

	init();

	instance = construct(1920, 1080);
}

Frei0rEffect::~Frei0rEffect() {
	if (modulePtr != nullptr) {
		FreeModule(modulePtr);

		f0rDestructFunc destruct = reinterpret_cast<f0rDestructFunc>(GetProcAddress(modulePtr, "f0r_destruct"));
		destruct(instance);

		f0rDeinitFunc deinit = reinterpret_cast<f0rDeinitFunc>(GetProcAddress(modulePtr, "f0r_deinit"));
		deinit();
	}
}

void Frei0rEffect::process_image(double timecode, uint8_t *input, uint8_t *output, int size) {
	f0rUpdateFunc update_func = reinterpret_cast<f0rUpdateFunc>(GetProcAddress(modulePtr, "f0r_update"));
	update_func(instance, timecode, reinterpret_cast<uint32_t*>(input), reinterpret_cast<uint32_t*>(output));
}
