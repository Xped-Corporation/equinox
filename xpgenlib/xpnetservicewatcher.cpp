// This module implements the network service watcher of the XPGENLIB library.
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
#include <QTextStream>
#include <QStringList>
#include <QNetworkConfigurationManager>

#ifdef ANDROID
#include <QAndroidJniEnvironment>
#endif

#include "xpnetservicewatcher.h"

#define REGISTER_INTERVAL 5000
#define UNREGISTER_INTERVAL 60000


XPNetServiceWatcher::XPNetServiceWatcher(const QString &name, const QString &protocol, QObject *parent) :
    QObject(parent), m_name(name), m_protocol(protocol)
{
    // Initialize members
    m_serviceFoundState = false;
    discoveryProcess = 0;
#if defined(ANDROID)
    jNsdHelper = 0;
#endif

    // Create discovery timer
    discoveryTimer = new QTimer(this);
    connect(discoveryTimer, SIGNAL(timeout()), this, SLOT(onDiscoveryTimer()));

    // Create network configuration manager
    QNetworkConfigurationManager *manager = new QNetworkConfigurationManager(this);
    connect(manager, SIGNAL(onlineStateChanged(bool)), this, SLOT(onNetOnlineStateChanged(bool)));
    manager->updateConfigurations();

    // Try to go online straight away
    if (manager->isOnline())
    {
        // Trick to allow thread to begin running
        connect(this, SIGNAL(networkOnline(bool)), this, SLOT(onNetOnlineStateChanged(bool)), Qt::QueuedConnection);
        emit networkOnline(true);
    }
}

XPNetServiceWatcher::~XPNetServiceWatcher()
{
    if (discoveryTimer)
        delete discoveryTimer;

#if defined(ANDROID)
    if (jNsdHelper)
    {
        jNsdHelper->callMethod<void>("stopDiscovery");
        jNsdHelper->callMethod<void>("tearDown");
        delete jNsdHelper;
    }
#elif defined(LINUX_PLATFORM)
    if (discoveryProcess)
        delete discoveryProcess;
#endif
}

void XPNetServiceWatcher::onDiscoveryTimer()
{
    qDebug() << "XPNetServiceWatcher::onDiscoveryTimer";

    QString addressAndPort = zeroconfBrowse();

    // Browse in progress
    if (addressAndPort.isEmpty())
        return;

    // Browse got a result
    if (m_serviceFoundState == false)
    {
        if (!addressAndPort.contains("none:none"))
        {
            qDebug() << "XPNetServiceWatcher: host found";

            discoveryTimer->setInterval(UNREGISTER_INTERVAL);
#ifdef ANDROID
            jNsdHelper->callMethod<void>("stopDiscovery");
#endif

            hostAddress = addressAndPort;
            m_serviceFoundState = true;

            emit serviceRegistered(hostAddress);
        }
    }
    else // m_serviceFoundState == true
    {
        if (addressAndPort.contains("none:none"))
        {
            qDebug() << "XPNetServiceWatcher: host disappeared";

            discoveryTimer->setInterval(REGISTER_INTERVAL);
#ifdef ANDROID
            jNsdHelper->callMethod<void>("startDiscovery");
#endif

            m_serviceFoundState = false;
            emit serviceUnregistered(hostAddress);
        }
    }
}

QString XPNetServiceWatcher::zeroconfBrowse()
{
    QString result;

#if defined(ANDROID)
    // Android uses NSD
    QAndroidJniObject oAddressAndPort = jNsdHelper->callObjectMethod<jstring>("getServiceAddressAndPort");
    if (oAddressAndPort.isValid())
        gatewayAddressAndPort = oAddressAndPort.toString();
#elif defined(LINUX_PLATFORM)
    // Linux uses AVAHI
    if (discoveryProcess == 0)
    {
        gatewayAddressAndPort.clear();

        QString program = "avahi-browse";
        QStringList args;
        args << "-prtd" << "local" << m_protocol; // e.g. _rcp._tcp

        discoveryProcess = new QProcess(this);
        connect(discoveryProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onAvahiFinished(int,QProcess::ExitStatus)));
        connect(discoveryProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onAvahiError(QProcess::ProcessError)));
        discoveryProcess->start(program, args);

        return result;
    }
#elif defined(WIN_PLATFORM)
        // Windows uses ?
#elif defined(MAC_PLATFORM)
        // Macintosh uses Bonjour
#endif

    // Determine if the discovery process has found an ADRC service
    if (!gatewayAddressAndPort.isEmpty() || gatewayAddressAndPort == "none:none")
    {
#if defined(LINUX_PLATFORM)
        discoveryProcess->deleteLater();
        discoveryProcess = 0;
#endif

        return gatewayAddressAndPort;
    }

    return result;
}

void XPNetServiceWatcher::onAvahiFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    (void) exitCode;
    (void) exitStatus;

    qDebug() << "XPNetServiceWatcher::onAvahiFinished code=" << exitCode << "status=" << exitStatus;

    // Read output of the process
    QString text = discoveryProcess->readAll();
    if (text.isEmpty())
    {
        //qDebug() << "XPNetServiceWatcher::onAvahiFinished process returned no result";
        gatewayAddressAndPort = "none:none";
        return;
    }

    // Parse the output of the avahi-browse command
    QString address, port;
    QTextStream stream(&text);
    while (!stream.atEnd())
    {
        QString line = stream.readLine();

        if (!line.startsWith("=") || !line.contains("IPv4"))
            continue;

        if (!line.contains(m_name))
            continue;

        // Select the first suitable one (for now)
        QStringList tokens = line.split(";");
        if (tokens.count() >= 8)
        {
            address = tokens[7];
            port = tokens[8];
        }
        break;
    }

    // No service found
    if (address.isEmpty() || port.isEmpty())
    {
        gatewayAddressAndPort = "none:none";
        return;
    }

    // Found a service
    gatewayAddressAndPort = address + ":" + port;
}

void XPNetServiceWatcher::onAvahiError(QProcess::ProcessError error)
{
    qDebug() << "XPNetServiceWatcher::onAvahiError=" << error;
}

void XPNetServiceWatcher::onNetOnlineStateChanged(bool online)
{
    qDebug() << "XPNetServiceWatcher::onNetOnlineStateChanged: online=" << online;

    // If off-line stop network services
    //
    if (online == false)
    {
        discoveryTimer->stop();

#if defined(ANDROID)
        // Stop NSD discovery
        if (jNsdHelper)
        {
            jNsdHelper->callMethod<void>("stopDiscovery");
            jNsdHelper->callMethod<void>("tearDown");
            delete jNsdHelper;
            jNsdHelper = 0;
        }
#elif defined(LINUX_PLATFORM)
        if (discoveryProcess)
        {
            discoveryProcess->deleteLater();
            discoveryProcess = 0;
        }
#endif

        m_serviceFoundState = false;

        if (!hostAddress.isEmpty())
            emit serviceUnregistered(hostAddress);

        return;
    }

    // Otherwise we are on-line start network services
    //
#if defined(ANDROID)
    // Use JNI to get the NSD helper Java class going
    QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
    QAndroidJniObject appctx = activity.callObjectMethod("getApplicationContext","()Landroid/content/Context;");

    // Create a Java NsdHelper object
    jNsdHelper = new QAndroidJniObject("com/xped/navigator/NsdHelper", "(Landroid/content/Context;)V", appctx.object<jobject>());
    if (jNsdHelper->isValid())
    {
        // Initialise the NsdHelper
        QAndroidJniObject oServiceType = QAndroidJniObject::fromString("_rcp._tcp.");
        QAndroidJniObject oServiceName = QAndroidJniObject::fromString("ADRC mobile service");
        jNsdHelper->callMethod<void>("initializeNsd",
                                     "(Ljava/lang/String;Ljava/lang/String;)V",
                                     oServiceType.object<jstring>(),
                                     oServiceName.object<jstring>());

        jNsdHelper->callMethod<void>("startDiscovery");
    }
#endif

    // Start discovery timer
    discoveryTimer->start(REGISTER_INTERVAL);
}

// end of file
