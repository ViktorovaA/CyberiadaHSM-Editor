#include "verectangle.h"
#include <QPainter>
#include <QDebug>
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <math.h>
#include "dotsignal.h"

static const double Pi = 3.14159265358979323846264338327950288419717;
static double TwoPi = 2.0 * Pi;

static qreal normalizeAngle(qreal angle)
{
    while (angle < 0)
        angle += TwoPi;
    while (angle > TwoPi)
        angle -= TwoPi;
    return angle;
}

VERectangle::VERectangle(QObject *parent) :
    QObject(parent),
    m_cornerFlags(0),
    m_actionFlags(ResizeState)
{
    setAcceptHoverEvents(true);
    setFlags(ItemIsSelectable|ItemSendsGeometryChanges);
    for(int i = 0; i < 8; i++){
        cornerGrabber[i] = new DotSignal(this);
    }
    setPositionGrabbers();
}

VERectangle::~VERectangle()
{
    for(int i = 0; i < 8; i++){
        delete cornerGrabber[i];
    }
}

QRectF VERectangle::boundingRect() const
{
    return m_rect;
}

void VERectangle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setPen(Qt::black);
    painter->setBrush(Qt::lightGray);
    painter->drawRect(m_rect);
}

QRectF VERectangle::rect() const
{
    return m_rect;
}

void VERectangle::setPen(const QPen &pen)
{
    m_pen = pen;
}

void VERectangle::setBrush(const QBrush &brush)
{
    m_brush = brush;
}

QPen VERectangle::pen()
{
    return m_pen;
}

QBrush VERectangle::brush()
{
    return m_brush;
}

QPointF VERectangle::previousPosition() const
{
    return m_previousPosition;
}

void VERectangle::setPreviousPosition(const QPointF previousPosition)
{
    if (m_previousPosition == previousPosition)
        return;

    m_previousPosition = previousPosition;
    emit previousPositionChanged();
}

void VERectangle::setRect(qreal x, qreal y, qreal w, qreal h)
{
    setRect(QRectF(x,y,w,h));
}

void VERectangle::setRect(const QRectF &rect)
{
    QGraphicsRectItem::setRect(rect);
    m_rect = rect;
}

void VERectangle::setName(const QString &name)
{
    m_name = name;
}

QString VERectangle::name() const
{
    if (m_name.length() == 0) {
        return QString("");
    }
    return m_name;
}

void VERectangle::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() & Qt::LeftButton) {
        m_leftMouseButtonPressed = true;
        setPreviousPosition(event->scenePos());
        emit clicked(this);
    }
    QGraphicsItem::mousePressEvent(event);
}

void VERectangle::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pt = event->pos();
    if(m_actionFlags == ResizeState){
        switch (m_cornerFlags) {
        case Top:
            resizeTop(pt);
            emit signalResized(this, CornerFlags::Top);
            break;
        case Bottom:
            resizeBottom(pt);
            emit signalResized(this, CornerFlags::Bottom);
            break;
        case Left:
            resizeLeft(pt);
            emit signalResized(this, CornerFlags::Left);
            break;
        case Right:
            resizeRight(pt);
            emit signalResized(this, CornerFlags::Right);
            break;
        case TopLeft:
            resizeTop(pt);
            resizeLeft(pt);
            emit signalResized(this, CornerFlags::TopLeft);
            break;
        case TopRight:
            resizeTop(pt);
            resizeRight(pt);
            emit signalResized(this, CornerFlags::TopRight);
            break;
        case BottomLeft:
            resizeBottom(pt);
            resizeLeft(pt);
            emit signalResized(this, CornerFlags::BottomLeft);
            break;
        case BottomRight:
            resizeBottom(pt);
            resizeRight(pt);
            emit signalResized(this, CornerFlags::BottomRight);
            break;
        default:
            if (m_leftMouseButtonPressed) {
                setCursor(Qt::ClosedHandCursor);
                auto dx = event->scenePos().x() - m_previousPosition.x();
                auto dy = event->scenePos().y() - m_previousPosition.y();
                moveBy(dx,dy);
                setPreviousPosition(event->scenePos());
                emit signalMove(this, dx, dy);
            }
            break;
        }
    } else {
        switch (m_cornerFlags) {
        case TopLeft:
        case TopRight:
        case BottomLeft:

        case BottomRight: {
            rotateItem(pt);
            break;
        }
        default:
            if (m_leftMouseButtonPressed) {
                setCursor(Qt::ClosedHandCursor);
                auto dx = event->scenePos().x() - m_previousPosition.x();
                auto dy = event->scenePos().y() - m_previousPosition.y();
                moveBy(dx,dy);
                setPreviousPosition(event->scenePos());
                emit signalMove(this, dx, dy);
            }
            break;
        }
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void VERectangle::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() & Qt::LeftButton) {
        m_leftMouseButtonPressed = false;
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

void VERectangle::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    m_actionFlags = (m_actionFlags == ResizeState)?RotationState:ResizeState;
    setVisibilityGrabbers();
    QGraphicsItem::mouseDoubleClickEvent(event);
}

void VERectangle::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    setPositionGrabbers();
    setVisibilityGrabbers();
    QGraphicsItem::hoverEnterEvent(event);
}

void VERectangle::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_cornerFlags = 0;
    hideGrabbers();
    setCursor(Qt::CrossCursor);
    QGraphicsItem::hoverLeaveEvent( event );
}

void VERectangle::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    QPointF pt = event->pos();              // The current position of the mouse
    qreal drx = pt.x() - rect().right();    // Distance between the mouse and the right
    qreal dlx = pt.x() - rect().left();     // Distance between the mouse and the left

    qreal dby = pt.y() - rect().top();      // Distance between the mouse and the top
    qreal dty = pt.y() - rect().bottom();   // Distance between the mouse and the bottom

    // If the mouse position is within a radius of 7
    // to a certain side( top, left, bottom or right)
    // we set the Flag in the Corner Flags Register

    m_cornerFlags = 0;
    if( dby < 7 && dby > -7 ) m_cornerFlags |= Top;       // Top side
    if( dty < 7 && dty > -7 ) m_cornerFlags |= Bottom;    // Bottom side
    if( drx < 7 && drx > -7 ) m_cornerFlags |= Right;     // Right side
    if( dlx < 7 && dlx > -7 ) m_cornerFlags |= Left;      // Left side

    if(m_actionFlags == ResizeState){
        // поворот самой картинки курсора
        QPixmap p(":/icons/arrow-up-down.png");
        QPixmap pResult;
        // трансформация в координатах объекта
        QTransform trans = transform();
        switch (m_cornerFlags) {
        case Top:
        case Bottom:
            pResult = p.transformed(trans);
            setCursor(pResult.scaled(24,24,Qt::KeepAspectRatio));
            break;
        case Left:
        case Right:
            trans.rotate(90);
            pResult = p.transformed(trans);
            // Qt::KeepAspectRatio - флаг для сохранения пропорции
            setCursor(pResult.scaled(24,24,Qt::KeepAspectRatio));
            break;
        case TopRight:
        case BottomLeft:
            trans.rotate(45);
            pResult = p.transformed(trans);
            setCursor(pResult.scaled(24,24,Qt::KeepAspectRatio));
            break;
        case TopLeft:
        case BottomRight:
            trans.rotate(135);
            pResult = p.transformed(trans);
            setCursor(pResult.scaled(24,24,Qt::KeepAspectRatio));
            break;
        default:
            setCursor(Qt::CrossCursor);
            break;
        }
    } else {
        switch (m_cornerFlags) {
        case TopLeft:
        case TopRight:
        case BottomLeft:
        case BottomRight: {
            QPixmap p(":/icons/rotate-right.png");
            setCursor(QCursor(p.scaled(24,24,Qt::KeepAspectRatio)));
            break;
        }
        default:
            setCursor(Qt::CrossCursor);
            break;
        }
    }
    QGraphicsItem::hoverMoveEvent( event );
}

QVariant VERectangle::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case QGraphicsItem::ItemSelectedChange:
        m_actionFlags = ResizeState;
        break;
    default:
        break;
    }
    return QGraphicsItem::itemChange(change, value);
}

void VERectangle::resizeLeft(const QPointF &pt)
{
    QRectF tmpRect = rect();
    if( pt.x() > tmpRect.right() )
        return;
    qreal widthOffset =  ( pt.x() - tmpRect.right() );
    if( widthOffset > -10 )
        return;
    if( widthOffset < 0 ) {
        tmpRect.setWidth( -widthOffset );
//        emit signalResized(this, -widthOffset, 0);
    }
    else {
        tmpRect.setWidth( widthOffset );
//        emit signalResized(this, widthOffset, 0);
    }

//    emit signalResized(this, rect().width() - tmpRect.width(), 0, CornerFlags::Left);
//    emit signalResized(this, CornerFlags::Left);
    // Since it's a left side , the rectange will increase in size
    // but keeps the topLeft as it was
    tmpRect.translate( rect().width() - tmpRect.width() , 0 );
    prepareGeometryChange();
    setRect( tmpRect );
    update();
    setPositionGrabbers();

}

void VERectangle::resizeRight(const QPointF &pt)
{
    QRectF tmpRect = rect();
    if( pt.x() < tmpRect.left() )
        return;
    qreal widthOffset =  ( pt.x() - tmpRect.left() );
    if( widthOffset < 10 ) /// limit
        return;
    if( widthOffset < 10) {
        tmpRect.setWidth( -widthOffset );
//        emit signalResized(this, -widthOffset, 0);
    }
    else {
        tmpRect.setWidth( widthOffset );
//        emit signalResized(this, widthOffset, 0);
    }
//    emit signalResized(this, CornerFlags::Right);
//    emit signalResized(this, widthOffset, 0, CornerFlags::Right);
    prepareGeometryChange();
    setRect( tmpRect );
    update();
    setPositionGrabbers();

}

void VERectangle::resizeBottom(const QPointF &pt)
{
    QRectF tmpRect = rect();
    if( pt.y() < tmpRect.top() )
        return;
    qreal heightOffset =  ( pt.y() - tmpRect.top() );
    if( heightOffset < 11 ) { /// limit
        return;
    }
    if( heightOffset < 0) {
        tmpRect.setHeight( -heightOffset );
//        emit signalResized(this, 0, -heightOffset);
    }
    else {
        tmpRect.setHeight( heightOffset );
//        emit signalResized(this, 0, heightOffset);
    }

//    emit signalResized(this, widthOffset, 0, CornerFlags::Right);
    prepareGeometryChange();
    setRect( tmpRect );
    update();
    setPositionGrabbers();


}

void VERectangle::resizeTop(const QPointF &pt)
{
    QRectF tmpRect = rect();
    if( pt.y() > tmpRect.bottom() )
        return;
    qreal heightOffset =  ( pt.y() - tmpRect.bottom() );
    if( heightOffset > -11 ) /// limit
        return;
    if( heightOffset < 0) {
        tmpRect.setHeight( -heightOffset );
//        emit signalResized(this, 0, heightOffset);
    }
    else {
        tmpRect.setHeight( heightOffset );
//        emit signalResized(this, 0, heightOffset);
    }

//    emit signalResized(this, 0, rect().height() - tmpRect.height(), CornerFlags::Top);
    tmpRect.translate( 0 , rect().height() - tmpRect.height() );
    prepareGeometryChange();
    setRect( tmpRect );
    update();
    setPositionGrabbers();

}

void VERectangle::rotateItem(const QPointF &pt)
{
    QRectF tmpRect = rect();
    QPointF center = boundingRect().center();
    QPointF corner;
    switch (m_cornerFlags) {
    case TopLeft:
        corner = tmpRect.topLeft();
        break;
    case TopRight:
        corner = tmpRect.topRight();
        break;
    case BottomLeft:
        corner = tmpRect.bottomLeft();
        break;
    case BottomRight:
        corner = tmpRect.bottomRight();
        break;
    default:
        break;
    }

    QLineF lineToTarget(center,corner);
    QLineF lineToCursor(center, pt);
    qreal angleToTarget = ::acos(lineToTarget.dx() / lineToTarget.length());
    qreal angleToCursor = ::acos(lineToCursor.dx() / lineToCursor.length());

    if (lineToTarget.dy() < 0)
        angleToTarget = TwoPi - angleToTarget;
    angleToTarget = normalizeAngle((Pi - angleToTarget) + Pi / 2);

    if (lineToCursor.dy() < 0)
        angleToCursor = TwoPi - angleToCursor;
    angleToCursor = normalizeAngle((Pi - angleToCursor) + Pi / 2);

    auto resultAngle = angleToTarget - angleToCursor;

    QTransform trans = transform();
    // перемещение системы координат в центр
    trans.translate( center.x(), center.y());
    // вращение
    trans.rotateRadians(rotation() + resultAngle, Qt::ZAxis);
    // перемещение системы координат обратно
    trans.translate( -center.x(),  -center.y());
    setTransform(trans);
}

void VERectangle::setPositionGrabbers()
{
    QRectF tmpRect = rect();
    cornerGrabber[GrabberTop]->setPos(tmpRect.left() + tmpRect.width()/2, tmpRect.top());
    cornerGrabber[GrabberBottom]->setPos(tmpRect.left() + tmpRect.width()/2, tmpRect.bottom());
    cornerGrabber[GrabberLeft]->setPos(tmpRect.left(), tmpRect.top() + tmpRect.height()/2);
    cornerGrabber[GrabberRight]->setPos(tmpRect.right(), tmpRect.top() + tmpRect.height()/2);
    cornerGrabber[GrabberTopLeft]->setPos(tmpRect.topLeft().x(), tmpRect.topLeft().y());
    cornerGrabber[GrabberTopRight]->setPos(tmpRect.topRight().x(), tmpRect.topRight().y());
    cornerGrabber[GrabberBottomLeft]->setPos(tmpRect.bottomLeft().x(), tmpRect.bottomLeft().y());
    cornerGrabber[GrabberBottomRight]->setPos(tmpRect.bottomRight().x(), tmpRect.bottomRight().y());
}

void VERectangle::setVisibilityGrabbers()
{
    cornerGrabber[GrabberTopLeft]->setVisible(true);
    cornerGrabber[GrabberTopRight]->setVisible(true);
    cornerGrabber[GrabberBottomLeft]->setVisible(true);
    cornerGrabber[GrabberBottomRight]->setVisible(true);

    if(m_actionFlags == ResizeState){
        cornerGrabber[GrabberTop]->setVisible(true);
        cornerGrabber[GrabberBottom]->setVisible(true);
        cornerGrabber[GrabberLeft]->setVisible(true);
        cornerGrabber[GrabberRight]->setVisible(true);
    } else {
        cornerGrabber[GrabberTop]->setVisible(false);
        cornerGrabber[GrabberBottom]->setVisible(false);
        cornerGrabber[GrabberLeft]->setVisible(false);
        cornerGrabber[GrabberRight]->setVisible(false);
    }
}

void VERectangle::hideGrabbers()
{
    for(int i = 0; i < 8; i++){
        cornerGrabber[i]->setVisible(false);
    }
}
