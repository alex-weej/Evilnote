#pragma once
#include <QObject>
#include <QByteArray>
#include <CoreMIDI/MIDIServices.h>

namespace En {

class MidiDevice : public QObject
{
    Q_OBJECT
public:
    MidiDevice(QObject *parent);
    ~MidiDevice();
    void postMidiEvent(const QByteArray& event);
signals:
    void eventPosted(const QByteArray& event);
private:
    MIDIClientRef m_midiClient;
    MIDIPortRef m_inputPort;
};
}
