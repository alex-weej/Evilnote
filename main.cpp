#include "core.h"
#include "nodegroup.h"
#include "mixernode.h"
#include "hostthread.h"
#include "mainwindow.h"
#include <QtGui>

void testMidi();

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Evilnote");

    En::Core core;

    core.scanVstDirs();

    En::NodeGroup* rootGroup = new En::NodeGroup();

    En::MixerNode* mixerNode = new En::MixerNode();
    mixerNode->setPosition(QPointF(0, 0));
    rootGroup->addNode(mixerNode);
    rootGroup->setOutputNode(mixerNode);

    En::HostThread* hostThread = new En::HostThread(rootGroup);
    hostThread->start(QThread::TimeCriticalPriority);

    En::MainWindow main(rootGroup);
    hostThread->connect(hostThread, SIGNAL(utilisation(float)), &main, SLOT(utilisation(float)));
    main.show();

    app.exec();

    hostThread->quit();
    //qDebug() << "Waiting for host thread to finish up...";
    hostThread->wait();
    //qDebug() << "Host thread finished";

    delete rootGroup;

    return 0;
}
