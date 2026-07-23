// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

#pragma once

#include "ICompatibilityHelper.h"

using namespace Qt::Literals::StringLiterals;

class DebCompatibilityHelper : public ICompatibilityHelper
{
    Q_OBJECT

public:
    explicit DebCompatibilityHelper(const QUrl &filePath, QObject *parent = nullptr);
    ~DebCompatibilityHelper() override = default;

    QString windowTitle() const override;
    QString heading() const override;
    QString icon() const override;
    QString description() const override;
    bool hasNativeApp() const override;
    QString nativeAppActionText() const override;
    QString nativeAppActionIcon() const override;

    Q_INVOKABLE void nativeAppAction() const override;

private:
    QString m_nativeAppName;
    QString m_nativeAppRef;

    // Whether a corresponding Flatpak application was found.
    bool m_hasFlatpakApp = false;

    // Whether the DEB package being opened is an actual application.
    // This is used to determine if the helper should offer to search Discover or not.
    // This is set to true if the DEB package has a metainfo file with an application name.
    bool m_isAnApp = false;

    QString nativeAppName() const override;
    QString nativeAppRef() const override;
    bool isNativeAppInstalled() const override;
};
