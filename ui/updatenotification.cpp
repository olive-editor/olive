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

#include "updatenotification.h"

#include <QNetworkAccessManager>
#include <QStatusBar>

#include "mainwindow.h"

UpdateNotification olive::update_notifier;

UpdateNotification::UpdateNotification()
{

}

void UpdateNotification::check()
{
#if defined(GITHASH) && defined(UPDATEMSG)
  QNetworkAccessManager* manager = new QNetworkAccessManager();

  connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(finished_slot(QNetworkReply *)));
  connect(manager, SIGNAL(finished(QNetworkReply *)), manager, SLOT(deleteLater()));

  QString update_url = QString("http://olivevideoeditor.org/update.php?version=0&hash=%1");

  QNetworkRequest request(QUrl(update_url.arg(GITHASH)));
  manager->get(request);
#endif
}

void UpdateNotification::finished_slot(QNetworkReply *reply)
{
  QString response = QString::fromUtf8(reply->readAll());

  if (response == "1") {
    olive::MainWindow->statusBar()->showMessage(tr("An update is available from the Olive website. "
                                                   "Visit www.olivevideoeditor.org to download it."));
  }
}
