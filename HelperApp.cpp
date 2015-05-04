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
#include "UserSession.h"

#include "backend/PamHandle.h"

#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QtCore/QDebug>

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>

namespace SDDM {
    HelperApp::HelperApp(int& argc, char** argv)
            : QCoreApplication(argc, argv)
            , m_session(new UserSession(this))
    {        
        QStringList args = QCoreApplication::arguments();
        int pos;
        

        if ((pos = args.indexOf("--exec")) >= 0) {
            if (pos >= args.length() - 1) {
                exit(-1);
            }
            m_session->setPath(args[pos + 1]);
        }

        if ((pos = args.indexOf("--user")) >= 0) {
            if (pos >= args.length() - 1) {
                exit(-1);
            }
            m_user = args[pos + 1];
        }
        
        if (m_session->path().isEmpty() || m_user.isEmpty()) 
            qFatal("pass some args please");
        
        PamHandle *pamHandle = new PamHandle; //TODO fix leak

        if (! pamHandle->start("sddm-autologin" /*PAM session*/, m_user)) //Martin check this exists
            qFatal("Could not start PAM");

        QProcessEnvironment env = m_session->processEnvironment();

//         env.insert("DISPLAY", name());
        env.insert("XDG_SEAT", "seat0");
//         env.insert("XDG_SEAT_PATH", daemonApp->displayManager()->seatPath(seat()->name()));
//         env.insert("XDG_SESSION_PATH", daemonApp->displayManager()->sessionPath(QString("Session%1").arg(daemonApp->newSessionId())));
        env.insert("XDG_VTNR", QString::number(0));
        env.insert("DESKTOP_SESSION", "KDE");
        env.insert("XDG_CURRENT_DESKTOP", "KDE");
        env.insert("XDG_SESSION_CLASS", "user");
//         env.insert("XDG_SESSION_TYPE", m_displayServer->sessionType());
//         env.insert("XDG_SESSION_DESKTOP", xdgSessionName);
        pamHandle->putEnv(env);

        pamHandle->setItem(PAM_TTY, "1"); //I have a feeling this might be ":0"



        if (!pamHandle->authenticate())
            qFatal("Could not auth");

        if (!pamHandle->acctMgmt())
            qFatal("Could not do account management");

        if (!pamHandle->setCred(PAM_ESTABLISH_CRED))
            qFatal("Could not establish credentials");
        
        if (!pamHandle->openSession())
            qFatal("Could not open pam session");
        
        qDebug() << "startng process";
        
        connect(m_session, SIGNAL(finished(int)), SLOT(sessionFinished(int)));

        connect(m_session, &QProcess::readyReadStandardError, m_session, [this](){qDebug() << m_session->readAllStandardError();});
        connect(m_session, &QProcess::readyReadStandardOutput, m_session, [this](){qDebug() << m_session->readAllStandardOutput();});

        env.insert(pamHandle->getEnv());
        m_session->setProcessEnvironment(env);
        m_session->start();
        
    }

    void HelperApp::sessionFinished(int status) {
        exit(status);
    }
    
    const QString& HelperApp::user() const
    {
        return m_user;
    }


    HelperApp::~HelperApp() {

    }
}

int main(int argc, char** argv) {
    SDDM::HelperApp app(argc, argv);
    return app.exec();
}
