#include "CmodEnvironment.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace CmodEnvironment {

QStringList lilyPondSearchDirectories(const QString& applicationDirectory)
{
    QStringList directories{
        applicationDirectory + QStringLiteral("/tools/lilypond/bin")
    };
#ifdef Q_OS_MACOS
    directories << QStringLiteral("/opt/homebrew/bin")
                << QStringLiteral("/usr/local/bin")
                << QStringLiteral("/opt/local/bin");
#endif
    return directories;
}

QProcessEnvironment withLilyPondOnPath(
    const QProcessEnvironment& environment,
    const QStringList& searchDirectories)
{
    const QString lilyPond = QStandardPaths::findExecutable(
        QStringLiteral("lilypond"), searchDirectories);
    if (lilyPond.isEmpty()) {
        return environment;
    }

    QProcessEnvironment configured = environment;
    const QString lilyPondDirectory = QFileInfo(lilyPond).absolutePath();
    const QString currentPath = configured.value(QStringLiteral("PATH"));
    configured.insert(
        QStringLiteral("PATH"),
        currentPath.isEmpty()
            ? lilyPondDirectory
            : lilyPondDirectory + QDir::listSeparator() + currentPath);
    return configured;
}

void configure(QProcess* cmod)
{
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QProcessEnvironment configured = withLilyPondOnPath(
        environment,
        lilyPondSearchDirectories(QCoreApplication::applicationDirPath()));
    if (configured.value(QStringLiteral("PATH"))
        != environment.value(QStringLiteral("PATH"))) {
        cmod->setProcessEnvironment(configured);
    }
}

} // namespace CmodEnvironment
