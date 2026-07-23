// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

#pragma once

#include <QObject>
#include <QStringList>

#include <atomic>

typedef struct _GCancellable GCancellable;
class QThread;

class CompatibilityToolInstaller : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(QString appId READ appId CONSTANT)
    Q_PROPERTY(QString remoteName READ remoteName CONSTANT)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool sizesKnown READ sizesKnown NOTIFY sizesChanged)
    Q_PROPERTY(QString downloadSizeText READ downloadSizeText NOTIFY sizesChanged)
    Q_PROPERTY(QString installedSizeText READ installedSizeText NOTIFY sizesChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
    Q_PROPERTY(bool cancelled READ cancelled NOTIFY cancelledChanged)

public:
    static CompatibilityToolInstaller *load(const QString &name, QObject *parent = nullptr);
    static CompatibilityToolInstaller *findForMimeType(const QString &mimeType, QObject *parent = nullptr);

    ~CompatibilityToolInstaller() override;

    QString displayName() const;
    QString icon() const;
    QString appId() const;
    QString remoteName() const;
    int progress() const;
    bool sizesKnown() const;
    QString downloadSizeText() const;
    QString installedSizeText() const;
    QString errorText() const;
    bool cancelled() const;

    bool isInstalled() const;
    void runPostInstall() const;
    void takeOverMimeTypes() const;

    // Adds the remote if needed and resolves the remote ref.
    Q_INVOKABLE void prepare();
    Q_INVOKABLE void start();
    Q_INVOKABLE void cancel();
    // Removes the remote again if prepare() added it.
    Q_INVOKABLE void discardPreparation();

    // Exit paths for the standalone --install mode.
    Q_INVOKABLE void decline();
    Q_INVOKABLE void completeSuccess();
    Q_INVOKABLE void quitError();
    Q_INVOKABLE void windowClosed();

    void updateProgress(int percent);
    void setResolvedRef(const QString &ref, quint64 downloadSize, quint64 installedSize);
    void setErrorText(const QString &text);

Q_SIGNALS:
    void progressChanged();
    void sizesChanged();
    void errorTextChanged();
    void cancelledChanged();
    void finished(bool success);

private:
    explicit CompatibilityToolInstaller(QObject *parent = nullptr);
    static CompatibilityToolInstaller *loadConfigFile(const QString &path, QObject *parent);
    void exitWith(int code);

    QString m_appId;
    QString m_displayName;
    QString m_icon;
    QString m_remoteName;
    QString m_remoteUrl;
    QString m_postInstall;
    QStringList m_mimeTypes;
    bool m_takeOverMimeTypes = false;
    QThread *m_workThread = nullptr;
    GCancellable *m_cancellable = nullptr;
    QString m_ref;
    QString m_errorText;
    quint64 m_downloadSize = 0;
    quint64 m_installedSize = 0;
    bool m_sizesKnown = false;
    int m_progress = 0;
    bool m_exitHandled = false;
    std::atomic<bool> m_remoteAddedByUs = false;
    std::atomic<bool> m_cancelled = false;
};
