// This module implements the main function of the RML IDE.
//
// Copyright (c) 2015 Xped Holdings Limited <info@xped.com>
//
// This file is part of the Equinox.
//
// This file may be used under the terms of the GNU General Public
// License version 3.0 as published by the Free Software Foundation
// and appearing in the file LICENSE included in the packaging of
// this file. Alternatively you may (at your option) use any later 
// version of the GNU General Public License if such license has been
// publicly approved by Xped Holdings Limited (or its successors,
// if any) and the KDE Free Qt Foundation.
//
// If you are unsure which license is appropriate for your use, please
// contact the sales department at sales@xped.com.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

#include <QDebug>
#include <QApplication>
#include <QErrorMessage>
#include "mainwindow.h"

void reportCriticalErrors(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  fprintf(stderr, "%s\n", msg.toLocal8Bit().data());
  QErrorMessage *m = new QErrorMessage();
  switch(type) {
  case QtCriticalMsg:
  case QtFatalMsg:
    m->showMessage(msg);
    break;
  default:
    break;
  }
}

int main(int argc, char *argv[])
{
    qDebug() << "Equinox V0.0.1 BUILD-0011 $Rev: 3680 $ $LastChangedDate: 2015-07-10 10:08:00 +0930 (Fri, 10 July 2015) $";

    QApplication a(argc, argv);
    qInstallMessageHandler(reportCriticalErrors);
    MainWindow w;
    w.show();

    return a.exec();
}

// end of file
