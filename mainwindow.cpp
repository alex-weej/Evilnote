#include "mainwindow.h"

#include "nodegraph.h"

namespace En
{

MainWindow::MainWindow(NodeGroup* group, QWidget* parent)
    : QMainWindow(parent)
    , m_group(group)
{
    setWindowTitle("Evilnote");

    QWidget* widget = new QWidget();

    QVBoxLayout* mainLayout = new QVBoxLayout(widget);
    setCentralWidget(widget);

    //m_utilisationMeter = new QProgressBar();
    //m_utilisationMeter->show();
    //m_utilisationMeter->setRange(0, INT_MAX);

    //mainLayout->addWidget(m_utilisationMeter);
    //setCentralWidget(m_utilisationMeter);

    NodeGraphEditor* graph = new NodeGraphEditor(group);
    mainLayout->addWidget(graph);

    m_utilisationText = new QLabel();
    statusBar()->addPermanentWidget(m_utilisationText);

}

void MainWindow::utilisation(float utilisationAmount) {
    //m_utilisationMeter->setValue(utilisationAmount * INT_MAX);
    m_utilisationText->setText(tr("CPU: %1%").arg(QString::number(utilisationAmount * 100, 'f', 0)));
}

}
