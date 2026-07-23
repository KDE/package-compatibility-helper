// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

#include "CompatibilityHelperFactory.h"
#include "CompatibilityToolInstaller.h"
#include "DebCompatibilityHelper.h"
#include "GenericCompatibilityHelper.h"
#include "ICompatibilityHelper.h"
#include "RpmCompatibilityHelper.h"
#include "WindowsCompatibilityHelper.h"
#include "directories.h"

#include <QMimeDatabase>

ICompatibilityHelper *CompatibilityHelperFactory::create(const QUrl &filePath)
{
    if (!filePath.isValid() || !filePath.isLocalFile()) {
        return nullptr;
    }

    QMimeDatabase mimeDb;
    QString mimeTypeName = mimeDb.mimeTypeForFile(filePath.toLocalFile()).name();

    CompatibilityToolInstaller *installer = CompatibilityToolInstaller::findForMimeType(mimeTypeName);
    ICompatibilityHelper *helper = nullptr;

    if (mimeTypeName == u"application/x-ms-dos-executable"_s || mimeTypeName == u"application/x-msi"_s || mimeTypeName == u"application/x-ms-shortcut"_s
        || mimeTypeName == u"application/vnd.microsoft.portable-executable"_s || mimeTypeName == u"application/x-msdownload"_s) {
        helper = createWindowsCompatibilityHelper(QUrl::fromLocalFile(WINDOWSCOMPATIBILITYHELPER_DB_PATH), filePath);
    } else if (mimeTypeName == u"application/x-rpm"_s) {
        helper = createRpmCompatibilityHelper(filePath);
    } else if (mimeTypeName == u"application/vnd.debian.binary-package"_s || mimeTypeName == u"application-x-deb"_s) {
        helper = createDebCompatibilityHelper(filePath);
    }

    // TODO: Create an AppImage compatibility helper.
    /*if (mimeTypeName == u"application/vnd.appimage"_s || mimeTypeName == u"application/x-iso9660-appimage"_s) {
        helper = createAppImageCompatibilityHelper(filePath);
    }*/

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

ICompatibilityHelper *CompatibilityHelperFactory::createWindowsCompatibilityHelper(const QUrl &databaseFilePath, const QUrl &openedExePath)
{
    return new WindowsCompatibilityHelper(databaseFilePath, openedExePath);
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
