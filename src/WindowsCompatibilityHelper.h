// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

#pragma once

#include "ICompatibilityHelper.h"
#include <KIO/ApplicationLauncherJob>
#include <QFile>
#include <QObject>
#include <QQmlEngine>
#include <qobject.h>

using namespace Qt::Literals::StringLiterals;

class WindowsCompatibilityHelper : public ICompatibilityHelper
{
    Q_OBJECT

public:
    explicit WindowsCompatibilityHelper(const QUrl &databaseFilePath, const QUrl &openedExePath, bool isDosProgram = false, QObject *parent = nullptr);
    ~WindowsCompatibilityHelper() override = default;

    QString windowTitle() const override;
    QString heading() const override;
    QString icon() const override;
    QString description() const override;
    bool hasNativeApp() const override
    {
        // If it's in the database, it has a native app.
        // The database is read in the constructor, which is where this member is set.
        return m_hasNativeApp;
    };
    QString nativeAppActionText() const override;
    QString nativeAppActionIcon() const override;

    Q_INVOKABLE void nativeAppAction() const override;

private:
    QString m_nativeAppName;
    QString m_alternativeAppName;
    QString m_nativeAppRef;

    QString nativeAppName() const override
    {
        return m_nativeAppName;
    }
    QString nativeAppRef() const override
    {
        return m_nativeAppRef;
    }
    bool isNativeAppInstalled() const override;

    bool m_hasNativeApp = false;
    // e.g. if the user opens ie11.exe, this will be true as the Flatpak alternative is Microsoft Edge.
    bool m_needsAlternativeApp = false;
    // True for real-mode DOS programs, which are run in a DOS emulator instead of Wine.
    bool m_isDosProgram = false;
};
