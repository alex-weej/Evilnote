#include "core.h"

#include "vstmodule.h"

namespace En
{

Core* Core::s_instance = 0;

void Core::scanVstDir(QDir dir)
{
    //qDebug() << "scanning" << dir << dir.count();

    Q_FOREACH (QString entry, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {

        // better way to do type detection?
        QRegExp re("(.+)(\\.vst|\\.vst3)");
        if (!re.exactMatch(entry)) {
            scanVstDir(QDir(dir.filePath(entry)));
        }

        // better way to get the 'vst name' for the module?
        QString vstName = re.capturedTexts()[1];

        if (m_vstInfoMap.find(vstName) != m_vstInfoMap.end()) {
            // this vst name was already picked up in a directory
            // with higher precedence.
            continue;
        }

        VstInfo info;
        info.name = vstName;
        info.moduleFileName = dir.filePath(entry);

        m_vstInfoMap[vstName] = info;
    }
}


void Core::scanVstDirs()
{

    // FML!
    QList<QDir> dirs;
    dirs << QDir(QDir(QDir::homePath()).absoluteFilePath("Library/Audio/Plug-Ins/VST3"));
    dirs << QDir(QDir(QDir::homePath()).absoluteFilePath("Library/Audio/Plug-Ins/VST"));
    dirs << QDir("/Library/Audio/Plug-Ins/VST3");
    dirs << QDir("/Library/Audio/Plug-Ins/VST");

    Q_FOREACH (QDir dir, dirs) {
        scanVstDir(dir);
    }

}

VstModule* Core::vstModule(const QString& vstName)
{

    if (m_vstModuleMap.find(vstName) != m_vstModuleMap.end()) {
        return m_vstModuleMap[vstName]; // already loaded
    }

    if (m_vstInfoMap.find(vstName) == m_vstInfoMap.end()) {
        return 0; // no idea how to load
    }

    En::VstModule* module = new En::VstModule(m_vstInfoMap[vstName].moduleFileName);

    m_vstModuleMap[vstName] = module;

    return module;

}

Core::~Core()
{
    Q_FOREACH (VstModule* module, m_vstModuleMap) {
        delete module;
    }
}

}
