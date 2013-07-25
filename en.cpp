#include "en.h"

//#include "ui_mainwindow.h"

#include <QtWidgets>

namespace En {

VstTimeInfo s_vstTimeInfo = {0, 0, 0, 0, 135, 0, 0, 0, 4, 4, 0, 0, 0, kVstTransportPlaying | kVstPpqPosValid | kVstTimeSigValid | kVstTempoValid};

VstIntPtr hostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
    Q_UNUSED(effect);
    Q_UNUSED(opt);

    //qDebug() << "hostCallback in thread" << QThread::currentThreadId();

    VstIntPtr result = 0;

    // Filter idle calls...
    bool filtered = false;
    if (opcode == audioMasterIdle)
    {
        static bool wasIdle = false;
        if (wasIdle)
            filtered = true;
        else
        {
            //printf ("(Future idle calls will not be displayed!)\n");
            wasIdle = true;
        }
    }

    //qDebug("audioMasterGetCurrentProcessLevel is %d", audioMasterGetCurrentProcessLevel);

    if (!filtered)
        //qDebug("PLUG> HostCallback (opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f", opcode, index, FromVstPtr<void> (value), ptr, opt);

        switch (opcode)
        {
        case audioMasterVersion:
            result = kVstVersion;
            break;

        case DECLARE_VST_DEPRECATED (audioMasterPinConnected):
            //qDebug() << "pin connected";
            break;

        case DECLARE_VST_DEPRECATED (audioMasterWantMidi):
            break;

        case audioMasterGetTime:
            result = (VstIntPtr)&s_vstTimeInfo;
            break;

        case audioMasterGetCurrentProcessLevel:
            result = kVstProcessLevelUser;
            break;

        case audioMasterCanDo:
        {
            QString canDoQuery((char*)ptr);

            QMap<QString, int> canDos;
            canDos["sendVstEvents"] = 1;
            canDos["sendVstMidiEvent"] = 1;
            canDos["sendVstTimeInfo"] = 1;
            canDos["sizeWindow"] = 1;

            QMap<QString, int>::iterator it = canDos.find(canDoQuery);
            if (it == canDos.end()) {
                result = 0; // don't know
                qDebug() << "unknown audioMasterCanDo:" << canDoQuery;
            } else {
                result = *it;
            }

            break;
        }

        case audioMasterSizeWindow:
        {
            // TODO: handle this!
            int width = index;
            int height = value;
            qDebug() << "SIZE WINDOW" << width << height;
            break;
        }

        default:
            //qDebug() << "UNHANDLED HOST OPCODE" << opcode;
            //Q_ASSERT(false);
            break;

        }

    return result;
}


void graphicsItemStackOnTop(QGraphicsItem* item) {
   // this appears to be the only sensible way to re-order such that this is on top.
   // madness.
   QGraphicsItem* parentItem = item->parentItem();
   if (parentItem) {
       item->setParentItem(0);
       item->setParentItem(parentItem);
   }
}



} // namespace En
