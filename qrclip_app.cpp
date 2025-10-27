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

#include "qrclip_app.h"
#include "qrclip_config.h"
#include "qrclip_window.h"

//===========================================================================
// QrClipApp::Data
//===========================================================================

class QrClipApp::Data :
    public QObject
{
    Q_OBJECT

public:
    Data(QrClipApp*);
    ~Data();

private:
    void createWindow(QApplication*);

private Q_SLOTS:
    void onRestart();

private:
    QrClipConfig iConfig;
    QrClipWindow* iWindow;

};

QrClipApp::Data::Data(
    QrClipApp* aApp) :
    QObject(aApp),
    iWindow(nullptr)
{
    createWindow(aApp);
}

QrClipApp::Data::~Data()
{
    delete iWindow;
}

void
QrClipApp::Data::onRestart()
{
    QApplication* app = qobject_cast<QApplication*>(parent());

    // Delete the old window later because it's actually the one
    // who emitted the signal.
    iWindow->disconnect(app);
    iWindow->hide();
    iWindow->deleteLater();
    createWindow(app);
}

void
QrClipApp::Data::createWindow(
    QApplication* aApp)
{
    iWindow = new QrClipWindow(iConfig);
    connect(iWindow, &QrClipWindow::restart, this, &Data::onRestart);
    connect(iWindow, &QrClipWindow::closed, aApp, &QApplication::quit);
    iWindow->show();
}

//===========================================================================
// QrClipApp
//===========================================================================

QrClipApp::QrClipApp(
    int& aArgc,
    char** aArgv) :
    QApplication(aArgc, aArgv),
    d(new Data(this))
{
    // We are explicitly reacting to QrClipWindow::closed signal
    setQuitOnLastWindowClosed(false);
}

#include "qrclip_app.moc"
