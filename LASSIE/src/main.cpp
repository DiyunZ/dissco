#include "inst.hpp"

#include <QApplication>
#include <QFile>
#ifdef DISSCO_ENABLE_UI_LAYOUT_TESTS
#include <QDebug>
#include <QLabel>
#include <QLayout>
#include "windows/EnvelopeLibraryWindow.hpp"
#endif
#include "widgets/ComboBoxWheelGuard.hpp"
#include "widgets/TextOverflowDisplayPolicy.hpp"
#include "windows/MainWindow.hpp"

void registerAllFunctions();

int main(int argc, char *argv[])
{
    // QSettings (used here and in the Updater) needs stable org/app
    // metadata; otherwise its on-disk location is platform-default and
    // varies between binaries. Set before QApplication is constructed.
    QCoreApplication::setOrganizationName("DISSCO");
    QCoreApplication::setOrganizationDomain("dissco.illinois.edu");
    QCoreApplication::setApplicationName("LASSIE");

    QApplication a(argc, argv);
    ComboBoxWheelGuard comboBoxWheelGuard(a);
    TextOverflowDisplayPolicy textOverflowDisplayPolicy(a);

#ifdef DISSCO_ENABLE_UI_LAYOUT_TESTS
    if (qEnvironmentVariableIsSet("DISSCO_TEST_ENVELOPE_LAYOUT")) {
        EnvelopeLibraryWindow window;
        window.ensurePolished();
        window.show();
        for (int i = 0; i < 3; ++i) {
            window.centralWidget()->layout()->activate();
            QApplication::processEvents();
        }

        QLabel* legend = window.findChild<QLabel*>("envelopeLegend");
        if (!legend) {
            qCritical() << "Envelope legend was not found";
            return 1;
        }

        const QRect legendInCentral(
            legend->mapTo(window.centralWidget(), QPoint()), legend->size());
        const int requiredHeight = legend->heightForWidth(legend->width());
        const bool fullyVisible =
            window.centralWidget()->rect().contains(legendInCentral)
            && legend->visibleRegion().contains(legend->rect())
            && legend->height() >= requiredHeight;

        if (!fullyVisible) {
            qCritical() << "Envelope legend is clipped:"
                        << "actual" << legend->size()
                        << "required height" << requiredHeight
                        << "visible region" << legend->visibleRegion();
        }

        return fullyVisible ? 0 : 1;
    }
#endif

    registerAllFunctions();
    Inst *m = Inst::instance();
    MainWindow *w = new MainWindow(m);
    w->show();
    return a.exec();
}
