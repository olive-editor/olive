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

#include "searchtextbar.h"

#include <QKeyEvent>
#include <QTimer>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QSpacerItem>


namespace {
// duration of visual feedback when text is not found
const uint32_t TEXT_NOT_FOUND_FLASH_DURATION_ms = 600u;
}

namespace olive {

SearchTextBar::SearchTextBar(QWidget *parent) :
  QWidget(parent),
  timer_( new QTimer(this))
{
  // build GUI
  QGridLayout * main_layout = new QGridLayout();
  setLayout( main_layout);
  main_layout->setSpacing(2);

  QLabel * find_label = new QLabel(tr("Find:"), this);
  find_edit_ = new QLineEdit( this);
  find_label->setAlignment( Qt::AlignLeft);
  find_label->setBuddy( find_edit_);

  case_sensitive_box_ = new QCheckBox( "Aa", this);
  case_sensitive_box_->setToolTip(tr("case sensitive"));
  whole_word_box_ = new QCheckBox( "[]", this);
  whole_word_box_->setToolTip(tr("whole words only"));

  QPushButton * find_forward_button = new QPushButton( ">>", this);
  QPushButton * find_backword_button = new QPushButton( "<<", this);
  QPushButton * hide_button = new QPushButton( "x", this);

  find_edit_->setSizePolicy( QSizePolicy::Expanding , QSizePolicy::Preferred);
  main_layout->addWidget( find_label, 0,0,1,1);
  main_layout->addWidget( find_edit_, 0,1,1,1);
  main_layout->addWidget( case_sensitive_box_, 0,2,1,1);
  main_layout->addWidget( whole_word_box_, 0,3,1,1);
  main_layout->addWidget( find_backword_button, 0,4,1,1);
  main_layout->addWidget( find_forward_button, 0,5,1,1);
  main_layout->addItem( new QSpacerItem(10,10, QSizePolicy::Minimum, QSizePolicy::Minimum), 0,6,1,1);
  main_layout->addWidget( hide_button, 0,7,1,1);

  QLabel * replace_label = new QLabel(tr("Replace:"), this);
  replace_edit_ = new QLineEdit( this);
  replace_label->setAlignment( Qt::AlignLeft);
  replace_label->setBuddy( replace_edit_);

  QPushButton * replace_button = new QPushButton(tr("replace"), this);
  QPushButton * replace_and_find_button = new QPushButton(tr("replace & find"), this);

  replace_edit_->setSizePolicy( QSizePolicy::Expanding , QSizePolicy::Preferred);
  main_layout->addWidget( replace_label, 1,0,1,1);
  main_layout->addWidget( replace_edit_, 1,1,1,1);
  main_layout->addWidget( replace_button, 1,2,1,2);
  main_layout->addWidget( replace_and_find_button, 1,4,1,2);

  connect( find_backword_button, & QPushButton::clicked, this, & SearchTextBar::onPrevButtonClicked);
  connect( find_forward_button, & QPushButton::clicked, this, & SearchTextBar::onNextButtonClicked);
  connect( hide_button, & QPushButton::clicked, this, & SearchTextBar::onHideButtonClicked);
  connect( replace_button, & QPushButton::clicked, this, & SearchTextBar::onReplaceButtonCLicked);
  connect( replace_and_find_button, & QPushButton::clicked, this, & SearchTextBar::onReplaceAndFindButtonCLicked);

  find_edit_->setTabOrder( find_edit_, replace_edit_);

  // visual feedback timer
  connect( timer_, SIGNAL(timeout()), this, SLOT(onTimeoutExpired()));
}

SearchTextBar::~SearchTextBar()
{
}

void SearchTextBar::activateBar()
{
  setVisible( true);
  find_edit_->setFocus();
}

void SearchTextBar::onTextNotFound()
{
  // give a visual feedback that text can't be found
  find_edit_->setStyleSheet("background-color: red;");
  timer_->start( TEXT_NOT_FOUND_FLASH_DURATION_ms);
}

void SearchTextBar::showBar( const QString & search_text)
{
  show();
  find_edit_->setFocus();
  find_edit_->setText( search_text);
}

void SearchTextBar::onTimeoutExpired()
{
  // end of visual feedback
  find_edit_->setStyleSheet("");
  timer_->stop();
}

void SearchTextBar::keyReleaseEvent(QKeyEvent * event)
{
  QWidget::keyReleaseEvent( event);

  // enter or return key search forward; use shift to search backward
  if ((event->key() == Qt::Key_Enter) ||
      (event->key() == Qt::Key_Return))
  {
    QTextDocument::FindFlags flags = getCurrentFlags();

    if (event->modifiers() & Qt::ShiftModifier)
    {
      emit searchTextBackward( find_edit_->text(), flags);
    }
    else
    {
      emit searchTextForward( find_edit_->text(), flags);
    }
  }
  else if (event->key() == Qt::Key_Escape)
  {
    hide();
  }
}


void SearchTextBar::onHideButtonClicked()
{
  hide();
}

void SearchTextBar::onPrevButtonClicked()
{
  QTextDocument::FindFlags flags = getCurrentFlags();
  emit searchTextForward( find_edit_->text(), flags);
}

void SearchTextBar::onNextButtonClicked()
{
  QTextDocument::FindFlags flags = getCurrentFlags();
  emit searchTextBackward( find_edit_->text(), flags);
}

void SearchTextBar::onReplaceButtonCLicked()
{
  QTextDocument::FindFlags flags = getCurrentFlags();
  emit replaceText( find_edit_->text(), replace_edit_->text(), flags, false);
}

void SearchTextBar::onReplaceAndFindButtonCLicked()
{
  QTextDocument::FindFlags flags = getCurrentFlags();
  emit replaceText( find_edit_->text(), replace_edit_->text(), flags, false);
}

QTextDocument::FindFlags SearchTextBar::getCurrentFlags()
{
  // default constructor sets to 0
  QTextDocument::FindFlags flags;

  if (case_sensitive_box_->isChecked()) {
    flags |= QTextDocument::FindCaseSensitively;
  }

  if (whole_word_box_->isChecked()) {
    flags |= QTextDocument::FindWholeWords;
  }

  return flags;
}

}  // namespace olive
