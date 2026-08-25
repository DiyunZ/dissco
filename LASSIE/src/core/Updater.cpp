#include "Updater.hpp"
#include "ReleaseAssetSelector.hpp"

#include <QApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>

namespace {

QByteArray userAgent() {
    return QStringLiteral("LASSIE/%1 (+https://github.com/%2)")
        .arg(QStringLiteral(DISSCO_VERSION_STR), Updater::gitHubRepoSlug())
        .toUtf8();
}

void applyGitHubHeaders(QNetworkRequest &req) {
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    req.setRawHeader("User-Agent", userAgent());
}

// Strip an optional leading 'v' and parse. QVersionNumber::fromString
// stops at the first non-numeric character, so pre-release/build
// suffixes ("-rc1", "-test", "+abc") fall away naturally.
QVersionNumber parseTag(const QString &tag) {
    QString trimmed = tag.trimmed();
    if (trimmed.startsWith(QLatin1Char('v')) || trimmed.startsWith(QLatin1Char('V'))) {
        trimmed.remove(0, 1);
    }
    return QVersionNumber::fromString(trimmed);
}

}  // namespace

QVersionNumber Updater::currentVersion() {
    return QVersionNumber(DISSCO_VER_MAJOR, DISSCO_VER_MINOR, DISSCO_VER_PATCH);
}

QString Updater::currentCommit() {
    return QStringLiteral(GIT_COMMIT_SHA);
}

QString Updater::gitHubRepoSlug() {
    return QStringLiteral(DISSCO_GITHUB_REPO);
}

Updater::Updater(QWidget *uiParent)
    : QObject(uiParent),
      net_(new QNetworkAccessManager(this)),
      uiParent_(uiParent) {}

Updater::~Updater() = default;

bool Updater::shouldAutoCheckNow() {
    const QSettings s;
    if (!s.value(kAutoCheckKey, false).toBool()) {
        return false;
    }
    const QDateTime last = s.value(kLastCheckUtcKey).toDateTime();
    return !last.isValid() ||
           last.secsTo(QDateTime::currentDateTimeUtc()) >= 24 * 60 * 60;
}

void Updater::checkForUpdates(Trigger trigger) {
    trigger_ = trigger;
    QSettings().setValue(kLastCheckUtcKey, QDateTime::currentDateTimeUtc());

    const QUrl url(QStringLiteral("https://api.github.com/repos/%1/releases/latest")
                       .arg(gitHubRepoSlug()));
    QNetworkRequest req(url);
    applyGitHubHeaders(req);

    QNetworkReply *reply = net_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        onReleaseReply(reply);
    });
}

void Updater::onReleaseReply(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        // 404 from /releases/latest just means no published release exists —
        // common during early-stage projects. Treat as "up to date".
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 404) {
            if (trigger_ == Trigger::Manual) {
                QMessageBox::information(uiParent_, tr("Check for Updates"),
                    tr("You're up to date.\n\nInstalled: %1\n\n"
                       "No published releases on GitHub yet.")
                        .arg(currentVersion().toString()));
            }
            return;
        }
        if (trigger_ == Trigger::Manual) {
            reportError(tr("Couldn't reach GitHub: %1").arg(reply->errorString()));
        }
        return;
    }

    const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();

    ReleaseInfo info;
    info.tagName = obj.value("tag_name").toString();
    info.version = parseTag(info.tagName);
    info.releaseNotes = obj.value("body").toString();
    info.htmlUrl = QUrl(obj.value("html_url").toString());

    if (info.version.isNull()) {
        if (trigger_ == Trigger::Manual) {
            reportError(tr("Couldn't parse latest release tag '%1'.").arg(info.tagName));
        }
        return;
    }

    if (info.version <= currentVersion()) {
        if (trigger_ == Trigger::Manual) {
            QMessageBox::information(uiParent_, tr("Check for Updates"),
                tr("You're up to date.\n\nInstalled: %1\nLatest released: %2")
                    .arg(currentVersion().toString(), info.version.toString()));
        }
        return;
    }

    // Honour a prior "skip this version" choice on background checks.
    if (trigger_ == Trigger::Auto &&
        QSettings().value(kSkipVersionKey).toString() == info.version.toString()) {
        return;
    }

    // Releases can also contain CMOD-only packages. Select only the DISSCO
    // installer for this platform so the GUI updater never downloads CMOD.
    const auto selectedAsset = DISSCO::ReleaseAssets::selectDisscoAsset(
        obj.value("assets").toArray(),
        DISSCO::ReleaseAssets::currentPlatform(),
        QSysInfo::currentCpuArchitecture());
    if (selectedAsset.has_value()) {
        const QJsonObject &asset = selectedAsset.value();
        info.assetName = asset.value("name").toString();
        info.assetUrl = QUrl(asset.value("browser_download_url").toString());
        info.assetSize = static_cast<qint64>(asset.value("size").toDouble());
    }

    if (!info.assetUrl.isValid()) {
        QMessageBox::information(uiParent_, tr("Check for Updates"),
            tr("Version %1 is available, but no installer was published for "
               "this platform. Opening the release page.")
                .arg(info.version.toString()));
        QDesktopServices::openUrl(info.htmlUrl);
        return;
    }

    presentReleaseDialog(info);
}

void Updater::presentReleaseDialog(const ReleaseInfo &info) {
    QMessageBox box(uiParent_);
    box.setWindowTitle(tr("Update Available"));
    box.setIcon(QMessageBox::Information);
    box.setText(tr("<b>LASSIE %1 is available.</b><br>You have %2.")
                    .arg(info.version.toString(), currentVersion().toString()));
    if (!info.releaseNotes.isEmpty()) {
        QString notes = info.releaseNotes;
        if (notes.size() > 1500) notes = notes.left(1500) + QStringLiteral("…");
        box.setDetailedText(notes);
    }

    QPushButton *download     = box.addButton(tr("Download && Install"), QMessageBox::AcceptRole);
    QAbstractButton *openPage = box.addButton(tr("Open Release Page"), QMessageBox::ActionRole);
    QAbstractButton *skip     = box.addButton(tr("Skip This Version"), QMessageBox::DestructiveRole);
    box.addButton(tr("Remind Me Later"), QMessageBox::RejectRole);
    box.setDefaultButton(download);

    box.exec();
    QAbstractButton *clicked = box.clickedButton();
    if (clicked == download) {
        downloadAndInstall(info);
    } else if (clicked == openPage) {
        QDesktopServices::openUrl(info.htmlUrl);
    } else if (clicked == skip) {
        QSettings().setValue(kSkipVersionKey, info.version.toString());
    }
}

void Updater::downloadAndInstall(const ReleaseInfo &info) {
    const QString updatesDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                                + QStringLiteral("/updates");
    QDir().mkpath(updatesDir);
    const QString localPath = updatesDir + QLatin1Char('/') + info.assetName;

    // Reuse a prior download if it's the right size.
    if (info.assetSize > 0 && QFileInfo(localPath).size() == info.assetSize) {
        launchInstaller(localPath);
        return;
    }

    QFile *file = new QFile(localPath);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        reportError(tr("Couldn't write to %1: %2").arg(localPath, file->errorString()));
        delete file;
        return;
    }

    QNetworkRequest req(info.assetUrl);
    req.setRawHeader("User-Agent", userAgent());
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = net_->get(req);

    QProgressDialog *progress = new QProgressDialog(
        tr("Downloading %1…").arg(info.assetName),
        tr("Cancel"), 0, 100, uiParent_);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setAutoClose(true);

    connect(reply, &QNetworkReply::downloadProgress, progress,
            [progress](qint64 received, qint64 total) {
        if (total > 0) {
            progress->setMaximum(100);
            progress->setValue(static_cast<int>((received * 100) / total));
        } else {
            progress->setMaximum(0);  // indeterminate
        }
    });
    connect(reply, &QNetworkReply::readyRead, file, [reply, file] {
        file->write(reply->readAll());
    });
    connect(progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, file, progress, info, localPath] {
        file->write(reply->readAll());
        file->close();
        progress->close();
        progress->deleteLater();

        const auto err = reply->error();
        const qint64 onDisk = QFileInfo(localPath).size();
        reply->deleteLater();
        delete file;

        if (err == QNetworkReply::OperationCanceledError) {
            QFile::remove(localPath);
            return;
        }
        if (err != QNetworkReply::NoError) {
            QFile::remove(localPath);
            reportError(tr("Download failed: %1").arg(reply->errorString()));
            return;
        }
        if (info.assetSize > 0 && onDisk != info.assetSize) {
            QFile::remove(localPath);
            reportError(tr("Download size mismatch (got %1 bytes, expected %2).")
                            .arg(onDisk).arg(info.assetSize));
            return;
        }
        launchInstaller(localPath);
    });
}

void Updater::launchInstaller(const QString &localPath) {
#if defined(Q_OS_MACOS)
    // Mount the DMG in Finder; user drags lassie.app into Applications.
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(localPath))) {
        reportError(tr("Couldn't open downloaded installer at %1").arg(localPath));
        return;
    }
    QMessageBox::information(uiParent_, tr("Install LASSIE Update"),
        tr("The disk image has been mounted. Drag <b>LASSIE</b> into your "
           "Applications folder, then relaunch."));

#elif defined(Q_OS_WIN)
    // Launch the NSIS installer; it'll elevate, uninstall the current
    // version, and install the new one. We quit so it can replace lassie.exe.
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(localPath))) {
        reportError(tr("Couldn't launch installer at %1").arg(localPath));
        return;
    }
    QMessageBox::information(uiParent_, tr("Install LASSIE Update"),
        tr("The installer has been launched. LASSIE will now quit so the "
           "installer can replace it."));
    QApplication::quit();

#else
    // Linux (and unknown OSes): no automatic AppImage swap — too many
    // failure modes (PID race, alternate install locations, mounted
    // read-only filesystems). Show the file in a file manager and let
    // the user move it over their existing AppImage.
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(localPath).absolutePath()));
    QMessageBox::information(uiParent_, tr("Update Downloaded"),
        tr("Saved to:\n\n%1\n\nReplace your existing AppImage with this "
           "file and relaunch.").arg(localPath));
#endif
}

void Updater::reportError(const QString &message) {
    if (trigger_ == Trigger::Auto) return;  // silent on background checks
    QMessageBox::warning(uiParent_, tr("Check for Updates"), message);
}
