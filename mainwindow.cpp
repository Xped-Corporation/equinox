// This module implements the main window of the RML IDE.
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

#include <QDir>
#include <QUuid>
#include <QDebug>
#include <QMenuBar>
#include <QPrinter>
#include <QFileInfo>
#include <QSettings>
#include <QStatusBar>
#include <QFileDialog>
#include <QPrintDialog>
#include <QDockWidget>
#include <QMessageBox>
#include <QTextStream>
#include <QDomDocument>
#include <QMapIterator>
#include <QApplication>
#include <QTemporaryFile>
#include <QXmlStreamReader>
#include <QProcessEnvironment>

#include <Qsci/qscilexerxml.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qsciprinter.h>

#include <xpnetfile.h>
#include <xpunitfile.h>
#include <xpnetservicewatcher.h>

#include "rmltransferdialog.h"
#include "settingsdialog.h"
#include "finddialog.h"
#include "mainwindow.h"

// Definition of markers
#define MARKER_GEN 1
#define MARKER_DEBUG 2
//
#define MARKER_GEN_MASK ((int)(1<<MARKER_GEN))
#define MARKER_DEBUG_MASK ((int)(1<<MARKER_DEBUG))
//
#define RMLIDE_EMPTY_FILEPATH ":empty-file-path"
#define RMLIDE_NEW_FILEPATH ":empty.prf"
//
//#define RMLIDE_DOWNLOAD_UNIT


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), editPane(0), proxy(0), editTabWidget(0)
{
    setMinimumSize(800, 640);
    setWindowTitle(tr(WINDOW_TITLE) + tr(" - Searching for hub"));
    setUnifiedTitleAndToolBarOnMac(true);

    // Initialize Application settings
    QCoreApplication::setOrganizationName("Xped");
    QCoreApplication::setOrganizationDomain("xped.com");
    QCoreApplication::setApplicationName("Equinox");
    readSettings();

    // Save the RML cache path
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    rmlCachePath = env.value("XP_RDFCACHE_PATH", QDir::homePath()+"/xped");

    // Watch for directory changes in the RML cache area
    fsWatcher = new QFileSystemWatcher(this);
    connect(fsWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(onDirectoryChanged(QString)));

    // Watch for the adrc-service
    XPNetServiceWatcher *watcher = new XPNetServiceWatcher("adrc-gateway", "_rcp._tcp", this);
    connect(watcher, SIGNAL(serviceRegistered(QString)), this, SLOT(onServiceRegistered(QString)));
    connect(watcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(onServiceUnregistered(QString)));

    // Get file name passed on command line
    QString editorFileName;
    QStringList args = QApplication::arguments();
    if (args.count() > 1 && QFile::exists(args.at(1)))
        editorFileName = args.at(1);
    else
        editorFileName = RMLIDE_NEW_FILEPATH;

    // Create the GUI
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createDockWindows();
    createEditor(editorFileName);
}

MainWindow::~MainWindow()
{
    qDebug() << "~MainWindow";
}

void MainWindow::createEditor(const QString& filePath)
{
    QFileInfo fi(filePath);
    if (!fi.exists())
        return;

    // Create a new editor for filePath
    //
    editPane = new QsciScintilla;

    // Turn on line number in the margin
    editPane->setMarginsFont(editPane->font());
    editPane->setMarginWidth(0, "9999");
    editPane->setMarginLineNumbers(0, true);
    editPane->setMarginsBackgroundColor(QColor("#EEEEEE"));

    // Set syntax highlighting based on file extension
    QString fileSuffix = fi.suffix();
    if (filePath.isEmpty() || fileSuffix == "prf")
    {
        QsciLexerXML *lexer = new QsciLexerXML();
        lexer->setDefaultFont(editPane->font());
        editPane->setLexer(lexer);
    }
    else if (fileSuffix == "ino" || fileSuffix == "cpp" || fileSuffix == "h")
    {
        QsciLexerCPP *lexer = new QsciLexerCPP();
        lexer->setDefaultFont(editPane->font());
        editPane->setLexer(lexer);
    }

    // Initialise the caret line visible with special background color
    editPane->setCaretLineVisible(true);
    editPane->setCaretLineBackgroundColor(QColor("#ffe4e4"));

    // Set tabs and indentation
    editPane->setIndentationWidth(4);
    editPane->setTabWidth(4);

    // Auto indent
    editPane->setAutoIndent(true);

    // Create the general marker
    QIcon icon = QIcon(":images/bookmark-new.png");
    if (editPane->markerDefine(icon.pixmap(19,16), MARKER_GEN) < 0)
        qDebug() << "MainWindow::createEditor GEN markert not created";

    // Register editor signals
    connect(editPane, SIGNAL(cursorPositionChanged(int,int)), this, SLOT(onCursorPositionChanged(int,int)));
    connect(editPane, SIGNAL(textChanged()), this, SLOT(documentWasModified()));
    connect(editPane, SIGNAL(copyAvailable(bool)), cutAct, SLOT(setEnabled(bool)));
    connect(editPane, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));

    // Open each new editor in a tab
    //
    if (editTabWidget == 0)
    {
        editTabWidget = new QTabWidget;
        editTabWidget->setTabPosition(QTabWidget::North);
        editTabWidget->setFocusPolicy(Qt::NoFocus);
        editTabWidget->setTabsClosable(true);
        setCentralWidget(editTabWidget);

        connect(editTabWidget, SIGNAL(currentChanged(int)), this, SLOT(onEditorTabChanged(int)));
        connect(editTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(onTabCloseRequested(int)));
    }

    // Read file into editor
    if (!readFile(filePath))
    {
        qDebug() << "MainWindow::MainWindow failed opening file=" << filePath;
        return;
    }

    // Add new tab
    QString tabTitle = (filePath == RMLIDE_NEW_FILEPATH) ? getUntitledDocumentName() : fi.fileName();
    QString tabFilePath = (filePath == RMLIDE_NEW_FILEPATH) ? RMLIDE_EMPTY_FILEPATH : filePath;
    int index = editTabWidget->addTab(editPane, tabTitle);

    editTabFilePaths[editPane] = tabFilePath;
    editTabWidget->setCurrentIndex(index);
}

void MainWindow::createActions()
{
    // File actions
    newRMLAct = new QAction(QIcon(":/images/document-new"), tr("&New..."), this);
    newRMLAct->setShortcuts(QKeySequence::New);
    newRMLAct->setStatusTip(tr("Create a new empty RML document"));
    connect(newRMLAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openRMLAct = new QAction(QIcon(":/images/document-open"), tr("&Open..."), this);
    openRMLAct->setShortcuts(QKeySequence::Open);
    openRMLAct->setStatusTip(tr("Open a saved RML file"));
    connect(openRMLAct, SIGNAL(triggered()), this, SLOT(openFile()));

    closeRMLAct = new QAction(QIcon(":/images/document-close"), tr("&Close"), this);
    closeRMLAct->setShortcuts(QKeySequence::Close);
    closeRMLAct->setStatusTip(tr("Close the current RML"));
    connect(closeRMLAct, SIGNAL(triggered()), this, SLOT(closeFile()));

    saveRMLAct = new QAction(QIcon(":/images/document-save"), tr("&Save"), this);
    saveRMLAct->setShortcuts(QKeySequence::Save);
    saveRMLAct->setStatusTip(tr("Save the RML to a file"));
    connect(saveRMLAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(QIcon(":/images/document-save"), tr("Save &as..."), this);
    //saveAsAct->setShortcuts(QKeySequence::Save);
    saveAsAct->setStatusTip(tr("Save the RML to another file"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));

    settingsAct = new QAction(QIcon(":/images/document-properties"), tr("Se&ttings..."), this);
    settingsAct->setShortcuts(QKeySequence::Preferences);
    settingsAct->setStatusTip(tr("Change application settings"));
    connect(settingsAct, SIGNAL(triggered()), this, SLOT(settings()));

    printAct = new QAction(QIcon(":/images/print.png"), tr("&Print..."), this);
    printAct->setShortcuts(QKeySequence::Print);
    printAct->setStatusTip(tr("Print the current RML document"));
    connect(printAct, SIGNAL(triggered()), this, SLOT(print()));

    quitAct = new QAction(tr("&Quit"), this);
    quitAct->setShortcuts(QKeySequence::Quit);
    quitAct->setStatusTip(tr("Quit the application"));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

    // About actions
    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("About GPSi Simulator"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    // Project actions
    runRMLAct = new QAction(QIcon(":/images/media-playback-start"), tr("&Run in simulator"), this);
    runRMLAct->setShortcut(tr("Ctrl+R"));
    runRMLAct->setStatusTip(tr("Run RML in editor in the device simulator"));
    connect(runRMLAct, SIGNAL(triggered()), this, SLOT(runRML()));

    validateRMLAct = new QAction(QIcon(":/images/account-logged-in"), tr("&Validate"), this);
    validateRMLAct->setShortcut(tr("Ctrl+B"));
    validateRMLAct->setStatusTip(tr("Validate RML in editor"));
    connect(validateRMLAct, SIGNAL(triggered()), this, SLOT(validateRML()));

    uploadRMLAct = new QAction(QIcon(":/images/go-up"), tr("&Upload..."), this);
    uploadRMLAct->setStatusTip(tr("Upload RML in editor to selected device"));
    connect(uploadRMLAct, SIGNAL(triggered()), this, SLOT(uploadRML()));

    downloadRMLAct = new QAction(QIcon(":/images/go-down"), tr("&Download..."), this);
    downloadRMLAct->setStatusTip(tr("Download RML from selected device to editor"));
    connect(downloadRMLAct, SIGNAL(triggered()), this, SLOT(downloadRML()));

#if 0
    langXmlAct = new QAction(tr("&XML highlighting"), this);
    //langXmlAct->setShortcut(tr("Ctrl+R"));
    langXmlAct->setStatusTip(tr("Set XML syntax highlighting"));
    connect(langXmlAct, SIGNAL(triggered()), this, SLOT(setXmlLang()));

    langCppAct = new QAction(tr("&C++ highlighting"), this);
    //langCppAct->setShortcut(tr("Ctrl+R"));
    langCppAct->setStatusTip(tr("Set C++ syntax highlighting"));
    connect(langCppAct, SIGNAL(triggered()), this, SLOT(setCppLang()));
#endif

    // Edit actions
    undoAct = new QAction(tr("Undo"), this);
    undoAct->setShortcut(tr("Ctrl+Z"));
    undoAct->setStatusTip(tr("Undo"));
    connect(undoAct, SIGNAL(triggered()), this, SLOT(undo()));

    redoAct = new QAction(tr("Redo"), this);
    redoAct->setShortcut(tr("Ctrl+Y"));
    redoAct->setStatusTip(tr("Redo"));
    connect(redoAct, SIGNAL(triggered()), this, SLOT(redo()));

    copyAct = new QAction(tr("Copy"), this);
    copyAct->setShortcut(tr("Ctrl+C"));
    copyAct->setStatusTip(tr("Copy to clipboard"));
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));

    pasteAct = new QAction(tr("Paste"), this);
    pasteAct->setShortcut(tr("Ctrl+V"));
    pasteAct->setStatusTip(tr("Paste from clipboard"));
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));

    cutAct = new QAction(tr("Cut"), this);
    cutAct->setShortcut(tr("Ctrl+X"));
    cutAct->setStatusTip(tr("Cut to clipboard"));
    connect(cutAct, SIGNAL(triggered()), this, SLOT(cut()));

    selectAllAct = new QAction(tr("Select All"), this);
    selectAllAct->setShortcut(tr("Ctrl+A"));
    selectAllAct->setStatusTip(tr("Select all text"));
    connect(selectAllAct, SIGNAL(triggered()), this, SLOT(selectAll()));

    findAct = new QAction(tr("&Find and replace..."), this);
    findAct->setShortcut(tr("Ctrl+F"));
    findAct->setStatusTip(tr("Find text or regexp"));
    connect(findAct, SIGNAL(triggered()), this, SLOT(find()));

    matchBraceAct = new QAction(tr("&Match brace"), this);
    matchBraceAct->setShortcut(tr("Ctrl+{"));
    matchBraceAct->setStatusTip(tr("Find matching brace"));
    connect(matchBraceAct, SIGNAL(triggered()), this, SLOT(moveToMatchingBrace()));

    // Marker actions
    toggleMarkerAct = new QAction(tr("&Toggle marker"), this);
    toggleMarkerAct->setShortcut(tr("Ctrl+M"));
    toggleMarkerAct->setStatusTip(tr("Add/remove a marker"));
    connect(toggleMarkerAct, SIGNAL(triggered()), this, SLOT(toggleMarker()));

    nextMarkerAct = new QAction(tr("&Next marker"), this);
    nextMarkerAct->setShortcut(tr("Ctrl+>"));
    nextMarkerAct->setStatusTip(tr("Find next marker"));
    connect(nextMarkerAct, SIGNAL(triggered()), this, SLOT(nextMarker()));

    prevMarkerAct = new QAction(tr("&Previous marker"), this);
    prevMarkerAct->setShortcut(tr("Ctrl+<"));
    prevMarkerAct->setStatusTip(tr("Find previous marker"));
    connect(prevMarkerAct, SIGNAL(triggered()), this, SLOT(prevMarker()));

    // View actions
    clearErrorsAct = new QAction(tr("Clear &errors"), this);
    clearErrorsAct->setStatusTip(tr("Clear the error view"));
    connect(clearErrorsAct, SIGNAL(triggered()), this, SLOT(clearErrors()));

    clearSimulatorAct = new QAction(tr("Clear &simulator"), this);
    clearSimulatorAct->setStatusTip(tr("Clear the simulator view"));
    connect(clearSimulatorAct, SIGNAL(triggered()), this, SLOT(clearSimulator()));

    // Set up menu item enable/disable
#if 0
    langXmlAct->setEnabled(false);
    langCppAct->setEnabled(true);
#endif
    cutAct->setEnabled(false);
    copyAct->setEnabled(false);
    undoAct->setEnabled(false);
    redoAct->setEnabled(false);
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newRMLAct);
    fileMenu->addAction(openRMLAct);
    fileMenu->addAction(closeRMLAct);
    fileMenu->addAction(saveRMLAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(settingsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(printAct);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    editMenu->addAction(cutAct);
    editMenu->addAction(selectAllAct);
    editMenu->addSeparator();
    editMenu->addAction(findAct);
    editMenu->addAction(matchBraceAct);
    editMenu->addSeparator();
    editMenu->addAction(toggleMarkerAct);
    editMenu->addAction(nextMarkerAct);
    editMenu->addAction(prevMarkerAct);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addSeparator();
    viewMenu->addAction(clearErrorsAct);
    viewMenu->addAction(clearSimulatorAct);

    simMenu = menuBar()->addMenu(tr("&Project"));
    simMenu->addAction(validateRMLAct);
    simMenu->addAction(runRMLAct);
    simMenu->addSeparator();
    simMenu->addAction(downloadRMLAct);
    simMenu->addAction(uploadRMLAct);
    simMenu->addSeparator();

#if 0
    QMenu *langMenu = simMenu->addMenu(tr("&Language"));
    langMenu->addAction(langXmlAct);
    langMenu->addAction(langCppAct);
#endif

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(newRMLAct);
    fileToolBar->addAction(openRMLAct);
    fileToolBar->addAction(saveRMLAct);
    fileToolBar->addAction(settingsAct);

    simToolBar = addToolBar(tr("Project"));
    simToolBar->addAction(downloadRMLAct);
    simToolBar->addAction(validateRMLAct);
    simToolBar->addAction(uploadRMLAct);
    simToolBar->addAction(runRMLAct);
}

void MainWindow::createStatusBar()
{
    QLabel *statusSpacer = new QLabel(" ");
    statusLine = new QLabel;
    statusLine->setFrameStyle(QFrame::StyledPanel);
    statusCol = new QLabel;
    statusCol->setFrameStyle(QFrame::StyledPanel);
    statusMode = new QLabel;
    statusMode->setFrameStyle(QFrame::StyledPanel);
    //
    statusBar()->addWidget(statusLine, 1);
    statusBar()->addWidget(statusCol, 1);
    statusBar()->addWidget(statusMode, 1);
    statusBar()->addWidget(statusSpacer, 6);
    //
    updateStatusBar();
}

void MainWindow::createDockWindows()
{
    QDockWidget *dock;

    dock = new QDockWidget(tr("Devices"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    devicesPane = new DevicesPane(dock);
    dock->setWidget(devicesPane);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());

#if 0 // Create some test data
    world["0"] = "z_12345678_0;Lounge Lamp;Xped;PSW-240AU1V;std.prf";
    world["1"] = "z_23456789_0;Coffee maker;Xped;PSW-240AU1;std.prf";
    world["2"] = "z_34567890_0;TV;Toshiba;CT-90329;min.prf;std.prf;full.prf";
    world["3"] = "z_45678901_0;DVD;Pioneer;DV-466;min.prf;std.prf";
    fsWatcher->addPath(QString("%1/profiles/%2/%3").arg(rmlCachePath).arg("Xped").arg("PSW-240AU1"));
    devicesPane->update(QString(), "test", world);
#endif

    dock = new QDockWidget(tr("Explorer"), this);
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);
    explorerPane = new ExplorerPane(dock);
    dock->setWidget(explorerPane);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());
}

void MainWindow::newFile()
{
    QSettings settings;

    bool useDefaultTemplate = settings.value("settings-dialog/defaultRmlTemplate", true).toBool();
    QString templatePath = settings.value("settings-dialog/rmlTemplatePath").toString();

    if (useDefaultTemplate)
        createEditor(RMLIDE_NEW_FILEPATH);
    else if (!templatePath.isEmpty())
        createEditor(templatePath);
    else
    {
        QMessageBox::warning(this, tr("New RML"),
            tr("The RML template in the settings is not set.\n"
               "Please check the General settings and select a file."),
            QMessageBox::Close);
    }
}

void MainWindow::openFile()
{
    // Create a file open dilaog to allow the file to be selected
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open file"), QDir::homePath(),
                           tr("RML Files (*.prf);;Arduino Files (*.ino);;C++ Files (*.cpp *.h);;All Files (*.*)"));

    if (filePath.isEmpty())
        return;

    // Open file in a new editor
    createEditor(filePath);
}

void MainWindow::closeFile()
{
    // Check if we need to save document
    int ret = maybeSave();
    if (ret < 0)
        return;
    else if (ret > 0)
        save();

    QsciScintilla *pane = editPane;
    int index = editTabWidget->indexOf(editPane);

    // Remove the editTabFilePath
    editTabFilePaths.remove(editPane);

    // Delete the tab
    editTabWidget->removeTab(index);

    // Delete the editor
    delete pane;
}

void MainWindow::runRML()
{
    QSettings settings;

    bool validateBeforeRun = settings.value("settings-dialog/validateBeforeRun", true).toBool();
    if (validateBeforeRun)
    {
        if (!validateRML())
            return;
    }

    // Prepare the simulation files
    //
    QString manf, mmod;
    QString rml = editPane->text();
    QXmlStreamReader reader(rml);

    while (!reader.atEnd())
    {
        reader.readNext();
        if (!reader.isStartElement())
            continue;

        if (reader.name() == "manufacturer")
            manf = reader.readElementText();
        else if (reader.name() == "mmodel")
            mmod = reader.readElementText();
    }
    if (reader.hasError())
    {
        QString message = tr("XML syntax error in editor RML:\n%1.")
                             .arg(reader.errorString());
        explorerPane->updateOutput(message);
        return;
    }

    QString path = QString("%1/profiles/%2/%3").arg(rmlCachePath).arg(manf).arg(mmod);
    QDir dir(path);
    if (!dir.exists())
        dir.mkpath(path);

    // Save RML to file with name sim.prf
    QString filename = path+"/sim.prf";
    QFile file(filename);
    if (!file.open(QFile::WriteOnly))
    {
        QString message = tr("Cannot write simulation file %1:\n%2.")
                             .arg(filename)
                             .arg(file.errorString());
        explorerPane->updateOutput(message);
        return;
    }

    QTextStream out(&file);
    out << rml;
    file.close();

    // Execute the RML simulator with the file as argument
    //
    QString program = "rmlsim";
    QStringList args;
    args << filename;

    rmlSimulator = new QProcess(this);
    rmlSimulator->start(program, args);
    if (!rmlSimulator->waitForStarted())
    {
        explorerPane->updateOutput("RML simulator failed to start!");
        return;
    }

    // Attach to the process stdout/stderr and write it to the explorerPane
    connect(rmlSimulator, SIGNAL(readyReadStandardOutput()), this, SLOT(onSimulatorStdoutReadyRead()));
    connect(rmlSimulator, SIGNAL(readyReadStandardError()), this, SLOT(onSimulatorStderrReadyRead()));
    connect(rmlSimulator, SIGNAL(finished(int)), rmlSimulator, SLOT(deleteLater()));
}

bool MainWindow::validateRML()
{
    // If RML is empty warn user
    QString rml = editPane->text();
    if (rml.isEmpty())
    {
        QMessageBox::warning(this, tr("Validate RML"),
            tr("The RML document in the editor is empty."), QMessageBox::Cancel);
        return false;
    }

    // Generate a temp file for the RML to be validated
    QTemporaryFile rmlFile;
    if (!rmlFile.open())
    {
        QMessageBox::warning(this, tr("Validate RML"),
            tr("Unable to create temporary RML file."), QMessageBox::Cancel);
        return false;
    }

    QTextStream rmlOut(&rmlFile);
    rmlOut << rml;
    rmlFile.close();

    // Select which DTD to use
    QSettings settings;

    bool useDefaultDtd = settings.value("settings-dialog/defaultRmlDtd", true).toBool();
    QString dtdPath = settings.value("settings-dialog/rmlDtdPath").toString();

    QFile dtdFile;
    if (!useDefaultDtd)
    {
        dtdFile.setFileName(dtdPath);
        if (!dtdFile.exists())
        {
            QMessageBox::warning(this, tr("Validate RML"),
                tr("The DTD file does not exist.\nCheck Settings->General and correct."),
                QMessageBox::Cancel);
            return false;
        }
    }
    else // use the built-in DTD in the qrc
    {
        QString path = QDir::homePath()+"/xped/dtd";
        QDir dir(path);
        if (!dir.exists(path))
            dir.mkpath(path);
        dtdFile.setFileName(path+"/default.dtd");

        // Create the DTD file
        if (!dtdFile.open(QFile::WriteOnly))
        {
            QMessageBox::warning(this, tr("Validate RML"),
                tr("Failed to create default DTD file."), QMessageBox::Cancel);
            return false;
        }

        // Copy the DTD file from qrc to filesystem
        QFile dtdQrc(":rml.dtd");
        if (!dtdQrc.open(QFile::ReadOnly))
        {
            QMessageBox::warning(this, tr("Validate RML"),
                tr("The DTD file does not exist.\nCheck Settings->General and correct."),
                QMessageBox::Cancel);
            return false;
        }

        QTextStream dtdOut(&dtdFile);
        dtdOut << dtdQrc.readAll();
        dtdQrc.close();
        dtdFile.close();
    }

    // Execute the xmllint command
    QString program = "xmllint";
    QStringList args;
    args << "--valid" << "--noout" << "--nonet" << "--dtdvalid" << "file://"+dtdFile.fileName() << rmlFile.fileName();
    QString result = program + " " + args.join(" ") + "\n" + tr("RML is well formed and valid.");

    QProcess *validator = new QProcess(this);
    validator->start(program, args);
    if (!validator->waitForStarted(3*1000))
    {
        explorerPane->updateErrors("Validation failed: the xmllint utility is not installed!");
        return false;
    }

    int retcode = validator->waitForFinished();
    validator->deleteLater();

    // Handle parsing errors
    if (retcode != 0)
    {
        QString err = validator->readAllStandardError();
        if (err.contains("Validation failed: no DTD found !"))
        {
            int pos = err.indexOf("^\n");
            if (pos > 0)
                err = err.mid(pos+2);
            if (!err.isEmpty())
            {
                result = program + " " + args.join(" ") + "\n\n" + err;
                explorerPane->updateErrors(result);
                return false;
            }
        }
    }

    explorerPane->updateErrors(result);
    return true;
}

void MainWindow::uploadRML()
{
    // If proxy is null/invalid warn user
    if (proxy == 0 || !proxy->isValid())
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("The network is down or there is no hub detected.\n"
               "Check the equipment is powered on."),
            QMessageBox::Close);
        return;
    }

    // If no device/file is selected inform user
    QString deviceId = devicesPane->device();
    QString fileName = devicesPane->filename();
    if (deviceId.isEmpty() || fileName.isEmpty())
    {
        QMessageBox::information(this, tr("Upload RML"),
            tr("Please select a device file from the 'Devices List'\nand try again."),
            QMessageBox::Close);
        return;
    }

    // Extract details from the world
    QStringList values = world[deviceId].split(";");
    QString nick = values.at(1);
    QString manf = values.at(2);
    QString mmod = values.at(3);
    QString hostAddress = proxy->getHostAddress();
    //int hostPort = proxy->getHostPort();

    // Get input from user
    QString message = QString(tr("Uploading ")+fileName+tr(" to ")+nick);
    QCheckBox *fromDeviceCheck = new QCheckBox(tr("Direct to device"));
    RmlTransferDialog dialog(fromDeviceCheck, QIcon(":/images/go-up"), tr("Upload RML"), message, this);

    if (dialog.exec() == QDialog::Rejected)
        return;

    // Upload the RML and UNIT files
    if (fromDeviceCheck->isChecked())
        uploadToDevice(fileName, hostAddress, deviceId);
    else
        uploadToCache(manf, mmod, fileName, hostAddress);

    // *** TODO *** Refresh the device list (will also update the local cache)
}

bool MainWindow::uploadToDevice(QString fileName,
                                QString hostAddress,
                                QString deviceId)
{
    // >>> KLUDGE Hardcoded ADRC daemon root for now
    QString rcp = "<adrc><device id='%1'><put ln='/var/cache/xped/uploads/%2' at='2'>fs/%3</put></device></adrc>";
    // <<< KLUDGE Hardcoded ADRC daemon root for now

    // (1) Send RML to remote host
    //
    // Generate a temp file name for the RML
    QUuid uuid = QUuid::createUuid();
    QString tmpFilename = QString("%1.tmp").arg(uuid.toString());
    // get rid off leading { and trailing }
    tmpFilename.remove('{');
    tmpFilename.remove('}');

    // Transfer local RML file to host upload location using XPNetFile
    QUrl url = QUrl(QString("http://%1/uploads/%2").arg(hostAddress).arg(tmpFilename));
    //url.setUserName("anonymous");
    //url.setPassword("none");

    XPNetFile rmlFile(url, QString("/tmp/%1").arg(tmpFilename));
    if (!rmlFile.open(QFile::WriteOnly))
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to open local RML temp file for writing"),
            QMessageBox::Close);
        return false;
    }

    QString rml = editPane->text();

    if (rmlFile.write(rml.toLatin1()) != rml.size())
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to write RML data to local temp file"),
            QMessageBox::Close);
        return false;
    }

    if (!rmlFile.close()) // this initiates the file transfer
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to transfer RML temp file to remote host"),
            QMessageBox::Close);
        return false;
    }

    // Delete the local file
    rmlFile.remove();

    // (2) Ask remote host to send RML file to the device#if 1
    //
    QString oxml = rcp.arg(deviceId).arg(tmpFilename).arg(fileName);

    // Execute the file transfer
    forever
    {
        QString ixml = proxy->Execute(oxml);

        if (ixml.contains("<ack"))
            break;
        else // transfer failed
        {
            int ret = QMessageBox::warning(this, tr("Upload RML"),
                          tr("The RML file failed to upload.\n"
                             "Do you want to try again?"),
                          QMessageBox::Yes | QMessageBox::Default,
                          QMessageBox::Cancel | QMessageBox::Escape);
            if (ret == QMessageBox::Yes)
                continue;
            else // QMessageBox::Cancel
                return false;
        }
    }

    // (3) Send UNIT data to remote host
    //
    // Create the unit data
    XPUnitFile unit(rml);
    if (!unit.isValid())
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to generate UNIT data from RML"),
            QMessageBox::Close);
        return false;
    }

    // Generate a temp file name for the UNIT data
    uuid = QUuid::createUuid();
    tmpFilename = QString("%1.tmp").arg(uuid.toString());
    // get rid off leading { and trailing }
    tmpFilename.remove('{');
    tmpFilename.remove('}');

    // Transfer local UNIT file to host using XPNetFile
    url = QUrl(QString("http://%1/uploads/%2").arg(hostAddress).arg(tmpFilename));
    //url.setUserName("anonymous");
    //url.setPassword("none");

    XPNetFile unitFile(url, QString("/tmp/%1").arg(tmpFilename));
    if (!unitFile.open(QFile::WriteOnly))
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to open local UNIT temp file for writing"),
            QMessageBox::Close);
        return false;
    }

    if (unitFile.write(unit.data(), unit.size()) != unit.size())
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to write UNIT data to local temp file"),
            QMessageBox::Close);
        return false;
    }

    if (!unitFile.close()) // this initiates the file transfer
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to transfer UNIT temp file to remote host"),
            QMessageBox::Close);
        return false;
    }

    // Delete the local file
    unitFile.remove();

    // (4) Ask remote host to send UNIT file to the device
    //
    oxml = rcp.arg(deviceId).arg(tmpFilename).arg("unit");

    // Execute the file transfer
    forever
    {
        QString ixml = proxy->Execute(oxml);

        if (ixml.contains("<ack"))
            break;
        else // transfer failed
        {
            int ret = QMessageBox::warning(this, tr("Upload RML"),
                          tr("The RML file failed to upload.\n"
                             "Do you want to try again?"),
                          QMessageBox::Yes | QMessageBox::Default,
                          QMessageBox::Cancel | QMessageBox::Escape);
            if (ret == QMessageBox::Yes)
                continue;
            else // QMessageBox::Cancel
                return false;
        }
    }

    return true;
}

bool MainWindow::uploadToCache(QString manf,
                               QString mmod,
                               QString fileName,
                               QString hostAddress)
{
    QString rml = editPane->text();

    // (1) Ensure that the RML in the editor has the same manufacturer and mmodel
    if (!validateManfMmodel(rml))
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("The RML contains an invalid manufacturer or mmodel for this device."),
            QMessageBox::Close);
        return false;
    }

    // (2) Send RML to remote host
    //
    // Generate RML filename in the local cache
    QString tmpFilename = QString("%1/profiles/%2/%3/%4")
            .arg(rmlCachePath)
            .arg(manf)
            .arg(mmod)
            .arg(fileName);

    // Transfer local temp file to host RML cache location using XPNetFile
    QUrl url = QUrl(QString("http://%1/profiles/%2/%3/%4").arg(hostAddress).arg(manf).arg(mmod).arg(fileName));
    //url.setUserName("anonymous");
    //url.setPassword("none");

    XPNetFile rmlFile(url, tmpFilename);
    if (!rmlFile.open(QFile::WriteOnly))
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to open local RML temp file for writing"),
            QMessageBox::Close);
        return false;
    }

    if (rmlFile.write(rml.toLatin1()) != rml.size())
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to write RML data to local temp file"),
            QMessageBox::Close);
        return false;
    }

    if (!rmlFile.close()) // this initiates the file transfer
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to transfer RML temp file to remote host"),
            QMessageBox::Close);
        return false;
    }

#ifdef RMLIDE_DOWNLOAD_UNIT
    // (3) Send UNIT data to remote host
    //
    // Create the unit data
    XPUnitFile unit(rml);
    if (!unit.isValid())
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to generate UNIT data from RML"),
            QMessageBox::Close);
        return false;
    }

    // Generate RML filename in the local cache
    tmpFilename = QString("%1/profiles/%2/%3/unit")
            .arg(rmlCachePath)
            .arg(manf)
            .arg(mmod);

    // Transfer local UNIT file to host using XPNetFile
    url = QUrl(QString("http://%1/profiles/%2/%3/unit").arg(hostAddress).arg(manf).arg(mmod));
    //url.setUserName("anonymous");
    //url.setPassword("none");

    XPNetFile unitFile(url, tmpFilename);
    if (!unitFile.open(QFile::WriteOnly))
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to open local UNIT temp file for writing"),
            QMessageBox::Close);
        return false;
    }

    if (unitFile.write(unit.data(), unit.size()) != unit.size())
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to write UNIT data to local temp file"),
            QMessageBox::Close);
        return false;
    }

    if (!unitFile.close()) // this initiates the file transfer
    {
        QMessageBox::warning(this, tr("Upload RML"),
            tr("Failed to transfer UNIT temp file to remote host"),
            QMessageBox::Close);
        return false;
    }
#endif

    return true;
}

void MainWindow::downloadRML()
{
    // If proxy is null/invalid warn user
    if (proxy == 0 || !proxy->isValid())
    {
        QMessageBox::warning(this, tr("Download RML"),
            tr("The network is down or there is no hub detected.\n"
               "Check the equipment is powered on."),
            QMessageBox::Close);
        return;
    }

    // If no device/file is selected inform user
    QString deviceId = devicesPane->device();
    QString fileName = devicesPane->filename();
    if (deviceId.isEmpty() || fileName.isEmpty())
    {
        QMessageBox::information(this, tr("Download RML"),
            tr("Please select a device file from the 'Devices List'\nand try again."),
            QMessageBox::Close);
        return;
    }

    // Extract nick, manf and mmod from the world
    QStringList values = world[deviceId].split(";");
    QString nick = values.at(1);
    QString manf = values.at(2);
    QString mmod = values.at(3);
    QString hostAddress = proxy->getHostAddress();
    //int hostPort = proxy->getHostPort();

    // Warn user if file is already open in an editor
    QString requestPath = QString("%1/%2/%3/%4")
            .arg(rmlCachePath)
            .arg(manf)
            .arg(mmod)
            .arg(fileName);

    QMapIterator<QWidget *, QString> i(editTabFilePaths);
    while (i.hasNext())
    {
        i.next();
        if (i.value() == requestPath)
        {
            int ret = QMessageBox::information(this, tr("Download RML"),
                            tr("The RML document is already open in another editor"),
                            QMessageBox::Ok,
                            QMessageBox::Cancel | QMessageBox::Default);
            if (ret == QMessageBox::Cancel)
                return;
        }
    }

    // Get input from the user
    QString message = QString(tr("Downloading ")+fileName+tr(" from ")+nick);
    QCheckBox *fromDeviceCheck = new QCheckBox(tr("Direct from device"));
    RmlTransferDialog dialog(fromDeviceCheck, QIcon(":/images/go-down"), tr("Download RML"), message, this);

    if (dialog.exec() == QDialog::Rejected)
        return;

    // Remove the device's directory from QFileSystemWatcher
    fsWatcher->removePath(QString("%1/profiles/%2/%3").arg(rmlCachePath).arg(manf).arg(mmod));

    // Downlaod the RML and UNIT files
    QString filePath;
    if (fromDeviceCheck->isChecked())
        filePath = downloadFromDevice(manf, mmod, fileName, hostAddress, deviceId);
    else
        filePath = downloadFromCache(manf, mmod, fileName, hostAddress);

    // Reinstate the device's directory to QFileSystemWatcher
    fsWatcher->addPath(QString("%1/profiles/%2/%3").arg(rmlCachePath).arg(manf).arg(mmod));

    // Put RML into an new editor
    if (!filePath.isEmpty())
    {
        devicesPane->update(hostAddress, hubName, world);
        createEditor(filePath);
    }
}

QString MainWindow::downloadFromDevice(QString manf,
                                       QString mmod,
                                       QString fileName,
                                       QString hostAddress,
                                       QString deviceId)
{
    // >>> KLUDGE Hardcoded ADRC daemon root for now
    QString rcp = "<adrc><device id='%1'><get ln='/var/cache/xped/profiles/%2/%3/%4' at='2'>fs/%4</get></device></adrc>";
    // <<< KLUDGE Hardcoded ADRC daemon root for now

    // Download RML file into the user's local cache
    //
    {
        QString localFilename = QString("%1/profiles/%2/%3/%4")
                .arg(rmlCachePath)
                .arg(manf)
                .arg(mmod)
                .arg(fileName);

        QString oxml = rcp.arg(deviceId).arg(manf).arg(mmod).arg(fileName);

        // Execute the file transfer
        forever
        {
            QString ixml = proxy->Execute(oxml);

            if (ixml.contains("<ack"))
            {
                // Use XPNetFile to get the file from the HUB
                QUrl url(QString("http://%1/profiles/%2/%3/%4").arg(hostAddress).arg(manf).arg(mmod).arg(fileName));
                //url.setUserName("anonymous");
                //url.setPassword("none");

                // Save file to local directory
                XPNetFile file(url, localFilename);
                if (file.exists())
                    break;
                else // transfer failed
                {
                    int ret = QMessageBox::warning(this, tr("Download RML"),
                                  tr("The RML file failed to download.\n"
                                     "Do you want to try again?"),
                                  QMessageBox::Yes | QMessageBox::Default,
                                  QMessageBox::Cancel | QMessageBox::Escape);
                    if (ret == QMessageBox::Yes)
                        continue;
                    else // QMessageBox::Cancel
                        return QString();
                }
            }
            else // transfer failed
            {
                int ret = QMessageBox::warning(this, tr("Download RML"),
                              tr("The RML file failed to download.\n"
                                 "Do you want to try again?"),
                              QMessageBox::Yes | QMessageBox::Default,
                              QMessageBox::Cancel | QMessageBox::Escape);
                if (ret == QMessageBox::Yes)
                    continue;
                else // QMessageBox::Cancel
                    return QString();
            }
        }
    }

#ifdef RMLIDE_DOWNLOAD_UNIT
    // Download UNIT file into the user's local cache
    //
    {
        QString localFilename = QString("%1/profiles/%2/%3/unit")
                .arg(rmlCachePath)
                .arg(manf)
                .arg(mmod);

        QString oxml = rcp.arg(deviceId).arg(manf).arg(mmod).arg("unit");

        // Execute the file transfer
        forever
        {
            QString ixml = proxy->Execute(oxml);

            if (ixml.contains("<ack"))
            {
                // Use XPNetFile to get the file from the HUB
                QUrl url(QString("http://%1/profiles/%2/%3/unit").arg(hostAddress).arg(manf).arg(mmod));
                //url.setUserName("anonymous");
                //url.setPassword("none");

                // Save file to local directory
                XPNetFile file(url, localFilename);
                if (file.exists())
                    break;
                else // transfer failed
                {
                    int ret = QMessageBox::warning(this, tr("Download RML"),
                                  tr("The UNIT file failed to download.\n"
                                     "Do you want to try again?"),
                                  QMessageBox::Yes | QMessageBox::Default,
                                  QMessageBox::Cancel | QMessageBox::Escape);
                    if (ret == QMessageBox::Yes)
                        continue;
                    else // QMessageBox::Cancel
                        return QString();
                }
            }
            else // transfer failed
            {
                int ret = QMessageBox::warning(this, tr("Download RML"),
                              tr("The UNIT file failed to download.\n"
                                 "Do you want to try again?"),
                              QMessageBox::Yes | QMessageBox::Default,
                              QMessageBox::Cancel | QMessageBox::Escape);
                if (ret == QMessageBox::Yes)
                    continue;
                else // QMessageBox::Cancel
                    return QString();
            }
        }
    }
#endif

    // Return the file path of the RML
    QString localFilepath = QString("%1/profiles/%2/%3/%4")
            .arg(rmlCachePath)
            .arg(manf)
            .arg(mmod)
            .arg(fileName);

    return localFilepath;
}

QString MainWindow::downloadFromCache(QString manf,
                                      QString mmod,
                                      QString fileName,
                                      QString hostAddress)
{
    // Download RML file into the user's local cache
    //
    {
        QString localFilename = QString("%1/profiles/%2/%3/%4")
                .arg(rmlCachePath)
                .arg(manf)
                .arg(mmod)
                .arg(fileName);

        // Execute the file transfer
        forever
        {
            // Use XPNetFile to get file from remote host
            QUrl url(QString("http://%1/profiles/%2/%3/%4").arg(hostAddress).arg(manf).arg(mmod).arg(fileName));
            //url.setUserName("anonymous");
            //url.setPassword("none");

            // Save file to local cache directory
            XPNetFile file(url, localFilename);
            if( file.exists())
                break;
            else // transfer failed
            {
                int ret = QMessageBox::warning(this, tr("Download RML"),
                              tr("The RML file failed to download.\n"
                                 "Do you want to try again?"),
                              QMessageBox::Yes | QMessageBox::Default,
                              QMessageBox::Cancel | QMessageBox::Escape);
                if (ret == QMessageBox::Yes)
                    continue;
                else // QMessageBox::Cancel
                    return QString();
            }
        }
    }

#ifdef RMLIDE_DOWNLOAD_UNIT
    // Download UNIT file into the user's local cache
    //
    {
        QString localFilename = QString("%1/profiles/%2/%3/unit")
                .arg(rmlCachePath)
                .arg(manf)
                .arg(mmod);

        // Execute the file transfer
        forever
        {
            // Use XPNetFile to get file from remote host
            QUrl url(QString("http://%1/profiles/%2/%3/unit").arg(hostAddress).arg(manf).arg(mmod));
            url.setUserName("anonymous");
            url.setPassword("none");

            // Save file to local cache directory
            XPNetFile file(url, localFilename);
            if(file.exists())
                break;
            else // transfer failed
            {
                int ret = QMessageBox::warning(this, tr("Download RML"),
                              tr("The UNIT file failed to download.\n"
                                 "Do you want to try again?"),
                              QMessageBox::Yes | QMessageBox::Default,
                              QMessageBox::Cancel | QMessageBox::Escape);
                if (ret == QMessageBox::Yes)
                    continue;
                else // QMessageBox::Cancel
                    return QString();
            }
        }
    }
#endif

    // Return the file path of the RML
    QString localFilepath = QString("%1/profiles/%2/%3/%4")
            .arg(rmlCachePath)
            .arg(manf)
            .arg(mmod)
            .arg(fileName);

    return localFilepath;
}

#if 0
void MainWindow::setXmlLang()
{
    QsciLexerXML *lexer = new QsciLexerXML();
    lexer->setDefaultFont(editPane->font());
    editPane->setLexer(lexer);
    //
    langXmlAct->setEnabled(false);
    langCppAct->setEnabled(true);
}

void MainWindow::setCppLang()
{
    QsciLexerCPP *cppLexer = new QsciLexerCPP();
    cppLexer->setDefaultFont(editPane->font());
    cppLexer->setFoldComments(true);
    editPane->setLexer(cppLexer);
    //
    langXmlAct->setEnabled(true);
    langCppAct->setEnabled(false);
}
#endif

void MainWindow::about()
{
   QMessageBox::about(this, tr("About Equinox"),
                      tr("<b>Equinox</b> is an IDE for developing <b>RML</b> documents.<br><br>"
                         "Resource Modelling Language (RML) is used to describe "
                         "any kind of device or <i>thing</i> including software APIs and has been "
                         "developed to facilitate human to thing and machine to thing interaction.<br><br>"
                         "Copyright (C) 2015 Xped Holdings Limited and/or its subsidiary(-ies).<br><br>"

                         "This program is free software; you can redistribute it and/or modify "
                         "it under the terms of the GNU General Public License as published by "
                         "the Free Software Foundation; either version 3 of the License, or "
                         "(at your option) any later version.<br><br>"

                         "This program is distributed in the hope that it will be useful, "
                         "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                         "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
                         "GNU General Public License for more details.<br><br>"

                         "You should have received a copy of the GNU General Public License "
                         "along with this program; if not, write to the Free Software Foundation, Inc., "
                         "51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA"));
}

bool MainWindow::save()
{
    // Get the file name for the current tab
    QString filePath = editTabFilePaths[editPane];

    if (filePath.isEmpty() || filePath == RMLIDE_EMPTY_FILEPATH)
    {
        if (saveAs())
        {
            editPane->setModified(false);
            setEditorContentModified(false);
            return true;
        }
    }
    else if (writeFile(filePath))
    {
        editPane->setModified(false);
        setEditorContentModified(false);
        return true;
    }

    return false;
}

bool MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), QDir::homePath(),
                           tr("RML Files (*.prf);;Arduino Files (*.ino);;C++ Files (*.cpp *.h);;All Files (*.*)"));

    if (fileName.isEmpty())
        return false;

    return writeFile(fileName);
}

void MainWindow::find()
{
    FindDialog *dialog = new FindDialog(editPane);
    connect(dialog, SIGNAL(finished(int)), dialog, SLOT(deleteLater()));

    dialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    dialog->show();
    dialog->raise();
}

void MainWindow::undo()
{
    editPane->undo();
}

void MainWindow::redo()
{
    editPane->redo();
}

void MainWindow::copy()
{
    editPane->copy();
}

void MainWindow::paste()
{
    editPane->paste();
}

void MainWindow::cut()
{
    editPane->cut();
}

void MainWindow::selectAll()
{
    editPane->selectAll();
}

void MainWindow::moveToMatchingBrace()
{
    editPane->moveToMatchingBrace();
}

void MainWindow::clearErrors()
{
    explorerPane->clearErrors();
}

void MainWindow::clearSimulator()
{
    explorerPane->clearSimulator();
}

void MainWindow::settings()
{
    SettingsDialog dialog(&proxy, this);
    dialog.setFixedWidth(2 * width() / 3);
    dialog.exec();
}

void MainWindow::print()
{
    QsciPrinter printer;
    QPrintDialog printDialog(&printer, this);

    if (printDialog.exec() == QDialog::Accepted)
        printer.printRange(editPane);
}

void MainWindow::toggleMarker()
{
    int line, index, mask;
    editPane->getCursorPosition(&line, &index);
    mask = editPane->markersAtLine(line);

    // If no marker on current line add one
    if ((mask & MARKER_GEN_MASK) == 0) // no marker
        editPane->markerAdd(line, MARKER_GEN);
    else // Otherwise remove marker
        editPane->markerDelete(line, MARKER_GEN);
}

void MainWindow::nextMarker()
{
    int line, index, nextLine;
    editPane->getCursorPosition(&line, &index);
    nextLine = editPane->markerFindNext(line+1, MARKER_GEN_MASK);

    if (nextLine != line)
        editPane->setCursorPosition(nextLine, 0);
}

void MainWindow::prevMarker()
{
    int line, index, prevLine;
    editPane->getCursorPosition(&line, &index);
    prevLine = editPane->markerFindPrevious(line > 0 ? line-1 : 0, MARKER_GEN_MASK);

    if (prevLine != line)
        editPane->setCursorPosition(prevLine, 0);
}

void MainWindow::documentWasModified()
{
    setEditorContentModified(editPane->isModified());

    undoAct->setEnabled(editPane->isUndoAvailable());
    redoAct->setEnabled(editPane->isRedoAvailable());
}

void MainWindow::onCursorPositionChanged(int line, int col)
{
    statusLine->setText(QString(tr("Line: %1")).arg(line+1));
    statusCol->setText(QString(tr("Col: %1")).arg(col+1));
}

bool MainWindow::readFile(const QString& filePath)
{
    // Read the file as text
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString text = file.readAll();
    file.close();
    QApplication::restoreOverrideCursor();

    if (!text.isEmpty())
    {
        QFileInfo fi(filePath);
        QString fileSuffix = fi.suffix();

        // Check if unicode header is in file data and remove
        if (fileSuffix == "prf")
            removeUnicodeHeader(text);

        // Display in the editor
        editPane->setText(text);
        editPane->setModified(false);
        setEditorContentModified(false);

        // Get the address of the hub
        QString hostAddress;
        if (proxy && proxy->isValid())
            hostAddress = proxy->getHostAddress();

        // Update the views
        if (fileSuffix == "prf" && filePath != RMLIDE_NEW_FILEPATH)
            explorerPane->updateMetadata(text, filePath, hostAddress);

        updateStatusBar();
    }

    return true;
}

bool MainWindow::writeFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly))
    {
        QMessageBox::warning(this, tr("Save to file"),
            tr("Cannot write file %1:\n%2.").arg(filePath).arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << editPane->text();
    QApplication::restoreOverrideCursor();

    QString tabFilePath = editTabFilePaths[editPane];
    if (filePath != tabFilePath) // file name changed
    {
        // Change the tab title
        QFileInfo fi(filePath);
        int index = editTabWidget->indexOf(editPane);
        editTabWidget->setTabText(index, fi.fileName());

        // Change the editTabFilePath
        editTabFilePaths[editPane] = filePath;
    }

    return true;
}

void MainWindow::onSimulatorStdoutReadyRead()
{
    QString text = rmlSimulator->readAllStandardOutput();
    explorerPane->updateOutput(text);
}

void MainWindow::onSimulatorStderrReadyRead()
{
    QString text = rmlSimulator->readAllStandardError();
    explorerPane->updateOutput(text);
}

void MainWindow::onServiceRegistered(QString addressAndPort)
{
    qDebug() << "MainWindow::onServiceRegistered=" << addressAndPort;

    // Create ADRC TCP proxy
    if (proxy == 0)
    {
        proxy = new AdrcTcpProxy(addressAndPort, this);
        connect(proxy, SIGNAL(proxyOnline(bool)), this, SLOT(onProxyOnline(bool)));
        connect(proxy, SIGNAL(DeviceEvent(QString)), this, SLOT(onDeviceEvent(QString)));
    }
}

void MainWindow::onServiceUnregistered(QString addressAndPort)
{
    qDebug() << "MainWindow::onServiceUnregistered=" << addressAndPort;
    statusMode->setText(tr("Hub: off-line"));

    // Destroy ADRC TCP proxy
    if (proxy)
    {
        proxy->deleteLater();
        proxy = 0;
    }
}

void MainWindow::onProxyOnline(bool online)
{
    qDebug() << "MainWindow::onProxyOnline=" << online;

    // Network is offline so do nothing
    if (online == false)
    {
        setWindowTitle(tr(WINDOW_TITLE) + tr(" - Searching for hub"));
        statusMode->setText(tr("Hub: off-line"));
        qDebug() << "MainWindow::onNetworkOnline network is off-line";
        return;
    }

    // Network is online
    QString hostAddress = proxy->getHostAddress();
    setWindowTitle(tr(WINDOW_TITLE));
    statusMode->setText(QString("Hub: on-line@%1").arg(hostAddress));

    // Update the world from the hub
    updateWorld();

    // Update the views
    QString rml = editPane->text();
    QString uri = editTabFilePaths[editPane];
    if (!uri.isEmpty() && !rml.isEmpty())
        explorerPane->updateMetadata(rml, uri, hostAddress);
    devicesPane->update(hostAddress, hubName, world);
}

void MainWindow::updateWorld()
{
    // Query the ADRC daemon for all devices
    //
    QString inxml = QString("<adrc><device id='*'><exec>list</exec></device></adrc>");
    QString outxml = proxy->Execute(inxml);
    if (outxml.isEmpty())
    {
        qDebug() << "MainWindow::onNetworkOnline proxy execute failed";
        return;
    }

    // Parse reply from the daemon
    //
    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;

    if (!doc.setContent(outxml, &errorMsg, &errorLine, &errorColumn))
    {
        qDebug() << "MainWindow::onNetworkOnline"
                 << errorMsg << " at line:" << errorLine << " col:" << errorColumn << endl;
        return;
    }

    QDomNode node = doc.firstChild();
    if (node.isNull() || node.nodeName() != "adrc")
        return;

    // Get the hub name (domain)
    hubName = node.toElement().attribute( "domain", "unnamed" );

    // Rebuild the world based on what the daemon has reported
    //
    world.clear();

    for (node=node.firstChild(); !node.isNull(); node=node.nextSibling())
    {
        if (node.nodeName() == "device")
        {
            QString ouid, nick, manf, mmod, rmlFiles;
            QString deviceId = node.toElement().attribute( "id", "0" );

            if (deviceId == "-1")
                break;

            for (QDomNode valueNode = node.firstChild(); !valueNode.isNull(); valueNode=valueNode.nextSibling())
            {
                if (valueNode.nodeName() == "value")
                {
                    QDomNamedNodeMap attributes = valueNode.attributes();
                    QDomNode attribute = attributes.namedItem("name");

                    if (attribute.isNull())
                    {
                        qDebug() << "MainWindow::onNetworkOnline() value node is not named";
                        return;
                    }

                    QString contentText = valueNode.firstChild().nodeValue();
                    QString attrName = attribute.firstChild().nodeValue();

                    if (attrName == "ouid")
                        ouid = contentText;
                    else if (attrName == "nickname")
                        nick = contentText;
                    else if (attrName == "path")
                    {
                        QStringList tokens = contentText.split("/");
                        int numtok = tokens.count();
                        if (numtok >= 3)
                        {
                            manf = tokens.at(numtok-3);
                            mmod = tokens.at(numtok-2);
                            rmlFiles += QString("%1;").arg(tokens.at(numtok-1));
                        }
                    }
                }
            }

            // Add device to the world (map value data is positional)
            world[deviceId] = QString("%1;%2;%3;%4;%5").arg(ouid).arg(nick).arg(manf).arg(mmod).arg(rmlFiles);

            // Add the device's directory to the QFileSystemWatcher
            fsWatcher->addPath(QString("%1/profiles/%2/%3").arg(rmlCachePath).arg(manf).arg(mmod));
        }
    }
}

void MainWindow::updateStatusBar()
{
    int line = 0, col = 0;

    if (editPane)
        editPane->getCursorPosition(&line, &col);

    statusLine->setText(QString(tr("Line: %1")).arg(line+1));
    statusCol->setText(QString(tr("Col: %1")).arg(col+1));

    if (proxy && proxy->isValid())
        statusMode->setText(tr("Hub: on-line"));
    else
        statusMode->setText(tr("Hub: off-line"));
}

void MainWindow::removeUnicodeHeader(QString& rml)
{
    int pos = rml.indexOf("<");
    if (pos > 0)
        rml.remove(0, pos);
}

int MainWindow::maybeSave()
{
    if (editPane && editPane->isModified())
    {
        int ret = QMessageBox::warning(this, tr("Save to file"),
                      tr("The document has been modified.\n"
                         "Do you want to save your changes?"),
                      QMessageBox::Yes | QMessageBox::Default,
                      QMessageBox::No,
                      QMessageBox::Cancel | QMessageBox::Escape);
        if (ret == QMessageBox::Yes)
            return 1;
        else if (ret == QMessageBox::Cancel)
            return -1;
    }
    return 0;
}

void MainWindow::readSettings()
{
    QSettings settings;

    // Restore the main window state
    restoreGeometry(settings.value("application/geometry").toByteArray());
    restoreState(settings.value("application/windowState").toByteArray());

    // TODO Save list of 10 most recent files to QSettings
    // TODO Save open tabs to QSettings
    // TODO Save editor markers for all open files to QSettings
}

void MainWindow::writeSettings()
{
    QSettings settings;

    // Save the main window state
    settings.setValue("application/geometry", saveGeometry());
    settings.setValue("application/windowState", saveState());
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    // Check if we need to save any open documents
    QMapIterator<QWidget *, QString> i(editTabFilePaths);

    while (i.hasNext())
    {
        i.next();

        QsciScintilla *editor = static_cast<QsciScintilla *>(i.key());
        if (editor->isModified())
        {
            QString filePath = i.value();

            if (filePath.isEmpty() || filePath == RMLIDE_EMPTY_FILEPATH)
                saveAs();
            else
                writeFile(filePath);
        }
    }

    // Save settings
    writeSettings();
    ev->accept();
}

void MainWindow::onDirectoryChanged(const QString& path)
{
    Q_UNUSED(path);
    //qDebug() << "MainWindow::onDirectoryChanged=" << path;

    if (proxy != 0 && !hubName.isEmpty())
    {
        QString hostAddress = proxy->getHostAddress();
        devicesPane->update(hostAddress, hubName, world);
    }
}

void MainWindow::onEditorTabChanged(int index)
{
    if (index < 0)
    {
        explorerPane->clear();
        editPane = 0;
        return;
    }

    // Set the current editor
    editPane = static_cast<QsciScintilla *>(editTabWidget->widget(index));
    QString filePath = editTabFilePaths[editPane];

    //qDebug() << "MainWindow::onEditorTabChanged index=" << index << "file=" << filePath;

    // Update the views
    QString suffix = QFileInfo(filePath).suffix();
    if (suffix != "prf")
        explorerPane->clearMetadata();
    else // update metadata for RML file
    {
        QString hostAddress;
        if (proxy && proxy->isValid())
            hostAddress = proxy->getHostAddress();

        explorerPane->updateMetadata(editPane->text(), filePath, hostAddress);
    }

    updateStatusBar();
}

QString MainWindow::getUntitledDocumentName()
{
    // Generate a unique suffix starting from 1
    int suffix = 1;

    QList<QString> values = editTabFilePaths.values();
    for (int i=0, n=values.count(); i<n; i++)
    {
        if (values.at(i).startsWith(RMLIDE_EMPTY_FILEPATH))
            suffix++;
    }

    return QString("%1 %2").arg(tr("Untitled Document")).arg(suffix);
}

void MainWindow::setEditorContentModified(bool modified)
{
    int index = editTabWidget->indexOf(editPane);
    QString title = editTabWidget->tabText(index);

    // Add or remove an asterisk from the tab title
    if (modified)
    {
        if (!title.endsWith("*"))
            editTabWidget->setTabText(index, title + " *");
    }
    else // not modified
    {
        if (title.endsWith("*"))
            editTabWidget->setTabText(index, title.left(title.length()-1));
    }
}

void MainWindow::onTabCloseRequested(int index)
{
    if (index < 0)
        return;

    // Set the requested tab
    editTabWidget->setCurrentIndex(index);

    // Close the file
    closeFile();
}

void MainWindow::onDeviceEvent(QString sigxml)
{
    qDebug() << "MainWindow::onDeviceEvent sig=" << sigxml;

    if (proxy == 0 || !proxy->isValid())
        return;

    // Update the world from the hub
    updateWorld();

    // Update the devices pane
    QString hostAddress = proxy->getHostAddress();
    devicesPane->update(hostAddress, hubName, world);
}

bool MainWindow::validateManfMmodel(QString& rml)
{
    // *** TODO ***

    return true;
}

// End of file
