#pragma once

#include "nodegroup.h"

namespace En
{

class MixerNode: public Node
{
    Q_OBJECT

    QList<Node*> m_inputs;
    //QVector<QVector<float> > m_outputChannelBuffers;

public:

    class Factory: public Node::Factory
    {

    public:

        virtual Node* create()
        {
            return new MixerNode;
        }

    };

    MixerNode()
    {
        m_outputChannelData << ChannelData("Mix L", blockSize());
        m_outputChannelData << ChannelData("Mix R", blockSize());
    }

    void addInput(Node* node)
    {
        m_inputs << node;
        inputsChanged();
    }

    void removeInput(int i)
    {
        m_inputs.removeAt(i);
        inputsChanged();
    }

    void removeInput(Node* node)
    {
        // definite bug if we allow more than one connection from the same node!
        m_inputs.removeOne(node);
        inputsChanged();
    }

    QList<Node*> inputs() const
    {
        return m_inputs;
    }

    void inputsChanged() {
        m_inputChannelData.clear();
        for (unsigned i = 0; i < numInputs(); ++i) {
            for (int c = 0; c < 2; ++c) {
                m_inputChannelData << ChannelData(QString("In %1 %2").arg(QString::number(i), QString(c == 0 ? "L" : "R")), blockSize());
            }
        }
        nodeGroup()->emitNodeInputsChanged(this);
    }

    unsigned numInputs() const {
        return m_inputs.size();
    }

    Node* input(int i) const {
        return m_inputs[i];
    }

    virtual void accept(Visitor& visitor) {
        visitor.visit(this);
    }

    virtual QString displayLabel() const {
        return tr("Mixer");
    }

    virtual void processAudio() {

        // zero the result
        for (int c = 0; c < 2; ++c) {
            for (int s = 0; s < blockSize(); ++s) {
                outputChannelBuffer(c)[s] = 0;
            }
        }

        // accumulate
        for (unsigned i = 0; i < numInputs(); ++i) {
            for (int c = 0; c < 2; ++c) {
                for (int s = 0; s < blockSize(); ++s) {
                    outputChannelBuffer(c)[s] += input(i)->outputChannelBufferConst(c)[s];
                }
            }
        }

    }

};
} // namespace En
