#include "mainwindow.h"
#include "comline.h"
#include "circularbuffer.h"
#include <QApplication>
#include <QDebug>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTextStream out(stdout);
    if (argc > 1) {
        QString args;
        QStringList argList;
        for (int p = 0;p <= argc;p++) {
            args += argv[p];
            argList << argv[p];
        }
        argList.removeAt(0);

        if (argList.contains("--test-circular")) {
            CircularBuffer::runTest();
        }
        out << "Starting..." << endl;
        Comline* com = new Comline(&args);
        com->start();
        out << "Done." << endl;
        return a.exec();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
