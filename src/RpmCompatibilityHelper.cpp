// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

#include "RpmCompatibilityHelper.h"
#include "PackageUtils.h"

#include <KIO/ApplicationLauncherJob>
#include <KIO/CommandLauncherJob>
#include <KIO/JobUiDelegateFactory>
#include <KLocalizedContext>
#include <KLocalizedString>
#include <QProcess>

RpmCompatibilityHelper::RpmCompatibilityHelper(const QUrl &filePath, QObject *parent)
    : ICompatibilityHelper(filePath, parent)
{
    m_nativeAppName = m_filePath.fileName();

    QStringList metainfoFilesContent;

    if (!extractMetainfoFiles(m_filePath, metainfoFilesContent)) {
        qWarning() << "An alternative native application will not be matched for this RPM package.";
        return;
    }

    if (metainfoFilesContent.isEmpty()) {
        m_isAnApp = false;
        return;
    }

    matchFlatpakFromMetainfo(metainfoFilesContent, m_nativeAppRef, m_nativeAppName, m_hasFlatpakApp, m_isAnApp);
}

QString RpmCompatibilityHelper::windowTitle() const
{
    return nativeAppName();
}

QString RpmCompatibilityHelper::heading() const
{
    if (hasNativeApp()) {
        if (isNativeAppInstalled()) {
            return i18n("Open the native version of %1 instead", nativeAppName());
        } else if (m_hasFlatpakApp) {
            return i18n("Install %1 from %2 instead", nativeAppName(), appStoreName());
        } else if (m_isAnApp) {
            return i18n("Search for %1 in %2 instead", nativeAppName(), appStoreName());
        }
    } else {
        return i18n("RPM packages are not natively supported on %1", distroName());
    }

    return QString();
}

QString RpmCompatibilityHelper::icon() const
{
    if (hasNativeApp() && hasIcon(nativeAppRef())) {
        return nativeAppRef();
    }
    return u"application-x-rpm"_s;
}

QString RpmCompatibilityHelper::description() const
{
    QString desc;

    if (hasNativeApp()) {
        if (isNativeAppInstalled()) {
            desc = i18n("A native %1 version of %2 is already installed on your system. ", distroName(), nativeAppName());
            desc += i18n("It's recommended to use the native version for better system integration.");
        } else if (m_hasFlatpakApp) {
            desc = i18n("A native %1 version of %2 is available for installation. ", distroName(), nativeAppName());
            desc += i18n("Installing the native version is recommended for better system integration.");
        } else if (m_isAnApp) {
            desc += i18n("A native %1 version of %2 may be available for installation from %3. ", distroName(), nativeAppName(), appStoreName());
            desc += i18n("Installing the native version is recommended for better system integration.");
        }
    } else {
        desc = i18n("You can search for alternatives online or in %1.", appStoreName());
    }

    desc += u"<br><br>"_s;
    desc += i18n("Alternatively, you may be able to create a Distrobox to run this RPM package in a containerized environment. ");
    desc += i18n("This is not recommended for most users, as it requires additional advanced setup.");

    return desc;
}

bool RpmCompatibilityHelper::hasNativeApp() const
{
    // The native app action will be to open/install the native app if it exists in Flatpak,
    // or to search for the name in the app store if it doesn't.
    return m_hasFlatpakApp || m_isAnApp;
}

QString RpmCompatibilityHelper::nativeAppName() const
{
    return m_nativeAppName;
}

QString RpmCompatibilityHelper::nativeAppRef() const
{
    return m_nativeAppRef;
}

bool RpmCompatibilityHelper::isNativeAppInstalled() const
{
    return isAppInstalled(nativeAppRef());
}

QString RpmCompatibilityHelper::nativeAppActionText() const
{
    if (isNativeAppInstalled()) {
        return i18n("Open %1", nativeAppName());
    } else if (m_hasFlatpakApp) {
        return i18n("Install %1", nativeAppName());
    } else if (m_isAnApp) {
        return i18n("Search for %1 in %2", nativeAppName(), appStoreName());
    }

    return QString();
}

QString RpmCompatibilityHelper::nativeAppActionIcon() const
{
    if (isNativeAppInstalled()) {
        return nativeAppRef();
    } else {
        return appStoreIcon();
    }
}

void RpmCompatibilityHelper::nativeAppAction() const
{
    if (!hasNativeApp()) {
        qWarning() << "Invalid operation: No native application was found for the provided RPM package.";
        return;
    }

    if (isNativeAppInstalled()) {
        openApp(nativeAppRef());
    } else if (m_hasFlatpakApp) {
        openAppInAppStore(nativeAppRef());
    } else if (m_isAnApp) {
        openAppInAppStore(nativeAppName());
    }
}
