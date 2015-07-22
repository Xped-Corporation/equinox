// This module implements the explorer pane of the RML IDE.
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

#include <QtGui>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QProcessEnvironment>

#include <xpnetfile.h>
#include <xpcategory.h>

#include "explorerpane.h"


ExplorerPane::ExplorerPane(QWidget *parent) :
    QWidget(parent)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    rmlCachePath = env.value("XP_RDFCACHE_PATH", QDir::homePath()+"/xped");

    // Create the tab pages
    metadataPage = createMetadataPage();
    errorsPage = createErrorsPage();
    outputPage = createOutputPage();

    // Create the tab widget
    tabWidget = new QTabWidget;
    tabWidget->setTabPosition(QTabWidget::South);
    //
    tabWidget->addTab(metadataPage, "Metadata");
    tabWidget->addTab(errorsPage, "Errors");
    tabWidget->addTab(outputPage, "Simulator");

    // Create the main layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(tabWidget);

    setLayout(layout);

    // Connect signals
    //connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));

    clear();
}

QWidget *ExplorerPane::createErrorsPage()
{
    QTextEdit *page = new QTextEdit;
    page->setReadOnly(true);
    page->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    page->setTabStopWidth(3);

    return (QWidget *)page;
}

QWidget *ExplorerPane::createOutputPage()
{
    QTextEdit *page = new QTextEdit;
    page->setReadOnly(true);
    page->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    return (QWidget *)page;
}

QWidget *ExplorerPane::createMetadataPage()
{
    QWidget *page = new QWidget;

    // Create the Device layouts
    //
    QIcon devIcon = QIcon(":/images/unknown-device");
    labelDevIcon = new QLabel;
    labelDevIcon->setPixmap(devIcon.pixmap(48, 48));
    labelDevManf = new QLabel("Xped");
    labelDevMmod = new QLabel("Unknown");
    labelDevNick = new QLabel("Unknown");
    //
    QVBoxLayout *devLayoutA = new QVBoxLayout;
    devLayoutA->setSpacing(8);
    devLayoutA->setContentsMargins(0,0,0,0);
    devLayoutA->addWidget(labelDevManf);
    devLayoutA->addWidget(labelDevMmod);
    devLayoutA->addWidget(labelDevNick);
    devLayoutA->addStretch(1);
    //
    QHBoxLayout *devLayout = new QHBoxLayout;
    devLayout->setSpacing(8);
    devLayout->setContentsMargins(0,0,0,0);
    devLayout->addWidget(labelDevIcon, 0, Qt::AlignTop);
    devLayout->addLayout(devLayoutA);
    devLayout->addStretch(1);

    // Create the Device layouts
    //
    //labelDevManf->setStyleSheet("* { font-weight: bold }");
    labelFileName = new QLabel("File:\tstd.prf");
    labelFileSize = new QLabel("Size:\t1903 bytes");
    labelFileZsize = new QLabel("Zip size:\t892 bytes");
    //
    QVBoxLayout *fileLayout = new QVBoxLayout;
    fileLayout->setSpacing(8);
    fileLayout->setContentsMargins(0,0,0,0);
    fileLayout->addWidget(labelFileName);
    fileLayout->addWidget(labelFileSize);
    fileLayout->addWidget(labelFileZsize);
    fileLayout->addStretch(1);

    // Create the main layout
    //
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setSpacing(48);
    layout->addLayout(devLayout);
    layout->addLayout(fileLayout);
    layout->addStretch(1);
    //
    page->setLayout(layout);

    return page;
}

void ExplorerPane::clear()
{
    clearMetadata();
    clearErrors();
    clearSimulator();
}

void ExplorerPane::clearMetadata()
{
    // Metadata page
    labelDevIcon->clear();
    labelDevManf->clear();
    labelDevMmod->clear();
    labelDevNick->clear();
    labelFileName->clear();
    labelFileSize->clear();
    labelFileZsize->clear();
}

void ExplorerPane::clearErrors()
{
    QTextEdit *errorWidget = dynamic_cast<QTextEdit *>(errorsPage);
    if (errorWidget)
    {
        errorWidget->clear();
    }
}

void ExplorerPane::clearSimulator()
{
    // Output page
    QTextEdit *outputWidget = dynamic_cast<QTextEdit *>(outputPage);
    if (outputWidget)
    {
        outputWidget->clear();
    }
}

void ExplorerPane::updateMetadata(const QString &rml, const QString &uri, const QString& hostAddress)
{
    // Parse RML and extract the metadata
    QXmlStreamReader reader(rml);

    while (!reader.atEnd())
    {
        reader.readNext();
        if (!reader.isStartElement())
            continue;

        //qDebug() << "start-element=" << reader.name();

        if (reader.name() == "manufacturer")
            labelDevManf->setText(reader.readElementText());
        else if (reader.name() == "mmodel")
            labelDevMmod->setText(reader.readElementText());
        else if (reader.name() == "nickname")
        {
            // >>> KLUDGE get the language specified in the ENV
            reader.readNextStartElement(); // <localetitle>
            reader.readNextStartElement(); // <en>
            if (reader.name() == "en")
                labelDevNick->setText(reader.readElementText());
            // << KLUDGE get the language specified in the ENV
        }
        else if (reader.name() == "category")
        {
            XPCategory category(rmlCachePath, hostAddress);
            QIcon icon = category.getIcon(reader.readElementText(), 72);
            if (icon.isNull())
                icon = QIcon(":/images/unknown-device");
            labelDevIcon->setPixmap(icon.pixmap(48,48));
        }
    }
    if (reader.hasError())
    {
        qDebug() << "ExplorerPane::updateMetadata RML error=" << reader.errorString();
    }

    // Update the RML file information
    labelFileName->setText(QString("File:\t%1").arg(uri));
    labelFileSize->setText(QString("Size:\t%1").arg(rml.length()));
    // >>> KLUDGE Actually gZip the file and get the correct size
    labelFileZsize->setText(QString("Zip size:\t%1").arg(rml.length()/2)); //TODO gzip file and get size
    // <<< KLUDGE Actually gZip the file and get the correct size
}

void ExplorerPane::updateErrors(const QString& errors)
{
    QTextEdit *errorWidget = dynamic_cast<QTextEdit *>(errorsPage);
    if (errorWidget)
    {
        // Get the current text
        QString text = errorWidget->toPlainText();
        if (!text.isEmpty())
        {
            // Set the text color to light grey
            errorWidget->setTextColor(QColor("grey"));
            // Restore the current text
            errorWidget->setPlainText(text);
        }
        // Set the text color to red
        errorWidget->setTextColor(QColor("red"));
        // Append the new error message
        errorWidget->append(errors);
        // Place cursor at end of text
        errorWidget->moveCursor(QTextCursor::End);
        // Raise the Error tab
        tabWidget->setCurrentWidget(errorsPage);
    }
}

void ExplorerPane::updateOutput(const QString& output)
{
    QTextEdit *outputWidget = dynamic_cast<QTextEdit *>(outputPage);
    if (outputWidget)
    {
        // Append the new output
        outputWidget->append(output);
        // Raise the Output tab
        tabWidget->setCurrentWidget(outputPage);
    }
}

// End of file
