#ifndef PROJECT_CLIPBOARD_HPP
#define PROJECT_CLIPBOARD_HPP

#include "project_struct.hpp"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QPointer>
#include <utility>

// Objects may refer to other project objects by name or library number, so
// these snapshots are deliberately local to their originating open project.
// QClipboard owns the snapshot: copying text replaces it, just like any copy.
namespace ProjectClipboard {

namespace Detail {
template<typename T>
class Snapshot final : public QMimeData
{
public:
    Snapshot(Project* source, T value, const QString& description)
        : project(source), value(std::move(value))
    {
        setData("application/x-dissco-project-object", QByteArray("1"));
        setText(description);
    }

    QPointer<Project> project;
    T value;
};
} // namespace Detail

template<typename T>
void copy(Project* project, T value, const QString& description)
{
    if (project) {
        QGuiApplication::clipboard()->setMimeData(
            new Detail::Snapshot<T>(project, std::move(value), description));
    }
}

template<typename T>
const T* get(Project* project)
{
    const auto* snapshot = dynamic_cast<const Detail::Snapshot<T>*>(
        QGuiApplication::clipboard()->mimeData());
    return project && snapshot && snapshot->project == project
        ? &snapshot->value : nullptr;
}

} // namespace ProjectClipboard

#endif
