// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

#include <QApplication>
#include <QtGlobal>

#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QUrl>

#include "ICompatibilityHelper.h"
#include "version-package-compatibility-helper.h"
#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>
#include <qcoreapplication.h>

#include "CompatibilityHelperFactory.h"
#include "CompatibilityToolInstaller.h"
#include "GenericCompatibilityHelper.h"

using namespace Qt::Literals::StringLiterals;

// Handles `package-compatibility-helper --install <config-name>`.
// Exit codes (package-compatibility-helper-run relies on these):
// 0 the Flatpak is installed,
// 1 the user declined or the installation failed, 2 bad usage or config.
static int runGui(QApplication &app, ICompatibilityHelper *helper)
{
    QQmlApplicationEngine engine;
    qmlRegisterSingletonInstance("org.kde.packagecompatibilityhelper", 1, 0, "PackageCompatibilityHelper", helper);
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule("org.kde.packagecompatibilityhelper", u"Main");
    return engine.rootObjects().isEmpty() ? 1 : app.exec();
}

static int runInstall(QApplication &app, const QStringList &args)
{
    if (args.size() != 3) {
        qWarning() << "Usage: package-compatibility-helper --install <config-name>";
        return 2;
    }

    auto *helper = new GenericCompatibilityHelper(QUrl(), &app);
    helper->setInstallOnly(true);
    auto *installer = CompatibilityToolInstaller::load(args.at(2), helper);
    if (!installer) {
        qWarning() << "No valid installer config named" << args.at(2) << "was found.";
        return 2;
    }

    // Already installed, no GUI needed.
    if (installer->isInstalled()) {
        installer->runPostInstall();
        return 0;
    }

    helper->setCompatibilityToolInstaller(installer);
    return runGui(app, helper);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const QStringList args = QCoreApplication::arguments();

    // Ensure there's actually something to run.
    if (args.size() < 2) {
        qWarning() << "No executable file provided.";
        qWarning() << "Usage: package-compatibility-helper <path to file>";
        qWarning() << "       package-compatibility-helper --install <config-name>";
        return -1;
    }

    // Default to org.kde.desktop style unless the user forces another style
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(u"org.kde.desktop"_s);
    }

    KLocalizedString::setApplicationDomain("package-compatibility-helper");
    QCoreApplication::setOrganizationName(u"KDE"_s);

    KAboutData aboutData(
        // The program name used internally.
        u"package-compatibility-helper"_s,
        // A displayable program name string.
        i18nc("@title", "Package Compatibility Helper"),
        // The program version string.
        QStringLiteral(PACKAGE_COMPATIBILITY_HELPER_VERSION_STRING),
        // Short description of what the app does.
        i18n("Provides support for running or finding alternatives to certain package types on immutable distributions."),
        // The license this code is released under.
        KAboutLicense::GPL,
        // Copyright Statement.
        i18n("(c) 2025"));
    aboutData.addAuthor(i18nc("@info:credit", "Thomas Duckworth"), i18nc("@info:credit", "Maintainer"), u"tduck@filotimoproject.org"_s, u"https://kde.org/"_s);
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    KAboutData::setApplicationData(aboutData);
    QGuiApplication::setWindowIcon(QIcon::fromTheme(u"apper"_s));

    // Install mode, used by the package-compatibility-helper-run launcher.
    if (args.at(1) == u"--install"_s) {
        return runInstall(app, args);
    }

    const QUrl filePath = QUrl::fromLocalFile(QString::fromLocal8Bit(argv[1]));
    ICompatibilityHelper *helper = CompatibilityHelperFactory::create(filePath);
    if (!helper) {
        qWarning() << "No compatible helper found for the provided file type.";
        return -1;
    }
    helper->setParent(&app);

    return runGui(app, helper);
}
