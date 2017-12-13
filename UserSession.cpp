/*
 * Session process wrapper
 * Copyright (C) 2014 Martin B????za <mbriza@redhat.com>
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

#include "UserSession.h"
#include "HelperApp.h"

#include <QDebug>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <fcntl.h>

namespace SDDM {
    UserSession::UserSession(HelperApp *parent)
            : QProcess(parent) {
    }

    UserSession::~UserSession() {

    }

    bool UserSession::start() {
        //NOTE FOR DAVE GO BACK AND FIX THIS IN SDDM, DON'T WAIT FOR STARTED IN HERE. IT'S WEIRD
        
        qDebug() << "starting "<< m_path;
        
        QProcess::start(m_path);

        return waitForStarted();
    }

    void UserSession::setPath(const QString& path) {
        m_path = path;
    }

    QString UserSession::path() const {
        return m_path;
    }

    void UserSession::setupChildProcess() {        
        QString username = qobject_cast<HelperApp*>(parent())->user();


            // open VT and get the fd
            QString ttyString = QString("/dev/tty%1").arg(processEnvironment().value("XDG_VTNR"));
            int vtFd = ::open(qPrintable(ttyString), O_RDWR | O_NOCTTY);

            // when this is true we'll take control of the tty
            bool takeControl = false;

            if (vtFd > 0) {
                dup2(vtFd, STDIN_FILENO);
                ::close(vtFd);
                takeControl = true;
            } else {
                int stdinFd = ::open("/dev/null", O_RDWR);
                dup2(stdinFd, STDIN_FILENO);
                ::close(stdinFd);
            }

            // set this process as session leader
            if (setsid() < 0) {
                qCritical("Failed to set pid %lld as leader of the new session and process group: %s",
                          QCoreApplication::applicationPid(), strerror(errno));
                exit(-1);
            }

            // take control of the tty
            if (takeControl) {
                if (ioctl(STDIN_FILENO, TIOCSCTTY) < 0) {
                    qCritical("Failed to take control of the tty: %s", strerror(errno));
                }
            }

        struct passwd *pw = getpwnam(username.toLocal8Bit());
        if (setgid(pw->pw_gid) != 0) {
            qCritical() << "setgid(" << pw->pw_gid << ") failed for user: " << username;
            exit(-1);
        }
        if (initgroups(pw->pw_name, pw->pw_gid) != 0) {
            qCritical() << "initgroups(" << pw->pw_name << ", " << pw->pw_gid << ") failed for user: " << username;
            exit(-1);
        }
        if (setuid(pw->pw_uid) != 0) {
            qCritical() << "setuid(" << pw->pw_uid << ") failed for user: " << username;
            exit(-1);
        }
        if (chdir(pw->pw_dir) != 0) {
            qCritical() << "chdir(" << pw->pw_dir << ") failed for user: " << username;
            qCritical() << "verify directory exist and has sufficient permissions";
            exit(-1);
        }

        //from here onwards we redirect stderr to a file, doesn't seem to work 
        return; 
        
                
        //we cannot use setStandardError file as this code is run in the child process
        //we want to redirect after we setuid so that .xsession-errors is owned by the user

        //swap the stderr pipe of this subprcess into a file .xsession-errors
        int fd = ::open(".xsession-errors-dave", O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd >= 0)
        {
            dup2 (fd, STDERR_FILENO);
            ::close(fd);
        } else {
            qWarning() << "Could not open stderr to .xsession-errors file";
        }

        //redirect any stdout to /dev/null
        fd = ::open("/dev/null", O_WRONLY);
        if (fd >= 0)
        {
            dup2 (fd, STDOUT_FILENO);
            ::close(fd);
        } else {
            qWarning() << "Could not redirect stdout";
        }
    }
}
