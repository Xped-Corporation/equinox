// This module implements the find/replace dialog of the RML IDE.
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

#include <QDebug>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "finddialog.h"


FindDialog::FindDialog(QsciScintilla *editor, QWidget *parent) :
    QDialog(parent), editor(editor)
{
    // Set window properties
    setWindowTitle(tr("Find and replace"));

    // Create the UI objects
    findButton = new QPushButton(tr("Find"));
    nextButton = new QPushButton(tr("Next"));
    replaceButton = new QPushButton(tr("Replace"));
    replaceAllButton = new QPushButton(tr("Replace all"));
    QPushButton *closeButton = new QPushButton(tr("Close"));
    //
    wrapFind = new QCheckBox(tr("Wrap search"));
    matchCase = new QCheckBox(tr("Match case"));
    wholeMatch = new QCheckBox(tr("Match whole words"));
    regExp = new QCheckBox(tr("Use regular expression"));
    reverse = new QCheckBox(tr("Search reverse direction"));
    //
    findText = new QLineEdit;
    findText->setMinimumWidth(300);
    replaceText = new QLineEdit;
    replaceText->setMinimumWidth(300);
    //
    findButton->setEnabled(false);
    nextButton->setEnabled(false);
    replaceButton->setEnabled(false);
    replaceAllButton->setEnabled(false);
    //
    QLabel *replaceCaption = new QLabel(tr("Replace with"));

    // Create the layouts
    QHBoxLayout *findLayout = new QHBoxLayout;
    findLayout->setSpacing(8);
    findLayout->addWidget(findText);
    findLayout->addWidget(findButton);
    //
    QHBoxLayout *replaceLayout = new QHBoxLayout;
    replaceLayout->setSpacing(8);
    replaceLayout->addWidget(replaceText);
    replaceLayout->addWidget(replaceCaption);

    QVBoxLayout *optionLayout = new QVBoxLayout;
    optionLayout->setSpacing(8);
    optionLayout->addWidget(wrapFind);
    optionLayout->addWidget(matchCase);
    optionLayout->addWidget(regExp);
    optionLayout->addWidget(reverse);
    optionLayout->addWidget(regExp);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(8);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(replaceButton);
    buttonLayout->addWidget(replaceAllButton);
    buttonLayout->addWidget(nextButton);
    buttonLayout->addWidget(closeButton);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addLayout(findLayout);
    layout->addLayout((replaceLayout));
    layout->addSpacing(16);
    layout->addLayout(optionLayout);
    layout->addLayout(buttonLayout);
    layout->setSizeConstraint( QLayout::SetFixedSize );

    connect(findButton, SIGNAL(clicked()), this, SLOT(onFindButton()));
    connect(nextButton, SIGNAL(clicked()), this, SLOT(onNextButton()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(replaceButton, SIGNAL(clicked()), this, SLOT(onReplaceButton()));
    connect(replaceAllButton, SIGNAL(clicked()), this, SLOT(onReplaceAllButton()));
    connect(findText, SIGNAL(textEdited(QString)), this, SLOT(onFindTextEdited(QString)));
    connect(replaceText, SIGNAL(textEdited(QString)), this, SLOT(onReplaceTextEdited(QString)));

    setLayout(layout);
}

void FindDialog::onFindButton()
{
    editor->findFirst(findText->text(),
          regExp->isChecked(),
          matchCase->isChecked(),
          wholeMatch->isChecked(),
          wrapFind->isChecked(),
          !reverse->isChecked());

    nextButton->setEnabled(true);

    // Handle Replace button
    if (editor->hasSelectedText())
    {
        QString findContent = findText->text();
        if (!findContent.isEmpty())
            replaceButton->setEnabled(true);
        else
            replaceButton->setEnabled(false);
    }
}

void FindDialog::onNextButton()
{
    editor->findNext();
}

void FindDialog::onReplaceButton()
{
    QString replaceContent = replaceText->text();
    editor->replace(replaceContent);
}

void FindDialog::onReplaceAllButton()
{
    QString replaceContent = replaceText->text();

    bool found = editor->findFirst(findText->text(),
          regExp->isChecked(),
          matchCase->isChecked(),
          wholeMatch->isChecked(),
          false,    //wrap stop recursive replace
          true,     //forward
          0, 0);    // start from beginning

    if (found)
        editor->replace(replaceContent);

    while (editor->findNext())
        editor->replace(replaceContent);
}

void FindDialog::onFindTextEdited(QString text)
{
    if (text.isEmpty())
    {
        findButton->setEnabled(false);
        nextButton->setEnabled(false);
        replaceButton->setEnabled(false);
        replaceAllButton->setEnabled(false);
    }
    else // findText has content
    {
        findButton->setEnabled(true);
        replaceAllButton->setEnabled(true);
    }
}

void FindDialog::onReplaceTextEdited(QString text)
{
    if (text.isEmpty())
        replaceButton->setEnabled(false);
    else if (editor->hasSelectedText())
    {
        QString findContent = findText->text();
        if (!findContent.isEmpty())
            replaceButton->setEnabled(true);
        else
            replaceButton->setEnabled(false);
    }
}

// end of file
