#pragma once

#include <QtCore>
#include "pluginterfaces/vst2.x/aeffectx.h"

class QGraphicsItem;

namespace En
{

extern VstTimeInfo s_vstTimeInfo;

typedef AEffect* (*PluginEntryProc) (audioMasterCallback audioMaster);

VstIntPtr hostCallback (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);

void graphicsItemStackOnTop(QGraphicsItem* item);

}


