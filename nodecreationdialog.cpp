#include "nodecreationdialog.h"

#include "mixernode.h"
#include "vstnode.h"
#include "core.h"

namespace En
{

NodeCreationDialog::NodeCreationDialog(NodeGroup* nodeGroup, QPointF pos, QWidget* parent)
    : QDialog(parent, Qt::Tool)

    , m_nodeGroup(nodeGroup)
    , m_createdNode(0)
{
    setWindowTitle("Create Node");

    QHBoxLayout* layout = new QHBoxLayout(this);
    NodeCreationWidget* creationWidget = new NodeCreationWidget(m_nodeGroup, pos, this);
    QLabel* label = new QLabel(tr("Create Node:"));
    layout->addWidget(label);
    layout->addWidget(creationWidget);
    connect(creationWidget, SIGNAL(nodeCreated(Node*)), SLOT(nodeCreated(Node*)));

    //creationWidget->setFocus();
}

void NodeCreationDialog::nodeCreated(Node *newNode)
{
    m_createdNode = newNode;
    accept();
}

NodeCreationWidget::NodeCreationWidget(NodeGroup* nodeGroup, QPointF pos, QWidget* parent)
    : QLineEdit(parent)
    , m_nodeGroup(nodeGroup)
    , m_pos(pos)
{
    connect(this, SIGNAL(returnPressed()), SLOT(createNode()));
    // TODO: escape handling

    m_factories["Mixer"] = new MixerNode::Factory();

    Core* core = Core::instance();
    Q_FOREACH (QString name, core->vstInfoMap().keys()) {
        m_factories[name] = new VstNode::Factory(name);
    }

    QCompleter* comp = new QCompleter(m_factories.keys(), this);
    // argh, this disables the popup. lame.
    //comp->setCompletionMode(QCompleter::InlineCompletion);
    comp->setCaseSensitivity(Qt::CaseInsensitive);
    setCompleter(comp);
}

NodeCreationWidget::~NodeCreationWidget()
{
    Q_FOREACH (Node::Factory* factory, m_factories) {
        delete factory;
    }
}

void NodeCreationWidget::createNode()
{
    QString name = this->text().trimmed();

    // if there's exactly 1 item in the completion, just run with it. bit crappy, but useful.
    if (completer()->completionCount() == 1) {
        completer()->setCurrentRow(0);
        name = completer()->currentCompletion();
    }

    //qDebug() << "Creating node for:" << name;

    FactoryMap::Iterator it = m_factories.find(name);
    if (it == m_factories.end()) {
        //qDebug() << "nope";
        // TODO: some kind of error feedback
        return;
    }

    Node* node = it.value()->create();
    node->setPosition(m_pos);
    m_nodeGroup->addNode(node);

    emit nodeCreated(node);
}



}
