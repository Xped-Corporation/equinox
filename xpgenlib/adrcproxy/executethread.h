// This module defines the execute thread of the XPGENLIB library.
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

#ifndef EXECUTETHREAD_H_
#define EXECUTETHREAD_H_

#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

class ExecuteThread : public QThread
{
    Q_OBJECT

public:
    ExecuteThread(const QString& address, quint16 port, QObject *parent = 0);
    ~ExecuteThread();
    //
    void executeRequest(const QString& xml);
    QString waitForReply(int timeout = 30000);

signals:
    void connected(QString address, quint16 port);
    void error(int socketError, const QString& message);

protected:
    virtual void run();

private:
    bool quit;
    //
    QMutex m_execMutex;
    QWaitCondition m_execReceived;
    //
    QMutex m_replyMutex;
    QWaitCondition m_replyReady;
    bool m_waitingForReply;
    //
    QString m_address;
    quint16 m_port;
    QString m_inxml;
    QString m_outxml;
};

#endif /* EXECUTETHREAD_H_ */
