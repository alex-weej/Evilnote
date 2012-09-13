#include "node.h"
#include "vstnode.h"
#include "mixernode.h"

namespace En
{

const float* Node::s_nullInputBuffer = 0;

void Node::disconnectNode()
{
    if (nodeGroup()) {

        Q_FOREACH (Node* node, inputs()) {
            removeInput(node);
        }
        nodeGroup()->emitNodeInputsChanged(this);

        Q_FOREACH (Node* depNode, nodeGroup()->dependentNodes(this)) {
            depNode->removeInput(this);
        }

    }
}

void Node::Visitor::visit(VstNode* node) {
    visit(static_cast<Node*>(node));
}

void Node::Visitor::visit(MixerNode* node) {
    visit(static_cast<Node*>(node));
}

} // namespace En
