#ifndef EN_MIDIINPUTNODE_H
#define EN_MIDIINPUTNODE_H

#include "node.h"

namespace En {

class MidiInputNode : public En::Node
{
    Q_OBJECT

public:

    class Factory: public Node::Factory
    {
    public:
        virtual Node* create(Host*)
        {
            return new MidiInputNode;
        }
    };

    MidiInputNode();

    ~MidiInputNode() {

    }

    QString displayLabel() const {
        return tr("MIDI Input");
    }

    virtual void accept(Visitor& visitor) {
        visitor.visit(this);
    }

    QList<Node*> inputs() const {
        return QList<Node*>();
    }

    void removeInput(Node* node) {
        Q_UNUSED(node);
    }

    void processAudio();

    virtual QList<QByteArray> midiEvents() const {
        return m_midiEvents;
    }

    Q_SLOT void postMidiEvent(const QByteArray& midiEvent);

private:
    MidiDevice *m_midiDevice;
    QMutex m_mutex;
    QList<QByteArray> m_midiEvents;
    QList<QByteArray> m_midiEventsBackBuffer;
};

} // namespace En

#endif // EN_MIDIINPUTNODE_H
