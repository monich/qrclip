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
#include "qrclip_widget.h"

#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QFileDialog>

//===========================================================================
// QrClipWindow::Data
//===========================================================================

class QrClipWindow::Data :
    public QObject
{
    Q_OBJECT

public:
    Data(const QrClipConfig&, QrClipWindow*);

    QrClipWindow* parentWindow() const;
    QByteArray windowGeometry() const;
    void saveWindowGeometry(QByteArray);
    bool alwaysOnTop() const;

public Q_SLOTS:
    void onCopyTriggered();
    void onSaveTriggered();
    void onAlwaysOnTopToggled(bool);

public:
    QrClipConfig iConfig;
    const QString iGeometryKey;
    const QString iAlwaysOnTopKey;
    QrClipWidget* iClipWidget;
};

QrClipWindow::Data::Data(
    const QrClipConfig& aConfig,
    QrClipWindow* aParent) :
    QObject(aParent),
    iConfig(aConfig),
    iGeometryKey("geometry"),
    iAlwaysOnTopKey("alwaysOnTop"),
    iClipWidget(new QrClipWidget(aParent))
{
    // Set up the actions
    QAction* copy = new QAction(QIcon::fromTheme("edit-copy"), "Copy", this);
    copy->setShortcut(QKeySequence::Copy);
    copy->setShortcutContext(Qt::WindowShortcut);
    copy->setEnabled(iClipWidget->haveQrCode());
    connect(iClipWidget, &QrClipWidget::haveQrCodeChanged, copy, &QAction::setEnabled);
    connect(copy, &QAction::triggered, this, &Data::onCopyTriggered);

    QAction* save = new QAction(QIcon::fromTheme("document-save"), "Save", this);
    save->setShortcut(QKeySequence::Save);
    save->setShortcutContext(Qt::WindowShortcut);
    copy->setEnabled(iClipWidget->haveQrCode());
    connect(iClipWidget, &QrClipWidget::haveQrCodeChanged, save, &QAction::setEnabled);
    connect(save, &QAction::triggered, this, &Data::onSaveTriggered);

    QAction* separator = new QAction(this);
    separator->setSeparator(true);

    QAction* onTop = new QAction("Always on top", this);
    onTop->setCheckable(true);
    onTop->setChecked(alwaysOnTop());
    connect(onTop, &QAction::toggled, this, &Data::onAlwaysOnTopToggled);

    iClipWidget->addAction(copy);
    iClipWidget->addAction(save);
    iClipWidget->addAction(separator);
    iClipWidget->addAction(onTop);
    iClipWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
}

inline
QrClipWindow*
QrClipWindow::Data::parentWindow() const
{
    return qobject_cast<QrClipWindow*>(parent());
}

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

bool
QrClipWindow::Data::alwaysOnTop() const
{
    // It's false by default, which is fine
    return iConfig.get(iAlwaysOnTopKey).toBool();
}

void
QrClipWindow::Data::onCopyTriggered()
{
    QImage image(iClipWidget->image());

    if (!image.isNull()) {
        DBG("Copying the image into the clipboard");
        QGuiApplication::clipboard()->setImage(image);
    }
}

void
QrClipWindow::Data::onSaveTriggered()
{
    QImage image(iClipWidget->image());

    if (!image.isNull()) {
        DBG("Saving the image");

        QrClipWidget::Blocker block(iClipWidget->blockUpdates());
        QString name = QFileDialog::getSaveFileName(parentWindow(),
            QStringLiteral("Save QR code image"),
            QStringLiteral("qrcode.png"),
            QStringLiteral("Image (*.png)"));

        if (!name.isEmpty()) {
            DBG("Writing" << qPrintable(name));
            image.save(name);
        }
    }
}

void
QrClipWindow::Data::onAlwaysOnTopToggled(
    bool aAlwaysOnTop)
{
    DBG("Always on top:" << aAlwaysOnTop);
    iConfig.set(iAlwaysOnTopKey, QVariant::fromValue(aAlwaysOnTop));

    // Disassociate window from the data to stop the window from modifying
    // the config.
    QrClipWindow* window = parentWindow();
    window->d = nullptr;

    // Setting Qt::WindowStaysOnTopHint here (which hides the window) and
    // showing the window again moves it up a bit (by the size of the title
    // bar). Recreating the window leaves it where it is.
    Q_EMIT window->restart();
}

//===========================================================================
// QrClipWindow
//===========================================================================

QrClipWindow::QrClipWindow(
    const QrClipConfig& aConfig) :
    d(nullptr)
{
    // Not assign it yet
    Data* data = new Data(aConfig, this);

    // First set up the window
    setCentralWidget(data->iClipWidget);
    setWindowTitle("QR Clip");
    setWindowIcon(QIcon(":/qrclip/app_icon"));
    setWindowFlag(Qt::WindowMinMaxButtonsHint, false);
    if (data->alwaysOnTop()) {
        setWindowFlag(Qt::WindowStaysOnTopHint);
    }

    // Restore the geometry
    restoreGeometry(data->windowGeometry());

    // Then start updating the config when window geometry changes
    d = data;
}

void
QrClipWindow::moveEvent(
    QMoveEvent* aEvent)
{
    QMainWindow::moveEvent(aEvent);
    if (d) {
        DBG("Window position" << qPrintable(QString("(%1,%2)").arg(x()).arg(y())));
        d->saveWindowGeometry(saveGeometry());
    }
}

void
QrClipWindow::resizeEvent(
    QResizeEvent* aEvent)
{
    QMainWindow::resizeEvent(aEvent);
    if (d) {
        DBG("Window size" << qPrintable(QString("%1x%2").arg(width()).arg(height())));
        d->saveWindowGeometry(saveGeometry());
    }
}

void
QrClipWindow::closeEvent(
    QCloseEvent* aEvent)
{
    QMainWindow::closeEvent(aEvent);
    Q_EMIT closed();
}

#include "qrclip_window.moc"
