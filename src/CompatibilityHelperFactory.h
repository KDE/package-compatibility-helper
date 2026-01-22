// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

#pragma once

#include "ICompatibilityHelper.h"

class CompatibilityHelperFactory
{
public:
    // Returns nullptr if no compatible helper was found for the given file type.
    // In that case, the application should exit.
    static ICompatibilityHelper *create(const QUrl &filePath);

private:
    static ICompatibilityHelper *createWindowsCompatibilityHelper(const QUrl &databaseFilePath, const QUrl &openedExePath);
    static ICompatibilityHelper *createRpmCompatibilityHelper(const QUrl &filePath);
    static ICompatibilityHelper *createDebCompatibilityHelper(const QUrl &filePath);
};
