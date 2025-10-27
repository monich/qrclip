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
#include <QtCore/QPointer>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtGui/QPixmap>
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
    class BlockImpl;

    Data(QLabel*);
    ~Data();

    void connectClipboard();
    void disconnectClipboard();
    QImage makeImage() const;
    QImage makeImage(int) const;
    bool haveQrCode() const;

private Q_SLOTS:
    void updateQrCode();

private:
    static QString clipboardText();
    static QRcode* makeQrCode(const QString&);
    QrClipWidget* parentWidget() const;
    void updateQrCodeWidget(QLabel*);

public:
    const int iBorder;
    const int iSaveScale;
    int iUpdatesBlocked;
    QString iAppIconPngBase64;
    QString iLastText;
    QRcode* iCode;
};

QrClipWidget::Data::Data(
    QLabel* aLabel) :
    QObject(aLabel),
    iBorder(2),
    iSaveScale(5),
    iUpdatesBlocked(0),
    iLastText(clipboardText()),
    iCode(makeQrCode(iLastText))
{
    QPixmap appIconPixmap(":/qrclip/app_icon");
    QBuffer appIconBuffer;
    appIconBuffer.open(QIODevice::WriteOnly);
    appIconPixmap.save(&appIconBuffer, "png");
    appIconBuffer.close();
    iAppIconPngBase64 = QString::fromLatin1(appIconBuffer.data().toBase64());

    connectClipboard();
    updateQrCodeWidget(aLabel);
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
QrClipWidget*
QrClipWidget::Data::parentWidget() const
{
    return qobject_cast<QrClipWidget*>(parent());
}

inline
bool
QrClipWidget::Data::haveQrCode() const
{
    return iCode && iCode->width;
}

void
QrClipWidget::Data::connectClipboard()
{
    QClipboard* clip = QGuiApplication::clipboard();

    if (clip) {
        connect(clip, &QClipboard::dataChanged, this, &Data::updateQrCode);
        connect(clip, &QClipboard::selectionChanged, this, &Data::updateQrCode);
    }
}

void
QrClipWidget::Data::disconnectClipboard()
{
    QClipboard* clip = QGuiApplication::clipboard();

    if (clip) {
        clip->disconnect(this);
    }
}

QImage
QrClipWidget::Data::makeImage() const
{
    const QLabel* l = parentWidget();

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
QrClipWidget::Data::updateQrCode()
{
    QString text(clipboardText());

    if (iLastText != text) {
        QrClipWidget* widget = parentWidget();
        const bool hadQrCode = haveQrCode();

        DBG(text);
        iLastText = text;
        QRcode_free(iCode);
        iCode = makeQrCode(iLastText);
        updateQrCodeWidget(widget);
        if (hadQrCode != haveQrCode()) {
            Q_EMIT widget->haveQrCodeChanged(!hadQrCode);
        }
    }
}

void
QrClipWidget::Data::updateQrCodeWidget(
    QLabel* aLabel)
{
    if (haveQrCode()) {
        aLabel->setToolTip(iLastText);
        aLabel->setPixmap(QPixmap::fromImage(makeImage()));
    } else {
        aLabel->setToolTip(QString());
        aLabel->setPixmap(QPixmap());
        aLabel->setText(QString("<p align='center'>"
            "<img src='data:image/png;base64,%1'/></p>"
            "<p align='center'>%2</p>").
            arg(iAppIconPngBase64, iLastText.isEmpty() ?
                QStringLiteral("Clipboard is empty") :
                QStringLiteral("Too much text for a QR code")));
    }
}

//===========================================================================
// QrClipWidget::Data::BlockImpl
//===========================================================================

class QrClipWidget::Data::BlockImpl :
    public QrClipWidget::Block
{
public:
    BlockImpl(Data*);
    ~BlockImpl() override;
    QPointer<Data> iData;
};

QrClipWidget::Data::BlockImpl::BlockImpl(Data* aData) :
    iData(aData)
{
    if (!aData->iUpdatesBlocked++) {
        DBG("Blocking QR code updates");
        aData->disconnectClipboard();
    }
}

QrClipWidget::Data::BlockImpl::~BlockImpl()
{
    Data* d = iData;
    if (d && !--d->iUpdatesBlocked) {
        DBG("Resuming QR code updates");
        d->connectClipboard();
        d->updateQrCode();
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

bool
QrClipWidget::haveQrCode() const
{
    return d->haveQrCode();
}

QImage
QrClipWidget::image() const
{
    return d->haveQrCode() ? d->makeImage(d->iSaveScale) : QImage();
}

QrClipWidget::Blocker
QrClipWidget::blockUpdates()
{
    return Blocker(new Data::BlockImpl(d));
}

QSize
QrClipWidget::minimumSizeHint() const
{
    if (d->haveQrCode()) {
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
    if (d->haveQrCode()) {
        setPixmap(QPixmap::fromImage(d->makeImage()));
    }
    QLabel::resizeEvent(aEvent);
}

#include "qrclip_widget.moc"
