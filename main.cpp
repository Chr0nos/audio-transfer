#include "mainwindow.h"
#ifdef COMLINE
#include "comline.h"
#endif
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifdef COMLINE
    //argc++;
    if (argc > 1) {
        QStringList argList;
        for (int p = 0;p <= argc;p++) {
            argList << argv[p];
        }
        argList.removeAt(0);

        //i use this to debug the app, ignore it
        //argList.clear();
        //argList << "--server" << "--server-type" << "udp";

        if (!argList.isEmpty()) {
            Comline* com = new Comline(&argList);
            (void) com;
            //com->start();
            return a.exec();
        }
    }
#endif
    MainWindow w;
    w.show();
    return a.exec();
}
