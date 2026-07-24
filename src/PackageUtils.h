// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

#pragma once

#include <QDebug>
#include <QString>

using namespace Qt::Literals::StringLiterals;

// Match a Flatpak application based on an app's metainfo file.
// This is used to find a corresponding Flatpak application for an RPM/DEB package.
void matchFlatpakFromMetainfo(QStringList metainfoFilesContent, QString &nativeAppRef, QString &nativeAppName, bool &hasFlatpakApp, bool &isAnApp);

// Extract metainfo files in a package format-agnostic way, using bsdtar.
bool extractMetainfoFiles(const QUrl &archivePath, QStringList &metainfoFilesContent);

// Classifies DOS/Windows files by their MZ/NE/PE headers, since the MIME
// types reported for MZ executables vary between systems.
enum class WindowsBinaryKind {
    NotWindows,
    DosProgram, // Real-mode MZ, incl. LE/LX
    Win16, // NE
    WindowsPE,
    Batch, // Ambiguous .bat: could be a DOS or a Windows batch script.
    WindowsBatch, // .cmd: an NT-only extension, so always a Windows batch script.
    Msi,
    Shortcut,
};

WindowsBinaryKind classifyWindowsBinary(const QString &localFilePath, const QString &mimeTypeName);
