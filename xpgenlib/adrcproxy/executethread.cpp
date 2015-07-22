// This module implements the execute thread of the XPGENLIB library.
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
#include <QMutexLocker>

#include "executethread.h"


ExecuteThread::ExecuteThread(const QString& address, quint16 port, QObject *parent)
    : QThread(parent)
{
    m_address = address;
    m_port = port;
    quit = false;
    m_waitingForReply = false;
}

ExecuteThread::~ExecuteThread()
{
    qDebug() << "~ExecuteThread()";

    // Wake UI thread if waiting
    m_replyMutex.lock();
    m_replyReady.wakeOne();
    m_replyMutex.unlock();

    // Wake this thread and quit
    m_execMutex.lock();
    quit = true;
    m_execReceived.wakeOne();
    m_execMutex.unlock();

    // Wait until thread exits
    wait();
}

void ExecuteThread::run()
{
    // SETUP THREAD
    //
    QTcpSocket socket;
    socket.connectToHost(m_address, m_port); // read/write
    if (!socket.waitForConnected(5*1000))
    {
        emit error(socket.error(), socket.errorString());
        return;
    }
    socket.setSocketOption(QAbstractSocket::LowDelayOption, QVariant(1));

    emit connected(m_address, m_port);

    m_execMutex.lock();
    //
    QString outxml = m_outxml;
    //
    m_execMutex.unlock();

    // MAIN THREAD LOOP
    //
    while (!quit)
    {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_0);

        QString inxml;
        quint16 blockSize;
        QDataStream in(&socket);
        in.setVersion(QDataStream::Qt_4_0);

        // Send request to host
        //
        // Send the XML host document to the host
        out << (quint16) 0; // Dont know the size yet
        out << outxml;
        out.device()->seek(0); // Seek back to the size
        out << (quint16)(block.size() - sizeof(quint16));
        //
        //qDebug() << "execute write outxml.size=" << outxml.size() << "block.size=" << block.size();
        if (socket.write(block) < 0 || socket.state() != QAbstractSocket::ConnectedState)
        {
            qDebug() << "ExecuteThread::run(1) socket write error=" << socket.errorString();
            emit error(socket.error(), socket.errorString());
            goto thread_reqabort;
        }
        socket.waitForBytesWritten();

        // Wait for host reply
        //
        // Wait for block size from the host
        while (socket.bytesAvailable() < (int)sizeof(quint16))
        {
            if (quit)
                goto thread_exit;

            if (!socket.waitForReadyRead(3*1000))
            {
                if (socket.state() != QAbstractSocket::ConnectedState)
                {
                    qDebug() << "ExecuteThread::run(2) socket error=" << socket.errorString();
                    emit error(socket.error(), socket.errorString());
                    goto thread_reqabort;
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
                    qDebug() << "ExecuteThread::run(3) socket error=" << socket.errorString();
                    emit error(socket.error(), socket.errorString());
                    goto thread_reqabort;
                }
            }
        }
        qDebug() << "socket(2) blockSize=" << blockSize << "bytesAvailable=" << socket.bytesAvailable();

        // Got the block data now
        in >> inxml;

        // Return result to the client
        //
        m_replyMutex.lock();
        //
        while (!m_waitingForReply)
        {
            m_replyMutex.unlock();
            msleep(100); // sleep until client is waiting
            qDebug() << "sleeping...";
            m_replyMutex.lock();

            if (quit)
                goto thread_exit;
        }
        m_inxml = inxml;
        m_replyReady.wakeOne();
        //
        m_replyMutex.unlock();

        // Aborted requests jump here
        //
thread_reqabort:
        if (quit)
            goto thread_exit;

        // Thread waits here for next request
        //
        m_execMutex.lock();
        //
        m_execReceived.wait(&m_execMutex);
        outxml = m_outxml;
        //
        m_execMutex.unlock();

        // Re-estabish the host connection if necessary
        //
        while (socket.state() != QAbstractSocket::ConnectedState)
        {
            socket.close();
            socket.connectToHost(m_address, m_port); // read/write
            if (!socket.waitForConnected(5*1000))
            {
                qDebug() << "ExecuteThread::run(4) socket error=" << socket.errorString();
                emit error(socket.error(), socket.errorString());

                if (quit)
                    goto thread_exit;

                sleep(9); // wait for 9 seconds and try again
                continue;
            }
            else // reconnected
            {
                socket.setSocketOption(QAbstractSocket::LowDelayOption, QVariant(1));
                qDebug() << "ExecuteThread reconnected to host=" << socket.peerAddress() << "port=" << socket.peerPort();
                //emit connected(m_address, m_port);
                break;
            }
        }
    }

thread_exit:
    qDebug() << "ExecuteThread closing socket for exit...";
    socket.disconnectFromHost();
}

void ExecuteThread::executeRequest(const QString &xml)
{
    qDebug() << "Executing request=" << xml;

    QMutexLocker locker(&m_execMutex);

    m_waitingForReply = false;

    m_outxml = xml;

    if (!isRunning())
        start();
    else
        m_execReceived.wakeOne();
}

QString ExecuteThread::waitForReply(int timeout)
{
    QMutexLocker locker(&m_replyMutex);

    // Client does not want to wait for a reply
    if (timeout == 0)
    {
        m_waitingForReply = true;
        qDebug() << "Not waiting for reply";

        return QString();
    }

    qDebug() << "Waiting for reply...";

    m_waitingForReply = true;
    m_replyReady.wait(&m_replyMutex, timeout);
    m_waitingForReply = false;

    QString reply = m_inxml;

    return reply;
}

// End of file
