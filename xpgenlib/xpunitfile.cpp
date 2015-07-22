// This module implements the unit file class of the XPGENLIB library.
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
#include <QDomDocument>
#include "xpunitfile.h"

XPUnitFile::XPUnitFile(const QString& rml)
{
    m_valid = false;

    // Parse RML document using DOM parser
    //
    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;

    if (!doc.setContent(rml, &errorMsg, &errorLine, &errorColumn))
    {
        qDebug() << "XPUnitFile::XPUnitFile"
                 << QString("RML parse error: %1 at line: %2 col: %3").arg(errorMsg).arg(errorLine).arg(errorColumn);
        return;
    }

    // Extract elements into the UNIT structure
    //
    QDomNodeList nl;
    QString text;
    ::memset((void *)&m_hdr, 0, sizeof(m_hdr));

    // Get the manufacturer
    if ((nl = doc.elementsByTagName(("manufacturer"))).isEmpty()
        || (text = nl.at(0).toElement().text()).isEmpty())
    {
        qDebug() << "XPUnitFile::XPUnitFile <manufacturer> tag missing or empty";
        return;
    }
    strncpy(m_hdr.brand, text.toLatin1().data(), UNIT_BRAND_NAME_LEN-1);

    // Get the mmodel
    if ((nl = doc.elementsByTagName(("mmodel"))).isEmpty()
        || (text = nl.at(0).toElement().text()).isEmpty())
    {
        qDebug() << "XPUnitFile::XPUnitFile <mmodel> tag missing or empty";
        return;
    }
    strncpy(m_hdr.model, text.toLatin1().data(), UNIT_MODEL_NAME_LEN-1);

    // Get the category
    if (!(nl = doc.elementsByTagName(("category"))).isEmpty()
        && !(text = nl.at(0).toElement().text()).isEmpty())
    {
        m_hdr.dev_category = (quint16)(text.toUInt(0, 16));
    }

    // TODO Extract the nickname
    // <nickname><localetitle><en>nickname</en></localetitle></nickname>
    //

    // Get the nickname
    if (!(nl = doc.elementsByTagName(("nickname"))).isEmpty()
        && !(text = nl.at(0).toElement().text()).isEmpty())
    {
        strncpy(m_hdr.nickname, text.toLatin1().data(), UNIT_NICKNAME_LEN-1);
    }

    // Get the URL
    strcpy(m_hdr.url, "xped.com"); // default
    if (!(nl = doc.elementsByTagName(("url"))).isEmpty()
        && !(text = nl.at(0).toElement().text()).isEmpty())
    {
        strncpy(m_hdr.url, text.toLatin1().data(), UNIT_URL_LEN-1);
    }

    // Get the baudrate
    m_hdr.appMcuBaudRate = 115200 / 100; // default
    if (!(nl = doc.elementsByTagName(("baud"))).isEmpty()
        && !(text = nl.at(0).toElement().text()).isEmpty())
    {
        m_hdr.appMcuBaudRate = (quint16)(text.toUInt() / 100);
    }

    // These are fixed for now
    m_hdr.ndef_type = 0x55;
    m_hdr.uri_type = 0x01;

    // TODO Calculate 32-bit FNV hash on RML string
    m_hdr.etag = 0;

    // All done
    m_valid = true;
}

XPUnitFile::XPUnitFile(QFile &unitFile)
{
    m_valid = false;

    if (!unitFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "XPUnitFile::XPUnitFile Failed to open UNIT file";
        return;
    }

    if (unitFile.read((char *)&m_hdr, sizeof(m_hdr)) != sizeof(m_hdr))
    {
        qDebug() << "XPUnitFile::XPUnitFile Failed reading UNIT file";
        unitFile.close();
        return;
    }

    unitFile.close();

    m_valid = true;
}

bool XPUnitFile::isValid()
{
    return m_valid;
}

char *XPUnitFile::data()
{
    return (char *)&m_hdr;
}

int XPUnitFile::size()
{
    return sizeof(m_hdr);
}

QString XPUnitFile::manufacturer()
{
    return QString(m_hdr.brand);
}

QString XPUnitFile::mmodel()
{
    return QString(m_hdr.model);
}

QString XPUnitFile::nickName()
{
    return QString(m_hdr.nickname);
}

QString XPUnitFile::url()
{
    return QString(m_hdr.url);
}

quint16 XPUnitFile::category()
{
    return m_hdr.dev_category;
}

quint16 XPUnitFile::baudRate()
{
    return m_hdr.appMcuBaudRate;
}

quint32 XPUnitFile::eTag()
{
    return m_hdr.etag;
}

quint8 XPUnitFile::ndefType()
{
    return m_hdr.ndef_type;
}

quint8 XPUnitFile::uriType()
{
    return m_hdr.uri_type;
}

// End of file
