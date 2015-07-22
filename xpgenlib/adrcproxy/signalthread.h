// This module defines the signal thread of the XPGENLIB library.
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

#ifndef SIGNALTHREAD_H_
#define SIGNALTHREAD_H_

#include <QObject>
#include <QString>
#include <QThread>

class SignalThread : public QThread
{
    Q_OBJECT

public:
    SignalThread(const QString& address, quint16 port, QObject *parent = 0);
    ~SignalThread();

signals:
    void connected(QString address, quint16 port);
    void error(int socketError, const QString& message);
    //
    void hostSignal(QString sigxml);

protected:
    virtual void run();

private:
    bool quit;
    //
    QString m_address;
    quint16 m_port;
};

#endif /* SIGNALTHREAD_H_ */
