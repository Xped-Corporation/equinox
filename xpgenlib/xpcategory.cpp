// This module implements the category class of the XPGENLIB library.
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

#include <QUrl>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QFileInfoList>
#include <QXmlStreamReader>

#include <xpnetfile.h>
#include "xpcategory.h"

XPCategory::XPCategory(const QString &rmlCachePath, const QString &hostAddress, QObject *parent) :
    QObject(parent), rmlCachePath(rmlCachePath), hostAddress(hostAddress)
{
    m_valid = false;

    indexFilePath = QString("%1/categories/categories.index").arg(rmlCachePath);

    // Get category index file from HUB (webroot is /var/cache/xped)
    if (!hostAddress.isEmpty())
    {
        QUrl url = QUrl(QString("http://%1/categories/categories.index").arg(hostAddress));
        XPNetFile indexFile(url, indexFilePath);
        if( !indexFile.exists())
        {
            qDebug() << "XPCategory::getCategoryIcon categories.index file not found";
            return;
        }
    }

    // Get category metadata
    QSettings ini(indexFilePath, QSettings::IniFormat);
    m_theme = ini.value("Category/Theme", "default").toString();
    m_comment = ini.value("Category/Comment", "System default theme").toString();
    QStringList sizes = ini.value("Category/Sizes").toStringList();
    for (int i=0, n=sizes.count(); i<n; i++)
        m_sizes.append(sizes.at(i).toInt());

    m_valid = true;
}

QIcon XPCategory::getIcon(const QString& category, int size)
{
    if (!m_valid)
        return QIcon(":images/document-close.svg");

    // Find the entry for 'category' in the group for size
    QString group = QString("%1x%1").arg(size);
    QSettings ini(indexFilePath, QSettings::IniFormat);
    QStringList values = ini.value(QString("%1/%2").arg(group).arg(category),
                                   QString("%1/0000").arg(group)).toStringList();
    if (values.count() != 2)
    {
        qDebug() << "XPCategory::getIcon category code not found";
        return QIcon(":images/unknown-device.png"); // unknown device icon
    }

    // Get category icon file from HUB
    QString iconPath = QString("categories/default/%1/%2").arg(group).arg(values[1]);
    QUrl url = QUrl(QString("http://%1/%2").arg(hostAddress).arg(iconPath));

    XPNetFile iconFile(url, QString("%1/%2").arg(rmlCachePath).arg(iconPath));
    if (!iconFile.exists())
    {
        qDebug() << "XPCategory::getIcon icon file not found";
        return QIcon(":images/document-close.svg"); // error icon
    }

    return QIcon(QString("%1/%2").arg(rmlCachePath).arg(iconPath));
}

QString XPCategory::getDescription(const QString& category, int size)
{
    if (!m_valid)
        return QString("Invalid");

    // Find the entry for 'category' in the group for size
    QString group = QString("%1x%1").arg(size);
    QSettings ini(indexFilePath, QSettings::IniFormat);
    QStringList values = ini.value(QString("%1/%2").arg(group).arg(category),
                                   QString("%1/0000").arg(group)).toStringList();
    if (values.count() != 2)
    {
        qDebug() << "XPCategory::getDescription category code not found";
        return QString("Unknown"); // unknown device icon
    }

    return QString(values[1]);
}

QString XPCategory::getCategory(const QString& manufacturer, const QString& mmodel)
{
    QIODevice *rmlFile = 0;

    // Try and get an RML file from the local cache
    //
    QString profilePath = QString("%1/profiles/%2/%3").arg(rmlCachePath).arg(manufacturer).arg(mmodel);
    QDir dir(profilePath);

    if (dir.exists())
    {
        QStringList filters("*.prf");
        QFileInfoList fl = dir.entryInfoList(filters, QDir::Files);
        if (fl.count()) // the first one will do
        {
            QString filePath = fl.at(0).filePath();
            QFile *localFile = new QFile(filePath);
            localFile->open(QFile::ReadOnly);
            rmlFile = localFile;
            qDebug() << "XPCategory::getCategory local path=" << filePath;
        }
    }

    // Get default RML file from HUB (std.prf)
    //
    if (rmlFile == 0)
    {
        QString modelPath = QString("profiles/%1/%2/std.prf").arg(manufacturer).arg(mmodel);
        QString filePath = QString("%1/%2").arg(rmlCachePath).arg(modelPath);
        QUrl url = QUrl(QString("http://%1/%2").arg(hostAddress).arg(modelPath));

        XPNetFile *remoteFile = new XPNetFile(url, filePath);
        if (!remoteFile->exists())
        {
            qDebug() << "XPCategory::getCategory RML file not found";
            return QString();
        }
        remoteFile->open(QFile::ReadOnly);
        rmlFile = remoteFile;
    }

    // Parse the RML and get the category code
    //
    QXmlStreamReader reader(rmlFile);

    while (!reader.atEnd())
    {
        reader.readNext();
        if (!reader.isStartElement())
            continue;

        if (reader.name() == "category")
        {
            return reader.readElementText();
        }
    }
    if (reader.hasError())
    {
        QString message = tr("XML syntax error in RML:\n%1.").arg(reader.errorString());
        qDebug() << "XPCategory::getCategory" << message;
    }
    delete rmlFile;

    return QString();
}

QStringList XPCategory::getGroupData(int size)
{
    QStringList result;

    if (!m_valid)
        return result;

    // Open category index file
    //
    QFile indexFile(indexFilePath);
    if (!indexFile.open(QFile::ReadOnly))
    {
        qDebug() << "XPCategory::getGroupData open error=" << indexFile.errorString();
        return result;
    }

    // Read all index data and find the group [sizexsize]
    //
    int pos1, pos2;
    QString group = QString("[%1x%1]").arg(size);
    QString ini = indexFile.readAll();

    // Find first line of group
    pos1 = ini.indexOf(group);
    if (pos1 <= 0)
    {
        qDebug() << "XPCategory::getGroupData group does not exist for:" << group;
        return result;
    }
    pos1 = ini.indexOf("\n", pos1)+1;

    // Read all group data
    for (int i=0; true; i++)
    {
        pos2 = ini.indexOf("\n", pos1);
        if (pos2 <= pos1)
            break;

        QString line = ini.mid(pos1, pos2-pos1).trimmed();
        if (line.isEmpty() || (line.startsWith("[") && line.endsWith("]")))
            break;

        // Format category data as: 8009;ADRC shield;8009.png
        QStringList tokens = line.split(QRegExp("[=,]"));
        if (tokens.count() != 3)
            continue;
        result << QString("%1;%2;%3").arg(tokens[0]).arg(tokens[1]).arg(tokens[2]);

        // Setup for next line
        pos1 = pos2+1;
    }

    return result;
}

bool XPCategory::setGroupData(const QStringList& data, int size)
{
    // TODO *** HERE *** If size == -1 write the data back to both [72x72] and [128x128]
    // Else write back to the requested group only
    // Scale icons for 128x128 and 72x72
    return false;
}

// end of file
