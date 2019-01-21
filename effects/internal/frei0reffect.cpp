#include "frei0reffect.h"

#ifndef NOFREI0R

#include <QMessageBox>
#include <QDir>

typedef f0r_instance_t (*f0rConstructFunc)(unsigned int width, unsigned int height);
typedef int (*f0rInitFunc) ();
typedef void (*f0rDeinitFunc) ();
typedef void (*f0rUpdateFunc) (f0r_instance_t instance,
						double time, const uint32_t* inframe, uint32_t* outframe);
typedef void (*f0rDestructFunc)(f0r_instance_t instance);
typedef void (*f0rGetPluginInfo)(f0r_plugin_info_t* info);
typedef void (*f0rSetParamValue) (f0r_instance_t instance,
				f0r_param_t param, int param_index);

Frei0rEffect::Frei0rEffect(Clip *c, const EffectMeta *em) : Effect(c, em) {
	enable_image = true;

	// Windows DLL loading routine
	QString dll_fn = QDir(em->path).filePath(em->filename);

	handle = LibLoad(dll_fn);
	if(handle == nullptr) {
		QString dll_error;

#ifdef _WIN32
		DWORD dll_err = GetLastError();
		dll_error = QString::number(dll_err);
#elif __linux__
		dll_error = dlerror();
#endif
		qCritical() << "Failed to load Frei0r plugin" << dll_fn << "-" << dll_error;

		QString msg_err = tr("Failed to load Frei0r plugin \"%1\": %2").arg(dll_fn, dll_error);

#ifdef _WIN32
		if (dll_err == 193) {
#ifdef _WIN64
			msg_err += "\n\n" + tr("NOTE: You can't load 32-bit Frei0r plugins into a 64-bit build of Olive. Please find a 64-bit version of this plugin or switch to a 32-bit build of Olive.");
#elif _WIN32
			msg_err += "\n\n" + tr("NOTE: You can't load 64-bit Frei0r plugins into a 32-bit build of Olive. Please find a 32-bit version of this plugin or switch to a 64-bit build of Olive.");
#endif
		}
#endif

		QMessageBox::critical(nullptr, tr("Error loading Frei0r plugin"), msg_err);

		return;
	}

	f0rInitFunc init = reinterpret_cast<f0rInitFunc>(LibAddress(handle, "f0r_init"));
	init();

	f0rConstructFunc construct = reinterpret_cast<f0rConstructFunc>(LibAddress(handle, "f0r_construct"));
	instance = construct(1920, 1080);

	f0r_plugin_info_t info;
	f0rGetPluginInfo info_func = reinterpret_cast<f0rGetPluginInfo>(LibAddress(handle, "f0r_get_plugin_info"));
	info_func(&info);

	param_count = info.num_params;

//	qDebug() << "Frei0r Name:" << info.name;
//	qDebug() << "Frei0r Param Count:" << info.num_params;
//	qDebug() << "Frei0r Explanation:" << info.explanation;

	get_param_info = reinterpret_cast<f0rGetParamInfo>(LibAddress(handle, "f0r_get_param_info"));
	for (int i=0;i<param_count;i++) {
		f0r_param_info_t param_info;
		get_param_info(&param_info, i);

		if (param_info.type >= 0 && param_info.type <= F0R_PARAM_STRING) {
			EffectRow* row = add_row(param_info.name);
			switch (param_info.type) {
			case F0R_PARAM_BOOL:
				row->add_field(EFFECT_FIELD_BOOL, QString::number(i));
				break;
			case F0R_PARAM_DOUBLE:
			{
				EffectField* f = row->add_field(EFFECT_FIELD_DOUBLE, QString::number(i));
				f->set_double_minimum_value(0);
				f->set_double_maximum_value(100);
			}
				break;
			case F0R_PARAM_COLOR:
				row->add_field(EFFECT_FIELD_COLOR, QString::number(i));
				break;
			case F0R_PARAM_POSITION:
			{
				EffectField* fx = row->add_field(EFFECT_FIELD_DOUBLE, QString("%1X").arg(QString::number(i)));
				fx->set_double_minimum_value(0);
				fx->set_double_maximum_value(100);
				EffectField* fy = row->add_field(EFFECT_FIELD_DOUBLE, QString("%1Y").arg(QString::number(i)));
				fy->set_double_minimum_value(0);
				fy->set_double_maximum_value(100);
			}
				break;
			case F0R_PARAM_STRING:
				row->add_field(EFFECT_FIELD_STRING, QString::number(i));
				break;
			}
		}
	}
}

Frei0rEffect::~Frei0rEffect() {
	if (handle != nullptr) {
		f0rDestructFunc destruct = reinterpret_cast<f0rDestructFunc>(LibAddress(handle, "f0r_destruct"));
		destruct(instance);

		f0rDeinitFunc deinit = reinterpret_cast<f0rDeinitFunc>(LibAddress(handle, "f0r_deinit"));
		deinit();

		LibClose(handle);
	}
}

void Frei0rEffect::process_image(double timecode, uint8_t *input, uint8_t *output, int) {
	f0rUpdateFunc update_func = reinterpret_cast<f0rUpdateFunc>(LibAddress(handle, "f0r_update"));

	for (int i=0;i<param_count;i++) {
		EffectRow* param_row = row(i);

		f0r_param_info_t param_info;
		get_param_info(&param_info, i);

		f0rSetParamValue set_param = reinterpret_cast<f0rSetParamValue>(LibAddress(handle, "f0r_set_param_value"));
		switch (param_info.type) {
		case F0R_PARAM_BOOL:
		{
			double b = param_row->field(0)->get_bool_value(timecode);
			set_param(instance, &b, i);
		}
			break;
		case F0R_PARAM_DOUBLE:
		{
			double d = param_row->field(0)->get_double_value(timecode)*0.01;
			set_param(instance, &d, i);
		}
			break;
		case F0R_PARAM_COLOR:
		{
			QColor qcolor = param_row->field(0)->get_color_value(timecode);;

			f0r_param_color fcolor;
			fcolor.r = float(qcolor.redF());
			fcolor.g = float(qcolor.greenF());
			fcolor.b = float(qcolor.blueF());

			set_param(instance, &fcolor, i);
		}
			break;
		case F0R_PARAM_POSITION:
		{
			f0r_param_position pos;
			pos.x = param_row->field(0)->get_double_value(timecode);
			pos.y = param_row->field(1)->get_double_value(timecode);
			set_param(instance, &pos, i);
		}
			break;
		case F0R_PARAM_STRING:
		{
			QByteArray bytes = param_row->field(0)->get_string_value(timecode).toUtf8();
			char* byte_data = bytes.data();
			set_param(instance, &byte_data, i);
		}
			break;
		}
	}

	update_func(instance, timecode, reinterpret_cast<uint32_t*>(input), reinterpret_cast<uint32_t*>(output));
}

#endif
