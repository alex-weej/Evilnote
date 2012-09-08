#include <QtGui>
#include <QtCore>
#include <QtMultimedia>
#include <QAudioOutput>
#include "mainwindow.h"


int main(int argc, char*argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Evilnote");

    QString vstFileName;


    //vstFileName = "/Library/Audio/Plug-Ins/VST/Massive.vst";
    //vstFileName = "/Library/Audio/Plug-Ins/VST/Omnisphere.vst";
    En::VstModule omnisphereModule("/Library/Audio/Plug-Ins/VST/Omnisphere.vst");
    //vstFileName = "/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Sylenth1.vst";
    //vstFileName = "/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Omnisphere.vst";
    //vstFileName = "/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Omnisphere.vst";

    //vstFileName = "/Library/Audio/Plug-Ins/VST/ValhallaUberMod_x64.vst";

    En::VstModule massiveModule("/Library/Audio/Plug-Ins/VST/Massive.vst");
    //En::VstModule massiveModule("/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Massive.vst");
    //En::VstModule vstModule1("/Library/Audio/Plug-Ins/VST/Omnisphere.vst");
    En::VstModule curveCMModule("/Library/Audio/Plug-Ins/VST/CurveCM.vst");
    //En::VstModule sylenthModule("/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Sylenth1.vst");
    //En::VstModule vstModule2("/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/ArtsAcousticReverb.vst");
    En::VstModule ubermodModule("/Library/Audio/Plug-Ins/VST/ValhallaUberMod_x64.vst");
    //En::VstModule vstModule2("/Library/Audio/Plug-Ins/VST/FabFilter Volcano 2.vst");
    //En::VstModule vstModule2("/Library/Audio/Plug-Ins/VST/FabFilter Timeless 2.vst");

    En::VstModule stutterModule("/Library/Audio/Plug-Ins/VST/iZotope Stutter Edit.vst");
    En::VstModule oneModule("/Library/Audio/Plug-Ins/VST3/Steinberg/Padshop.vst3");
    //En::VstModule ubermodModule("/Library/Audio/Plug-Ins/VST/ValhallaUberMod_x64.vst");

    En::NodeGroup* rootGroup = new En::NodeGroup();

    En::MixerNode* mixerNode = new En::MixerNode(rootGroup);

    En::VstNode* ubermod1 = new En::VstNode(&ubermodModule, rootGroup);
    mixerNode->addInput(ubermod1);

    En::VstNode* massive1 = new En::VstNode(&massiveModule, rootGroup);
    ubermod1->setInput(massive1);

//    En::VstNode* massive2 = new En::VstNode(&massiveModule, rootGroup);
    En::VstNode* stutter1 = new En::VstNode(&stutterModule, rootGroup);
    En::VstNode* omnisphere1 = new En::VstNode(&omnisphereModule, rootGroup);
    mixerNode->addInput(omnisphere1);

    En::VstNode* curveCM1 = new En::VstNode(&curveCMModule, rootGroup);
    mixerNode->addInput(curveCM1);

    //En::VstNode* sylenth1 = new En::VstNode(&sylenthModule, rootGroup);
    En::VstNode* one1 = new En::VstNode(&oneModule, rootGroup);


    const int stressTestInstances = 0;

    for (int i = 0; i < stressTestInstances; ++i) {
        En::Node* node = new En::VstNode(&massiveModule, rootGroup);
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
