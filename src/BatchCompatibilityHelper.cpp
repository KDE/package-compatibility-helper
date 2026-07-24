// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#include "BatchCompatibilityHelper.h"
#include "CompatibilityToolInstaller.h"

#include <KLocalizedString>

BatchCompatibilityHelper::BatchCompatibilityHelper(const QUrl &filePath, QObject *parent)
    : ICompatibilityHelper(filePath, parent)
{
    // Wine (the primary tool) is attached by the factory. Load DOSBox as the
    // secondary tool for the rare case of a genuine MS-DOS batch file.
    if (CompatibilityToolInstaller *dosbox = CompatibilityToolInstaller::load(u"dosbox"_s, this)) {
        setSecondaryCompatibilityToolInstaller(dosbox);
    }
}

QString BatchCompatibilityHelper::windowTitle() const
{
    return m_filePath.fileName();
}

QString BatchCompatibilityHelper::heading() const
{
    return i18n("Batch scripts are not natively supported on %1", distroName());
}

QString BatchCompatibilityHelper::icon() const
{
    return u"application-x-ms-dos-executable"_s;
}

QString BatchCompatibilityHelper::description() const
{
    QString desc = i18n(
        "This is a batch script. It could be either a modern Windows batch file or an old MS-DOS batch "
        "file, and the two cannot be told apart automatically, so please choose how to run it.");

    if (hasCompatibilityTool() && hasSecondaryCompatibilityTool()) {
        desc += u"<br><br>"_s;
        // %1 is Wine, %2 is DOSBox.
        desc += i18n(
            "If you don't know what DOS is, or the software was written within roughly the last 30 years, "
            "choose <b>%1</b>. Only choose <b>%2</b> if you know this is an MS-DOS batch file.",
            compatibilityToolName(),
            secondaryCompatibilityToolName());
    }

    return desc;
}

bool BatchCompatibilityHelper::hasNativeApp() const
{
    return false;
}

QString BatchCompatibilityHelper::nativeAppActionText() const
{
    return QString();
}

QString BatchCompatibilityHelper::nativeAppActionIcon() const
{
    return QString();
}

QString BatchCompatibilityHelper::nativeAppName() const
{
    return QString();
}

QString BatchCompatibilityHelper::nativeAppRef() const
{
    return QString();
}

bool BatchCompatibilityHelper::isNativeAppInstalled() const
{
    return false;
}
