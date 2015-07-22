// This module defines the network service watcher of the XPGENLIB library.
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

#ifndef XPNETSERVICEWATCHER_H
#define XPNETSERVICEWATCHER_H

#include <QString>
#include <QTimer>
#include <QProcess>

#ifdef ANDROID
#include <QAndroidJniObject>
#endif


class XPNetServiceWatcher : public QObject
{
    Q_OBJECT

public:
    explicit XPNetServiceWatcher(const QString& name, const QString& protocol, QObject *parent = 0);
    ~XPNetServiceWatcher();
    
signals:
    void serviceRegistered(QString serviceName);
    void serviceUnregistered(QString serviceName);
    void networkOnline(bool online);

private slots:
    void onDiscoveryTimer();
    void onAvahiFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onAvahiError(QProcess::ProcessError error);
    void onNetOnlineStateChanged(bool online);

private: // methods
    QString zeroconfBrowse();

private: // data
    QString m_name;
    QString m_protocol;
    bool m_serviceFoundState;
    //
    QString hostAddress;
    QTimer *discoveryTimer;
    QProcess *discoveryProcess;
    QString gatewayAddressAndPort;
    //
#ifdef ANDROID
    QAndroidJniObject *jNsdHelper;
#endif
};

#endif // XPNETSERVICEWATCHER_H
