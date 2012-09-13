#include "nodegraph.h"

#include "vstnode.h"
#include "mixernode.h"
#include "nodecontextmenuhelper.h"
#include "nodecreationdialog.h"

namespace En
{

NodeGraphicsItem::NodeGraphicsItem(Node* node)
    : m_node(node)
    , m_isOutputNode(false)
{
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
    setFlag(ItemIsSelectable);

    setAcceptHoverEvents(true);

    setPos(m_node->position());

    QGraphicsSimpleTextItem* label = new QGraphicsSimpleTextItem(this);
    label->setText(m_node->displayLabel());
    label->setPos(-label->boundingRect().width() / 2.0, -label->boundingRect().height() / 2.0);

}

void NodeGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);

    QRectF rect = nodeRect();
    qreal radius = 10.;

    QPen pen(painter->pen());
    pen.setWidthF(2.0);

    if (isSelected()) {
        QBrush selectedBrush = widget->palette().highlight();
        painter->setPen(Qt::NoPen);
        painter->setBrush(selectedBrush);
        qreal selectedRadius = radius + s_selectedPadding;
        painter->drawRoundedRect(boundingRect(), selectedRadius, selectedRadius);
    }

    QBrush brush;
    if (isOutputNode()) {
        brush = QBrush(Qt::yellow);
    } else {
        //brush = QBrush(QColor(240, 240, 240));
        brush = widget->palette().button();
    }
    painter->setBrush(brush);
    // FIXME: use a standard brush somehow? or just use a palette
    painter->setPen(pen);

    qreal halfPenWidth = painter->pen().widthF() / 2.;
    rect.adjust(halfPenWidth, halfPenWidth, -halfPenWidth, -halfPenWidth);
    painter->drawRoundedRect(rect, radius, radius);

}

void NodeGraphicsItem::addConnectionArrow(NodeConnectionArrow* arrow)
{
    m_connectionArrows << arrow;
}

void NodeGraphicsItem::removeConnectionArrow(NodeConnectionArrow* arrow)
{
    m_connectionArrows.remove(arrow);
}

void NodeGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
   // not sure if this re-stacking is gonna be slow...

   graphicsItemStackOnTop(this);

   Q_FOREACH (NodeConnectionArrow* arrow, m_connectionArrows) {
       graphicsItemStackOnTop(arrow);
   }

   QGraphicsItem::mousePressEvent(event);
}

void NodeGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
    m_node->openEditorWindow();
}

void NodeGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    scene()->clearSelection();
    setSelected(true);

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
    menu.addAction("Set As Output", helper, SLOT(setNodeAsOutput()));

    menu.exec(event->screenPos());

    delete helper;

}

void NodeGraphicsItem::updateConnectionArrows() const
{
    Q_FOREACH (NodeConnectionArrow* arrow, m_connectionArrows) {
        arrow->updatePositions();
    }
}

void NodeGraphEditor::launchNodeCreationDialog()
{
    QPoint cursorPos = QCursor::pos();
    QPointF graphPos = mapToScene(mapFromGlobal(cursorPos));
    NodeCreationDialog d(m_nodeGroup, graphPos, this);
    d.adjustSize();
    d.move(cursorPos - d.rect().center());
    d.exec();
    if (d.createdNode()) {
        scene()->clearSelection();
        m_nodeItemMap[d.createdNode()]->setSelected(true);
    }
}

void NodeGraphEditor::outputNodeChanged()
{
    Node* newOutputNode = m_nodeGroup->outputNode();
    if (newOutputNode == m_currentOutputNode) {
        return;
    }
    if (m_currentOutputNode) {
        m_nodeItemMap[m_currentOutputNode]->setIsOutputNode(false);
    }
    m_currentOutputNode = newOutputNode;
    if (m_currentOutputNode) {
        m_nodeItemMap[m_currentOutputNode]->setIsOutputNode(true);
    }
}

void NodeGraphEditor::deleteSelectedNodes()
{
    Q_FOREACH (Node* node, selectedNodes()) {
        m_nodeGroup->deleteNode(node);
    }
}

void NodeGraphEditor::setSelectedNodeAsOutput()
{
    // only set selected as output if there is a single node selected!
    Node* selectedNode = 0;
    Q_FOREACH (Node* node, selectedNodes()) {
        if (selectedNode) {
            return;
        }
        selectedNode = node;
    }
    m_nodeGroup->setOutputNode(selectedNode);
}

}
