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
    //vstFileName = "/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Sylenth1.vst";
    //vstFileName = "/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Omnisphere.vst";
    //vstFileName = "/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Omnisphere.vst";

    //vstFileName = "/Library/Audio/Plug-Ins/VST/ValhallaUberMod_x64.vst";

    En::VstModule massiveModule("/Library/Audio/Plug-Ins/VST/Massive.vst");
    //En::VstModule vstModule1("/Library/Audio/Plug-Ins/VST/Omnisphere.vst");
    //En::VstModule vstModule1("/Library/Audio/Plug-Ins/VST/CurveCM.vst");
    //En::VstModule vstModule1("/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Sylenth1.vst");
    //En::VstModule vstModule2("/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/ArtsAcousticReverb.vst");
    En::VstModule ubermodModule("/Library/Audio/Plug-Ins/VST/ValhallaUberMod_x64.vst");
    //En::VstModule vstModule2("/Library/Audio/Plug-Ins/VST/FabFilter Volcano 2.vst");
    //En::VstModule vstModule2("/Library/Audio/Plug-Ins/VST/FabFilter Timeless 2.vst");

    En::VstModule stutterModule("/Library/Audio/Plug-Ins/VST/iZotope Stutter Edit.vst");
    //En::VstModule ubermodModule("/Library/Audio/Plug-Ins/VST/ValhallaUberMod_x64.vst");

    En::NodeGroup* rootGroup = new En::NodeGroup();
    En::VstNode* massive1 = new En::VstNode(&massiveModule, rootGroup);
    En::VstNode* massive2 = new En::VstNode(&massiveModule, rootGroup);
    En::VstNode* ubermod1 = new En::VstNode(&ubermodModule, rootGroup);
    En::VstNode* stutter1 = new En::VstNode(&stutterModule, rootGroup);
    //En::VstNode* krreverb1 = new En::VstNode(&reverbModule);


    ubermod1->setInput(massive1);
    stutter1->setInput(massive1);


    En::MixerNode* mixerNode = new En::MixerNode(rootGroup);
    mixerNode->addInput(massive2);
    mixerNode->addInput(ubermod1);
    //mixerNode->addInput(massive1);
    mixerNode->addInput(stutter1);


    En::HostThread* hostThread = new En::HostThread(mixerNode);
    hostThread->start();

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
