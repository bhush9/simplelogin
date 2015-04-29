/*
 * Main authentication application class
 * Copyright (C) 2013 Martin B????za <mbriza@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "HelperApp.h"
#include "Backend.h"
#include "UserSession.h"

#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtNetwork/QLocalSocket>

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

namespace SDDM {
    HelperApp::HelperApp(int& argc, char** argv)
            : QCoreApplication(argc, argv)
            , m_backend(Backend::get(this))
            , m_session(new UserSession(this))
    {
        m_backend->setAutologin(true);
        setUp());
    }

    void HelperApp::setUp() {
        QStringList args = QCoreApplication::arguments();
        QString server;
        int pos;


        if ((pos = args.indexOf("--id")) >= 0) {
            if (pos >= args.length() - 1) {
                qCritical() << "This application is not supposed to be executed manually";
                exit(Auth::HELPER_OTHER_ERROR);
                return;
            }
            m_id = QString(args[pos + 1]).toLongLong();
        }

        if ((pos = args.indexOf("--start")) >= 0) {
            if (pos >= args.length() - 1) {
                qCritical() << "This application is not supposed to be executed manually";
                exit(Auth::HELPER_OTHER_ERROR);
                return;
            }
            m_session->setPath(args[pos + 1]);
        }

        if ((pos = args.indexOf("--user")) >= 0) {
            if (pos >= args.length() - 1) {
                qCritical() << "This application is not supposed to be executed manually";
                exit(Auth::HELPER_OTHER_ERROR);
                return;
            }
            m_user = args[pos + 1];
        }


        doAuth();
        connect(m_session, SIGNAL(finished(int)), this, SLOT(sessionFinished(int)));
        m_socket->connectToServer(server, QIODevice::ReadWrite | QIODevice::Unbuffered);
    }

    void HelperApp::doAuth() {
        SafeDataStream str(m_socket);
        str << Msg::HELLO << m_id;
        str.send();
        if (str.status() != QDataStream::Ok)
            qCritical() << "Couldn't write initial message:" << str.status();

        if (!m_backend->start(m_user)) {
            authenticated(QString(""));
            exit(Auth::HELPER_AUTH_ERROR);
            return;
        }

        if (!m_backend->authenticate()) {
            authenticated(QString(""));
            exit(Auth::HELPER_AUTH_ERROR);
            return;
        }

        m_user = m_backend->userName();
        QProcessEnvironment env = authenticated(m_user);

        if (!m_session->path().isEmpty()) {
            env.insert(m_session->processEnvironment());
            m_session->setProcessEnvironment(env);

            if (!m_backend->openSession()) {
                sessionOpened(false);
                exit(Auth::HELPER_SESSION_ERROR);
                return;
            }

            sessionOpened(true);
        }
        else
            exit(Auth::HELPER_SUCCESS);
        return;
    }

    void HelperApp::sessionFinished(int status) {
        exit(status);
    }

    QProcessEnvironment HelperApp::authenticated(const QString &user) {
       qDebug() << "authenticated";
    }

    void HelperApp::sessionOpened(bool success) {
        qDebug() << "yay, session opened";
    }

    UserSession *HelperApp::session() {
        return m_session;
    }

    const QString& HelperApp::user() const {
        return m_user;
    }

    HelperApp::~HelperApp() {

    }
}

int main(int argc, char** argv) {
    SDDM::HelperApp app(argc, argv);
    return app.exec();
}
