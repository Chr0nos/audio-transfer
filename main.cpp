#ifdef COMLINE
#include <QCoreApplication>
#include "comline.h"
#endif
#ifdef GUI
#include <QApplication>
#include "mainwindow.h"
#endif

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
            //the start is triggered by the constructor
            Comline* com = new Comline(&argList);
            (void) com;
            return a.exec();
        }
        return a.exec();
    }
#endif
#ifdef GUI
    QApplication a(argc,argv);
    MainWindow w;
    w.show();
    return a.exec();
#endif
    return 0;
}
