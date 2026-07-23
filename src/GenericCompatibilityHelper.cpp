// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#include "GenericCompatibilityHelper.h"

#include <KLocalizedString>
#include <QMimeDatabase>
#include <QMimeType>

GenericCompatibilityHelper::GenericCompatibilityHelper(const QUrl &filePath, QObject *parent)
    : ICompatibilityHelper(filePath, parent)
{
    if (m_filePath.isValid() && m_filePath.isLocalFile()) {
        const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(m_filePath.toLocalFile());
        m_mimeComment = mimeType.comment();
        if (hasIcon(mimeType.iconName())) {
            m_mimeIcon = mimeType.iconName();
        } else if (hasIcon(mimeType.genericIconName())) {
            m_mimeIcon = mimeType.genericIconName();
        }
    }
    if (m_mimeIcon.isEmpty()) {
        m_mimeIcon = u"application-x-executable"_s;
    }
}

QString GenericCompatibilityHelper::windowTitle() const
{
    // In --install mode there is no file, so use the tool's name.
    if (installOnly()) {
        return compatibilityToolName();
    }
    return m_filePath.fileName();
}

QString GenericCompatibilityHelper::heading() const
{
    if (!m_mimeComment.isEmpty()) {
        return i18nc("@title %1 is a file type name like \"Java archive\", %2 is the distro name",
                     "%1 files are not natively supported on %2",
                     m_mimeComment,
                     distroName());
    }
    return i18nc("@title %1 is the distro name", "This type of file is not natively supported on %1", distroName());
}

QString GenericCompatibilityHelper::icon() const
{
    return m_mimeIcon;
}

QString GenericCompatibilityHelper::description() const
{
    QString desc = i18n("You can search for alternatives online or in %1.", appStoreName());

    if (hasCompatibilityTool()) {
        desc += u"<br><br>"_s;
        if (isCompatibilityToolInstalled()) {
            desc += i18n("Alternatively, you can open this file with %1.", compatibilityToolName());
        } else {
            desc += i18n("Alternatively, you can install %1 to open this type of file.", compatibilityToolName());
        }
    } else {
        desc += u"<br><br>"_s;
        desc += i18n("Learn about options for getting it by clicking <b>Get Help</b> below.");
    }

    return desc;
}

bool GenericCompatibilityHelper::hasNativeApp() const
{
    return false;
}

QString GenericCompatibilityHelper::nativeAppActionText() const
{
    return QString();
}

QString GenericCompatibilityHelper::nativeAppActionIcon() const
{
    return QString();
}

QString GenericCompatibilityHelper::nativeAppName() const
{
    return QString();
}

QString GenericCompatibilityHelper::nativeAppRef() const
{
    return QString();
}

bool GenericCompatibilityHelper::isNativeAppInstalled() const
{
    return false;
}
