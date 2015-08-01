# This is the Qt project file for the RML IDE.
#
# Copyright (c) 2015 Xped Holdings Limited <info@xped.com>
#
# This file is part of the Equinox.
#
# This file may be used under the terms of the GNU General Public
# License version 3.0 as published by the Free Software Foundation
# and appearing in the file LICENSE included in the packaging of
# this file. Alternatively you may (at your option) use any later 
# version of the GNU General Public License if such license has been
# publicly approved by Xped Holdings Limited (or its successors,
# if any) and the KDE Free Qt Foundation.
#
# If you are unsure which license is appropriate for your use, please
# contact the sales department at sales@xped.com.
#
# This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
# WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# You MUST install the package libqt5scintilla2-dev or libqt4scintilla2-dev
#

QT       += core gui xml network

greaterThan(QT_MAJOR_VERSION, 4) {
        QT += widgets printsupport

    greaterThan(QT_MINOR_VERSION, 1) {
            macx:QT += macextras
    }

    # Work around QTBUG-39300.
    CONFIG -= android_install
}

TARGET = equinox
TEMPLATE = app

win32 {
DEFINES += WIN_PLATFORM
}

unix:!macx {
DEFINES += LINUX_PLATFORM
}

macx {
DEFINES += MAC_PLATFORM
}

INCLUDEPATH += \
    ./xpgenlib \
    ./xpgenlib/netfile \
    ./xpgenlib/adrcproxy

LIBS += \
    -lqscintilla2

SOURCES += main.cpp \
    mainwindow.cpp \
    devicespane.cpp \
    explorerpane.cpp \
    rmltransferdialog.cpp \
    addusercatdialog.cpp \
    finddialog.cpp \
    xpgenlib/xpcategory.cpp \
    xpgenlib/xpnetservicewatcher.cpp \
    xpgenlib/xpunitfile.cpp \
    xpgenlib/netfile/xpnetfile.cpp \
    xpgenlib/netfile/xpprivategetworker.cpp \
    xpgenlib/netfile/xpprivateputworker.cpp \
    xpgenlib/adrcproxy/xpadrctcpproxy.cpp \
    xpgenlib/adrcproxy/executethread.cpp \
    xpgenlib/adrcproxy/signalthread.cpp \
    settingsdialog.cpp

HEADERS  += \
    mainwindow.h \
    devicespane.h \
    explorerpane.h \
    rmltransferdialog.h \
    addusercatdialog.h \
    finddialog.h \
    xpgenlib/xpcategory.h \
    xpgenlib/xpnetservicewatcher.h \
    xpgenlib/xpunitfile.h \
    xpgenlib/netfile/xpnetfile.h \
    xpgenlib/netfile/xpprivategetworker.h \
    xpgenlib/netfile/xpprivateputworker.h \
    xpgenlib/adrcproxy/xpadrctcpproxy.h \
    xpgenlib/adrcproxy/executethread.h \
    xpgenlib/adrcproxy/signalthread.h \
    settingsdialog.h

RESOURCES += \
    equinox.qrc

OTHER_FILES += \
    README.md
