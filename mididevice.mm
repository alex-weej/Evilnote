#include <mididevice.h>

#import <CoreMIDI/CoreMIDI.h>
#import <Foundation/Foundation.h>
#import <Foundation/NSObjCRuntime.h>
#include <QDebug>

static void midiInputCallback(const MIDIPacketList *list,
                              void *procRef,
                              void *srcRef) {

    auto device = reinterpret_cast<En::MidiDevice::MidiDevice*>(procRef);

    for (UInt32 i = 0; i < list->numPackets; i++) {
        const MIDIPacket *packet = &list->packet[i];

        for (UInt16 j = 0, size = 0; j < packet->length; j += size) {
            UInt8 status = packet->data[j];

            if      (status <  0xC0)  size = 3;
            else if (status <  0xE0)  size = 2;
            else if (status <  0xF0)  size = 3;
            else if (status <  0xF3)  size = 3;
            else if (status == 0xF3)  size = 2;
            else                      size = 1;

            QByteArray midiEvent(reinterpret_cast<const char*>(packet->data + j), size);
            device->postMidiEvent(midiEvent);
        }
    }
}

En::MidiDevice::MidiDevice(QObject *parent) : QObject(parent)
{
    MIDIClientRef   midiClient;
    MIDIPortRef     inputPort;
    OSStatus        status;

    status = MIDIClientCreate(CFSTR("MIDI client"), NULL, NULL, &midiClient);
    NSCAssert1(status == noErr, @"Error creating MIDI client: %d", status);

    status = MIDIInputPortCreate(midiClient, CFSTR("MIDI input"), midiInputCallback, this, &inputPort);
    NSCAssert1(status == noErr, @"Error creating MIDI input port: %d", status);

    ItemCount numOfDevices = MIDIGetNumberOfDevices();

    for (ItemCount i = 0; i < numOfDevices; i++) {
        qDebug() << "Connecting MIDI device" << i;
        MIDIEndpointRef src = MIDIGetSource(i);
        MIDIPortConnectSource(inputPort, src, NULL);
    }
}

void En::MidiDevice::postMidiEvent(const QByteArray& event)
{
    emit eventPosted(event);
}
