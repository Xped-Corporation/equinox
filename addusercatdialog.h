// This module defines the RML add user category dialog of the RML IDE.
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

#ifndef ADDUSERCATDIALOG_H
#define ADDUSERCATDIALOG_H

#include <QLabel>
#include <QString>
#include <QDialog>
#include <QWidget>
#include <QLineEdit>


class AddUserCategoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddUserCategoryDialog(QString& catIconPath,
                                   QString& catNumber,
                                   QString& catText,
                                   QWidget *parent = 0);

private slots:
    void onOkButtonClicked();
    void onBrowseButtonClicked();

private:
    QString& m_catIconPath;
    QString& m_catNumber;
    QString& m_catText;
    QString m_iconFilePath;
    //
    QLabel *catImage;
    QLineEdit *catNumberEdit;
    QLineEdit *catTextEdit;
};

#endif // ADDUSERCATDIALOG_H
