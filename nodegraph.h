#pragma once

#include <QtGui>
#include <QtOpenGL>

#include "node.h"
#include "vstnode.h"
#include "mixernode.h"
#include "nodegroup.h"

namespace En
{

class RootGraphicsItem;
class NodeGraphicsItem;
class NodeConnectionArrow;
class NodeGraphEditor;

class RootGraphicsItem : public QGraphicsItem
{

public:

    QRectF boundingRect() const
    {
        return QRectF(0, 0, 0, 0);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
        Q_UNUSED(painter);
        Q_UNUSED(option);
        Q_UNUSED(widget);
        // do nothing;
    }

};

class NodeConnectionArrow;

class NodeGraphicsItem : public QGraphicsItem
{

    Node* m_node; // not owned!
    QSet<NodeConnectionArrow*> m_connectionArrows;

public:

    NodeGraphicsItem(Node* node);

    Node* node() const
    {
        return m_node;
    }

    QRectF boundingRect() const
    {
        return QRectF(QPointF(-100, -20), QSizeF(200, 40));
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    void addConnectionArrow(NodeConnectionArrow* arrow);

    void removeConnectionArrow(NodeConnectionArrow* arrow);

protected:

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);

    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value)
    {
        if (change == ItemPositionChange) {
            // value is the new position.
            QPointF newPos = value.toPointF();
            m_node->setPosition(newPos);
            updateConnectionArrows();
        }
        return QGraphicsItem::itemChange(change, value);
    }

private:

    void updateConnectionArrows() const;

};

class NodeConnectionArrow: public QGraphicsLineItem {

    Node* m_node;
    Node* m_inputNode;
    QGraphicsPolygonItem* m_arrowhead;
    QGraphicsEllipseItem* m_arrowtail;

public:

    NodeConnectionArrow(Node* node, Node* inputNode)
        : m_node(node)
        , m_inputNode(inputNode)
        , m_arrowhead(0)
        , m_arrowtail(0) {

        QPen pen(this->pen());
        pen.setWidth(2);
        setPen(pen);

        QPolygonF polygon;
        polygon << QPointF(0, 0) << QPointF(-12, 6) << /*QPointF(10, 0) <<*/ QPointF(-12, -6);
        m_arrowhead = new QGraphicsPolygonItem(polygon);
        m_arrowhead->setPen(Qt::NoPen);
        QBrush brush(m_arrowhead->brush());
        brush.setStyle(Qt::SolidPattern);
        m_arrowhead->setBrush(brush);
        m_arrowhead->setParentItem(this);

        m_arrowtail = new QGraphicsEllipseItem(-5, -5, 10, 10);
        m_arrowtail->setPen(Qt::NoPen);
        m_arrowtail->setBrush(brush);
        m_arrowtail->setParentItem(this);

        updatePositions();
    }

    void updatePositions() {
        // FIXME: these offsets are hardcoded...
        QLineF arrowLine(m_inputNode->position() + QPointF(0, 20), m_node->position() + QPointF(0, -20));
        qreal angle = arrowLine.angle();

        QLineF arrowBodyLine(arrowLine);
        arrowBodyLine.setLength(arrowBodyLine.length() - 5); // shave a bit off the line so we don't overdraw at the tip of the arrowhead.
        setLine(arrowBodyLine);

        m_arrowhead->setPos(arrowLine.p2());
        m_arrowhead->setRotation(-angle);
        m_arrowtail->setPos(arrowLine.p1());
    }

    Node* node() const
    {
        return m_node;
    }

    Node* inputNode() const
    {
        return m_inputNode;
    }

};


class NodeGraphEditor: public QGraphicsView {

    Q_OBJECT

    NodeGroup* m_nodeGroup; // not owned
    QGraphicsScene* m_scene;
    QGraphicsItem* m_rootItem;
    QMap<Node*, NodeGraphicsItem*> m_nodeItemMap;
    QMap<Node*, QSet<NodeConnectionArrow*> > m_nodeInputArrows;

public:

    NodeGraphEditor(NodeGroup* nodeGroup, QWidget* parent = 0)
        : QGraphicsView(parent)
        , m_nodeGroup(nodeGroup)
        , m_scene(0)
        , m_rootItem(0)
    {

        setFocusPolicy(Qt::StrongFocus);

        // Enable multisampling for AA in the graph
        // arse. enabling this causes us to somehow lose our graphics buffer when fiddling with CurveCM's interface
        if (false) {
            QGLFormat glFormat;
            glFormat.setSampleBuffers(true);
            glFormat.setSamples(4);
            setViewport(new QGLWidget(glFormat));
        }
        setRenderHint(QPainter::Antialiasing);

        setDragMode(QGraphicsView::RubberBandDrag);

        m_scene = new QGraphicsScene(this);
        m_scene->setSceneRect(-1000, -1000, 2000, 2000);
        setScene(m_scene);

        m_rootItem = new RootGraphicsItem;
        scene()->addItem(m_rootItem);

        // this code /will/ move at some point
        for (unsigned i = 0; i < m_nodeGroup->numChildNodes(); ++i) {
            nodeAdded(m_nodeGroup->childNode(i));
        }

        // dodgy, but it'll do for now
        for (unsigned i = 0; i < m_nodeGroup->numChildNodes(); ++i) {
            nodeInputsChanged(m_nodeGroup->childNode(i));
#if 0
            Node* node = m_nodeGroup->childNode(i);
            MixerNode* mixerNode = dynamic_cast<MixerNode*>(node);
            if (mixerNode) {
                for (int i = 0; i < mixerNode->numInputs(); ++i) {
                    Node* inputNode = mixerNode->input(i);
                    NodeConnectionArrow* arrow = new NodeConnectionArrow(node, inputNode);
                    m_nodeItemMap[node]->addConnectionArrow(arrow);
                    m_nodeItemMap[inputNode]->addConnectionArrow(arrow);
                    arrow->setParentItem(m_rootItem);
                }
            }
            VstNode* vstNode = dynamic_cast<VstNode*>(node);
            if (vstNode) {
                Node* inputNode = vstNode->input();
                if (inputNode) {
                    NodeConnectionArrow* arrow = new NodeConnectionArrow(node, inputNode);
                    m_nodeItemMap[node]->addConnectionArrow(arrow);
                    m_nodeItemMap[inputNode]->addConnectionArrow(arrow);
                    arrow->setParentItem(m_rootItem);
                }
            }
#endif
        }


        connect(m_nodeGroup, SIGNAL(nodeAdded(Node*)), SLOT(nodeAdded(Node*)));
        connect(m_nodeGroup, SIGNAL(nodePreRemoved(Node*)), SLOT(nodePreRemoved(Node*)));
        connect(m_nodeGroup, SIGNAL(nodeInputsChanged(Node*)), SLOT(nodeInputsChanged(Node*)));

        QShortcut* shortcut = 0;
        shortcut = new QShortcut(Qt::Key_Tab, this, SLOT(launchNodeCreationDialog()));
        shortcut = new QShortcut(Qt::Key_Delete, this, SLOT(deleteSelected()));

    }

protected slots:

    void nodeAdded(Node* node)
    {
        //qDebug() << "NODE ADDED" << node->displayLabel();

        NodeGraphicsItem* item = new NodeGraphicsItem(node);
        item->setParentItem(m_rootItem);
        m_nodeItemMap[node] = item;

        //nodeInputsChanged(node);
    }

    void nodePreRemoved(Node* node)
    {
        // FIXME: don't assume the connections have already been removed.
        // we need to do that before deleting this object!
        delete m_nodeItemMap[node];
    }

    void nodeInputsChanged(Node* node)
    {
        // FIXME: do nothing if the node isn't in the graph yet - we'll get to that after nodeAdded!
        //qDebug() << "rejigging node inputs for node" << node->displayLabel();
        // delete all current arrows and re-add
        // this could probably use a re-think. contain the madness!
        // so bored of this code, apologies that it's retarded.
        Q_FOREACH (NodeConnectionArrow* arrow, m_nodeInputArrows[node]) {
            m_nodeItemMap[arrow->inputNode()]->removeConnectionArrow(arrow);
            m_nodeItemMap[node]->removeConnectionArrow(arrow);
            delete arrow;
        }
        // clear set of dangling pointers
        m_nodeInputArrows[node].clear();

        MixerNode* mixerNode = dynamic_cast<MixerNode*>(node);
        if (mixerNode) {
            for (unsigned i = 0; i < mixerNode->numInputs(); ++i) {
                Node* inputNode = mixerNode->input(i);
                NodeConnectionArrow* arrow = new NodeConnectionArrow(node, inputNode);
                m_nodeItemMap[node]->addConnectionArrow(arrow);
                m_nodeItemMap[inputNode]->addConnectionArrow(arrow);
                m_nodeInputArrows[node] << arrow;
                arrow->setParentItem(m_rootItem);
            }
        }
        VstNode* vstNode = dynamic_cast<VstNode*>(node);
        if (vstNode) {
            Node* inputNode = vstNode->input();
            if (inputNode) {
                NodeConnectionArrow* arrow = new NodeConnectionArrow(node, inputNode);
                m_nodeItemMap[node]->addConnectionArrow(arrow);
                m_nodeItemMap[inputNode]->addConnectionArrow(arrow);
                m_nodeInputArrows[node] << arrow;
                arrow->setParentItem(m_rootItem);
            }
        }

    }

    void deleteSelected()
    {
        Q_FOREACH (QGraphicsItem* item, scene()->selectedItems()) {
            NodeGraphicsItem* nodeItem = qgraphicsitem_cast<NodeGraphicsItem*>(item);
            if (nodeItem) {
                Node* node = nodeItem->node();
                m_nodeGroup->deleteNode(node);
            }
        }
    }

    void launchNodeCreationDialog();

};

}
