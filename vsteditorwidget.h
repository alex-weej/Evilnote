#pragma once

#include <QMacCocoaViewContainer>

#ifdef __OBJC__
//@class NSView;
#else
//class NSView;
#endif

namespace En
{

class VstNode;

class VstEditorWidget : public QMacCocoaViewContainer
{
    Q_OBJECT

    VstNode* m_vstNode;
    NSView* m_view;

public:

    VstEditorWidget(VstNode* vstNode, QWidget* parent = 0);

    virtual ~VstEditorWidget();

    QSize sizeHint() const;

    void showEvent(QShowEvent* event);

    void hideEvent(QHideEvent* event);

};

} // namespace En
