#include "mainwindow.h"
#include "comline.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTextStream out(stdout);
    if (argc > 1) {
        out << "Starting..." << endl;
        Comline* com = new Comline();
        com->start();
        out << "Done." << endl;
        return a.exec();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
