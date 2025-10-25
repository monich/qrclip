// Copyright (C) 2025 Slava Monich <slava@monich.com>
//
// You may use this file under the terms of the BSD license as follows:
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  1. Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer
//     in the documentation and/or other materials provided with the
//     distribution.
//
//  3. Neither the names of the copyright holders nor the names of its
//     contributors may be used to endorse or promote products derived
//     from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation
// are those of the authors and should not be interpreted as representing
// any official policies, either expressed or implied.

#include "qrclip_window.h"

#include "qrclip_debug.h"
#include "qrclip_config.h"

#include <QtGui/QIcon>

//===========================================================================
// QrClipWindow::Data
//===========================================================================

class QrClipWindow::Data
{
public:
    Data(const QrClipConfig&);

    QByteArray windowGeometry() const;
    void saveWindowGeometry(QByteArray);

public:
    QrClipConfig iConfig;
    const QString iGeometryKey;
};

QrClipWindow::Data::Data(
    const QrClipConfig& aConfig) :
    iConfig(aConfig),
    iGeometryKey("geometry")
{}

QByteArray
QrClipWindow::Data::windowGeometry() const
{
    return QByteArray::fromHex(iConfig.get(iGeometryKey).toString().toLatin1());
}

void
QrClipWindow::Data::saveWindowGeometry(
    QByteArray aGeometry)
{
    iConfig.set(iGeometryKey, QString::fromLatin1(aGeometry.toHex()));
}

//===========================================================================
// QrClipWindow
//===========================================================================

QrClipWindow::QrClipWindow(
    const QrClipConfig& aConfig) :
    d(nullptr)
{
    // First set up the window attributes
    setWindowFlag(Qt::WindowMinMaxButtonsHint, false);
    setWindowTitle("QR Clip");
    setWindowIcon(QIcon(":/qrclip/app_icon"));

    // Now restore the geometry and start updating the config on window
    // state changes
    d = new Data(aConfig);
    restoreGeometry(d->windowGeometry());
}

QrClipWindow::~QrClipWindow()
{
    delete d;
}

void
QrClipWindow::moveEvent(
    QMoveEvent* aEvent)
{
    if (d) {
        DBG("Window position" << qPrintable(QString("(%1,%2)").arg(x()).arg(y())));
        d->saveWindowGeometry(saveGeometry());
    }
    QMainWindow::moveEvent(aEvent);
}

void
QrClipWindow::resizeEvent(
    QResizeEvent* aEvent)
{
    if (d) {
        DBG("Window size" << qPrintable(QString("%1x%2").arg(width()).arg(height())));
        d->saveWindowGeometry(saveGeometry());
    }
    QMainWindow::resizeEvent(aEvent);
}

void
QrClipWindow::closeEvent(
    QCloseEvent* aEvent)
{
    Q_EMIT closed();
    QMainWindow::closeEvent(aEvent);
}
