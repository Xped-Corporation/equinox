// This module implements the RML transfer dialog of the RML IDE.
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
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "rmltransferdialog.h"


RmlTransferDialog::RmlTransferDialog(QCheckBox *fromDeviceCheck,
                                     const QIcon &icon,
                                     const QString &captionText,
                                     const QString &bodyText,
                                     QWidget *parent)
    : QDialog(parent)
{
    // Create dialog contents
    QLabel *image = new QLabel;
    image->setPixmap(icon.pixmap(64,64));
    //
    QLabel *caption = new QLabel(captionText);
    caption->setStyleSheet("* { font-weight: bold }");
    QLabel *message = new QLabel(bodyText);
    //
    QPushButton *okButton = new QPushButton(tr("Ok"));
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));

    // Create combo layout
    QHBoxLayout *msgLayout = new QHBoxLayout;
    msgLayout->setSpacing(16);
    msgLayout->setContentsMargins(0,0,0,0);
    msgLayout->addWidget(image);
    msgLayout->addWidget(message);
    msgLayout->addStretch(1);

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
    layout->addLayout(msgLayout);
    layout->addWidget(fromDeviceCheck);
    layout->addStretch(1);
    layout->addLayout(btnLayout);

    // Show the dialog
    setFixedSize(480, 170);
    setLayout(layout);
    //
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

// end of file
