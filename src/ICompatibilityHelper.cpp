// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

#include "ICompatibilityHelper.h"
#include "CompatibilityToolInstaller.h"

#include <KIO/ApplicationLauncherJob>
#include <KIO/CommandLauncherJob>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenUrlJob>
#include <KLocalizedContext>
#include <KLocalizedString>
#include <KShell>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QRegularExpression>

#define DOCUMENTATION_URL QUrl(u"https://kde.org/linux/docs/more-software"_s)

void ICompatibilityHelper::openAppInAppStore(const QString &ref) const
{
    // TODO: Add actual logic to determine the default app store.
    // ...or, get a specific app store which is configurable by the vendor.
    // This can be done with a KConfig object as a dependency to ICompatibilityHelper.
    // This can also be used for subclass-specific configuration, i.e. for setting specific compatibility tools.
    KIO::CommandLauncherJob *job = new KIO::CommandLauncherJob(u"plasma-discover"_s, QStringList() << u"--search"_s << ref);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
    job->start();
}

void ICompatibilityHelper::openApp(const QString &ref, const QList<QUrl> &urls, const QString &transientFolderAccess) const
{
    // Exported Flatpak entries spell out the path to flatpak, e.g.
    // "/usr/bin/flatpak run --command=wine --file-forwarding org.winehq.Wine @@u %u @@".
    static const QRegularExpression flatpakRun(u"^(?:\\S*/)?flatpak\\s+run\\s+"_s);
    static const QRegularExpression fileForwarding(u"\\s--file-forwarding\\b|\\s@@u?(?=\\s|$)"_s);

    KService::Ptr service = KService::serviceByDesktopName(ref);

    if (service && !urls.isEmpty()) {
        QString exec = service->exec();
        const QRegularExpressionMatch match = flatpakRun.match(exec);

        if (match.hasMatch()) {
            // Document portal paths put the file in a directory of its own, so
            // hand over the real path and open its folder instead.
            exec.remove(fileForwarding);
            exec.replace(u"%u"_s, u"%f"_s);
            exec.replace(u"%U"_s, u"%F"_s);

            // Amending the line keeps --branch, --arch and --command.
            if (!transientFolderAccess.isEmpty()) {
                exec.insert(match.capturedEnd(), u"--filesystem=%1:rw "_s.arg(KShell::quoteArg(transientFolderAccess)));
            }
            service->setExec(exec);
        } else if (!transientFolderAccess.isEmpty()) {
            qWarning() << "The desktop entry for" << ref << "does not launch a Flatpak, so its Exec line was left as-is:" << exec;
        }
    }

    KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob(service);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
    job->setUrls(urls);
    job->start();
}

bool ICompatibilityHelper::isAppInstalled(const QString &ref) const
{
    KService::Ptr service = KService::serviceByDesktopName(ref);
    return service && service->isValid() && service->isApplication();
}

QString ICompatibilityHelper::appStoreIcon() const
{
    // TODO: See openAppInAppStore.
    return u"plasmadiscover"_s; // Defaulting to Discover for now.
}

QString ICompatibilityHelper::appStoreName() const
{
    // TODO: See openAppInAppStore.
    return i18n("Discover"); // Defaulting to Discover for now.
}

void ICompatibilityHelper::openWithAction() const
{
    // Running with no KService will invoke the "Open With" dialog.
    // See https://api.kde.org/frameworks/kio/html/classKIO_1_1ApplicationLauncherJob.html
    KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob();
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
    job->setUrls({m_filePath});
    job->start();
}

QString ICompatibilityHelper::distroName() const
{
    static QString distroName = []() {
        QFile file(u"/etc/os-release"_s);

        // Check if the file can be opened for reading
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Could not open /etc/os-release.";
            qWarning() << "The distro name could not be determined from /etc/os-release. Defaulting to \"Linux\".";
            return i18n("Linux"); // Default to "Linux".
        }

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith(u"NAME="_s)) {
                QString distroName = line.section(u"="_s, 1, 1); // Get the part after the '='

                // Remove surrounding quotes if they exist
                if (distroName.startsWith(u"\""_s) && distroName.endsWith(u"\""_s)) {
                    distroName = distroName.mid(1, distroName.length() - 2);
                }

                file.close();
                return distroName;
            }
        }

        file.close();

        qWarning() << "The distro name could not be determined from /etc/os-release. Defaulting to \"Linux\".";
        return i18n("Linux"); // Default to "Linux".
    }();

    return distroName;
}

bool ICompatibilityHelper::hasIcon(const QString &ref) const
{
    return QIcon::hasThemeIcon(ref);
}

QUrl ICompatibilityHelper::documentationUrl() const
{
    return DOCUMENTATION_URL;
}

void ICompatibilityHelper::documentationAction() const
{
    KIO::OpenUrlJob *job = new KIO::OpenUrlJob(documentationUrl());
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
    job->start();
}

QString ICompatibilityHelper::documentationActionIcon() const
{
    return u"help-contents-symbolic"_s;
}

QString ICompatibilityHelper::documentationActionText() const
{
    return i18n("Get Help");
}

// Default implementation for the pure virtual Q_INVOKABLE in ICompatibilityHelper.
// This should be overridden in subclasses to provide specific functionality.
// This is to avoid linker errors as the MOC is not able to resolve these without default implementations.
void ICompatibilityHelper::nativeAppAction() const
{
    qWarning() << "nativeAppAction() was called on base class - this should always be overridden in subclasses.";
    qWarning() << "Please file a bug report.";
}

void ICompatibilityHelper::setCompatibilityToolInstaller(CompatibilityToolInstaller *installer)
{
    m_compatibilityToolInstaller = installer;
}

void ICompatibilityHelper::setSecondaryCompatibilityToolInstaller(CompatibilityToolInstaller *installer)
{
    m_secondaryCompatibilityToolInstaller = installer;
}

bool ICompatibilityHelper::installOnly() const
{
    return m_installOnly;
}

void ICompatibilityHelper::setInstallOnly(bool installOnly)
{
    m_installOnly = installOnly;
}

bool ICompatibilityHelper::hasCompatibilityTool() const
{
    return m_compatibilityToolInstaller != nullptr;
}

QString ICompatibilityHelper::compatibilityToolName() const
{
    return m_compatibilityToolInstaller != nullptr ? m_compatibilityToolInstaller->displayName() : QString();
}

QObject *ICompatibilityHelper::compatibilityToolInstaller() const
{
    return m_compatibilityToolInstaller;
}

bool ICompatibilityHelper::isCompatibilityToolInstalled() const
{
    if (!m_compatibilityToolInstaller) {
        return false;
    }
    return m_compatibilityToolInstaller->isInstalled();
}

bool ICompatibilityHelper::compatibilityToolInstalled() const
{
    return isCompatibilityToolInstalled();
}

QString ICompatibilityHelper::compatibilityToolActionText() const
{
    if (!hasCompatibilityTool()) {
        return QString();
    }
    if (isCompatibilityToolInstalled()) {
        return i18n("Run with %1", compatibilityToolName());
    }
    return i18n("Install %1", compatibilityToolName());
}

QString ICompatibilityHelper::compatibilityToolActionIcon() const
{
    if (!m_compatibilityToolInstaller) {
        return QString();
    }
    if (isCompatibilityToolInstalled() && hasIcon(m_compatibilityToolInstaller->appId())) {
        return m_compatibilityToolInstaller->appId();
    }
    return m_compatibilityToolInstaller->icon();
}

QString ICompatibilityHelper::compatibilityToolWarning() const
{
    if (!hasCompatibilityTool()) {
        return QString();
    }

    // With two tools on offer the user has not picked one yet, so don't name one.
    if (hasSecondaryCompatibilityTool()) {
        if (isCompatibilityToolInstalled() && isSecondaryCompatibilityToolInstalled()) {
            return i18nc("@info",
                         "Running this file temporarily gives the compatibility tool access to everything in this folder, in addition to its existing "
                         "permissions. The file is not checked for safety. <b>Only run files from trusted sources.</b>");
        }
        return i18nc("@info",
                     "Installing a compatibility tool and running this file with it temporarily gives it access to everything in this folder, in addition to "
                     "the permissions it comes with. The file is not checked for safety. <b>Only run files from trusted sources.</b>");
    }

    if (isCompatibilityToolInstalled()) {
        return i18nc("@info %1 is an application name, e.g. \"Wine\"",
                     "Running this file temporarily gives %1 access to everything in this folder, in addition to its existing permissions. The file is not "
                     "checked for safety. <b>Only run files from trusted sources.</b>",
                     compatibilityToolName());
    }
    return i18nc("@info %1 is an application name, e.g. \"Wine\"",
                 "Installing %1 and running this file with it temporarily gives it access to everything in this folder, in addition to the permissions it "
                 "comes with. The file is not checked for safety. <b>Only run files from trusted sources.</b>",
                 compatibilityToolName());
}

void ICompatibilityHelper::launchCompatibilityTool() const
{
    if (!hasCompatibilityTool()) {
        qWarning() << "Invalid operation: No compatibility tool is available for this file type.";
        return;
    }
    if (!isCompatibilityToolInstalled()) {
        qWarning() << "Invalid operation: The compatibility tool is not installed.";
        return;
    }
    m_compatibilityToolInstaller->takeOverMimeTypes();
    openApp(m_compatibilityToolInstaller->appId(), {m_filePath}, fileFolder());
}

void ICompatibilityHelper::compatibilityToolInstallFinished() const
{
    if (!m_compatibilityToolInstaller) {
        return;
    }
    m_compatibilityToolInstaller->runPostInstall();
    openApp(m_compatibilityToolInstaller->appId(), {m_filePath}, fileFolder());
}

bool ICompatibilityHelper::hasSecondaryCompatibilityTool() const
{
    return m_secondaryCompatibilityToolInstaller != nullptr;
}

QString ICompatibilityHelper::secondaryCompatibilityToolName() const
{
    return m_secondaryCompatibilityToolInstaller != nullptr ? m_secondaryCompatibilityToolInstaller->displayName() : QString();
}

QObject *ICompatibilityHelper::secondaryCompatibilityToolInstaller() const
{
    return m_secondaryCompatibilityToolInstaller;
}

bool ICompatibilityHelper::isSecondaryCompatibilityToolInstalled() const
{
    if (!m_secondaryCompatibilityToolInstaller) {
        return false;
    }
    return m_secondaryCompatibilityToolInstaller->isInstalled();
}

bool ICompatibilityHelper::secondaryCompatibilityToolInstalled() const
{
    return isSecondaryCompatibilityToolInstalled();
}

QString ICompatibilityHelper::secondaryCompatibilityToolActionText() const
{
    if (!hasSecondaryCompatibilityTool()) {
        return QString();
    }
    if (isSecondaryCompatibilityToolInstalled()) {
        return i18n("Run with %1", secondaryCompatibilityToolName());
    }
    return i18n("Install %1", secondaryCompatibilityToolName());
}

QString ICompatibilityHelper::secondaryCompatibilityToolActionIcon() const
{
    if (!m_secondaryCompatibilityToolInstaller) {
        return QString();
    }
    if (isSecondaryCompatibilityToolInstalled() && hasIcon(m_secondaryCompatibilityToolInstaller->appId())) {
        return m_secondaryCompatibilityToolInstaller->appId();
    }
    return m_secondaryCompatibilityToolInstaller->icon();
}

void ICompatibilityHelper::launchSecondaryCompatibilityTool() const
{
    if (!hasSecondaryCompatibilityTool()) {
        qWarning() << "Invalid operation: No secondary compatibility tool is available for this file type.";
        return;
    }
    if (!isSecondaryCompatibilityToolInstalled()) {
        qWarning() << "Invalid operation: The secondary compatibility tool is not installed.";
        return;
    }
    // Deliberately not calling takeOverMimeTypes() here: the secondary tool is
    // the non-default choice, so it should not claim the MIME type handler.
    openApp(m_secondaryCompatibilityToolInstaller->appId(), {m_filePath}, fileFolder());
}

void ICompatibilityHelper::secondaryCompatibilityToolInstallFinished() const
{
    if (!m_secondaryCompatibilityToolInstaller) {
        return;
    }
    m_secondaryCompatibilityToolInstaller->runPostInstall();
    openApp(m_secondaryCompatibilityToolInstaller->appId(), {m_filePath}, fileFolder());
}

QString ICompatibilityHelper::fileFolder() const
{
    if (!m_filePath.isValid() || !m_filePath.isLocalFile()) {
        return QString();
    }
    return QDir::cleanPath(QFileInfo(m_filePath.toLocalFile()).absolutePath());
}
