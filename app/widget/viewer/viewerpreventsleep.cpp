/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "viewerpreventsleep.h"

#include <QtGlobal>

#if defined(Q_OS_WINDOWS)
#include <windows.h>
#elif defined(Q_OS_MAC)
#include <IOKit/pwr_mgt/IOPMLib.h>
#elif defined(Q_OS_LINUX)
#include <QtDBus/QtDBus>
#endif

namespace olive
{

#if defined(Q_OS_MAC)
IOPMAssertionID assertionID = 0;
#elif defined(Q_OS_LINUX)

#endif

void PreventSleep(bool on)
{
#if defined(Q_OS_WINDOWS)
	SetThreadExecutionState(on ? ES_DISPLAY_REQUIRED | ES_CONTINUOUS :
								 ES_CONTINUOUS);
#elif defined(Q_OS_MAC)
	if (on) {
		static const CFStringRef reasonForActivity = CFSTR("Video Playback");

		IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
									kIOPMAssertionLevelOn, reasonForActivity,
									&assertionID);
	} else if (assertionID) {
		IOPMAssertionRelease(assertionID);
		assertionID = 0;
	}
#elif defined(Q_OS_LINUX)
	QDBusConnection bus = QDBusConnection::sessionBus();
	if (bus.isConnected()) {
		static const QStringList sleep_services = {
			QStringLiteral("org.freedesktop.ScreenSaver"),
			//QStringLiteral("org.gnome.SessionManager")
		};
		static const QStringList sleep_paths = {
			QStringLiteral("/org/freedesktop/ScreenSaver"),
			//QStringLiteral("/org/gnome/SessionManager")
		};
		static QVector<uint> sleep_cookies;

		// Initialize vector to 0
		if (sleep_cookies.isEmpty()) {
			sleep_cookies.resize(sleep_services.size());
			sleep_cookies.fill(0);
		}

		for (int i = 0; i < sleep_cookies.size(); i++) {
			QDBusInterface interface(sleep_services.at(i), sleep_paths.at(i),
									 sleep_services.at(i), bus);
			if (interface.isValid()) {
				QDBusReply<uint> reply;

				if (on) {
					reply = interface.call(QStringLiteral("Inhibit"),
										   QStringLiteral("Oak Video Editor"),
										   QStringLiteral("Video Playback"));
				} else {
					reply = interface.call(QStringLiteral("UnInhibit"),
										   sleep_cookies.at(i));
				}

				if (reply.isValid()) {
					sleep_cookies[i] = reply.value();
				}
			}
		}
	}
#endif
}

}
