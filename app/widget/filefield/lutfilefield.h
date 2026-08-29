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

#ifndef LUTFILEFIELD_H
#define LUTFILEFIELD_H

#include <QComboBox>

#include "filefield.h"

namespace olive
{

/**
 * @brief A FileField with a combo box for picking LUTs from the global LUT
 * library
 *
 * The combo lists every LUT found by LUTLibrary::GetLutFiles() plus an
 * "Other" entry for custom file paths. Picking a library entry fills in the
 * file path (through the regular FilenameChanged signal, so undo keeps
 * working); picking "Other" or entering a path that is not in the library
 * leaves the path untouched and shows the combo's "Other" entry.
 */
class LutFileField : public FileField {
	Q_OBJECT
public:
	LutFileField(QWidget *parent = nullptr);

	virtual void SetFilename(const QString &s) override;

	/**
	 * @brief The combo box listing the LUT library entries
	 *
	 * Exposed for inspection and UI tests; prefer SetFilename()/GetFilename()
	 * for interacting with the field itself.
	 */
	QComboBox *library_combo() const
	{
		return library_combo_;
	}

private:
	/**
	 * @brief Repopulates the combo from the LUT library and syncs the
	 * selection with the current filename
	 */
	void RefreshLibraryEntries();

	QComboBox *library_combo_;
};

}

#endif // LUTFILEFIELD_H
