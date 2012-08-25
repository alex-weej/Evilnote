#include <Cocoa/Cocoa.h>
#include "mainwindow.h"

namespace En
{

VstEditorWidget::VstEditorWidget(VstNode* vstNode, QWidget* parent/*=0*/)
    : QMacCocoaViewContainer(0, parent),
      m_vstNode(vstNode),
      m_view(0)
{
    // TODO: learn what this pool stuff is for...
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSView* superview = [[NSView alloc] init];
    setCocoaView(superview);
    [pool release];

}

VstEditorWidget::~VstEditorWidget() {
    if (m_view) {
        [m_view release];
    }
}


void VstEditorWidget::showEvent(QShowEvent* event) {

    AEffect* vstInstance = m_vstNode->vstInstance();


    NSView* superview = (NSView*)cocoaView();
    NSArray* subviews;

    Q_ASSERT(m_view == 0);

    vstInstance->dispatcher(vstInstance, effEditOpen, 0, 0, (void*)superview, 0);
    subviews = [superview subviews];
    Q_ASSERT([subviews count] == 1);

    m_view = [[subviews objectAtIndex:0] retain];
    [[NSNotificationCenter defaultCenter] addObserverForName:@"NSViewFrameDidChangeNotification" object:m_view queue:nil usingBlock:^(NSNotification* notification) {
        //qDebug() << "adjust editor size to" << sizeHint();
        adjustSize();
    }];

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QMacCocoaViewContainer::showEvent(event);

}



void VstEditorWidget::hideEvent(QHideEvent* event) {

    AEffect* vstInstance = m_vstNode->vstInstance();
    vstInstance->dispatcher(vstInstance, effEditClose, 0, 0, 0, 0);

    // some plugins don't actually remove their NSView from the view, but we'll release our ref anyway
    [m_view release];
    m_view = 0;

    // sigh. clean up after massive and any other plugin that leaves an (empty?) view around
    NSView* superview = (NSView*)cocoaView();
    NSArray* subviews = [superview subviews];
    while ([subviews count] > 0) {
        //qDebug() << "Cleaning up dead views from VST editor";
        [[subviews objectAtIndex:0] removeFromSuperview];
    }

    QMacCocoaViewContainer::hideEvent(event);

}

QSize VstEditorWidget::sizeHint() const {

    QSize ret;

    if (m_view == 0) {
        ret = QSize(0, 0);
    }

    NSRect frame = [m_view frame];
    ret = QSize(frame.size.width, frame.size.height);

    //qDebug() << "sizeHint" << ret;

    return ret;

}


}
