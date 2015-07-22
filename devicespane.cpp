// This module implements the device pane of the RML IDE.
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

#include <QDir>
#include <QDebug>
#include <QVBoxLayout>
#include <QTreeWidgetItem>
#include <QProcessEnvironment>

#include <xpcategory.h>
#include "devicespane.h"


DevicesPane::DevicesPane(QWidget *parent) :
    QWidget(parent)
{
    // Save the RML cache path
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    m_rmlCachePath = env.value("XP_RDFCACHE_PATH", QDir::homePath()+"/xped");

    // Create the tree widget
    m_outline = new QTreeWidget();
    m_outline->setColumnCount(1);
    m_outline->setHeaderHidden(true);

    // Build the layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_outline);
    setLayout(layout);

    // Configure the tree
    connect(m_outline, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onItemClicked(QTreeWidgetItem*,int)));
}

void DevicesPane::clear()
{
    m_outline->clear();
}

void DevicesPane::update(QString &hostAddress, QString &hostName, const QMap<QString, QString> &world)
{
    clear();

    // Set up for category actions
    XPCategory category(m_rmlCachePath, hostAddress);

    // Hub level node
    m_outline->insertTopLevelItem(0, new QTreeWidgetItem((QTreeWidget*)0, QStringList(hostName+" - hub"), DEV_TYPE_HUB));
    QFont f = m_outline->topLevelItem(0)->font(0);
    f.setBold(true);
    m_outline->topLevelItem(0)->setFont(0, f);

    // Add the child nodes
    QMapIterator<QString,QString> i(world);
    while (i.hasNext())
    {
        i.next();

        QString deviceId = i.key();
        QStringList values = i.value().split(";", QString::SkipEmptyParts); // ouid;nick;manf;mmod;file1;...filen;

        if (values.count() >= 5)
        {
            // Add device level node
            QString nickname = values.at(1);
            QTreeWidgetItem *deviceItem = new QTreeWidgetItem((QTreeWidget*)0, QStringList(nickname), DEV_TYPE_DEVICE);
            QString catcode = category.getCategory(values.at(2), values.at(3));
            if (catcode.isEmpty())
                deviceItem->setIcon(0, QIcon(":/images/download.png"));
            else // got the category
            {
                QIcon icon = category.getIcon(catcode, 72);
                if (icon.isNull())  // index file missing
                {
                    clear();

                    m_outline->insertTopLevelItem(0, new QTreeWidgetItem((QTreeWidget*)0, QStringList(hostName), DEV_TYPE_HUB));
                    QFont f = m_outline->topLevelItem(0)->font(0);
                    f.setBold(true);
                    m_outline->topLevelItem(0)->setFont(0, f);
                    m_outline->topLevelItem(0)->setIcon(0, QIcon(":/images/document-close.svg"));

                    break;
                }

                // Show the device icon
                deviceItem->setIcon(0, icon);
            }

            // Add file level nodes to device
            QTreeWidgetItem *fileItem;
            for (int i=4, n=values.count(); i<n; i++)
            {
                fileItem = new QTreeWidgetItem((QTreeWidget*)0, QStringList(values.at(i)), DEV_TYPE_FILE);

                // Check if file exists in cache
                QString fileName = QString("%1/profiles/%2/%3/%4")
                        .arg(m_rmlCachePath)
                        .arg(values.at(2))
                        .arg(values.at(3))
                        .arg(values.at(i));

                // Display appropriate icon
                if (!QFile::exists(fileName))
                    fileItem->setIcon(0, QIcon(":/images/document-close.svg"));
                else
                    fileItem->setIcon(0, QIcon(":/images/file.png"));
                fileItem->setData(0, Qt::UserRole, deviceId);
                deviceItem->addChild(fileItem);
            }

            // Add the device to the tree
            m_outline->topLevelItem(0)->addChild(deviceItem);
        }
    }

    m_outline->expandAll();
}

void DevicesPane::onItemClicked(QTreeWidgetItem *item, int column)
{
    //qDebug() << "onItemClicked item=" << item->text(column) << "col=" << column;

    QString fileName = item->text(column);
    QString deviceId = item->data(column, Qt::UserRole).toString();
    int type = item->type();

    if (type == DEV_TYPE_FILE)
    {
        qDebug() << "deviceid=" << deviceId << "fileName=" << fileName;
        m_device = deviceId;
        m_filename = fileName;
    }
    else // there is no selection
    {
        m_device.clear();
        m_filename.clear();
    }
}

// end of file
