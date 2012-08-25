#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(En::VstNode* vstNode, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_vstNode(vstNode),
    m_note(60)
{
    ui->setupUi(this);

    //interface->show();

//    En::VstInterfaceWidget* interface = new En::VstInterfaceWidget(vstNode, this);
//    ui->vstPlaceholder->addWidget(interface);
//    QLineEdit* le = new QLineEdit;
//    ui->vstPlaceholder->addWidget(le);

    QVBoxLayout* layout = new QVBoxLayout(ui->vstPlaceholder);
    En::VstEditorWidget* interface = new En::VstEditorWidget(vstNode, ui->vstPlaceholder);
    //QLineEdit* interface = new QLineEdit;
    layout->addWidget(interface);


    m_funkyTownPos = 0;
    m_funkyTown << 0 << 0 << -2 << 0 << -5 << -5 << 0 << 5 << 4 << 0;


    //En::VstInterfaceWidget* interface = new En::VstInterfaceWidget(vstNode, this);
    //setCentralWidget(interface);

    //ui->verticalLayout->addWidget(interface);
    //layout.addWidget(interface);
    //setCentralWidget(interface);

    //QPushButton* interface = new QPushButton("HELLO");

    //centralWidget()->layout()->addWidget(interface);

    //interface->resize(QSize(500, 500));
    //this->setCentralWidget(interface);

    //setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    //ui->vstPlaceholder->setFixedSize(interface->sizeHint());

}

MainWindow::~MainWindow()
{
    delete ui;
}


static void sendNote(En::VstNode* vstNode, unsigned char noteValue, unsigned char velocity) {
    VstMidiEvent event = {kVstMidiType, sizeof (VstMidiEvent), 0, 0, 0, 0, {0x90, noteValue, velocity, 0}, 0, 0, 0, 0};
    VstMidiEvent* pEvent = new VstMidiEvent;
    memcpy(pEvent, &event, sizeof(event));
    vstNode->queueEvent((VstEvent*)pEvent);
}

void MainWindow::on_testNoteButton_pressed()
{
    int note = 0;

    // FUNKYTOWN
    if (QApplication::keyboardModifiers() == Qt::ShiftModifier) {

        note = m_funkyTown[m_funkyTownPos] + 72;
        m_funkyTownPos = (m_funkyTownPos + 1) % m_funkyTown.size();

    } else {

        note = m_note++;
        if (m_note > 72) {
            m_note = 60;
        }
    }


    sendNote(m_vstNode, note, 100);
    m_lastNote = note;

}

void MainWindow::on_testNoteButton_released()
{
    sendNote(m_vstNode, m_lastNote, 0);
}

static int keyEventToNote(QKeyEvent* event) {

    int note = -1;

    switch (event->key()) {

    case Qt::Key_Z:         note = 0;   break;
    case Qt::Key_S:         note = 1;   break;
    case Qt::Key_X:         note = 2;   break;
    case Qt::Key_D:         note = 3;   break;
    case Qt::Key_C:         note = 4;   break;
    case Qt::Key_V:         note = 5;   break;
    case Qt::Key_G:         note = 6;   break;
    case Qt::Key_B:         note = 7;   break;
    case Qt::Key_H:         note = 8;   break;
    case Qt::Key_N:         note = 9;   break;
    case Qt::Key_J:         note = 10;  break;
    case Qt::Key_M:         note = 11;  break;
    case Qt::Key_Comma:     note = 12;  break;
    case Qt::Key_L:         note = 13;  break;
    case Qt::Key_Period:    note = 14;  break;

    case Qt::Key_Q:         note = 12;  break;
    case Qt::Key_2:         note = 13;  break;
    case Qt::Key_W:         note = 14;  break;
    case Qt::Key_3:         note = 15;  break;
    case Qt::Key_E:         note = 16;  break;
    case Qt::Key_R:         note = 17;  break;
    case Qt::Key_5:         note = 18;  break;
    case Qt::Key_T:         note = 19;  break;
    case Qt::Key_6:         note = 20;  break;
    case Qt::Key_Y:         note = 21;  break;
    case Qt::Key_7:         note = 22;  break;
    case Qt::Key_U:         note = 23;  break;
    case Qt::Key_I:         note = 24;  break;
    case Qt::Key_9:         note = 25;  break;
    case Qt::Key_O:         note = 26;  break;
    case Qt::Key_0:         note = 27;  break;
    case Qt::Key_P:         note = 28;  break;

    default: return -1;

    }

    int transpose = 48;

    return note + transpose;
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        return;
    }

    int note = keyEventToNote(event);
    if (note == -1) {
        return QMainWindow::keyPressEvent(event);
    }

    sendNote(m_vstNode, note, 100);

}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    int note = keyEventToNote(event);
    if (note == -1) {
        return QMainWindow::keyPressEvent(event);
    }
    sendNote(m_vstNode, note, 0);
}

void MainWindow::on_showEditorButton_toggled(bool checked)
{
    ui->vstPlaceholder->setVisible(checked);
    // this seems to be necessary to force the sizeHint to be updated so we actually shrink to fit.
    QCoreApplication::processEvents();
    adjustSize();
}


namespace En {

VstTimeInfo s_vstTimeInfo = {0, 0, 0, 0, 120, 0, 0, 0, 4, 4, 0, 0, 0, kVstTransportPlaying | kVstTimeSigValid};

}
