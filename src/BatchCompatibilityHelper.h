// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 2026 Hadi Chokr <hadichokr@icloud.com>

#pragma once

#include "ICompatibilityHelper.h"

using namespace Qt::Literals::StringLiterals;

// Helper for .bat files, which are indistinguishable between old MS-DOS batch
// scripts and modern Windows batch scripts. Rather than guess, it offers both
// compatibility tools: Wine as the primary (default) tool and DOSBox as the
// secondary one, with a description explaining which to pick.
//
// The primary (Wine) installer is attached by CompatibilityHelperFactory via
// the usual MIME-type lookup; the DOSBox installer is loaded here.
class BatchCompatibilityHelper : public ICompatibilityHelper
{
    Q_OBJECT

public:
    explicit BatchCompatibilityHelper(const QUrl &filePath, QObject *parent = nullptr);
    ~BatchCompatibilityHelper() override = default;

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
};
