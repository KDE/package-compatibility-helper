// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#include "CompatibilityToolInstaller.h"
#include "directories.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QIcon>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>

#include <KFormat>
#include <KLocalizedString>

#include <flatpak/flatpak.h>

#include <algorithm>

using namespace Qt::Literals::StringLiterals;

namespace
{

// Aggregates per-operation progress into a single 0-100 value.
struct TransactionWatcher {
    CompatibilityToolInstaller *installer = nullptr;
    int totalOperations = 1;
    int doneOperations = 0;
};

gboolean onTransactionReady(FlatpakTransaction *transaction, gpointer userData)
{
    auto *watcher = static_cast<TransactionWatcher *>(userData);
    GList *operations = flatpak_transaction_get_operations(transaction);
    watcher->totalOperations = std::max<int>(static_cast<int>(g_list_length(operations)), 1);
    g_list_free_full(operations, g_object_unref);
    return TRUE;
}

gboolean onAddNewRemote(FlatpakTransaction *, gint /*FlatpakTransactionRemoteReason*/, const char *, const char *, const char *, gpointer)
{
    // Allow flatpak to add dependency remotes, e.g. the runtime's origin.
    return TRUE;
}

void onProgressChanged(FlatpakTransactionProgress *progress, gpointer userData)
{
    auto *watcher = static_cast<TransactionWatcher *>(userData);
    const int current = flatpak_transaction_progress_get_progress(progress);
    const int overall = std::min((watcher->doneOperations * 100 + current) / watcher->totalOperations, 100);
    QMetaObject::invokeMethod(
        watcher->installer,
        [installer = watcher->installer, overall]() {
            installer->updateProgress(overall);
        },
        Qt::QueuedConnection);
}

void onNewOperation(FlatpakTransaction *, FlatpakTransactionOperation *, FlatpakTransactionProgress *progress, gpointer userData)
{
    flatpak_transaction_progress_set_update_frequency(progress, 250);
    g_signal_connect(progress, "changed", G_CALLBACK(onProgressChanged), userData);
}

void onOperationDone(FlatpakTransaction *, FlatpakTransactionOperation *, const char *, gint, gpointer userData)
{
    auto *watcher = static_cast<TransactionWatcher *>(userData);
    watcher->doneOperations++;
}

// Worker thread only.
QByteArray fetchUrl(const QUrl &url, QString *errorString)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QEventLoop loop;
    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        *errorString = reply->errorString();
        return {};
    }
    return reply->readAll();
}

bool installationHasApp(FlatpakInstallation *installation, const QByteArray &appId, QString *errorText)
{
    g_autoptr(GError) error = nullptr;
    g_autoptr(GPtrArray) refs = flatpak_installation_list_installed_refs_by_kind(installation, FLATPAK_REF_KIND_APP, nullptr, &error);
    if (!refs) {
        if (errorText) {
            *errorText = QString::fromUtf8(error ? error->message : "unknown error");
        }
        return false;
    }

    for (guint i = 0; i < refs->len; ++i) {
        auto *ref = FLATPAK_REF(g_ptr_array_index(refs, i));
        if (appId == flatpak_ref_get_name(ref)) {
            return true;
        }
    }
    return false;
}

} // namespace

CompatibilityToolInstaller::CompatibilityToolInstaller(QObject *parent)
    : QObject(parent)
    , m_cancellable(g_cancellable_new())
{
}

static QStringList configSearchDirs()
{
    return {COMPATIBILITY_TOOL_SYSCONF_CONFIG_DIR, COMPATIBILITY_TOOL_DATA_CONFIG_DIR};
}

CompatibilityToolInstaller *CompatibilityToolInstaller::loadConfigFile(const QString &path, QObject *parent)
{
    QSettings ini(path, QSettings::IniFormat);
    auto *installer = new CompatibilityToolInstaller(parent);
    installer->m_appId = ini.value(u"App/Id"_s).toString();
    installer->m_displayName = ini.value(u"App/Name"_s, installer->m_appId).toString();
    installer->m_icon = ini.value(u"App/Icon"_s, u"application-x-executable"_s).toString();
    installer->m_mimeTypes = ini.value(u"App/MimeTypes"_s).toStringList();
    installer->m_remoteName = ini.value(u"Remote/Name"_s).toString();
    installer->m_remoteUrl = ini.value(u"Remote/Url"_s).toString();
    installer->m_postInstall = ini.value(u"Install/PostInstall"_s).toString();
    installer->m_takeOverMimeTypes = ini.value(u"Install/TakeOverMimeTypes"_s, false).toBool();

    if (installer->m_appId.isEmpty() || installer->m_remoteName.isEmpty() || installer->m_remoteUrl.isEmpty()) {
        delete installer;
        return nullptr;
    }
    return installer;
}

CompatibilityToolInstaller *CompatibilityToolInstaller::load(const QString &name, QObject *parent)
{
    if (name.isEmpty() || name.contains(u'/') || name.startsWith(u'.')) {
        return nullptr;
    }
    for (const QString &dir : configSearchDirs()) {
        const QString candidate = dir + u'/' + name + u".conf"_s;
        if (QFileInfo(candidate).isFile()) {
            return loadConfigFile(candidate, parent);
        }
    }
    return nullptr;
}

CompatibilityToolInstaller *CompatibilityToolInstaller::findForMimeType(const QString &mimeType, QObject *parent)
{
    if (mimeType.isEmpty()) {
        return nullptr;
    }

    QStringList seenNames;
    for (const QString &dir : configSearchDirs()) {
        const QDir configDir(dir);
        const QStringList entries = configDir.entryList({u"*.conf"_s}, QDir::Files, QDir::Name);
        for (const QString &entry : entries) {
            if (seenNames.contains(entry)) {
                continue;
            }
            seenNames.append(entry);

            CompatibilityToolInstaller *installer = loadConfigFile(configDir.filePath(entry), parent);
            if (installer && installer->m_mimeTypes.contains(mimeType)) {
                return installer;
            }
            delete installer;
        }
    }
    return nullptr;
}

CompatibilityToolInstaller::~CompatibilityToolInstaller()
{
    g_cancellable_cancel(m_cancellable);
    if (m_workThread) {
        m_workThread->wait();
    }
    g_object_unref(m_cancellable);
}

QString CompatibilityToolInstaller::displayName() const
{
    return m_displayName;
}

QString CompatibilityToolInstaller::icon() const
{
    if (QIcon::hasThemeIcon(m_icon)) {
        return m_icon;
    }
    return u"install-symbolic"_s;
}

QString CompatibilityToolInstaller::appId() const
{
    return m_appId;
}

QString CompatibilityToolInstaller::remoteName() const
{
    return m_remoteName;
}

int CompatibilityToolInstaller::progress() const
{
    return m_progress;
}

bool CompatibilityToolInstaller::sizesKnown() const
{
    return m_sizesKnown;
}

QString CompatibilityToolInstaller::downloadSizeText() const
{
    return m_sizesKnown ? KFormat().formatByteSize(m_downloadSize) : QString();
}

QString CompatibilityToolInstaller::installedSizeText() const
{
    return m_sizesKnown ? KFormat().formatByteSize(m_installedSize) : QString();
}

QString CompatibilityToolInstaller::errorText() const
{
    return m_errorText;
}

bool CompatibilityToolInstaller::cancelled() const
{
    return m_cancelled.load();
}

void CompatibilityToolInstaller::setErrorText(const QString &text)
{
    if (text != m_errorText) {
        m_errorText = text;
        Q_EMIT errorTextChanged();
    }
}

void CompatibilityToolInstaller::updateProgress(int percent)
{
    if (percent != m_progress) {
        m_progress = percent;
        Q_EMIT progressChanged();
    }
}

void CompatibilityToolInstaller::setResolvedRef(const QString &ref, quint64 downloadSize, quint64 installedSize)
{
    m_ref = ref;
    m_downloadSize = downloadSize;
    m_installedSize = installedSize;
    m_sizesKnown = true;
    Q_EMIT sizesChanged();
}

bool CompatibilityToolInstaller::isInstalled() const
{
    const QByteArray appId = m_appId.toUtf8();
    g_autoptr(GError) userError = nullptr;
    g_autoptr(FlatpakInstallation) userInstallation = flatpak_installation_new_user(nullptr, &userError);
    if (userInstallation) {
        QString errorText;
        if (installationHasApp(userInstallation, appId, &errorText)) {
            return true;
        }
        if (!errorText.isEmpty()) {
            qWarning() << "Could not list user Flatpaks:" << errorText;
        }
    }

    g_autoptr(GError) systemError = nullptr;
    g_autoptr(GPtrArray) systemInstallations = flatpak_get_system_installations(nullptr, &systemError);
    if (!systemInstallations) {
        qWarning() << "Could not list system Flatpak installations:" << (systemError ? systemError->message : "unknown error");
        return false;
    }
    for (guint i = 0; i < systemInstallations->len; ++i) {
        auto *installation = FLATPAK_INSTALLATION(g_ptr_array_index(systemInstallations, i));
        QString errorText;
        if (installationHasApp(installation, appId, &errorText)) {
            return true;
        }
        if (!errorText.isEmpty()) {
            qWarning() << "Could not list system Flatpaks:" << errorText;
        }
    }

    return false;
}

void CompatibilityToolInstaller::runPostInstall() const
{
    const QString exports = QDir::homePath() + u"/.local/share/flatpak/exports/share/applications/"_s;
    QProcess::execute(u"update-desktop-database"_s, {exports});

    if (!QStandardPaths::findExecutable(u"kbuildsycoca6"_s).isEmpty()) {
        QProcess::execute(u"kbuildsycoca6"_s, {});
    }

    if (!m_postInstall.isEmpty()) {
        QStringList argv = QProcess::splitCommand(m_postInstall);
        if (!argv.isEmpty()) {
            const QString program = argv.takeFirst();
            QProcess::execute(program, argv);
        }
    }

    takeOverMimeTypes();
}

void CompatibilityToolInstaller::takeOverMimeTypes() const
{
    if (!m_takeOverMimeTypes) {
        return;
    }

    const QString desktopFile = m_appId + u".desktop"_s;
    for (const QString &mimeType : m_mimeTypes) {
        QProcess::execute(u"xdg-mime"_s, {u"default"_s, desktopFile, mimeType});
    }
}

// Worker thread only.
static bool ensureRemote(const QString &remoteName,
                         const QString &remoteUrl,
                         FlatpakInstallation *installation,
                         GCancellable *cancellable,
                         std::atomic<bool> &remoteAddedByUs,
                         QString *errorString)
{
    const QByteArray remoteNameBytes = remoteName.toUtf8();

    g_autoptr(FlatpakRemote) existing = flatpak_installation_get_remote_by_name(installation, remoteNameBytes.constData(), cancellable, nullptr);
    if (existing) {
        return true;
    }

    const QByteArray repoFile = fetchUrl(QUrl(remoteUrl), errorString);
    if (repoFile.isEmpty()) {
        return false;
    }

    g_autoptr(GBytes) bytes = g_bytes_new(repoFile.constData(), repoFile.size());
    g_autoptr(GError) error = nullptr;
    g_autoptr(FlatpakRemote) remote = flatpak_remote_new_from_file(remoteNameBytes.constData(), bytes, &error);
    if (!remote) {
        *errorString = QString::fromUtf8(error ? error->message : "invalid .flatpakrepo file");
        return false;
    }

    if (!flatpak_installation_add_remote(installation, remote, TRUE /* if_needed */, cancellable, &error)) {
        *errorString = QString::fromUtf8(error ? error->message : "could not add remote");
        return false;
    }

    remoteAddedByUs = true;
    return true;
}

// Worker thread only.
static QString resolveRemoteRef(const QString &remoteName,
                                const QString &appId,
                                FlatpakInstallation *installation,
                                GCancellable *cancellable,
                                quint64 *downloadSize,
                                quint64 *installedSize)
{
    const QByteArray remoteNameBytes = remoteName.toUtf8();
    const QByteArray appIdBytes = appId.toUtf8();

    g_autoptr(GError) error = nullptr;
    g_autoptr(GPtrArray) refs = flatpak_installation_list_remote_refs_sync(installation, remoteNameBytes.constData(), cancellable, &error);
    if (!refs) {
        qWarning() << "Could not list refs of remote" << remoteName << ":" << (error ? error->message : "unknown error");
        return QString();
    }

    FlatpakRemoteRef *match = nullptr;
    for (guint i = 0; i < refs->len; ++i) {
        auto *remoteRef = FLATPAK_REMOTE_REF(g_ptr_array_index(refs, i));
        auto *ref = FLATPAK_REF(remoteRef);
        if (flatpak_ref_get_kind(ref) != FLATPAK_REF_KIND_APP || appIdBytes != flatpak_ref_get_name(ref)) {
            continue;
        }
        match = remoteRef;
        if (g_strcmp0(flatpak_ref_get_arch(ref), flatpak_get_default_arch()) == 0) {
            break;
        }
    }

    if (!match) {
        qWarning() << "Remote" << remoteName << "has no app named" << appId;
        return QString();
    }

    *downloadSize = flatpak_remote_ref_get_download_size(match);
    *installedSize = flatpak_remote_ref_get_installed_size(match);

    g_autofree char *formatted = flatpak_ref_format_ref(FLATPAK_REF(match));
    return QString::fromUtf8(formatted);
}

void CompatibilityToolInstaller::prepare()
{
    if (m_workThread || m_sizesKnown) {
        return;
    }

    m_workThread = QThread::create([this]() {
        g_autoptr(GError) error = nullptr;
        g_autoptr(FlatpakInstallation) installation = flatpak_installation_new_user(m_cancellable, &error);
        if (!installation) {
            qWarning() << "Could not open the user Flatpak installation:" << (error ? error->message : "unknown error");
            return;
        }

        QString errorString;
        if (!ensureRemote(m_remoteName, m_remoteUrl, installation, m_cancellable, m_remoteAddedByUs, &errorString)) {
            qWarning() << "Could not set up remote" << m_remoteName << ":" << errorString;
            return;
        }

        quint64 downloadSize = 0;
        quint64 installedSize = 0;
        const QString ref = resolveRemoteRef(m_remoteName, m_appId, installation, m_cancellable, &downloadSize, &installedSize);
        if (ref.isEmpty()) {
            return;
        }

        QMetaObject::invokeMethod(
            this,
            [this, ref, downloadSize, installedSize]() {
                setResolvedRef(ref, downloadSize, installedSize);
            },
            Qt::QueuedConnection);
    });
    connect(m_workThread, &QThread::finished, this, [this, thread = m_workThread]() {
        thread->deleteLater();
        if (m_workThread == thread) {
            m_workThread = nullptr;
        }
    });
    m_workThread->start();
}

void CompatibilityToolInstaller::start()
{
    if (m_workThread) {
        m_workThread->wait();
    }

    if (m_cancelled.exchange(false)) {
        Q_EMIT cancelledChanged();
    }
    g_cancellable_reset(m_cancellable);

    m_workThread = QThread::create([this]() {
        auto fail = [this](const QString &message) {
            qWarning() << message;
            QMetaObject::invokeMethod(
                this,
                [this, message]() {
                    setErrorText(message);
                },
                Qt::QueuedConnection);
            Q_EMIT finished(false);
        };

        g_autoptr(GError) error = nullptr;
        g_autoptr(FlatpakInstallation) installation = flatpak_installation_new_user(m_cancellable, &error);
        if (!installation) {
            fail(i18n("Could not open the Flatpak installation: %1", QString::fromUtf8(error ? error->message : "unknown error")));
            return;
        }

        QString errorString;
        if (!ensureRemote(m_remoteName, m_remoteUrl, installation, m_cancellable, m_remoteAddedByUs, &errorString)) {
            fail(i18n("Could not set up the remote \"%1\": %2", m_remoteName, errorString));
            return;
        }

        QString ref = m_ref;
        if (ref.isEmpty()) {
            quint64 downloadSize = 0;
            quint64 installedSize = 0;
            ref = resolveRemoteRef(m_remoteName, m_appId, installation, m_cancellable, &downloadSize, &installedSize);
        }
        if (ref.isEmpty()) {
            fail(i18n("The remote \"%1\" has no application named %2.", m_remoteName, m_appId));
            return;
        }

        g_autoptr(FlatpakTransaction) transaction = flatpak_transaction_new_for_installation(installation, m_cancellable, &error);
        if (!transaction) {
            fail(i18n("Could not create a Flatpak transaction: %1", QString::fromUtf8(error ? error->message : "unknown error")));
            return;
        }
        // Look for runtimes in the other configured installations too.
        flatpak_transaction_add_default_dependency_sources(transaction);

        const QByteArray remoteName = m_remoteName.toUtf8();
        const QByteArray refBytes = ref.toUtf8();
        if (!flatpak_transaction_add_install(transaction, remoteName.constData(), refBytes.constData(), nullptr, &error)) {
            fail(i18n("Could not queue the installation of %1: %2", ref, QString::fromUtf8(error ? error->message : "unknown error")));
            return;
        }

        TransactionWatcher watcher;
        watcher.installer = this;
        g_signal_connect(transaction, "ready", G_CALLBACK(onTransactionReady), &watcher);
        g_signal_connect(transaction, "add-new-remote", G_CALLBACK(onAddNewRemote), &watcher);
        g_signal_connect(transaction, "new-operation", G_CALLBACK(onNewOperation), &watcher);
        g_signal_connect(transaction, "operation-done", G_CALLBACK(onOperationDone), &watcher);

        const bool success = flatpak_transaction_run(transaction, m_cancellable, &error);
        if (!success && !m_cancelled) {
            fail(QString::fromUtf8(error ? error->message : "unknown error"));
            return;
        }
        Q_EMIT finished(success && !m_cancelled);
    });
    connect(m_workThread, &QThread::finished, this, [this, thread = m_workThread]() {
        thread->deleteLater();
        if (m_workThread == thread) {
            m_workThread = nullptr;
        }
    });
    m_workThread->start();
}

void CompatibilityToolInstaller::cancel()
{
    if (!m_cancelled.exchange(true)) {
        Q_EMIT cancelledChanged();
    }
    g_cancellable_cancel(m_cancellable);
}

void CompatibilityToolInstaller::discardPreparation()
{
    if (!m_remoteAddedByUs || isInstalled()) {
        return;
    }

    g_autoptr(GError) error = nullptr;
    g_autoptr(FlatpakInstallation) installation = flatpak_installation_new_user(nullptr, &error);
    if (!installation) {
        return;
    }

    const QByteArray remoteName = m_remoteName.toUtf8();
    if (!flatpak_installation_remove_remote(installation, remoteName.constData(), nullptr, &error)) {
        qWarning() << "Could not remove remote" << m_remoteName << "again:" << (error ? error->message : "unknown error");
        return;
    }
    m_remoteAddedByUs = false;
}

void CompatibilityToolInstaller::exitWith(int code)
{
    m_exitHandled = true;
    QCoreApplication::exit(code);
}

void CompatibilityToolInstaller::decline()
{
    discardPreparation();
    exitWith(1);
}

void CompatibilityToolInstaller::completeSuccess()
{
    runPostInstall();
    exitWith(0);
}

void CompatibilityToolInstaller::quitError()
{
    discardPreparation();
    exitWith(1);
}

void CompatibilityToolInstaller::windowClosed()
{
    if (m_exitHandled) {
        return;
    }
    cancel();
    discardPreparation();
    exitWith(1);
}
