#pragma once

#include <QtGui>

namespace En
{

class NodeGroup;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    // don't use this ATM - native style on Mac makes the framerate cap for some reason!
    //QProgressBar* m_utilisationMeter;
    QLabel* m_utilisationText;
    NodeGroup* m_group;

public:

    MainWindow(NodeGroup* group, QWidget* parent=0);

public slots:

    void utilisation(float utilisationAmount);

};

}
