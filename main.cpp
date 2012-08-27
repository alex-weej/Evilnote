#include <QtGui>
#include <QtCore>
#include <QtMultimedia>
#include <QAudioOutput>
#include "mainwindow.h"


int main(int argc, char*argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Audio Output Test");

    QString vstFileName;


    //vstFileName = ;
    //vstFileName = "/Library/Audio/Plug-Ins/VST/Massive.vst";
    //vstFileName = "/Library/Audio/Plug-Ins/VST/Omnisphere.vst";
    //vstFileName = "/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Sylenth1.vst";
    //vstFileName = "/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Omnisphere.vst";
    //vstFileName = "/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Omnisphere.vst";

    //vstFileName = "/Library/Audio/Plug-Ins/VST/ValhallaUberMod_x64.vst";

    En::VstModule vstModule1("/Library/Audio/Plug-Ins/VST/Massive.vst");
    //En::VstModule vstModule1("/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Sylenth1.vst");
    //En::VstModule vstModule2("/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/ArtsAcousticReverb.vst");
    En::VstModule vstModule2("/Library/Audio/Plug-Ins/VST/ValhallaUberMod_x64.vst");
    //En::VstModule vstModule2("/Library/Audio/Plug-Ins/VST/FabFilter Volcano 2.vst");
    //En::VstModule vstModule2("/Library/Audio/Plug-Ins/VST/FabFilter Timeless 2.vst");

    //En::VstModule vstModule2("/Library/Audio/Plug-Ins/VST/iZotope Stutter Edit.vst");

    En::VstNode* vstNode1 = new En::VstNode(&vstModule1);
    En::VstNode* vstNode2 = new En::VstNode(&vstModule2);


    QVector<En::VstNode*> vstNodes;
    vstNodes.push_back(vstNode1);
    vstNodes.push_back(vstNode2);

    //En::Host host(vstNodes);

    En::HostThread* hostThread = new En::HostThread(vstNodes);
    hostThread->start();

    MainWindow w1(vstNode1);
    w1.show();

    MainWindow w2(vstNode2);
    w2.show();


    QMainWindow other;
    other.setWindowTitle("Evilnote");
    other.show();

    a.exec();

    hostThread->quit();
    //qDebug() << "Waiting for host thread to finish up...";
    hostThread->wait();
    //qDebug() << "Host thread finished";

    delete vstNode1;
    delete vstNode2;

    return 0;
}
