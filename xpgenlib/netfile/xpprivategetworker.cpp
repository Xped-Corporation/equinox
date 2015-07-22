// This module implements the private get worker of the XPGENLIB library.
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
#include <QSettings>
#include <QFileInfo>
#include <QWaitCondition>
#include <QNetworkRequest>

#include "xpprivategetworker.h"

//
// XPPrivateGetWorker
//

void XPPrivateGetWorker::doWork()
{
    m_readyReadFlag = false;

    // Backup original file
    m_file.setFileName(m_controller->localFilePath());
    QFile::remove(m_controller->localFilePath()+"~");
    QFile::copy(m_controller->localFilePath(), m_controller->localFilePath()+"~");

    // Open a network manager to do the file transfer
    m_netManager = new QNetworkAccessManager(this);

    // Build the request
    QNetworkRequest request;
    request.setUrl(m_controller->m_url);

    qDebug() << "Executing GET request =" << m_file.fileName();

#ifdef XPNETFILE_USE_ETAG
    // Get eTag from the tag cache file
    QByteArray eTag = recoverFileEtag();
    if (!eTag.isEmpty() && m_file.exists())
    {
        // Add "If-None-Match" header
        request.setRawHeader(QString("If-None-Match").toLatin1(), eTag);
        qDebug() << ">> onDoWork() if-none-match etag=" << eTag;
    }
#endif

    // GET request is asynchronous
    m_netReply = m_netManager->get(request);
    connect(m_netReply, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(m_netReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));
    connect(m_netReply, SIGNAL(finished()), this, SLOT(onFinished()));
}

void XPPrivateGetWorker::onReadyRead()
{
    qDebug() << ">> onReadyRead() bytesAvailable=" << m_netReply->bytesAvailable();
    m_readyReadFlag = true;

    // Open the file for writing
    if (!m_file.isOpen())
        m_file.open(QFile::WriteOnly);

    // write available data to the local file
    if (m_file.isOpen())
        m_file.write(m_netReply->readAll());
}

void XPPrivateGetWorker::onError(QNetworkReply::NetworkError netError)
{
    // Close file
    if (m_file.isOpen())
        m_file.close();

    // Remove downloaded file and rename original file
    m_file.remove();
    QFile::rename(m_controller->localFilePath()+"~", m_controller->localFilePath());

    m_errorString = m_netReply->errorString();

    wakeController();

    qDebug() << ">> onError=" << netError << "msg=" << m_errorString;
}

void XPPrivateGetWorker::onFinished()
{
    // Close the file
    if (m_file.isOpen())
        m_file.close();

    // If no data downloaded restore original file from backup
    //if (m_readyReadFlag == false)
    //    QFile::rename(m_controller->localFilePath()+"~", m_controller->localFilePath());

#ifdef XPNETFILE_USE_ETAG
    //qDebug() << ">> " << m_netReply->rawHeaderList();

    // Get the ETag header from the reply and save it
    QByteArray eTag = m_netReply->rawHeader(QByteArray("ETag"));
    if (!eTag.isEmpty())
        saveFileEtag(eTag);
#endif

    wakeController();

    qDebug() << ">> onFinished() transfer complete!";
}

void XPPrivateGetWorker::wakeController()
{
    emit finished();
}

#ifdef XPNETFILE_USE_ETAG
//
// ETags saved in ".xpetagcache" file in same folder as target file
//

void XPPrivateGetWorker::saveFileEtag(QByteArray &eTag)
{
    QFileInfo fi(m_file.fileName());
    QString basePath = fi.filePath().remove(fi.fileName());
    QString key = QString("%1/etag").arg(fi.fileName());
    QSettings etagFile(basePath+"/.xpetagcache", QSettings::IniFormat);

    etagFile.setValue(key, eTag);

    //qDebug() << "XPPrivateGetWorker::saveFileEtag() name=" << fi.fileName() << "etag=" << eTag;
}

QByteArray XPPrivateGetWorker::recoverFileEtag()
{
    QFileInfo fi(m_file.fileName());
    QString basePath = fi.filePath().remove(fi.fileName());
    QString key = QString("%1/etag").arg(fi.fileName());
    QSettings etagFile(basePath+"/.xpetagcache", QSettings::IniFormat);

    return etagFile.value(key).toByteArray();

    //qDebug() << "XPPrivateGetWorker::wakeController name=" << fi.fileName() << "base=" << basePath;
}
#endif

// End of file
