// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

#include "CompatibilityHelperFactory.h"
#include "BatchCompatibilityHelper.h"
#include "CompatibilityToolInstaller.h"
#include "DebCompatibilityHelper.h"
#include "GenericCompatibilityHelper.h"
#include "ICompatibilityHelper.h"
#include "PackageUtils.h"
#include "RpmCompatibilityHelper.h"
#include "WindowsCompatibilityHelper.h"
#include "directories.h"

#include <QMimeDatabase>
#include <QMimeType>

ICompatibilityHelper *CompatibilityHelperFactory::create(const QUrl &filePath)
{
    if (!filePath.isValid() || !filePath.isLocalFile()) {
        return nullptr;
    }

    QMimeDatabase mimeDb;
    const QMimeType mimeType = mimeDb.mimeTypeForFile(filePath.toLocalFile());
    const QString mimeTypeName = mimeType.name();

    // Not MZ binaries, matched by name.
    static const QStringList windowsAuxMimeTypes = {
        u"application/x-msi"_s,
        u"application/x-ms-shortcut"_s,
        u"application/x-bat"_s,
        u"application/x-msdos-batch"_s,
    };

    // All MZ executables (DOS, NE, PE) inherit these two, regardless of
    // shared-mime-info version; inherits() also resolves aliases.
    const bool isWindowsExecutable = mimeType.inherits(u"application/x-ms-dos-executable"_s) || mimeType.inherits(u"application/x-msdownload"_s);

    // The MIME type used to look up the compatibility tool config.
    QString toolMimeType = mimeTypeName;
    ICompatibilityHelper *helper = nullptr;

    if (isWindowsExecutable || windowsAuxMimeTypes.contains(mimeTypeName)) {
        const WindowsBinaryKind kind = classifyWindowsBinary(filePath.toLocalFile(), mimeTypeName);

        if (kind == WindowsBinaryKind::Batch) {
            // A .bat file cannot be told apart from a DOS batch file, so offer
            // both Wine and DOSBox. Wine is the primary (highlighted) tool via
            // the PE MIME type below; BatchCompatibilityHelper loads DOSBox as
            // the secondary tool itself.
            toolMimeType = u"application/vnd.microsoft.portable-executable"_s;
            helper = createBatchCompatibilityHelper(filePath);
        } else {
            // DOS programs get the DOS emulator; PE and NE executables, MSI
            // packages, .cmd/shortcut files get Wine.
            const bool isDosProgram = kind == WindowsBinaryKind::DosProgram;
            toolMimeType = isDosProgram ? u"application/x-ms-dos-executable"_s : u"application/vnd.microsoft.portable-executable"_s;
            helper = createWindowsCompatibilityHelper(QUrl::fromLocalFile(WINDOWSCOMPATIBILITYHELPER_DB_PATH), filePath, isDosProgram);
        }
    } else if (mimeTypeName == u"application/x-rpm"_s) {
        helper = createRpmCompatibilityHelper(filePath);
    } else if (mimeTypeName == u"application/vnd.debian.binary-package"_s || mimeTypeName == u"application-x-deb"_s) {
        helper = createDebCompatibilityHelper(filePath);
    }

    // TODO: Create an AppImage compatibility helper.
    /*if (mimeTypeName == u"application/vnd.appimage"_s || mimeTypeName == u"application/x-iso9660-appimage"_s) {
        helper = createAppImageCompatibilityHelper(filePath);
    }*/

    CompatibilityToolInstaller *installer = CompatibilityToolInstaller::findForMimeType(toolMimeType);

    // This returns when no compatible helper was found for the given file type.
    // At this point, the program should exit.
    if (!helper && installer) {
        helper = createGenericCompatibilityHelper(filePath);
    }
    if (!helper) {
        return nullptr;
    }

    if (installer) {
        installer->setParent(helper);
    }
    helper->setCompatibilityToolInstaller(installer);

    return helper;
}

ICompatibilityHelper *CompatibilityHelperFactory::createWindowsCompatibilityHelper(const QUrl &databaseFilePath, const QUrl &openedExePath, bool isDosProgram)
{
    return new WindowsCompatibilityHelper(databaseFilePath, openedExePath, isDosProgram);
}

ICompatibilityHelper *CompatibilityHelperFactory::createBatchCompatibilityHelper(const QUrl &filePath)
{
    return new BatchCompatibilityHelper(filePath);
}

ICompatibilityHelper *CompatibilityHelperFactory::createRpmCompatibilityHelper(const QUrl &filePath)
{
    return new RpmCompatibilityHelper(filePath);
}

ICompatibilityHelper *CompatibilityHelperFactory::createDebCompatibilityHelper(const QUrl &filePath)
{
    return new DebCompatibilityHelper(filePath);
}

ICompatibilityHelper *CompatibilityHelperFactory::createGenericCompatibilityHelper(const QUrl &filePath)
{
    return new GenericCompatibilityHelper(filePath);
}
