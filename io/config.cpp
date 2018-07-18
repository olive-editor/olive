#include "config.h"

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#endif

float scale = 1.0f;
bool vsync = true;
bool hwaccel = false;
int padding = 8;
bool custom_scale = false;
bool saved_layout = false;
bool left_mouse_dominant = true;
bool show_track_lines = false;
bool scroll_zooms = false;

void load_config() {
	/*if (!custom_scale) {
		#ifdef _WIN32
		// Get Windows UI scale - TODO may not be compatible with XP
		HDC screen = GetDC(0);
		int dpiX = GetDeviceCaps (screen, LOGPIXELSX);
		//int dpiY = GetDeviceCaps (screen, LOGPIXELSY);

		scale = (float) dpiX / 96;

		ReleaseDC (0, screen);
		#endif
	}*/

//	padding *= scale;

	/*DIR* dir = opendir("conf");
	if (dir) {
		closedir(dir);
	} else if (ENOENT == errno) {
		// no config directory, assume this is the first time opening Olive
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "INFO", "NOTE: This version of Olive is INCOMPLETE and very likely to crash. It comes with NO WARRANTY or guarantee of functionality.\n\nRegardless, thanks for checking out the project and I hope you enjoy using it!", NULL);
	}*/
}

void save_config() {
	/*#ifdef _WIN32
		_mkdir("conf");
	#else
		mkdir("conf", 0777);
	#endif*/
}
