// This module implements the ADRC TCP proxy of the XPGENLIB library.
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
#include <QStringList>
#include <QNetworkConfigurationManager>

#include "xpadrctcpproxy.h"


/* Static class variables
 */

QString AdrcTcpProxy::hostAddress;
int AdrcTcpProxy::instanceCount = 0;
SignalThread *AdrcTcpProxy::signalListner = 0;


AdrcTcpProxy::AdrcTcpProxy(QString addressAndPort, QObject *parent)
    : QObject(parent)
{
    instanceCount++;
    executeThread = 0;

    // Extract the host IP address and port
    QStringList tokens = addressAndPort.split(QChar(':'));
    if (tokens.length() != 2)
        return;

    // Save our address details
    m_address = tokens.at(0);
    m_port = tokens.at(1).toInt();
    hostAddress = addressAndPort;

    // Start a network configuration manager
    QNetworkConfigurationManager *manager = new QNetworkConfigurationManager(this);
    connect(manager, SIGNAL(onlineStateChanged(bool)), this, SLOT(onNetOnlineStateChanged(bool)));
    manager->updateConfigurations();

    // Try to go online straight away
    if (manager->isOnline())
    {
        // Trick to allow thread to begin running
        connect(this, SIGNAL(onlineStateChanged(bool)), this, SLOT(onNetOnlineStateChanged(bool)), Qt::QueuedConnection);
        emit onlineStateChanged(true);
    }
}

AdrcTcpProxy::~AdrcTcpProxy()
{
    qDebug() << "~AdrcTcpProxy";

    instanceCount--;

    if (executeThread)
        delete executeThread;

    if (instanceCount == 0 && signalListner)
    {
        delete signalListner;
        signalListner = 0;
    }
}

bool AdrcTcpProxy::isValid()
{
    //return (!hostAddress.isEmpty() && instanceCount > 0 && signalListner);
    return (!hostAddress.isEmpty() && signalListner);
}

QString AdrcTcpProxy::Execute(const QString &outxml, unsigned long timeoutMs)
{
    if (!isValid())
    {
        qDebug() << "AdrcTcpProxy::Execute error network is off-line";
        return QString();
    }

    if (executeThread == 0)
    {
        executeThread = new ExecuteThread(m_address, m_port, this);
        connect(executeThread, SIGNAL(finished()), this, SLOT(onHostFinished()));
        connect(executeThread, SIGNAL(error(int,QString)), this, SLOT(onHostError(int,QString)));
        connect(executeThread, SIGNAL(connected(QString,quint16)), this, SLOT(onHostConnected(QString,quint16)));
    }

    executeThread->executeRequest(outxml);
    QString inxml = executeThread->waitForReply(timeoutMs);
    qDebug() << "host reply=" << inxml;

    return inxml;
}

void AdrcTcpProxy::onHostConnected(QString address, quint16 port)
{
    qDebug() << "AdrcTcpProxy: connected to exec service=" << address
             << "at port=" << port;
}

void AdrcTcpProxy::onHostError(int error, QString message)
{
    qDebug() << "AdrcTcpProxy: exec service error=" << error
             << "message=" << message;
}

void AdrcTcpProxy::onHostFinished()
{
    qDebug() << "AdrcTcpProxy: exec service finished";

    if (executeThread)
    {
        executeThread->deleteLater();
        executeThread = 0;
    }
}

void AdrcTcpProxy::onNetOnlineStateChanged(bool online)
{
    qDebug() << "AdrcTcpProxy::onNetOnlineStateChanged: online=" << online;

    // If offline stop services
    //
    if (online == false)
    {
        qDebug() << "AdrcTcpProxy: stopped listening due to network offline";

        if (executeThread)
        {
            delete executeThread;
            executeThread = 0;
        }

        if (signalListner)
        {
            delete signalListner;
            signalListner = 0;
        }

        emit proxyOnline(false);
        return;
    }

    // If online start services
    //
#if 0
    if (executeThread)
    {
        delete executeThread;
        executeThread = 0;
    }
#endif

    // Create only one signal listner (singleton)
    if (signalListner == 0)
    {
        signalListner = new SignalThread(m_address, m_port+1);
        //connect(signalListner, SIGNAL(hostSignal(QString)), this, SLOT(onHostSignal(QString)));
        connect(signalListner, SIGNAL(connected(QString,quint16)), this, SLOT(onSignalConnected(QString,quint16)));
        connect(signalListner, SIGNAL(error(int,QString)), this, SLOT(onSignalError(int,QString)));
        connect(signalListner, SIGNAL(finished()), this, SLOT(onSignalFinished()));
        //
        signalListner->start();
    }
    connect(signalListner, SIGNAL(hostSignal(QString)), this, SLOT(onHostSignal(QString)));

    emit proxyOnline(true);
}

void AdrcTcpProxy::onHostSignal(QString sigxml)
{
    qDebug() << "AdrcTcpProxy::onHostSignal sigxml=" << sigxml;

    // Process host XML and emit corresponding ADRC signals
    if (sigxml.contains(">/"))
        emit ModelEvent(sigxml);
    else if (sigxml.contains(">proximity"))
        emit ProximityEvent(sigxml);
    else if (sigxml.contains(">progress"))
        emit ProgressEvent(sigxml);
    else if (sigxml.contains(">device"))
        emit DeviceEvent(sigxml);
}

void AdrcTcpProxy::onSignalConnected(QString address, quint16 port)
{
    qDebug() << "AdrcTcpProxy: connected to signal service=" << address
             << "on port=" << port
             << "refs=" << instanceCount;
}

void AdrcTcpProxy::onSignalError(int error, const QString& message)
{
    qDebug() << "AdrcTcpProxy: signal service error=" << error
             << "message=" << message;
}

void AdrcTcpProxy::onSignalFinished()
{
    qDebug() << "AdrcTcpProxy: signal service finished";

    if (signalListner)
    {
        signalListner->deleteLater();
        signalListner = 0;
    }
}

// end of file
