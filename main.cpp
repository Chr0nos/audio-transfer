#include "mainwindow.h"
#include <QApplication>
#include "qrec.h"
#include <QTextStream>

int comline(int argc,char *argv[],QTextStream *out) {
    QString rawLine;
    for (int p = 1;p < argc;p++) rawLine.append(argv[p]);
    *out << "Comline mode: " + rawLine;
    QRec *rec = new QRec();
    rec->listDevices();
    rec->setSourceId(0);
    rec->setTargetTcp("192.168.1.1",1042);
    rec->startRecAlt();
    return 0;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    /*
    QTextStream out(stdout);
    out << "argument count: " << argc;
    if (argc > 1) {
        out << "Starting...";
        return comline(argc,argv,&out);
    }
    */

    MainWindow w;
    w.show();
    return a.exec();
}
