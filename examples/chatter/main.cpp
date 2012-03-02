#include <QtGui/QApplication>
#include "qmlapplicationviewer.h"
#include <connectionmanager.h>

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QScopedPointer<QApplication> app(createApplication(argc, argv));

    ConnectionManager manager;
    QmlApplicationViewer viewer;
    viewer.setOrientation(QmlApplicationViewer::ScreenOrientationAuto);
    viewer.rootContext()->setContextProperty("manager", &manager);
    viewer.setMainQmlFile(QLatin1String("qml/chatter/main.qml"));
    viewer.showExpanded();

    return app->exec();
}
