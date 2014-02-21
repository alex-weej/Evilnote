#include "midiinputnode.h"

#include <QMutexLocker>

namespace En {

MidiInputNode::MidiInputNode()
{
    m_midiDevice = new MidiDevice(this);
    // Use DirectConnection so it stays in the MIDI thread
    connect(m_midiDevice, SIGNAL(eventPosted(QByteArray)) , this, SLOT(postMidiEvent(QByteArray)), Qt::DirectConnection);
}

void MidiInputNode::processAudio() {
    QMutexLocker locker(&m_mutex);
    qSwap(m_midiEventsBackBuffer, m_midiEvents);
    m_midiEventsBackBuffer.clear();
}

void MidiInputNode::postMidiEvent(const QByteArray &midiEvent)
{
    // THIS IS CALLED FROM THE CORE MIDI THREAD
    QMutexLocker locker(&m_mutex);
    m_midiEventsBackBuffer.push_back(midiEvent);
}

} // namespace En
