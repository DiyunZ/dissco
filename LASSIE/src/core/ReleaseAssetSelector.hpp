#ifndef RELEASE_ASSET_SELECTOR_HPP
#define RELEASE_ASSET_SELECTOR_HPP

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <optional>

namespace DISSCO::ReleaseAssets {

enum class Platform {
    Unsupported,
    Linux,
    MacOS,
    Windows
};

inline Platform currentPlatform()
{
#if defined(Q_OS_MACOS)
    return Platform::MacOS;
#elif defined(Q_OS_WIN)
    return Platform::Windows;
#elif defined(Q_OS_LINUX)
    return Platform::Linux;
#else
    return Platform::Unsupported;
#endif
}

inline QString platformSuffix(Platform platform)
{
    switch (platform) {
    case Platform::Linux:
        return QStringLiteral(".AppImage");
    case Platform::MacOS:
        return QStringLiteral("-Darwin.dmg");
    case Platform::Windows:
        return QStringLiteral("-Windows.exe");
    case Platform::Unsupported:
        return QString();
    }
    return QString();
}

inline QString packageArchitecture(QString architecture)
{
    architecture = architecture.trimmed().toLower();
    if (architecture == QStringLiteral("arm64")) {
        return QStringLiteral("aarch64");
    }
    if (architecture == QStringLiteral("amd64") ||
        architecture == QStringLiteral("x64")) {
        return QStringLiteral("x86_64");
    }
    return architecture;
}

inline std::optional<QJsonObject> selectDisscoAsset(
    const QJsonArray &assets,
    Platform platform,
    const QString &architecture)
{
    const QString suffix = platformSuffix(platform);
    if (suffix.isEmpty()) {
        return std::nullopt;
    }

    const QString normalizedArchitecture = packageArchitecture(architecture);
    std::optional<QJsonObject> firstMatch;
    for (const QJsonValue &value : assets) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        if (!name.startsWith(QStringLiteral("DISSCO-")) ||
            !name.endsWith(suffix)) {
            continue;
        }

        if (!firstMatch.has_value()) {
            firstMatch = asset;
        }
        if (platform == Platform::Linux &&
            !normalizedArchitecture.isEmpty() &&
            name.endsWith(QStringLiteral("-Linux-%1.AppImage")
                              .arg(normalizedArchitecture))) {
            return asset;
        }
    }

    if (platform == Platform::Linux && !normalizedArchitecture.isEmpty()) {
        return std::nullopt;
    }
    return firstMatch;
}

}  // namespace DISSCO::ReleaseAssets

#endif  // RELEASE_ASSET_SELECTOR_HPP
