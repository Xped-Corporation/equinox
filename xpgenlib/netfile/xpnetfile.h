// This module implements the network file of the XPGENLIB library.
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

#ifndef XPNETFILE_H
#define XPNETFILE_H

#include "xpgenlib-mobile_global.h"

#include <QUrl>
#include <QFile>
#include <QList>
#include <QMutex>
#include <QStringList>
#include <QWaitCondition>

#define XPNETFILE_USE_ETAG
#define XPNETFILE_WEBPREFIX "/data"


class XPGENLIBMOBILESHARED_EXPORT XPNetFile : public QFile
{
    Q_OBJECT

public:
    // These are accessed directly by the worker thread
    QUrl m_url;
    QMutex m_mutex;
    QWaitCondition m_workComplete;

public:
    explicit XPNetFile(const QUrl& url, const QString& localPath, QObject *parent = 0);
    ~XPNetFile();

    bool open(OpenMode flags, int msecs = 30000);
    bool close(int msecs = 30000);
    bool exists(int msecs = 30000);
    //QStringList list(int msecs = 30000);

    QString errorString() { return m_errorString; }
    QString localFilePath() { return m_localPath; }

signals:
    void operate();

private:
    bool putToServer(int msecs = 30000);
    bool validateUrl();

private:
    QString m_localPath;
    bool m_open;
    //
    OpenMode m_flags;
    QString m_errorString;
    //QString m_localFilePath;
};

#endif // XPNETFILE_H
