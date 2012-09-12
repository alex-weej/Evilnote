#include "mainwindow.h"
#include "ui_mainwindow.h"

namespace En {

VstTimeInfo s_vstTimeInfo = {0, 0, 0, 0, 135, 0, 0, 0, 4, 4, 0, 0, 0, kVstTransportPlaying | kVstPpqPosValid | kVstTimeSigValid | kVstTempoValid};
const float* Node::s_nullInputBuffer = 0;

NodeWindow::NodeWindow(En::VstNode* vstNode, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_vstNode(vstNode),
    m_note(60)
{
    ui->setupUi(this);

    setWindowTitle(m_vstNode->displayLabel());

    QVBoxLayout* layout = new QVBoxLayout(ui->vstPlaceholder);
    En::VstEditorWidget* interface = new En::VstEditorWidget(vstNode, ui->vstPlaceholder);
    layout->addWidget(interface);

    m_vstEditorWidget = interface;

    m_funkyTownPos = 0;
    m_funkyTown << 0 << 0 << -2 << 0 << -5 << -5 << 0 << 5 << 4 << 0;
}

NodeWindow::~NodeWindow()
{
    delete ui;
}


static void sendNote(En::VstNode* vstNode, unsigned char noteValue, unsigned char velocity) {
    // temp testing stuff to force sending multiple notes at the exact same time.
    // change i to a number <= 4 to make this send chords.
    for (int i = 0; i < 1; ++i) {
        if (i == 1) noteValue += 4;
        else if (i == 2) noteValue += 3;
        else if (i == 3) noteValue += 5;

        VstMidiEvent event = {kVstMidiType, sizeof (VstMidiEvent), 0, kVstMidiEventIsRealtime, 0, 0, {0x90, noteValue, velocity, 0}, 0, 0, 0, 0};
        // heap that shit.
        VstMidiEvent* pEvent = new VstMidiEvent(event);
        vstNode->queueEvent((VstEvent*)pEvent);
    }
}

void NodeWindow::on_testNoteButton_pressed()
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

void NodeWindow::on_testNoteButton_released()
{
    sendNote(m_vstNode, m_lastNote, 0);
}

static int keyEventToNote(QKeyEvent* event) {

    // don't process if modifiers are used!
    if (event->modifiers()) {
        return -1;
    }

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

void NodeWindow::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        return;
    }

    int note = keyEventToNote(event);
    if (note == -1) {
        return QMainWindow::keyPressEvent(event);
    }

    sendNote(m_vstNode, note, 100);

}

void NodeWindow::keyReleaseEvent(QKeyEvent* event) {
    int note = keyEventToNote(event);
    if (note == -1) {
        return QMainWindow::keyReleaseEvent(event);
    }
    sendNote(m_vstNode, note, 0);
}

void NodeWindow::on_showEditorButton_toggled(bool checked)
{
    m_vstEditorWidget->setVisible(checked);
    if (!checked) {
        // this seems to be necessary to force the sizeHint to be updated so we actually shrink to fit
        // if we're hiding...
        QCoreApplication::processEvents();
        adjustSize();
    }
}

NodeGroup::~NodeGroup() {
    Q_FOREACH (Node* node, m_nodes) {
        delete node;
    }
}

void NodeGroup::addNode(Node *node)
{
    node->setNodeGroup(this);
    m_nodes.push_back(node);
    emit nodeAdded(node);
}

void NodeGroup::removeNode(Node *node)
{
    node->disconnectNode();
    emit nodePreRemoved(node);
    m_nodes.remove(m_nodes.indexOf(node));
    node->setNodeGroup(0);
    emit nodeRemoved(node);
}

QList<Node *> NodeGroup::dependentNodes(Node *node) const
{
    QList<Node*> deps;

    Q_FOREACH (Node* testNode, m_nodes) {
        VstNode* vstNode = dynamic_cast<VstNode*>(testNode);
        if (vstNode) {
            if (vstNode->input() == node) {
                deps << testNode;
            }
            continue;
        }

        MixerNode* mixerNode = dynamic_cast<MixerNode*>(testNode);
        if (mixerNode) {
            Q_FOREACH (Node* mixerInput, mixerNode->inputs()) {
                if (mixerInput == node) {
                    deps << testNode;
                }
            }
            continue;
        }
    }

    return deps;
}

void graphicsItemStackOnTop(QGraphicsItem* item) {
   // this appears to be the only sensible way to re-order such that this is on top.
   // madness.
   QGraphicsItem* parentItem = item->parentItem();
   if (parentItem) {
       item->setParentItem(0);
       item->setParentItem(parentItem);
   }
}



void NodeGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
   // not sure if this re-stacking is gonna be slow...

   graphicsItemStackOnTop(this);

   Q_FOREACH (NodeConnectionArrow* arrow, m_connectionArrows) {
       graphicsItemStackOnTop(arrow);
   }

   QGraphicsItem::mousePressEvent(event);
}

void NodeGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    m_node->openEditorWindow();
}

void NodeGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;
    VstNode* vstNode = dynamic_cast<VstNode*>(m_node);

    NodeContextMenuHelper* helper = 0;

    // maybe this is a sign we need to split this NodeGraphicsItem class...
    // or maybe we should just unify the Node interface.
    if (vstNode) {


        Node* currentInput = vstNode->input();
        QMenu* inputMenu = new QMenu("Set Input");

        QSignalMapper* signalMapper = new QSignalMapper(inputMenu);
        VstNodeContextMenuHelper* vstHelper = new VstNodeContextMenuHelper(vstNode);
        helper = vstHelper;

        QObject::connect(signalMapper, SIGNAL(mapped(QObject*)), vstHelper, SLOT(setInput(QObject*)));

        QAction* action = inputMenu->addAction("(None)", signalMapper, SLOT(map()));
        signalMapper->setMapping(action, (QObject*)0);
        action->setCheckable(true);
        action->setChecked(currentInput == 0);
        inputMenu->addSeparator();

        NodeGroup* group = vstNode->nodeGroup();
        Q_FOREACH (Node* childNode, group->childNodes()) {
            QAction* action = inputMenu->addAction(childNode->displayLabel(), signalMapper, SLOT(map()));
            signalMapper->setMapping(action, childNode);
            action->setCheckable(true);
            action->setChecked(childNode == currentInput);
        }

        menu.addMenu(inputMenu);

    }

    MixerNode* mixerNode = dynamic_cast<MixerNode*>(m_node);
    if (mixerNode) {

        QList<Node*> currentInputs = mixerNode->inputs();
        QMenu* addInputMenu = new QMenu("Add Input");
        QMenu* removeInputMenu = new QMenu("Remove Input");

        MixerNodeContextMenuHelper* mixerHelper = new MixerNodeContextMenuHelper(mixerNode);
        helper = mixerHelper;
        QSignalMapper* signalMapperAdd = new QSignalMapper(addInputMenu);
        QSignalMapper* signalMapperRemove = new QSignalMapper(removeInputMenu);
        QObject::connect(signalMapperAdd, SIGNAL(mapped(QObject*)), mixerHelper, SLOT(addInput(QObject*)));
        QObject::connect(signalMapperRemove, SIGNAL(mapped(QObject*)), mixerHelper, SLOT(removeInput(QObject*)));

        NodeGroup* group = mixerNode->nodeGroup();
        Q_FOREACH (Node* childNode, group->childNodes()) {
            QAction* action = addInputMenu->addAction(childNode->displayLabel(), signalMapperAdd, SLOT(map()));
            signalMapperAdd->setMapping(action, childNode);
        }

        Q_FOREACH (Node* inputNode, currentInputs) {
            QAction* action = removeInputMenu->addAction(inputNode->displayLabel(), signalMapperRemove, SLOT(map()));
            signalMapperRemove->setMapping(action, inputNode);
        }

        menu.addMenu(addInputMenu);
        menu.addMenu(removeInputMenu);

    }

    menu.addAction("Delete", helper, SLOT(deleteNode()));

    menu.exec(event->screenPos());

    delete helper;




}

void NodeGraphicsItem::updateConnectionArrows() const {
    Q_FOREACH (NodeConnectionArrow* arrow, m_connectionArrows) {
        arrow->updatePositions();
    }
}

void VstNode::openEditorWindow() {
    if (!m_editorWindow) {
        m_editorWindow = new NodeWindow(this);
    }
    m_editorWindow->show();
    m_editorWindow->raise();
}

void NodeButton::openEditorWindow() {
    m_node->openEditorWindow();
}



Core* Core::s_instance = 0;


void Core::scanVstDir(QDir dir)
{
    //qDebug() << "scanning" << dir << dir.count();

    Q_FOREACH (QString entry, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {

        // better way to do type detection?
        QRegExp re("(.+)(\\.vst|\\.vst3)");
        if (!re.exactMatch(entry)) {
            scanVstDir(QDir(dir.filePath(entry)));
        }

        // better way to get the 'vst name' for the module?
        QString vstName = re.capturedTexts()[1];

        if (m_vstInfoMap.find(vstName) != m_vstInfoMap.end()) {
            // this vst name was already picked up in a directory
            // with higher precedence.
            continue;
        }

        VstInfo info;
        info.name = vstName;
        info.moduleFileName = dir.filePath(entry);

        m_vstInfoMap[vstName] = info;
    }
}


void Core::scanVstDirs() {

    // FML!
    QList<QDir> dirs;
    dirs << QDir(QDir(QDir::homePath()).absoluteFilePath("Library/Audio/Plug-Ins/VST3"));
    dirs << QDir(QDir(QDir::homePath()).absoluteFilePath("Library/Audio/Plug-Ins/VST"));
    dirs << QDir("/Library/Audio/Plug-Ins/VST3");
    dirs << QDir("/Library/Audio/Plug-Ins/VST");

    Q_FOREACH (QDir dir, dirs) {
        scanVstDir(dir);
    }

}

VstModule* Core::vstModule(const QString &vstName)
{

    if (m_vstModuleMap.find(vstName) != m_vstModuleMap.end()) {
        return m_vstModuleMap[vstName]; // already loaded
    }

    if (m_vstInfoMap.find(vstName) == m_vstInfoMap.end()) {
        return 0; // no idea how to load
    }

    En::VstModule* module = new En::VstModule(m_vstInfoMap[vstName].moduleFileName);

    m_vstModuleMap[vstName] = module;

    return module;

}

Core::~Core()
{
    Q_FOREACH (VstModule* module, m_vstModuleMap) {
        delete module;
    }
}

void NodeGraphEditor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Tab) {
        QPoint cursorPos = QCursor::pos();
        QPointF graphPos = mapToScene(mapFromGlobal(cursorPos));
        NodeCreationDialog d(m_nodeGroup, graphPos, this);
        d.adjustSize();
        d.move(cursorPos - d.rect().center());
        d.exec();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

Node *VstNode::Factory::create()
{
    return new VstNode(Core::instance()->vstModule(m_vstName));
}

NodeCreationDialog::NodeCreationDialog(NodeGroup *nodeGroup, QPointF pos, QWidget *parent)
    : QDialog(parent, Qt::Popup)
    , m_nodeGroup(nodeGroup)
{
    initCocoa();

    setWindowTitle("Create Node");

    QHBoxLayout* layout = new QHBoxLayout(this);
    NodeCreationWidget* creationWidget = new NodeCreationWidget(m_nodeGroup, pos, this);
    QLabel* label = new QLabel(tr("Create Node:"));
    layout->addWidget(label);
    layout->addWidget(creationWidget);
    connect(creationWidget, SIGNAL(done()), SLOT(accept()));

    creationWidget->setFocus();
}

} // namespace En
