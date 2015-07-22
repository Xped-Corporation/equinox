// This module defines the device pane of the RML IDE.
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

#ifndef DEVICESPANE_H
#define DEVICESPANE_H

#include <QWidget>
#include <QTreeWidget>

// Define the item types.
//
#define DEV_TYPE_HUB 1
#define DEV_TYPE_DEVICE 2
#define DEV_TYPE_FILE 3

// HUB
// |
// |--- DEVICE
// |    |
// |    |---- FILE
// |    |
// |    |---- FILE
// |
// |--- DEVICE
//


class DevicesPane : public QWidget
{
    Q_OBJECT

public:
    explicit DevicesPane(QWidget *parent = 0);
    //
    void clear();
    void update(QString& hostAddress, QString& hostName, const QMap<QString, QString>& world);
    //
    QString device() { return m_device; }
    QString filename() { return m_filename; }

protected slots:
    void onItemClicked(QTreeWidgetItem *item, int column);

private:
    QTreeWidget *m_outline;
    QString m_device;
    QString m_filename;
    QString m_rmlCachePath;
};

#endif // DEVICESPANE_H
