#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include "en.h"

namespace En
{

class VstModule: public QObject {

    Q_OBJECT

private:

    CFBundleRef m_bundle;
    PluginEntryProc m_mainProc;

public:

    VstModule(const QString& fileName) : m_bundle(0), m_mainProc(0) {
        load(fileName);
    }

    virtual ~VstModule() {
        unload();
    }

    AEffect* createVstInstance() {
        if (!m_mainProc) {
            qDebug("createVstInstance failure: no main proc");
            return 0;
        }
        AEffect* effect = m_mainProc(hostCallback);
        if (!effect) {
            qDebug("createVstInstance failure: couldn't create vst instance");
            return 0;
        }
        return effect;
    }

private:

    bool load(const QString& fileName) {

        CFStringRef fileNameString = CFStringCreateWithCString(0, fileName.toUtf8(), kCFStringEncodingUTF8);
        if (fileNameString == 0) {
            qDebug("bad filename");
            return false;
        }
        CFURLRef url = CFURLCreateWithFileSystemPath (0, fileNameString, kCFURLPOSIXPathStyle, false);
        CFRelease(fileNameString);
        if (url == 0) {
            qDebug("bad url");
            return false;
        }
        m_bundle = CFBundleCreate(0, url);
        CFRelease(url);
        if (!m_bundle) {
            qDebug("couldn't create bundle");
            return false;
        }

        if (CFBundleLoadExecutable(m_bundle) == false) {
            qDebug("couldn't load executable for bundle");
            unload();
            return false;
        }

        m_mainProc = (PluginEntryProc)CFBundleGetFunctionPointerForName(m_bundle, CFSTR("VSTPluginMain"));
        if (!m_mainProc) {
            m_mainProc = (PluginEntryProc)CFBundleGetFunctionPointerForName(m_bundle, CFSTR("main_macho"));
        }
        if (!m_mainProc) {
            qDebug("couldn't get main proc");
            unload();
            return false;
        }

        qDebug("vst mainproc is: %p", m_mainProc);
        return true;
    }

    void unload() {
        if (m_bundle) {
            CFBundleUnloadExecutable(m_bundle);
            CFRelease(m_bundle);
        }
        m_mainProc = 0;
    }

};

} // namespace En
