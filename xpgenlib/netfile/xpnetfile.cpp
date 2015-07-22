// This module defines the network file of the XPGENLIB library.
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

#include <QDir>
#include <QDebug>
#include <QThread>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QEventLoop>

#include "xpprivategetworker.h"
#include "xpprivateputworker.h"
//#include "xpprivatelistworker.h"
#include "xpnetfile.h"

// NOTES:
// 1. Synchronous operation for open() and close().
// 2. The caller will usually setup the URL for HTTP.
// 3. Reading a file is possible after a successful call to open().
// 4. Writing file to server happens after a successful call to close().
// 5. Uses an ETAG to test whether local file is stale.
//

XPNetFile::XPNetFile(const QUrl& url, const QString& localPath, QObject *parent)
    : QFile(parent), m_url(url), m_localPath(localPath), m_open(false)
{
    // Modify the URL to add the webserver prefix
    QString webPrefix = QString(XPNETFILE_WEBPREFIX);
    if (!webPrefix.isEmpty())
    {
        QString origPath = m_url.path();
        m_url.setPath(webPrefix+origPath);
        qDebug() << "XPNetFile::XPNetFile url=" << m_url;
    }
}

XPNetFile::~XPNetFile()
{
}

bool XPNetFile::open(OpenMode flags, int msecs)
{
    m_flags = flags;

    if (flags & QFile::ReadOnly)
    {
        // exists() will fetch the file from the server if necessary
        if (!exists(msecs))
            return false;
    }
    else if (!validateUrl())
    {
        return false;
    }

    // Open file as requested
    setFileName(m_localPath);
    m_open = QFile::open(flags);

    return m_open;
}

bool XPNetFile::close(int msecs)
{
    // Must have opened first
    if (!m_open)
        return true;

    QFile::close();
    m_open = false;

    // ReadOnly operations simply close the file
    if (!(m_flags & QFile::WriteOnly))
        return true;

    // WriteOnly and ReadWrite operations send local file to remote host
    return putToServer(msecs);
}

bool XPNetFile::exists(int msecs)
{
    QFileInfo fi(m_localPath);

#ifndef XPNETFILE_USE_ETAG
    // Return straight away if already cached
    if (fi.exists())
    {
        qDebug() << "Local file exists =" << fi.fileName();
        return true;
    }
#endif

    // Can't continue without a URL
    if (!validateUrl())
    {
        qDebug() << "URL is invalid";
        return false;
    }

    // Create the cache path since it may not exist
    QDir dir;
    if (!dir.mkpath(fi.path()))
    {
        m_errorString = QString("Unable to create cache path=%1").arg( fi.path());
        return false;
    }

    // >>> EXEC REQUEST IN A SEPARATE THREAD TO LET THIS ONE BLOCK
    //
    QThread *workerThread = new QThread;
    XPPrivateGetWorker *worker = new XPPrivateGetWorker(this);
    worker->moveToThread(workerThread);
    connect(this, SIGNAL(operate()), worker, SLOT(doWork()));
    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));

    emit operate();
    workerThread->start();
    //
    // <<< EXEC REQUEST IN A SEPARATE THREAD TO LET THIS ONE BLOCK

    // Wait here for worker to finish
    //
    qDebug() << "XpNetFile exists waiting...";
    //
    QEventLoop loop;
    connect(worker, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    //
    qDebug() << "XpNetFile exists woken";

    m_errorString = worker->errorString();
    bool ok = m_errorString.isEmpty() ? true : false;

    // Get rid of thread and worker
    workerThread->quit();
    worker->deleteLater();

    return ok;
}

bool XPNetFile::putToServer(int msecs)
{
    if (!validateUrl())
    {
        qDebug() << "URL is invalid";
        return true;
    }

    // Make sure file exists and is not empty
    QFileInfo fi(m_localPath);
    if (!fi.exists() || fi.size() == 0)
    {
        qDebug() << "Local file does not exist or is empty";
        return false;
    }

    // >>> EXEC REQUEST IN A SEPARATE THREAD TO LET THIS ONE BLOCK
    //
    QThread *workerThread = new QThread;
    XPPrivatePutWorker *worker = new XPPrivatePutWorker(this);
    worker->moveToThread(workerThread);
    connect(this, SIGNAL(operate()), worker, SLOT(doWork()));
    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));

    emit operate();
    workerThread->start();
    //
    // <<< EXEC REQUEST IN A SEPARATE THREAD TO LET THIS ONE BLOCK

    // Wait here for worker to finish
    //
    qDebug() << "XpNetFile put waiting...";
    //
    m_mutex.lock();
    int ok = m_workComplete.wait(&m_mutex, msecs);
    m_mutex.unlock();
    //
    qDebug() << "XpNetFile put woken";

    if (!ok) // timed out
        m_errorString = "Network transfer timed out";
    else // woken
    {
        m_errorString = worker->errorString();
        ok = m_errorString.isEmpty() ? true : false;
    }

    // Get rid of thread and worker
    workerThread->quit();
    worker->deleteLater();

    return ok;
}

bool XPNetFile::validateUrl()
{
    // Clear last error
    m_errorString = "";

    // Make sure URL is ok
    if (m_localPath.isEmpty() || m_url.isEmpty() || !m_url.isValid())
    {
        m_errorString = "File local path or URL is empty or invalid";
        return false;
    }

    return true;
}

#if 0
QStringList XPNetFile::list(int msecs)
{
    QStringList result;

    // Clear last error
    m_errorString = "";

    // Make sure URL is ok
    if (m_url.isEmpty() || !m_url.isValid())
    {
        m_errorString = "File URL is empty or invalid";
        return result;
    }

    // >>> EXEC REQUEST IN A SEPARATE THREAD TO LET THIS ONE BLOCK
    //
    QThread *workerThread = new QThread;
    XPPrivateListWorker *worker = new XPPrivateListWorker(this);
    worker->moveToThread(workerThread);
    connect(this, SIGNAL(operate()), worker, SLOT(doWork()));
    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));

    emit operate();
    workerThread->start();
    //
    // <<< EXEC REQUEST IN A SEPARATE THREAD TO LET THIS ONE BLOCK

    // Wait here for worker to finish
    //
    qDebug() << "XpNetFile list waiting...";
    //
    m_mutex.lock();
    int ok = m_workComplete.wait(&m_mutex, msecs);
    m_mutex.unlock();
    //
    qDebug() << "XpNetFile list woken";

    if (!ok) // timed out
        m_errorString = "Network transfer timed out";
    else // woken
    {
        m_errorString = worker->errorString();
        if (m_errorString.isEmpty())
            result = worker->listItems();
    }

    // Get rid of thread and worker
    workerThread->quit();
    worker->deleteLater();

    return result;
}
#endif

// End of file
