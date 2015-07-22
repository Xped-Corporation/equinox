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

#include <QLabel>
#include <QDebug>
#include <QGroupBox>
#include <QTabWidget>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QApplication>
#include <xpcategory.h>

#include "addusercatdialog.h"
#include "settingsdialog.h"


SettingsDialog::SettingsDialog(AdrcTcpProxy **proxy, QWidget *parent) :
    QDialog(parent), proxy(proxy)
{
    // Page heading
    QLabel *caption = new QLabel(tr("IDE Settings"));
    caption->setStyleSheet(QString("* { font-weight: bold; font-size: %1pt; }").arg(font().pointSize()+2));

    // Tab widget
    tabWidget = new QTabWidget;
    tabWidget->setFocusPolicy(Qt::NoFocus);
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));

    // Create the tabs
    tabWidget->addTab(createGeneralTab(), tr("General"));
    // TODO tabWidget->addTab(createCategoriesTab(), tr("Categories"));

    // Create the layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(8);
    layout->addWidget(caption);
    layout->addWidget(tabWidget);

    setLayout(layout);
}

void SettingsDialog::onTabChanged(int index)
{
    qDebug() << "SettingsDialog::onTabChanged index=" << index;

    if (tabWidget->tabText(index) == tr("Categories"))
    {
        if (categoryList->count() == 0)
            loadUserCategories();
    }
}

// General tab
//

QWidget *SettingsDialog::createGeneralTab()
{
    QSettings settings;

    // Get the general settings
    //
    settings.beginGroup("settings-dialog");
    bool validateBeforeRun = settings.value("validateBeforeRun", true).toBool();
    bool useDefaultTemplate = settings.value("defaultRmlTemplate", true).toBool();
    bool useDefaultDtd = settings.value("defaultRmlDtd", true).toBool();
    QString templatePath = settings.value("rmlTemplatePath").toString();
    QString dtdPath = settings.value("rmlDtdPath").toString();
    settings.endGroup();

    // Create controls
    //
    // Options
    validateBeforeRunCb = new QCheckBox(tr("Validate RML before running"));
    validateBeforeRunCb->setChecked(validateBeforeRun);
    //
    QVBoxLayout *optionsVLayout = new QVBoxLayout;
    optionsVLayout->addWidget(validateBeforeRunCb, 0, Qt::AlignLeft);
    //
    QGroupBox *optionsGroup = new QGroupBox(tr("Options"));
    optionsGroup->setLayout(optionsVLayout);

    // Empty template selection
    templateCb = new QCheckBox(tr("Use default RML template"));
    templateCb->setChecked(useDefaultTemplate);
    templateEdit = new QLineEdit;
    templateEdit->setEnabled(!useDefaultTemplate);
    if (!templatePath.isEmpty())
        templateEdit->setText(templatePath);
    QPushButton *templateBrowse = new QPushButton(tr("Browse..."));
    //
    QHBoxLayout *templateHLayout = new QHBoxLayout;
    templateHLayout->addWidget(templateEdit);
    templateHLayout->addWidget(templateBrowse);
    QVBoxLayout *templateVLayout = new QVBoxLayout;
    templateVLayout->addLayout(templateHLayout);
    templateVLayout->addWidget(templateCb, 0, Qt::AlignLeft);
    //
    QGroupBox *templateGroup = new QGroupBox(tr("RML template selection"));
    templateGroup->setLayout(templateVLayout);

    // DTD selection
    dtdCb = new QCheckBox(tr("Use default RML DTD"));
    dtdCb->setChecked(useDefaultDtd);
    dtdEdit = new QLineEdit;
    dtdEdit->setEnabled(!useDefaultDtd);
    if (!dtdPath.isEmpty())
        dtdEdit->setText(dtdPath);
    QPushButton *dtdBrowse = new QPushButton(tr("Browse..."));
    //
    QHBoxLayout *dtdHLayout = new QHBoxLayout;
    dtdHLayout->addWidget(dtdEdit);
    dtdHLayout->addWidget(dtdBrowse);
    QVBoxLayout *dtdVLayout = new QVBoxLayout;
    dtdVLayout->addLayout(dtdHLayout);
    dtdVLayout->addWidget(dtdCb, 0, Qt::AlignLeft);
    //
    QGroupBox *dtdGroup = new QGroupBox(tr("RML DTD selection"));
    dtdGroup->setLayout(dtdVLayout);

    // Page buttons: Ok, Cancel, Apply
    QPushButton *okButton = new QPushButton(tr("Ok"));
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));
    QPushButton *applyButton = new QPushButton(tr("Apply"));
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(8);
    btnLayout->addStretch(1);
    btnLayout->addWidget(okButton);
    btnLayout->addWidget(cancelButton);
    btnLayout->addWidget(applyButton);

    // Connect slots
    connect(templateCb, SIGNAL(clicked(bool)), templateEdit, SLOT(setDisabled(bool)));
    connect(dtdCb, SIGNAL(clicked(bool)), dtdEdit, SLOT(setDisabled(bool)));
    connect(templateBrowse, SIGNAL(clicked()), this, SLOT(onGeneralTemplateBrowse()));
    connect(dtdBrowse, SIGNAL(clicked()), this, SLOT(onGeneralDtdBrowse()));
    connect(okButton, SIGNAL(clicked()), this, SLOT(onGeneralOk()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(onGeneralCancel()));
    connect(applyButton, SIGNAL(clicked()), this, SLOT(onGeneralApply()));

    // Create main layout
    //
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(16);
    layout->addWidget(optionsGroup);
    layout->addWidget(templateGroup);
    layout->addWidget(dtdGroup);
    layout->addLayout(btnLayout);

    // Create tab
    //
    QWidget *tab = new QWidget;
    tab->setLayout(layout);

    return tab;
}

void SettingsDialog::onGeneralOk()
{
    onGeneralApply();
    accept();
}

void SettingsDialog::onGeneralCancel()
{
    reject();
}

void SettingsDialog::onGeneralApply()
{
    QSettings settings;

    settings.beginGroup("settings-dialog");
    settings.setValue("validateBeforeRun", validateBeforeRunCb->isChecked());
    settings.setValue("defaultRmlTemplate", templateCb->isChecked());
    settings.setValue("defaultRmlDtd", dtdCb->isChecked());
    settings.setValue("rmlTemplatePath", templateEdit->text());
    settings.setValue("rmlDtdPath", dtdEdit->text());
    settings.endGroup();
}

void SettingsDialog::onGeneralTemplateBrowse()
{
    // Create a file open dilaog to allow the file to be selected
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open RML file"), QDir::homePath(),
                           tr("RML Files (*.prf)"));

    if (!filePath.isEmpty())
        templateEdit->setText(filePath);
}

void SettingsDialog::onGeneralDtdBrowse()
{
    // Create a file open dilaog to allow the file to be selected
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open DTD file"), QDir::homePath(),
                           tr("DTD Files (*.dtd)"));

    if (!filePath.isEmpty())
        dtdEdit->setText(filePath);
}

// Categories tab
//

QWidget *SettingsDialog::createCategoriesTab()
{
    // Create controls
    //
    // Category list
    categoryList = new QListWidget;
    categoryList->setIconSize(QSize(36,36));
    addCatButton = new QPushButton(tr("Add"));
    delCatButton = new QPushButton(tr("Delete"));
    delCatButton->setEnabled(false);
    QRadioButton *systemRb = new QRadioButton(tr("System"));
    QRadioButton *userRb = new QRadioButton(tr("User"));
    userRb->setChecked(true);
    //
    QHBoxLayout *radioLayout = new QHBoxLayout;
    radioLayout->addWidget((userRb));
    radioLayout->addWidget(systemRb);
    radioLayout->addStretch(1);
    //
    QVBoxLayout *listBtnLayout = new QVBoxLayout;
    listBtnLayout->addWidget(addCatButton);
    listBtnLayout->addWidget(delCatButton);
    listBtnLayout->addStretch(1);
    //
    QHBoxLayout *listLayout = new QHBoxLayout;
    listLayout->addWidget(categoryList);
    listLayout->addLayout(listBtnLayout);

    // Page buttons: Ok, Cancel, Apply
    QPushButton *okButton = new QPushButton(tr("Ok"));
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));
    QPushButton *applyButton = new QPushButton(tr("Apply"));
    //
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(8);
    btnLayout->addStretch(1);
    btnLayout->addWidget(okButton);
    btnLayout->addWidget(cancelButton);
    btnLayout->addWidget(applyButton);

    // Connect slots
    //
    connect(categoryList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(onCategoryItemClicked(QListWidgetItem*)));
    connect(addCatButton, SIGNAL(clicked()), this, SLOT(onCategoryAdd()));
    connect(delCatButton, SIGNAL(clicked()), this, SLOT(onCategoryDel()));
    connect(userRb, SIGNAL(clicked()), this, SLOT(onCategoryUserClicked()));
    connect(systemRb, SIGNAL(clicked()), this, SLOT(onCategorySystemClicked()));
    connect(okButton, SIGNAL(clicked()), this, SLOT(onCategoryOk()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(onCategoryCancel()));
    connect(applyButton, SIGNAL(clicked()), this, SLOT(onCategoryApply()));

    // Create main layout
    //
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(16);
    layout->addLayout(listLayout);
    layout->addLayout(radioLayout);
    layout->addLayout(btnLayout);

    // Create tab
    //
    QWidget *tab = new QWidget;
    tab->setLayout(layout);

    return tab;
}

void SettingsDialog::onCategoryOk()
{
    onCategoryApply();
    accept();
}

void SettingsDialog::onCategoryCancel()
{
    reject();
}

void SettingsDialog::onCategoryApply()
{
    // TODO Save changes to user category
}

void SettingsDialog::onCategoryAdd()
{
    QString iconPath;
    QString catNumber;
    QString catText;

    AddUserCategoryDialog dialog(iconPath, catNumber, catText, this);
    dialog.exec();

    // TODO *** HERE *** Add specified user category to memory list
}

void SettingsDialog::onCategoryDel()
{
    // TODO Delete a user category
}

void SettingsDialog::onCategoryUserClicked()
{
    loadUserCategories();
}

void SettingsDialog::onCategorySystemClicked()
{
    loadSystemCategories();
}

void SettingsDialog::loadUserCategories()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    categoryList->clear();
    addCatButton->setEnabled(true);
    loadCategories(false);

    QApplication::restoreOverrideCursor();
}

void SettingsDialog::loadSystemCategories()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    categoryList->clear();
    addCatButton->setEnabled(false);
    loadCategories(true);

    QApplication::restoreOverrideCursor();
}

void SettingsDialog::loadCategories(bool system)
{
    QString hostAddress = (*proxy == 0) ? QString() : (*proxy)->getHostAddress();
    XPCategory categories(QDir::homePath()+"/xped", hostAddress);
    QStringList list = categories.getGroupData();

    for (int i=0, n=list.count(); i<n; i++)
    {
        // Format is <number>;<descr>;<filename>
        QString category = list[i];
        QStringList tokens = category.split(";");
        if (tokens.count() != 3)
            continue;

        // User category range is 0x4000 to 0x7FFF
        int catno = tokens[0].toInt(0, 16);
        if ((system && (catno >= 0x8000 || catno < 0x4000))
            || (!system && catno < 0x8000 && catno >= 0x4000))
        {
            QIcon icon = categories.getIcon(tokens[0], 72);
            QString text = QString("%1: %2").arg(tokens[0]).arg(tokens[1]);
            QListWidgetItem *item = new QListWidgetItem(icon, text);
            categoryList->addItem(item);
        }
    }
}

void SettingsDialog::onCategoryItemClicked(QListWidgetItem *item)
{
    Q_UNUSED(item); // nothing for now
}

// end of file
