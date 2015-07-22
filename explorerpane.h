// This module defines the explorer pane of the RML IDE.
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

#ifndef EXPLORERPANE_H
#define EXPLORERPANE_H

#include <QWidget>
#include <QTextEdit>
#include <QTabWidget>
#include <QLabel>


class ExplorerPane : public QWidget
{
    Q_OBJECT

public:
    explicit ExplorerPane(QWidget *parent = 0);
    //
    void clear();
    void clearErrors();
    void clearMetadata();
    void clearSimulator();

public slots:
    void updateOutput(const QString& output);
    void updateErrors(const QString& errors);
    void updateMetadata(const QString& rml, const QString& uri, const QString& hostAddress);

protected:
    QWidget *createOutputPage();
    QWidget *createErrorsPage();
    QWidget *createMetadataPage();

private: // data
    QTabWidget *tabWidget;
    QWidget *outputPage;
    QWidget *errorsPage;
    QWidget *metadataPage;
    QString rmlCachePath;

    // Metadata page contents
    QLabel *labelDevIcon;
    QLabel *labelDevManf;
    QLabel *labelDevMmod;
    QLabel *labelDevNick;
    QLabel *labelFileName;
    QLabel *labelFileSize;
    QLabel *labelFileZsize; // compressed size
};

#endif // EXPLORERPANE_H
