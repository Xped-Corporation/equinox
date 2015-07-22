// This module implements the private put worker of the XPGENLIB library.
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
#include <QMutex>
#include <QWaitCondition>
#include <QNetworkRequest>

#include "xpprivateputworker.h"

//
// XPPrivatePutWorker
//

void XPPrivatePutWorker::doWork()
{
    m_file.setFileName(m_controller->localFilePath());
    if (!m_file.open(QFile::ReadOnly))
    {
        m_errorString = "Unable to open local file for reading";
        wakeController();
        return;
    }

    // Open a network manager to do the file transfer
    m_netManager = new QNetworkAccessManager(this);
    connect(m_netManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onFinished(QNetworkReply*)));

    // Build the request
    QNetworkRequest request;
    request.setUrl(m_controller->m_url);

    // POST request is asynchronous
    m_netReply = m_netManager->put(request, &m_file);
    connect(m_netReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));

    qDebug() << "Executing PUT request...";
}

void XPPrivatePutWorker::onError(QNetworkReply::NetworkError netError)
{
    m_file.close();
    //QFile::remove(m_controller->localFilePath());

    m_errorString = m_netReply->errorString();
    wakeController();

    qDebug() << ">> onError=" << netError << "msg=" << m_errorString;
}

void XPPrivatePutWorker::onFinished(QNetworkReply *reply)
{
    Q_UNUSED(reply);

    m_file.close();
    wakeController();

    qDebug() << ">> onFinished() file transfer complete!";
}

void XPPrivatePutWorker::wakeController()
{
    m_controller->m_mutex.lock();
    m_controller->m_workComplete.wakeOne();
    m_controller->m_mutex.unlock();
}

// End of file
