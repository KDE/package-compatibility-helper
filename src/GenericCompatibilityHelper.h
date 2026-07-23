// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#pragma once

#include "ICompatibilityHelper.h"

using namespace Qt::Literals::StringLiterals;

// Used for file types that have no dedicated helper but do have a
// compatibility tool config with a matching MimeTypes= entry.
//
// Also used (with an empty file path) as the backing object of
// `package-compatibility-helper --install <name>`, where Main.qml goes
// straight to the install page.
class GenericCompatibilityHelper : public ICompatibilityHelper
{
    Q_OBJECT

public:
    explicit GenericCompatibilityHelper(const QUrl &filePath, QObject *parent = nullptr);
    ~GenericCompatibilityHelper() override = default;

    QString windowTitle() const override;
    QString heading() const override;
    QString icon() const override;
    QString description() const override;
    bool hasNativeApp() const override;
    QString nativeAppActionText() const override;
    QString nativeAppActionIcon() const override;

private:
    QString nativeAppName() const override;
    QString nativeAppRef() const override;
    bool isNativeAppInstalled() const override;

    // The user-visible name of the file's MIME type, e.g. "Java archive".
    QString m_mimeComment;
    QString m_mimeIcon;
};
