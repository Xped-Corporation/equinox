// This module defines the private get worker of the XPGENLIB library.
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

#ifndef XPPRIVATEGETWORKER_H
#define XPPRIVATEGETWORKER_H

//#include "xpgenlib-mobile_global.h"

#include <QFile>
#include <QNetworkReply>
#include <QNetworkAccessManager>

#include "xpnetfile.h"


class XPPrivateGetWorker : public QObject
{
    Q_OBJECT

public:
    explicit XPPrivateGetWorker(XPNetFile* controller, QObject *parent = 0)
                : QObject(parent), m_controller(controller) {}

    QString errorString() { return m_errorString; }

signals:
    void finished();

public slots:
    void doWork();

private slots:
    void onReadyRead();
    void onError(QNetworkReply::NetworkError netError);
    void onFinished();

private:
    void wakeController();
#ifdef XPNETFILE_USE_ETAG
    void saveFileEtag(QByteArray& eTag);
    QByteArray recoverFileEtag();
#endif

private:
    XPNetFile* m_controller;
    //
    bool m_readyReadFlag;
    QFile m_file;
    QString m_errorString;
    QNetworkReply* m_netReply;
    QNetworkAccessManager *m_netManager;
};

#endif // XPPRIVATEGETWORKER_H
