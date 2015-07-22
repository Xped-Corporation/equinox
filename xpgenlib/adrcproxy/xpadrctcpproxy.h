// This module defines the ADRC TCP proxy of the XPGENLIB library.
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

#ifndef ADRCTCPPROXY_H_1279150723
#define ADRCTCPPROXY_H_1279150723

#include <QObject>
#include <QString>

#include "executethread.h"
#include "signalthread.h"


/*
 * Proxy class for network interface to com.xped.adrc.device
 */

class AdrcTcpProxy: public QObject
{
    Q_OBJECT

public:
    AdrcTcpProxy(QString addressAndPort = QString(), QObject *parent = 0);
    ~AdrcTcpProxy();
    //
    bool isValid();
    int getHostPort() { return m_port; }
    QString getHostAddress() { return m_address; }
    QString Execute(const QString& outxml, unsigned long timeoutMs = 5000);

signals:
    void ProximityEvent(QString sigxml);
    void ModelEvent(QString sigxml);
    void NetworkEvent(QString sigxml);
    void AutomationEvent(QString sigxml);
    void DeviceEvent(QString sigxml);
    void ProgressEvent(QString sigxml);
    //
    void proxyOnline(bool online);
    void onlineStateChanged(bool online);

private slots:
    void onHostConnected(QString address, quint16 port);
    void onHostError(int error, QString message);
    void onHostSignal(QString sigxml);
    void onHostFinished();
    void onNetOnlineStateChanged(bool online);
    void onSignalConnected(QString address, quint16 port);
    void onSignalError(int socketError, const QString& message);
    void onSignalFinished();

private: // data
    static SignalThread *signalListner; // this is a singleton
    static int instanceCount; // number of proxy instances
    static QString hostAddress;
    ExecuteThread *executeThread;
    QString m_address;
    int m_port;
};

#endif
