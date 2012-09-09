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

    QPointF pos;
    QPointF posMove(0, 50);

    En::NodeGroup* rootGroup = new En::NodeGroup();

    En::MixerNode* mixerNode = new En::MixerNode(rootGroup);
    mixerNode->setPosition(pos += posMove);


    En::VstNode* ubermod1 = new En::VstNode(core.vstModule("ValhallaUberMod_x64"), rootGroup);
    ubermod1->setPosition(pos += posMove);
    mixerNode->addInput(ubermod1);

    En::VstNode* vstNode = new En::VstNode(core.vstModule("Massive"), rootGroup);
    vstNode->setPosition(pos += posMove);
    ubermod1->setInput(vstNode);

    vstNode = new En::VstNode(core.vstModule("FM8"), rootGroup);
    vstNode->setPosition(pos += posMove);
    mixerNode->addInput(vstNode);

    vstNode = new En::VstNode(core.vstModule("Absynth 5 Stereo"), rootGroup);
    vstNode->setPosition(pos += posMove);
    mixerNode->addInput(vstNode);

//    En::VstNode* massive2 = new En::VstNode(&massiveModule, rootGroup);
    En::VstNode* stutter1 = new En::VstNode(core.vstModule("iZotope Stutter Edit"), rootGroup);
    stutter1->setPosition(pos += posMove);

    En::VstNode* omnisphere1 = new En::VstNode(core.vstModule("Omnisphere"), rootGroup);
    omnisphere1->setPosition(pos += posMove);
    mixerNode->addInput(omnisphere1);

    En::VstNode* curveCM1 = new En::VstNode(core.vstModule("CurveCM"), rootGroup);
    curveCM1->setPosition(pos += posMove);
    mixerNode->addInput(curveCM1);

    //    vstNode = new En::VstNode(core.vstModule("Sylenth1"), rootGroup);
    //    vstNode->setPosition(pos += posMove);
    //    mixerNode->addInput(vstNode);

    vstNode = new En::VstNode(core.vstModule("Padshop"), rootGroup);
    vstNode->setPosition(pos += posMove);
    mixerNode->addInput(vstNode);


    const int stressTestInstances = 0;

    for (int i = 0; i < stressTestInstances; ++i) {
        En::Node* node = new En::VstNode(core.vstModule("Massive"), rootGroup);
        node->setPosition(pos += posMove);
        mixerNode->addInput(node);
    }

    //En::VstNode* krreverb1 = new En::VstNode(&reverbModule);


//    ubermod1->setInput(massive1);
//    stutter1->setInput(massive1);


//    mixerNode->addInput(ubermod1);
//    //mixerNode->addInput(massive1);
//    //mixerNode->addInput(stutter1);
//    mixerNode->addInput(omnisphere1);
//    mixerNode->addInput(curveCM1);
//    //mixerNode->addInput(sylenth1);
//    mixerNode->addInput(one1);


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
