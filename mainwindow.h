// This module defines the main window of the RML IDE.
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMap>
#include <QMenu>
#include <QLabel>
#include <QAction>
#include <QToolBar>
#include <QString>
#include <QMainWindow>
#include <QProcess>
#include <QCloseEvent>
#include <QFileSystemWatcher>
#include <QTabWidget>

#include <xpadrctcpproxy.h>
#include <Qsci/qsciscintilla.h>

#include "devicespane.h"
#include "explorerpane.h"

#define WINDOW_TITLE "Equinox [*]"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *ev);

private slots:
    void newFile();
    void openFile();
    void closeFile();
    void about();
    bool save();
    bool saveAs();
    void settings();
    void print();
    //
    void runRML();
    bool validateRML();
    void uploadRML();
    void downloadRML();
#if 0
    void setXmlLang();
    void setCppLang();
#endif
    //
    void find();
    void undo();
    void redo();
    void copy();
    void paste();
    void cut();
    void selectAll();
    void toggleMarker();
    void nextMarker();
    void prevMarker();
    void moveToMatchingBrace();
    void clearErrors();
    void clearSimulator();
    //
    void onSimulatorStdoutReadyRead();
    void onSimulatorStderrReadyRead();
    //
    void onProxyOnline(bool online);
    void onServiceRegistered(QString addressAndPort);
    void onServiceUnregistered(QString addressAndPort);
    //
    int maybeSave(); // cancel <0, no =0, yes >0
    void readSettings();
    void writeSettings();
    void documentWasModified();
    void onCursorPositionChanged(int line, int col);
    //
    void onDirectoryChanged(const QString& path);
    void onEditorTabChanged(int index);
    void onTabCloseRequested(int index);
    //
    void onDeviceEvent(QString sigxml);

private:
    void createEditor(const QString& filePath);
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createDockWindows();
    //
    bool readFile(const QString& filePath);
    bool writeFile(const QString& filePath);
    //
    void updateWorld();
    void updateStatusBar();
    void removeUnicodeHeader(QString& rml);
    void setEditorContentModified(bool modified);
    QString getUntitledDocumentName();
    bool validateManfMmodel(QString& rml);
    //
    QString downloadFromDevice(QString manf,
                               QString mmod,
                               QString fileName,
                               QString hostAddress,
                               QString deviceId);
    QString downloadFromCache(QString manf,
                              QString mmod,
                              QString fileName,
                              QString hostAddress);
    //
    bool uploadToDevice(QString fileName,
                        QString hostAddress,
                        QString deviceId);
    bool uploadToCache(QString manf,
                       QString mmod,
                       QString fileName,
                       QString hostAddress);

private:
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *simMenu;
    QMenu *helpMenu;
    //
    QToolBar *fileToolBar;
    QToolBar *simToolBar;
    QToolBar *controlToolBar;
    //
    QAction *newRMLAct;
    QAction *openRMLAct;
    QAction *closeRMLAct;
    QAction *saveRMLAct;
    QAction *saveAsAct;
    QAction *settingsAct;
    QAction *printAct;
    QAction *quitAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *runRMLAct;
    QAction *validateRMLAct;
    QAction *uploadRMLAct;
    QAction *downloadRMLAct;
    QAction *langCppAct;
    QAction *langXmlAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *cutAct;
    QAction *selectAllAct;
    QAction *findAct;
    QAction *undoAct;
    QAction *redoAct;
    QAction *matchBraceAct;
    QAction *toggleMarkerAct;
    QAction *nextMarkerAct;
    QAction *prevMarkerAct;
    QAction *clearErrorsAct;
    QAction *clearSimulatorAct;
    //
    QsciScintilla *editPane; // currently active editor
    DevicesPane *devicesPane;
    ExplorerPane *explorerPane;
    //
    QLabel *statusLine;
    QLabel *statusCol;
    QLabel *statusMode;
    //
    AdrcTcpProxy *proxy;
    QMap<QString, QString> world;
    QString rmlCachePath;
    QString hubName;
    QProcess *rmlSimulator;
    QFileSystemWatcher *fsWatcher;
    //
    QTabWidget *editTabWidget;
    QMap<QWidget *, QString> editTabFilePaths; // editPane is the index
};

#endif // MAINWINDOW_H
