/***

  Oak - Non-Linear Video Editor
  Copyright (C) 2026 Oak Team

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

#include "lutlibrary.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include "config/config.h"

namespace olive
{

bool LUTLibrary::IsSupportedExtension(const QString &suffix)
{
	QString s = suffix;
	if (s.startsWith(QLatin1Char('.'))) {
		s.remove(0, 1);
	}

	const QString lower = s.toLower();
	return lower == QStringLiteral("cube") || lower == QStringLiteral("3dl");
}

QStringList LUTLibrary::GetDirectories()
{
	const QString serialized = OLIVE_CONFIG("LUTLibraryPaths").toString();

	QStringList dirs = serialized.split(QLatin1Char(';'), Qt::SkipEmptyParts);
	for (QString &dir : dirs) {
		dir = QDir::fromNativeSeparators(dir.trimmed());
	}
	return dirs;
}

void LUTLibrary::SetDirectories(const QStringList &dirs)
{
	QStringList cleaned;
	for (const QString &dir : dirs) {
		const QString trimmed = dir.trimmed();
		if (!trimmed.isEmpty() && !cleaned.contains(trimmed)) {
			cleaned.append(trimmed);
		}
	}

	Config::Current()[QStringLiteral("LUTLibraryPaths")] =
		cleaned.join(QLatin1Char(';'));
}

QStringList LUTLibrary::GetLutFiles()
{
	QStringList files;

	static const QStringList kFilters = { QStringLiteral("*.cube"),
										  QStringLiteral("*.3dl") };

	for (const QString &dir : GetDirectories()) {
		QDirIterator it(dir, kFilters, QDir::Files,
						QDirIterator::Subdirectories);
		while (it.hasNext()) {
			files.append(it.next());
		}
	}

	return files;
}

}
