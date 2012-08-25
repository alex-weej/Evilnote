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

    AEffect* vstInstance = m_vstNode->vstInstance();


    qDebug() << "view" << view;


    setCocoaView(view);


    vstInstance->dispatcher(vstInstance, effEditOpen, 0, 0, (void*)view, 0);


    ERect* eRect = 0;
    qDebug ("HOST> Get editor rect..");
    vstInstance->dispatcher(vstInstance, effEditGetRect, 0, 0, &eRect, 0);

    qDebug() << "Editor rect:" << eRect->left << eRect->bottom << eRect->right << eRect->top;

    NSRect frame = NSMakeRect(eRect->left, eRect->top, eRect->right - eRect->left, eRect->bottom - eRect->top);
    QSize size(eRect->right - eRect->left, eRect->bottom - eRect->top);

    qDebug() << "setting size to" << size;

    resize(size);
    setFixedSize(size);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);


    [pool release];
}

//VstInterfaceWidget::showEvent(QShowEvent*) {

//}

}
