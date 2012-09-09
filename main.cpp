#include <QtGui>
#include <QtCore>
#include <QtMultimedia>
#include <QAudioOutput>
#include "mainwindow.h"


int main(int argc, char*argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Evilnote");


    En::Core core;

    core.scanVstDirs();

    En::NodeGroup* rootGroup = new En::NodeGroup();

    En::MixerNode* mixerNode = new En::MixerNode();
    mixerNode->setPosition(QPointF(0, 0));
    rootGroup->addNode(mixerNode);


    En::VstNode* vstNode = new En::VstNode(core.vstModule("Massive"));
    vstNode->setPosition(QPointF(0, -100));
    rootGroup->addNode(vstNode);
    mixerNode->addInput(vstNode);

    En::HostThread* hostThread = new En::HostThread(mixerNode);
    hostThread->start(QThread::TimeCriticalPriority);

    En::MainWindow main(rootGroup);
    hostThread->connect(hostThread, SIGNAL(utilisation(float)), &main, SLOT(utilisation(float)));
    main.show();

    a.exec();

    hostThread->quit();
    //qDebug() << "Waiting for host thread to finish up...";
    hostThread->wait();
    //qDebug() << "Host thread finished";

    delete rootGroup;

    return 0;
}
