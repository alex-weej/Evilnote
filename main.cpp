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


    //vstFileName = "/Library/Audio/Plug-Ins/VST/Sylenth1.vst";
    vstFileName = "/Library/Audio/Plug-Ins/VST/Massive.vst";
    //vstFileName = "/Library/Audio/Plug-Ins/VST/Omnisphere.vst";
    //vstFileName = "/Users/alex/Library/Audio/Plug-Ins/VST/PluginsBridgedFor64BitVSTHosts/Sylenth1.vst";

    En::VstModule vstModule(vstFileName);

    En::VstNode* vstNode = new En::VstNode(&vstModule);

    En::Host host(vstNode);

    MainWindow w(vstNode);
    w.show();

    a.exec();

    return 0;
}
