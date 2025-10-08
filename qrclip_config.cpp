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

#include "qrclip_config.h"

#include "qrclip_debug.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>

//===========================================================================
// QrClipConfig::Data
//===========================================================================

class QrClipConfig::Data : public QObject
{
    Q_OBJECT

public:
    Data(QString);
    ~Data();

    void set(const QString&, const QVariant&);
    void scheduleSave();

private slots:
    void saveNow();

public:
    mutable QAtomicInt ref; // for QExplicitlySharedDataPointer
    QTimer* iMinSaveDelayTimer;
    QTimer* iMaxSaveDelayTimer;
    QDir iConfigDir;
    QString iConfigFile;
    QVariantMap iConfig;
};

QrClipConfig::Data::Data(
    QString aFileName) :
    iMinSaveDelayTimer(new QTimer(this)),
    iMaxSaveDelayTimer(new QTimer(this)),
    iConfigDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)),
    iConfigFile(iConfigDir.absoluteFilePath(aFileName))
{
    // Don't save changes more often than twice a second.
    // But if the config keeps changing, still save once in 5 sec.
    iMinSaveDelayTimer->setInterval(500);
    iMaxSaveDelayTimer->setInterval(5000);

    iMinSaveDelayTimer->setSingleShot(true);
    iMaxSaveDelayTimer->setSingleShot(true);

    connect(iMinSaveDelayTimer, &QTimer::timeout, this, &Data::saveNow);
    connect(iMaxSaveDelayTimer, &QTimer::timeout, this, &Data::saveNow);

    // Load the config file
    QFile f(iConfigFile);

    if (f.exists()) {
        if (!f.open(QIODevice::ReadOnly)) {
            DBG("Can't open" << qPrintable(iConfigFile));
        } else {
            DBG("Loading" << qPrintable(iConfigFile));
            iConfig = QJsonDocument::fromJson(f.readAll()).toVariant().toMap();
        }
    }
}

QrClipConfig::Data::~Data()
{
    if (iMinSaveDelayTimer->isActive()) {
        saveNow(); // Finish pending save
    }
}

void
QrClipConfig::Data::set(
    const QString& aKey,
    const QVariant& aValue)
{
    if (aValue.isValid()) {
        if (!iConfig.contains(aKey) || aValue != iConfig.value(aKey)) {
            iConfig.insert(aKey, aValue);
            scheduleSave();
        }
    } else if (iConfig.contains(aKey)) {
        iConfig.remove(aKey);
        scheduleSave();
    }
}

void
QrClipConfig::Data::scheduleSave()
{
    iMinSaveDelayTimer->start();
    if (!iMaxSaveDelayTimer->isActive()) {
        iMaxSaveDelayTimer->start();
    }
}

void
QrClipConfig::Data::saveNow()
{
    iMinSaveDelayTimer->stop();
    iMaxSaveDelayTimer->stop();

    if (!iConfigDir.exists() && !iConfigDir.mkpath(".")) {
        WARN("Failed to create" << qPrintable(iConfigDir.absolutePath()));
    }

    QFile f(iConfigFile);

    if (!f.open(QIODevice::WriteOnly)) {
        WARN("Failed to open" << qPrintable(iConfigFile) << f.errorString());
    } else if (f.write(QJsonDocument::fromVariant(iConfig).toJson()) < 0) {
        WARN("Failed to write" << qPrintable(iConfigFile) << f.errorString());
    } else {
        DBG("Saved" << qPrintable(iConfigFile));
    }
}

//===========================================================================
// QrClipConfig
//===========================================================================

QrClipConfig::QrClipConfig() :
    d(new Data("qrclip.json"))
{}

QrClipConfig::QrClipConfig(
    const QrClipConfig& aConfig) :
    d(aConfig.d)
{}

QrClipConfig::~QrClipConfig()
{}

QrClipConfig&
QrClipConfig::operator=(
    const QrClipConfig& aOther)
{
    d = aOther.d;
    return *this;
}

QVariant
QrClipConfig::get(
    const QString& aKey) const
{
    return d->iConfig.value(aKey);
}

void
QrClipConfig::set(
    const QString& aKey,
    const QVariant& aValue)
{
    d->set(aKey, aValue);
}

#include "qrclip_config.moc"
