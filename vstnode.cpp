#include "vstnode.h"
#include "nodegroup.h"
#include "core.h"

namespace En
{

VstNode::~VstNode()
{
    delete m_editorWindow;

    m_vstInstance->dispatcher (m_vstInstance, effClose, 0, 0, 0, 0);
    // TODO: how the f do we delete this once we're done with it?
    //delete m_vstInstance; // this SIGABRTs
    //free(m_vstInstance);// this malloc errors: pointer being freed was not allocated.
    m_vstInstance = 0;
}

void VstNode::postMidiEvent(const QByteArray &midiEvent)
{
    VstMidiEvent event = {kVstMidiType, sizeof(VstMidiEvent), 0, kVstMidiEventIsRealtime, 0, 0, {0, 0, 0, 0}, 0, 0, 0, 0};
    qCopy(midiEvent.begin(), midiEvent.end(), &event.midiData[0]);
    // Not sure why the copy is necessary... terser way of doing this right?
    VstMidiEvent* pEvent = new VstMidiEvent(event);
    queueEvent(reinterpret_cast<VstEvent*>(pEvent));
}

Node* VstNode::Factory::create(Host* host)
{
    return new VstNode(host, Core::instance()->vstModule(m_vstName));
}

void VstNode::setInput(Node* node)
{
    //qDebug() << "setting input to" << node;
    m_input = node;
    nodeGroup()->emitNodeInputsChanged(this);
}

void VstNode::setMidiInput(MidiDevice *midiInput)
{
    connect(midiInput, SIGNAL(eventPosted(QByteArray)) , this, SLOT(postMidiEvent(QByteArray)));
    m_midiInput = midiInput;
}

void VstNode::removeInput(Node* node)
{
    if (m_input == node) {
        m_input = 0;
        nodeGroup()->emitNodeInputsChanged(this);
    }
}

void VstNode::openEditorWindow()
{
    if (!m_editorWindow) {
        m_editorWindow = new NodeWindow(this);
    }
    m_editorWindow->show();
    m_editorWindow->raise();
}


void VstNode::processAudio()
{

    int numInputOutputChannels = 0;
    if (input()) {
        numInputOutputChannels = input()->numOutputChannels();
    }

    int numInputChannels = this->numInputChannels();

    const float* inputFloats[numInputChannels];

    for (int c = 0; c < numInputChannels; ++c) {
        if (c < numInputOutputChannels) {
            inputFloats[c] = input()->outputChannelBufferConst(c);
        } else {
            inputFloats[c] = s_nullInputBuffer;
        }
    }

    int numOutputChannels = this->numOutputChannels();

    float* outputFloats[numOutputChannels];

    for (int c = 0; c < numOutputChannels; ++c) {
        outputFloats[c] = outputChannelBuffer(c);
    }

    // Take a reference to the event queue, and buffer swap
    QMutexLocker locker(&m_eventQueueMutex);
    QVector<VstEvent*>& eventQueue = m_eventQueue[m_eventQueueWriteIndex];
    m_eventQueueWriteIndex = !m_eventQueueWriteIndex;
    locker.unlock();

    size_t numEvents = eventQueue.size();
    VstEvents* events = 0;
    if (numEvents) {
        //qDebug() << "processing events" << numEvents;
        events = (VstEvents*)malloc(sizeof(VstEvents) + sizeof(VstEvent*) * numEvents);
        events->numEvents = numEvents;
        events->reserved = 0;
        std::copy(eventQueue.begin(), eventQueue.end(), &events->events[0]);
        eventQueue.clear();

        events->events[numEvents] = 0;
        vstInstance()->dispatcher (vstInstance(), effProcessEvents, 0, 0, events, 0);
    }

    vstInstance()->processReplacing(vstInstance(), const_cast<float**>(inputFloats), outputFloats, blockSize());

    // Think I need to free the events here. This could be done in a different thread?
    for (unsigned i = 0; i < numEvents; ++i) {
        free(events->events[i]);
    }
    free(events);
}


}
