// This module defines the unit file class of the XPGENLIB library.
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

#ifndef UNITFILE_H
#define UNITFILE_H

#include <QFile>
#include <QString>

#define UNIT_BRAND_NAME_LEN 24
#define UNIT_MODEL_NAME_LEN 16
#define UNIT_NICKNAME_LEN   16
#define UNIT_URL_LEN        128

struct unit_file_header
{
   quint16 dev_category;  // 00
   quint16 appMcuBaudRate;// 02  (baudrate /100 )
   quint32 etag;          // 04
   quint8 ndef_type;      // 08
   quint8 uri_type;       // 09
   quint8 res1;           // 10
   quint8 res2;           // 11
   char   brand[UNIT_BRAND_NAME_LEN];  // 12[24]
   char   model[UNIT_MODEL_NAME_LEN];  // 36[16]
   char   nickname[UNIT_NICKNAME_LEN]; // 52[16]
   char   url[UNIT_URL_LEN];           // 68[128]
};// total 12+24+16+16+128=196


class XPUnitFile
{
public:
    explicit XPUnitFile(const QString& rml); // from RML string
    XPUnitFile(QFile &unitFile); // from UNIT file
    //
    char *data();
    int size();
    bool isValid();
    //
    QString manufacturer();
    QString mmodel();
    QString nickName();
    QString url();
    quint16 category();
    quint16 baudRate();
    quint32 eTag();
    quint8 ndefType();
    quint8 uriType();

private:
    bool m_valid;
    struct unit_file_header m_hdr;
};

#endif // UNITFILE_H
