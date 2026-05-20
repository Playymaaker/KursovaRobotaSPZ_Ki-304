#include "SystemConfigurationGUI.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    SystemConfigurationGUI window;
    window.show();
    return app.exec();
}
