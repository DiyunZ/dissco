#ifndef CMOD_ENVIRONMENT_HPP
#define CMOD_ENVIRONMENT_HPP

#include <QProcessEnvironment>
#include <QStringList>

class QProcess;

namespace CmodEnvironment {

QStringList lilyPondSearchDirectories(const QString& applicationDirectory);
QProcessEnvironment withLilyPondOnPath(
    const QProcessEnvironment& environment,
    const QStringList& searchDirectories);
void configure(QProcess* cmod);

} // namespace CmodEnvironment

#endif // CMOD_ENVIRONMENT_HPP
