// This module defines the settings dialog of the RML IDE.
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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QLineEdit>
#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>

#include <xpadrctcpproxy.h>


class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(AdrcTcpProxy **proxy, QWidget *parent = 0);

private slots:
    void onTabChanged(int index);
    void onGeneralOk();
    void onGeneralCancel();
    void onGeneralApply();
    void onGeneralTemplateBrowse();
    void onGeneralDtdBrowse();
    void onCategoryOk();
    void onCategoryCancel();
    void onCategoryApply();
    void onCategoryAdd();
    void onCategoryDel();
    void onCategoryUserClicked();
    void onCategorySystemClicked();
    void onCategoryItemClicked(QListWidgetItem *item);

private: // methods
    QWidget *createGeneralTab();
    QWidget *createCategoriesTab();
    void loadUserCategories();
    void loadSystemCategories();
    void loadCategories(bool system = true);

private: // data
    QTabWidget *tabWidget;
    // Options
    QCheckBox *validateBeforeRunCb;
    // General tab
    QCheckBox *templateCb;
    QCheckBox *dtdCb;
    QLineEdit *templateEdit;
    QLineEdit *dtdEdit;
    // Categories tab
    QListWidget *categoryList;
    QPushButton *addCatButton;
    QPushButton *delCatButton;
    // Adrc Tcp proxy
    AdrcTcpProxy **proxy;
};

#endif // SETTINGSDIALOG_H
