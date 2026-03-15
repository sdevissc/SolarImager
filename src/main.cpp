#include <QApplication>
#include "MainWindow.h"
#include "Theme.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Theme::applyLightFusion(app);

    MainWindow win;
    win.show();

    return app.exec();
}
