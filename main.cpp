#include "mainwindow.h"
#ifdef COMLINE
#include "comline.h"
#endif
#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
#ifdef COMLINE
    //argc++;
    if (argc > 1) {
        QCoreApplication a(argc,argv);
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
        return a.exec();
    }
#endif
    QApplication a(argc,argv);
    MainWindow w;
    w.show();
    return a.exec();
}
