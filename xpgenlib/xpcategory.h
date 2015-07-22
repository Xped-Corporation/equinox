// This module defines the category class of the XPGENLIB library.
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

#ifndef XPCATEGORY_H
#define XPCATEGORY_H

#include <QObject>
#include <QString>
#include <QIcon>

class XPCategory : public QObject
{
    Q_OBJECT
public:
    explicit XPCategory(const QString& rmlCachePath, const QString& hostAddress = QString(), QObject *parent = 0);

    // Categories index header info
    QString getTheme() { return m_theme; }
    QString getComment() { return m_comment; }
    QList<int> getSizes() { return m_sizes; }

    // Read and write group data
    QStringList getGroupData(int size = 72);
    bool setGroupData(const QStringList& data, int size = -1);

    // Category specific info
    QString getCategory(const QString& manufacturer, const QString& mmodel);
    QString getDescription(const QString& category, int size);
    QIcon getIcon(const QString& category, int size);

signals:

public slots:

private:
    bool m_valid;
    QString rmlCachePath;
    QString hostAddress;
    QString indexFilePath;
    //
    QString m_theme;
    QString m_comment;
    QList<int> m_sizes;
};

#endif // XPCATEGORY_H
