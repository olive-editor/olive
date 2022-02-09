/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef SEARCHTEXTBAR_H
#define SEARCHTEXTBAR_H

#include <QWidget>
#include <QTextDocument>

class QTimer;
class QCheckBox;
class QLineEdit;

namespace olive {

class SearchTextBar : public QWidget
{
  Q_OBJECT

public:
  explicit SearchTextBar(QWidget *parent = nullptr);
  ~SearchTextBar() override;

  /** make seach bar visible and set focus */
  void activateBar();

public slots:
  void onTextNotFound();
  void showBar(const QString &search_text);

signals:
  void searchTextForward( const QString & text, QTextDocument::FindFlags flgs);
  void searchTextBackward( const QString & text, QTextDocument::FindFlags flgs);
  // thenFind: when TRUE, replace and then find
  void replaceText( const QString & src, const QString & dest,
                    QTextDocument::FindFlags flags, bool thenFind);

private:
  QTimer * timer_;
  QCheckBox * case_sensitive_box_;
  QCheckBox * whole_word_box_;
  QLineEdit * find_edit_;
  QLineEdit * replace_edit_;

  // QWidget interface
protected:
  void keyReleaseEvent(QKeyEvent * event) override;

private slots:
  void onTimeoutExpired();
  void onHideButtonClicked();
  void onPrevButtonClicked();
  void onNextButtonClicked();
  void onReplaceButtonCLicked();
  void onReplaceAndFindButtonCLicked();

private:
  QTextDocument::FindFlags getCurrentFlags();
};

}  // namespace olive

#endif // SEARCHTEXTBAR_H
