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

#include "qrclip_widget.h"

#include "qrclip_debug.h"

#include <QtCore/QBuffer>
#include <QtCore/QSignalBlocker>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtWidgets/QAction>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QStyle>

#include <qrencode.h>

//===========================================================================
// QrClipWidget::Data
//===========================================================================

class QrClipWidget::Data :
    public QObject
{
    Q_OBJECT

public:
    Data(QLabel*);
    ~Data();

    QImage makeImage() const;

private Q_SLOTS:
    void copyQrCode();
    void saveQrCode();
    void updateQrCode();

private:
    static QString clipboardText();
    static QRcode* makeQrCode(const QString&);
    inline QLabel* parentLabel() const;
    QImage makeImage(int) const;
    void updateQrCodeLabel(QLabel*);

public:
    const int iBorder;
    const int iSaveScale;
    QAction* iCopyAction;
    QAction* iSaveAction;
    QString iAppIconPngBase64;
    QString iLastText;
    QRcode* iCode;
};

QrClipWidget::Data::Data(
    QLabel* aLabel) :
    QObject(aLabel),
    iBorder(2),
    iSaveScale(2),
    iCopyAction(new QAction(QIcon::fromTheme("edit-copy"), "Copy", this)),
    iSaveAction(new QAction(QIcon::fromTheme("document-save"), "Save", this)),
    iLastText(clipboardText()),
    iCode(makeQrCode(iLastText))
{
    QClipboard* clip = qGuiApp->clipboard();

    QPixmap appIconPixmap(":/qrclip/app_icon");
    QBuffer appIconBuffer;
    appIconBuffer.open(QIODevice::WriteOnly);
    appIconPixmap.save(&appIconBuffer, "png");
    appIconBuffer.close();
    iAppIconPngBase64 = QString::fromLatin1(appIconBuffer.data().toBase64());

    iCopyAction->setShortcut(QKeySequence::Copy);
    iCopyAction->setShortcutContext(Qt::WindowShortcut);
    connect(iCopyAction, &QAction::triggered, this, &Data::copyQrCode);
    aLabel->addAction(iCopyAction);

    iSaveAction->setShortcut(QKeySequence::Save);
    iSaveAction->setShortcutContext(Qt::WindowShortcut);
    connect(iSaveAction, &QAction::triggered, this, &Data::saveQrCode);
    aLabel->addAction(iSaveAction);

    connect(clip, &QClipboard::dataChanged, this, &Data::updateQrCode);
    connect(clip, &QClipboard::selectionChanged, this, &Data::updateQrCode);
    updateQrCodeLabel(aLabel);
}

QrClipWidget::Data::~Data()
{
    QRcode_free(iCode);
}

// static
QString
QrClipWidget::Data::clipboardText()
{
    QClipboard* clip = qGuiApp->clipboard();
    QString text(clip->text(QClipboard::Selection));

    return text.isEmpty() ? clip->text(QClipboard::Clipboard) : text;
}

// static
QRcode*
QrClipWidget::Data::makeQrCode(
    const QString& aText)
{
    if (aText.isEmpty()) {
        return nullptr;
    } else {
        const QByteArray utf8(aText.toUtf8());

        return QRcode_encodeString(utf8.constData(), 0, QR_ECLEVEL_M,
            QR_MODE_8, true);
    }
}

inline
QLabel*
QrClipWidget::Data::parentLabel() const
{
    return qobject_cast<QLabel*>(parent());
}

QImage
QrClipWidget::Data::makeImage() const
{
    const QLabel* l = parentLabel();

    return makeImage(qMax(1,
            qMin(l->width(), l->height())/(iCode->width + 2 * iBorder)));
}

QImage
QrClipWidget::Data::makeImage(
    int aScale) const
{
    const uchar* data = iCode->data;
    const uint size = iCode->width;
    const uint border = aScale * iBorder;
    const uint imageRowSize = size * aScale + 2 * border;

    // Each pixel is an 8-bit index into the colormap
    QImage img(imageRowSize, imageRowSize, QImage::Format_Indexed8);
    img.setColorTable({0xffffffff, 0xff000000});
    img.fill(0); // background, i.e. white

    for (uint y = 0; y < size; y++) {
        const uchar* src = data + y * size;
        const uint rowIndex = border + y * aScale;
        uchar* imageRow = img.scanLine(rowIndex);
        uchar* dest = imageRow + border;

        // Fill the line
        for (uint x = 0; x < size; x++) {
            // Each uchar in QRcode represents a module (dot). If the
            // less significant bit of the uchar is 1, the corresponding
            // module is black. The other bits are meaningless for usual
            // applications.
            const uchar dot = (*src++) & 1;

            // Each dot gets repeated aScale times
            for (uint k = 0; k < aScale; k++) {
                *dest++ = dot;
            }
        }

        // Repeat the entire row another (aScale - 1) times
        for (uint k = 1; k < aScale; k++) {
            memcpy(img.scanLine(rowIndex + k), imageRow, imageRowSize);
        }
    }
    return img;
}

void
QrClipWidget::Data::copyQrCode()
{
    if (iCode) {
        DBG("Copying the image into the clipboard");
        qGuiApp->clipboard()->setImage(makeImage(iSaveScale));
    }
}

void
QrClipWidget::Data::saveQrCode()
{
    if (iCode) {
        DBG("Saving the image");

        // Stop the clipboard from emitting the signals while QFileDialog
        // is running its own event loop. That keeps the same QR code that
        // we are saving on the screen and prevents other issues, too.
        QSignalBlocker block(qGuiApp->clipboard());
        QImage image(makeImage(iSaveScale));
        QString name = QFileDialog::getSaveFileName(parentLabel(),
            QStringLiteral("Save QR code image"),
            QStringLiteral("qrcode.png"),
            QStringLiteral("Image (*.png)"));

        if (!name.isEmpty()) {
            image.save(name);
        }

        // Unblock the signals and refresh the state
        block.unblock();
        updateQrCode();
    }
}

void
QrClipWidget::Data::updateQrCode()
{
    QString text(clipboardText());

    if (iLastText != text) {
        iLastText = text;
        DBG(text);
        QRcode_free(iCode);
        iCode = makeQrCode(iLastText);
        updateQrCodeLabel(parentLabel());
    }
}

void
QrClipWidget::Data::updateQrCodeLabel(
    QLabel* aLabel)
{
    if (iCode && iCode->width) {
        aLabel->setToolTip(iLastText);
        aLabel->setPixmap(QPixmap::fromImage(makeImage()));
        // Enable the context menu and actions
        aLabel->setContextMenuPolicy(Qt::ActionsContextMenu);
        iCopyAction->setEnabled(true);
        iSaveAction->setEnabled(true);
    } else {
        aLabel->setToolTip(QString());
        aLabel->setPixmap(QPixmap());
        aLabel->setText(QString("<p align='center'>"
            "<img src='data:image/png;base64,%1'/></p>"
            "<p align='center'>%2</p>").
            arg(iAppIconPngBase64, iLastText.isEmpty() ?
                QStringLiteral("Clipboard is empty") :
                QStringLiteral("Too much text for a QR code")));
        // Disable the context menu and actions
        aLabel->setContextMenuPolicy(Qt::NoContextMenu);
        iCopyAction->setEnabled(false);
        iSaveAction->setEnabled(false);
    }
}

//===========================================================================
// QrClipWidget
//===========================================================================

QrClipWidget::QrClipWidget(
    QWidget* aParent) :
    QLabel(aParent),
    d(new Data(this))
{
    setAlignment(Qt::AlignCenter);
    setMargin(style()->pixelMetric(QStyle::PM_ButtonMargin));
}

QSize
QrClipWidget::minimumSizeHint() const
{
    if (d->iCode) {
        const int size = d->iCode->width + 2 * (d->iBorder + margin());

        return QSize(size, size);
    } else {
        return QLabel::minimumSizeHint();
    }
}

void
QrClipWidget::resizeEvent(
    QResizeEvent* aEvent)
{
    if (d->iCode) {
        setPixmap(QPixmap::fromImage(d->makeImage()));
    }
    QLabel::resizeEvent(aEvent);
}

#include "qrclip_widget.moc"
