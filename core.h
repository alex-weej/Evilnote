#pragma once

#include <QtCore>
#include "vstmodule.h"
#include "vstinfo.h"

namespace En
{

class Core : public QObject
{
    Q_OBJECT

    typedef QMap<QString, VstInfo> VstInfoMap;
    VstInfoMap m_vstInfoMap;
    QMap<QString, VstModule*> m_vstModuleMap;

    static Core* s_instance;

public:

    Core()
    {
        Q_ASSERT(s_instance == 0);
        s_instance = this;
    }

    ~Core();

    static Core* instance()
    {
        return s_instance;
    }

    void scanVstDir(QDir dir);

    void scanVstDirs();

    VstModule* vstModule(const QString& vstName);

    const VstInfoMap& vstInfoMap()
    {
        return m_vstInfoMap;
    }

};

} // namespace En
