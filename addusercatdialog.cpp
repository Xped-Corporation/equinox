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

#include <QFile>
#include <QIcon>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QFileDialog>

#include "addusercatdialog.h"


AddUserCategoryDialog::AddUserCategoryDialog(QString &catIconPath,
                                             QString &catNumber,
                                             QString &catText,
                                             QWidget *parent)
    : QDialog(parent), m_catIconPath(catIconPath), m_catNumber(catNumber), m_catText(catText)
{
    // Create dialog contents
    QLabel *caption = new QLabel(tr("Add category"));
    caption->setStyleSheet("* { font-weight: bold }");
    //
    QLabel *catIconLabel = new QLabel(tr("Icon:"));
    QLabel *catNumberLabel = new QLabel(tr("Number:"));
    QLabel *catNumberNoteLabel = new QLabel(tr("Format is 4 hex digits in the range 4000 to 7FFF"));
    QLabel *catTextLabel = new QLabel(tr("Description:"));
    QLabel *catTextNoteLabel = new QLabel(tr("Maximum length is 16 characters"));
    QLabel *catIconResLabel = new QLabel(tr("An icon of 128x128 px resolution gives best results"));
    //
    catImage = new QLabel;
    catImage->setPixmap(QIcon(":images/unknown-device.png").pixmap(64,64));
    catNumberEdit = new QLineEdit;
    catNumberEdit->setMaxLength(4);
    catTextEdit = new QLineEdit;
    catTextEdit->setMaxLength(16);
    //
    QPushButton *okButton = new QPushButton(tr("Ok"));
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));
    QPushButton *browseButton = new QPushButton(tr("Browse..."));

    // Create browse button layout
    QHBoxLayout *browseLayout = new QHBoxLayout;
    browseLayout->setSpacing(16);
    browseLayout->addWidget(browseButton);
    browseLayout->addStretch(1);

    // Create text layout
    QGridLayout *catTextLayout = new QGridLayout;
    catTextLayout->setSpacing(8);
    catTextLayout->addWidget(catIconLabel, 0, 0, Qt::AlignRight);
    catTextLayout->addWidget(catImage, 0, 1);
    catTextLayout->addLayout(browseLayout, 0, 2);
    //
    catTextLayout->addWidget(catIconResLabel, 1, 1, 1, 2);
    //
    catTextLayout->addWidget(catNumberLabel, 2, 0, Qt::AlignRight);
    catTextLayout->addWidget(catNumberEdit, 2, 1);
    catTextLayout->addWidget(catNumberNoteLabel, 2, 2);
    //
    catTextLayout->addWidget(catTextLabel, 3, 0, Qt::AlignRight);
    catTextLayout->addWidget(catTextEdit, 3, 1);
    catTextLayout->addWidget(catTextNoteLabel, 3, 2);

    // Create button layout
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(16);
    btnLayout->setContentsMargins(0,0,0,0);
    btnLayout->addStretch(1);
    btnLayout->addWidget(okButton);
    btnLayout->addWidget(cancelButton);

    // Create main layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(8);
    layout->addStretch(1);
    layout->addWidget(caption);
    layout->addLayout(catTextLayout);
    layout->addStretch(1);
    layout->addLayout(btnLayout);
    //
    setLayout(layout);

    // Connect signals
    connect(okButton, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(browseButton, SIGNAL(clicked()), this, SLOT(onBrowseButtonClicked()));
}

void AddUserCategoryDialog::onOkButtonClicked()
{
    bool ok;

    // Validity checks
    if (m_iconFilePath.isEmpty() || m_catNumber.isEmpty() || m_catText.isEmpty())
    {
        QMessageBox::warning(this, tr("Add category"),
            tr("One or more of the fields is empty."),
            QMessageBox::Close);

        return;
    }

    // Check category number is in the user range
    int catNum = m_catNumber.toInt(&ok, 16);
    if (!ok || catNum < 0x4000 || catNum > 0x7FFF)
    {
        catNumberEdit->setStyleSheet("* { color: red; }");
        return;
    }

    // Return values to caller
    m_catIconPath = m_iconFilePath;
    m_catNumber = catNumberEdit->text();
    m_catText = catTextEdit->text();

    accept();
}

void AddUserCategoryDialog::onBrowseButtonClicked()
{
    // Create a file open dilaog to allow the file to be selected
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open icon file"), QDir::homePath(),
                           tr("Image Files (*.png *.jpg *.svg);;All Files (*.*)"));

    if (filePath.isEmpty())
        return;

    if (!QFile::exists(filePath))
        return;

    // Open the image file and display it
    catImage->setPixmap(QIcon(filePath).pixmap(64,64));
    m_iconFilePath = filePath;
}

// end of file
