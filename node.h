#pragma once

#include "en.h"
#include "host.h"

namespace En
{

class NodeGroup;
class VstNode;
class MixerNode;
class MidiInputNode;

class Node : public QObject
{
    Q_OBJECT

public:

    class Factory
    {
    public:
        virtual Node* create(Host*) = 0;
        virtual ~Factory() {}
    };

private:

    NodeGroup* m_nodeGroup;

    // position in node graph.
    QPointF m_position;

protected:

    class ChannelData {
    public:
        QString name;
        QVector<float> buffer;

        ChannelData(const QString& name, size_t bufferSize)
            : name(name)
            , buffer(bufferSize) {}

        // why the fuck is this necessary to use ChannelData in a QVector?
        ChannelData() {}

    };

    QVector<ChannelData> m_inputChannelData; // WARNING: unused buffer ATM
    QVector<ChannelData> m_outputChannelData;

public:

    static int blockSize() {
        return 512;
    }

    // Turns out, probably didn't need this...
    struct Visitor {
        virtual void visit(Node* node) { Q_UNUSED(node); }
        virtual void visit(VstNode* node);
        virtual void visit(MixerNode* node);
        virtual void visit(MidiInputNode* node);
    };

    static const float* s_nullInputBuffer;

    Node()
        : m_nodeGroup(0)
    {
    }

    virtual ~Node()
    {
        // disable this for now - we break because by this point
        // of destruction we cannot check our own inputs!
//        if (nodeGroup()) {
//            nodeGroup()->removeNode(this);
//        }
    }

    virtual NodeGroup* nodeGroup() const {
        return m_nodeGroup;
    }

    void setNodeGroup(NodeGroup* nodeGroup) {
        // WARNING: this doesn't actually add this node to the group!
        m_nodeGroup = nodeGroup;
    }

    virtual void accept(Visitor&) {}

    int numInputChannels() const {
        return m_inputChannelData.count();
    }

    int numOutputChannels() const {
        return m_outputChannelData.count();
    }

    float* outputChannelBuffer(int channelIndex) {
        return m_outputChannelData[channelIndex].buffer.data();
    }

    const float* outputChannelBufferConst(int channelIndex) const {
        if (channelIndex >= numOutputChannels()) {
            return s_nullInputBuffer;
        }
        return m_outputChannelData[channelIndex].buffer.constData();
    }

    virtual QList<QByteArray> midiEvents() const {
        return QList<QByteArray>();
    }

    virtual QString displayLabel() const = 0;

    QPointF position() const {
        return m_position;
    }

    void setPosition(const QPointF& position) {
        //qDebug() << "Setting node position to" << position;
        m_position = position;
    }

    virtual void openEditorWindow()
    {
        // do nothing for this virtual base method
    }

    virtual QList<Node*> inputs() const = 0;

    virtual void removeInput(Node* node) = 0;

    void disconnectNode();

    virtual void processAudio() = 0;

};

} // namespace En
