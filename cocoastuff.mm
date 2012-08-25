#include <Cocoa/Cocoa.h>
#include "mainwindow.h"

namespace En
{

VstInterfaceWidget::VstInterfaceWidget(VstNode* vstNode, QWidget* parent/*=0*/)
    : QMacCocoaViewContainer(0, parent),
      m_vstNode(vstNode)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSView* view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    setCocoaView(view);
    [pool release];
}


void VstInterfaceWidget::showEvent(QShowEvent* event) {

    AEffect* vstInstance = m_vstNode->vstInstance();

    vstInstance->dispatcher(vstInstance, effEditOpen, 0, 0, (void*)cocoaView(), 0);

    ERect* eRect = 0;
    vstInstance->dispatcher(vstInstance, effEditGetRect, 0, 0, &eRect, 0);

    NSRect frame = NSMakeRect(eRect->left, eRect->top, eRect->right - eRect->left, eRect->bottom - eRect->top);
    QSize size(eRect->right - eRect->left, eRect->bottom - eRect->top);

    resize(size);
    setFixedSize(size);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QMacCocoaViewContainer::showEvent(event);

}



void VstInterfaceWidget::hideEvent(QHideEvent* event) {

    AEffect* vstInstance = m_vstNode->vstInstance();
    vstInstance->dispatcher(vstInstance, effEditClose, 0, 0, 0, 0);
    QMacCocoaViewContainer::hideEvent(event);

}


}
