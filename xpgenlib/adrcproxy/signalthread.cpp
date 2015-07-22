// This module implements the signal thread of the XPGENLIB library.
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
#include <QTcpSocket>
#include <QDataStream>
#include <QHostAddress>

#include "signalthread.h"

// NOTES:
// 1. Try making run wait using a wait condition and using readRead() signal to wake it.
//

SignalThread::SignalThread(const QString& address, quint16 port, QObject *parent)
    : QThread(parent)
{
    m_address = address;
    m_port = port;
    quit = false;
}

SignalThread::~SignalThread()
{
    qDebug() << "~SignalThread";

    quit = true;

    // Wait until thread exits
    wait();
}

void SignalThread::run()
{
    // SETUP THREAD
    //
    QTcpSocket socket;
    socket.connectToHost(m_address, m_port); // read/write
    if (!socket.waitForConnected(5*1000))
    {
        qDebug() << "SignalThread::run(1) socket error=" << socket.errorString();
        emit error(socket.error(), socket.errorString());
        return;
    }
    socket.setSocketOption(QAbstractSocket::LowDelayOption, QVariant(1));

    qDebug() << "SignalThread connected to host=" << socket.peerAddress() << "port=" << socket.peerPort();
    emit connected(m_address, m_port);

    // MAIN THREAD LOOP
    //
    while (!quit)
    {
        // Wait for data from signal server on host
        //
        QString inxml;
        quint16 blockSize;
        QDataStream in(&socket);
        in.setVersion(QDataStream::Qt_4_0);

        // Wait for block size from the host
        while (socket.bytesAvailable() < (int)sizeof(quint16))
        {
            if (quit)
                goto thread_exit;

            if (!socket.waitForReadyRead(3*1000))
            {
                if (socket.state() != QAbstractSocket::ConnectedState)
                {
                    qDebug() << "SignalThread::run(2) socket error=" << socket.errorString();
                    emit error(socket.error(), socket.errorString());
                    goto thread_reconnect;
                }
            }
        }
        qDebug() << "socket(1) headerSize=" << sizeof(quint16) << "bytesAvailable=" << socket.bytesAvailable();

        // Got the block size reply
        in >> blockSize;

        // Now wait for the block data to come in
        while (socket.bytesAvailable() < blockSize)
        {
            if (quit)
                goto thread_exit;

            if (!socket.waitForReadyRead(3*1000))
            {
                if (socket.state() != QAbstractSocket::ConnectedState)
                {
                    qDebug() << "SignalThread::run(3) socket error=" << socket.errorString();
                    emit error(socket.error(), socket.errorString());
                    goto thread_reconnect;
                }
            }
        }
        qDebug() << "socket(2) blockSize=" << blockSize << "bytesAvailable=" << socket.bytesAvailable();

        // Got the block data now
        in >> inxml;

        // Emit the host signal
        //
        emit hostSignal(inxml);
        continue;

thread_reconnect:
        // Re-estabish the host connection if necessary
        while (socket.state() != QAbstractSocket::ConnectedState)
        {
            socket.close();
            socket.connectToHost(m_address, m_port); // read/write
            if (!socket.waitForConnected(5*1000))
            {
                qDebug() << "SignalThread::run(4) socket error=" << socket.errorString();
                emit error(socket.error(), socket.errorString());

                if (quit)
                    goto thread_exit;

                sleep(9); // wait for 9 seconds and try again
                continue;
            }
            else // reconnected
            {
                socket.setSocketOption(QAbstractSocket::LowDelayOption, QVariant(1));
                qDebug() << "SignalThread reconnected to host=" << socket.peerAddress() << "port=" << socket.peerPort();
                //emit connected(m_address, m_port);
                break;
            }
        }
    }

thread_exit:
    qDebug() << "SignalThread closing socket for exit...";
    socket.disconnectFromHost();
}

// End of file
