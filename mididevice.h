#pragma once
#include <QObject>
#include <QByteArray>

namespace En {

class MidiDevice : public QObject
{
    Q_OBJECT
public:
    MidiDevice(QObject *parent);
    void postMidiEvent(const QByteArray& event);
signals:
    void eventPosted(const QByteArray& event);
private:
};
}
