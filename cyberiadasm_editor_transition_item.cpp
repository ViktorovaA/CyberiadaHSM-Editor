/* -----------------------------------------------------------------------------
 * The Cyberiada State Machine Editor
 * -----------------------------------------------------------------------------
 *
 * The State Machine Editor Transition Item
 *
 * Copyright (C) 2025 Anastasia Viktorova <viktorovaa.04@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 * ----------------------------------------------------------------------------- */

#include <cstdio>
#include <QPainter>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QtMath>
#include <QTextDocument>
#include <QTextOption>

#include "myassert.h"
#include "cyberiadasm_editor_transition_item.h"
#include "cyberiada_constants.h"
#include "cyberiadasm_editor_scene.h"

/* -----------------------------------------------------------------------------
 * Transition Item
 * ----------------------------------------------------------------------------- */

CyberiadaSMEditorTransitionItem::CyberiadaSMEditorTransitionItem(QObject *parent_object,
                                                                 CyberiadaSMModel *model,
                                                                 Cyberiada::Element *element,
                                                                 QGraphicsItem *parent,
                                                                 QMap<Cyberiada::ID, QGraphicsItem*>& elementItem) :
    CyberiadaSMEditorAbstractItem(model, element, parent),
    // QObject(parent_object),
    elementIdToItemMap(elementItem)
{
    setAcceptHoverEvents(true);
    setFlags(ItemIsSelectable|ItemSendsGeometryChanges);

    isMouseTraking = false;

    transition = static_cast<const Cyberiada::Transition*>(element);

    actionItem = new TransitionText(text(), this);
    actionItem->setTextInteractionFlags(Qt::TextEditorInteraction);

    connect(actionItem, &EditableTextItem::sizeChanged, this, &CyberiadaSMEditorTransitionItem::onSourceGeomertyChanged);
    connect(actionItem, &EditableTextItem::editingFinished, this, &CyberiadaSMEditorTransitionItem::onSourceGeomertyChanged);
    connect(source(), &CyberiadaSMEditorAbstractItem::geometryChanged, this, &CyberiadaSMEditorTransitionItem::onSourceGeomertyChanged);
    connect(target(), &CyberiadaSMEditorAbstractItem::geometryChanged, this, &CyberiadaSMEditorTransitionItem::onSourceGeomertyChanged);

    updateTextPosition();
    initializeDots();
    hideDots();
}

CyberiadaSMEditorTransitionItem::~CyberiadaSMEditorTransitionItem()
{
    if(actionItem) { delete actionItem; }

    if(!listDots.isEmpty()){
        foreach (DotSignal *dot, listDots) {
            dot->deleteLater();
        }
        listDots.clear();
    }
}

QRectF CyberiadaSMEditorTransitionItem::boundingRect() const
{
    MY_ASSERT(model);
    MY_ASSERT(model->rootDocument());
    return path().boundingRect().adjusted(-10, -10, 10, 10); // Увеличиваем область для стрелки
}

void CyberiadaSMEditorTransitionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{   QColor color(Qt::black);
    if (isSelected()) {
        color.setRgb(255, 0, 0);
    }
    painter->setPen(QPen(color, 2, Qt::SolidLine));
    painter->setBrush(Qt::NoBrush);

    painter->drawPath(path());

    drawArrow(painter);
}

QPainterPath CyberiadaSMEditorTransitionItem::shape() const
{
    QPainterPathStroker stroker;
    stroker.setWidth(5);
    return stroker.createStroke(path());
}

CyberiadaSMEditorAbstractItem *CyberiadaSMEditorTransitionItem::source() const
{
    // MY_ASSERT(scene());
    // CyberiadaSMEditorScene* editorScene = static_cast<CyberiadaSMEditorScene*>(scene());
    // return static_cast<CyberiadaSMEditorAbstractItem*>((editorScene->getMap()).value(m_transition->source_element_id()));

    return static_cast<CyberiadaSMEditorAbstractItem*>(elementIdToItemMap.value(transition->source_element_id()));
}

// void CyberiadaSMEditorTransitionItem::setSource(State *source)
// {
//     if (m_source == source) return;

//     m_source = source;
//     if (!m_source) return;

//     m_previousSourceCenterPos = m_source->sceneBoundingRect().center();
//     connect(m_source, &State::signalMove, this, &CyberiadaSMEditorTransitionItem::slotMoveSource);
//     connect(m_source, &State::signalResized, this, &CyberiadaSMEditorTransitionItem::slotStateResized);
// }


QPointF CyberiadaSMEditorTransitionItem::sourcePoint() const
{
    if (transition->has_geometry_source_point()) {
        return QPointF(transition->get_source_point().x, transition->get_source_point().y);
    }
    return QPointF(); // что по умолчанию, возвращать ошибку?

    // return m_previousSourcePos; //+
}

// void CyberiadaSMEditorTransitionItem::setSourcePoint(const QPointF &point)
// {
//     if(m_sourcePoint != point) {
//         m_sourcePoint = point;
//     }
//     updatePath();
// }

QPointF CyberiadaSMEditorTransitionItem::sourceCenter() const
{
    Cyberiada::ID id = transition->source_element_id();
    // Cyberiada::ElementType sourceElementType = model->idToElement(QString::fromStdString(id))->get_type();
    // if (sourceElementType == Cyberiada::elementCompositeState ||
    //     sourceElementType == Cyberiada::elementSimpleState)
    // {
    //     return (static_cast<Rectangle*>(m_elementItem->value(m_transition->source_element_id())))->sceneBoundingRect().center();
    // }
    // if (sourceElementType == Cyberiada::elementInitial ||
    //     sourceElementType == Cyberiada::elementFinal)
    // {
    //     return static_cast<CyberiadaSMEditorVertexItem*>(m_elementItem->value(m_transition->source_element_id()))->sceneBoundingRect().center();
    // }

    // CyberiadaSMEditorScene* editorScene = static_cast<CyberiadaSMEditorScene*>(scene());
    // if(!(editorScene->getMap()).value(m_transition->source_element_id())) return QPoint(); //костыль
    // MY_ASSERT((editorScene->getMap()).value(m_transition->source_element_id()));
    // return ((editorScene->getMap()).value(m_transition->source_element_id()))->sceneBoundingRect().center();

    if(!elementIdToItemMap.value(transition->source_element_id())) return QPoint(); //костыль
    MY_ASSERT(elementIdToItemMap.value(transition->source_element_id()));
    return (elementIdToItemMap.value(transition->source_element_id()))->sceneBoundingRect().center();

    // return m_previousSourceCenterPos; //+
}

CyberiadaSMEditorAbstractItem *CyberiadaSMEditorTransitionItem::target() const
{
    // CyberiadaSMEditorScene* editorScene = static_cast<CyberiadaSMEditorScene*>(scene());
    // return static_cast<CyberiadaSMEditorAbstractItem*>((editorScene->getMap()).value(m_transition->target_element_id()));

    return static_cast<CyberiadaSMEditorAbstractItem*>(elementIdToItemMap.value(transition->target_element_id()));
}

// void CyberiadaSMEditorTransitionItem::setTarget(State *target)
// {
//     if (m_target == target) return;

//     m_target = target;
//     if (!m_target) return;

//     m_previousTargetCenterPos = m_target->sceneBoundingRect().center();
//     connect(m_target, &State::signalMove, this, &CyberiadaSMEditorTransitionItem::slotMoveSource);
//     connect(m_target, &State::signalResized, this, &CyberiadaSMEditorTransitionItem::slotStateResized);
//     m_mouseTraking = false;
// }

QPointF CyberiadaSMEditorTransitionItem::targetPoint() const
{
    if (transition->has_geometry_target_point()) {
        return QPointF(transition->get_target_point().x, transition->get_target_point().y);
    }
    return QPointF(); // что по умолчанию, возвращать ошибку?

    // return m_previousTargetPos; //+
}

// void CyberiadaSMEditorTransitionItem::setTargetPoint(const QPointF &point)
// {
//     // TO DO (изменение точки модели)
//     updatePath();
// }


QPointF CyberiadaSMEditorTransitionItem::targetCenter() const
{
    // Cyberiada::ElementType targetElementType = model->idToElement(QString::fromStdString(m_transition->target_element_id()))->get_type();
    // if (targetElementType == Cyberiada::elementCompositeState ||
    //     targetElementType == Cyberiada::elementSimpleState){
    //     // return (static_cast<State*>(m_elementItem->value(m_transition->source_element_id())))->sceneBoundingRect().center();
    //     return (static_cast<Rectangle*>(m_elementItem->value(m_transition->source_element_id())))->sceneBoundingRect().center();
    // }
    // if (targetElementType == Cyberiada::elementInitial ||
    //     targetElementType == Cyberiada::elementFinal){
    //     return static_cast<CyberiadaSMEditorVertexItem*>(m_elementItem->value(m_transition->source_element_id()))->sceneBoundingRect().center();
    // }

    // CyberiadaSMEditorScene* editorScene = static_cast<CyberiadaSMEditorScene*>(scene());
    // if(!(editorScene->getMap()).value(m_transition->target_element_id())) return QPoint();   //костыль
    // return ((editorScene->getMap()).value(m_transition->target_element_id()))->sceneBoundingRect().center();

    if(!elementIdToItemMap.value(transition->target_element_id())) return QPoint(); //костыль
    MY_ASSERT(elementIdToItemMap.value(transition->target_element_id()));
    return (elementIdToItemMap.value(transition->target_element_id()))->sceneBoundingRect().center();

    // return m_previousTargetCenterPos; //+
}

QPainterPath CyberiadaSMEditorTransitionItem::path() const
{
    MY_ASSERT(model);
    QPainterPath path = QPainterPath();

    path.moveTo(sourcePoint() + sourceCenter());

    if(transition->has_polyline()) {
        for (const auto& point : transition->get_geometry_polyline()) {
            path.lineTo(QPointF(point.x, point.y) + sourceCenter());
        }
    }

    path.lineTo(targetPoint() + targetCenter());

    return path;
    // return m_path; //+
}

// void CyberiadaSMEditorTransitionItem::setPath(const QPainterPath &path)
// {
//     if (m_path != path) {  // Проверка на изменение
//         m_path = path;
//     }
//     update();
//     prepareGeometryChange();
// }

// void CyberiadaSMEditorTransitionItem::updatePath()
// {
//     // if (!m_source) return;
//     if (!m_transition->has_geometry_source_point()) return;

//     m_previousSourceCenterPos = source()->sceneBoundingRect().center();

//     if (m_transition->has_geometry_target_point()) {
//         m_previousTargetCenterPos = target()->sceneBoundingRect().center();
//     }

//     QPainterPath path = QPainterPath();
//     path.moveTo(sourcePoint() + m_previousSourceCenterPos);

//     if (!m_transition->has_polyline()) { // TODO change poliline drawing
//         for (int i = 0; i < m_points.size(); ++i) {
//             path.lineTo(m_points[i] + m_previousSourceCenterPos);
//         }
//     }

//     if (m_mouseTraking) {
//         path.lineTo(targetPoint());
//     }
//     if (m_transition->has_geometry_target_point()) {
//         path.lineTo(targetPoint() + m_previousTargetCenterPos);
//     }

//     setPath(path);
//     update();
//     prepareGeometryChange();
// }

void CyberiadaSMEditorTransitionItem::drawArrow(QPainter* painter)
{
    QPen pen(Qt::black, 1);
    if (isSelected()) {
        pen.setColor(Qt::red);
        pen.setWidth(2);
    }
    painter->setPen(pen);

    QPointF p1 = sourcePoint() + sourceCenter(); // Предпоследняя точка
    if(transition->has_polyline()) {
        Cyberiada::Polyline polyline = transition->get_geometry_polyline();
        Cyberiada::Point lastPolylinePoint = *(std::next(polyline.begin(), polyline.size() - 1));
        p1 = QPointF(lastPolylinePoint.x, lastPolylinePoint.y) + sourceCenter();
    }

    QPointF p2;
    if (isMouseTraking) {
        p2 = targetPoint(); // TODO ПОМЕНЯТЬ НА КУРСОР МЫШИ
    } else {
        p2 = targetPoint() + targetCenter(); // Последняя точка
    }

    // Вычисляем направление
    QLineF line(p1, p2);
    double angle = std::atan2(-line.dy(), line.dx());

    // Размер стрелки
    qreal arrowSize = 10.0;

    // Вычисляем точки треугольника
    QPointF arrowP1 = p2 + QPointF(-arrowSize * std::cos(angle - M_PI / 6),
                                   arrowSize * std::sin(angle - M_PI / 6));
    QPointF arrowP2 = p2 + QPointF(-arrowSize * std::cos(angle + M_PI / 6),
                                   arrowSize * std::sin(angle + M_PI / 6));

    // Рисуем треугольник
    QPolygonF arrowHead;
    arrowHead << p2 << arrowP1 << arrowP2;

    painter->setBrush(QBrush(pen.color()));
    painter->drawPolygon(arrowHead);
}

QVector<QPointF> CyberiadaSMEditorTransitionItem::points() const
{
    return m_points;
    // if(m_transition->has_polyline()) {
    //     for (const auto& point : m_transition->get_geometry_polyline()) {
    //         path.lineTo(QPointF(point.x, point.y) + sourceCenter());
    //     }
    // }
}

// void CyberiadaSMEditorTransitionItem::setPoints(const QVector<QPointF> &points)
// {
//     if (m_points != points) {  // Проверка на изменение
//         m_points = points;
//     }
//     updatePath();
// }

QString CyberiadaSMEditorTransitionItem::text() const
{
    if(transition->has_action() & transition->get_action().has_trigger()){
        return QString(transition->get_action().get_trigger().c_str());
    }
    return QString();
}

// void CyberiadaSMEditorTransitionItem::setText(const QString &text)
// {
// TO DO (запись новоро текста в модель)

// if (!m_actionItem) {
//     m_actionItem = new QGraphicsTextItem(this);
//     m_actionItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
//     m_actionItem->setFlag(QGraphicsItem::ItemIsFocusable, true); // Позволяет получать фокус
//     m_actionItem->setTextInteractionFlags(Qt::TextEditorInteraction); // Включает редактирование текста
//     //        connect(m_text, &QGraphicsTextItem::textChanged, this, &Edge::onTextChanged);
// }
// m_actionItem->setPlainText(text);
// updateTextPosition();
// }

// void CyberiadaSMEditorTransitionItem::setTextPosition(const QPointF &pos)
// {
//     if (m_textPosition != pos) {  // Проверка на изменение
//         m_textPosition = pos;
//     }
//     updateTextPosition();
// }

void CyberiadaSMEditorTransitionItem::updateTextPosition() {
    if (!actionItem) return;

    // // костыль
    // m_previousSourceCenterPos = (m_elementItem->value(m_transition->source_element_id()))->sceneBoundingRect().center();
    // m_previousSourcePos = QPointF(m_transition->get_source_point().x, m_transition->get_source_point().y);
    // m_previousTargetCenterPos = (m_elementItem->value(m_transition->target_element_id()))->sceneBoundingRect().center();
    // m_previousTargetPos = QPointF(m_transition->get_target_point().x, m_transition->get_target_point().y);

    // Установить позицию текста
    QPointF lastPoint = sourcePoint();
    if(transition->has_polyline() && transition->get_geometry_polyline().size() > 0) {
        Cyberiada::Polyline polyline = transition->get_geometry_polyline();
        Cyberiada::Point lastPolylinePoint = *(std::next(polyline.begin(), polyline.size() - 1));
        lastPoint = QPointF(lastPolylinePoint.x, lastPolylinePoint.y);
    }
    QPointF textPos = (lastPoint + (targetPoint() + targetCenter() - sourceCenter())) / 2 + sourceCenter() - actionItem->boundingRect().center();

    actionItem->setPos(textPos);

    QPainterPath path = QPainterPath();

    path.moveTo(sourcePoint() + sourceCenter());

    if(transition->has_polyline()) {
        for (const auto& point : transition->get_geometry_polyline()) {
            path.lineTo(QPointF(point.x, point.y) + sourceCenter());
        }
    }

    path.lineTo(targetPoint() + targetCenter());

    m_path = path;
    update();
}

void CyberiadaSMEditorTransitionItem::onSourceGeomertyChanged()
{
    prepareGeometryChange();
    updateTextPosition();
    setDotsPosition();
}

// QPointF CyberiadaSMEditorTransitionItem::findIntersectionWithRect(const State *state)
// {
//     if (!state) return QPointF();

//     // Получаем прямоугольник объекта в глобальных координатах
//     QRectF rect = state->sceneBoundingRect();

//     // Прямая между центрами
//     QLineF line(m_source->sceneBoundingRect().center(), m_target->sceneBoundingRect().center());

//     // Проверяем пересечения прямой с каждой стороной прямоугольника
//     QPointF intersection;
//     QLineF edges[4] = {
//         QLineF(rect.topLeft(), rect.topRight()),     // Верхняя грань
//         QLineF(rect.topRight(), rect.bottomRight()), // Правая грань
//         QLineF(rect.bottomRight(), rect.bottomLeft()), // Нижняя грань
//         QLineF(rect.bottomLeft(), rect.topLeft())    // Левая грань
//     };

//     for (const QLineF& edge : edges) {
//         if (line.intersects(edge, &intersection) == QLineF::BoundedIntersection) {
//             return intersection - state->sceneBoundingRect().center(); // Возвращаем первую найденную точку пересечения
//         }
//     }

//     return QPointF(); // Пересечений нет
// }

// void CyberiadaSMEditorTransitionItem::slotMoveSource(QGraphicsItem *item, qreal dx, qreal dy)
// {
//     updatePath();
//     updateTextPosition();
// }

// void CyberiadaSMEditorTransitionItem::updateCoordinates(State *state, State::CornerFlags side, QPointF *point, QPointF* previousCenterPos)
// {
//     if (point->x() < 0 && (side & State::CornerFlags::Left) == State::CornerFlags::Left) {
//         *point = *point + QPointF(state->sceneBoundingRect().center().x() - previousCenterPos->x(), 0);
//     }
//     if (point->x() > 0 && (side & State::CornerFlags::Left) == State::CornerFlags::Left) {
//         *point = *point - QPointF(state->sceneBoundingRect().center().x() - previousCenterPos->x(), 0);
//     }
//     if (point->x() < 0 && (side & State::CornerFlags::Right) == State::CornerFlags::Right) {
//         *point = *point - QPointF(state->sceneBoundingRect().center().x() - previousCenterPos->x(), 0);
//     }
//     if (point->x() > 0 && (side & State::CornerFlags::Right) == State::CornerFlags::Right) {
//         *point = *point + QPointF(state->sceneBoundingRect().center().x() - previousCenterPos->x(), 0);
//     }
//     if (point->y() < 0 && (side & State::CornerFlags::Top) == State::CornerFlags::Top) {
//         *point = *point + QPointF(0, state->sceneBoundingRect().center().y() - previousCenterPos->y());
//     }
//     if (point->y() > 0 && (side & State::CornerFlags::Top) == State::CornerFlags::Top) {
//         *point = *point - QPointF(0, state->sceneBoundingRect().center().y() - previousCenterPos->y());
//     }
//     if (point->y() < 0 && (side & State::CornerFlags::Bottom) == State::CornerFlags::Bottom) {
//         *point = *point - QPointF(0, state->sceneBoundingRect().center().y() - previousCenterPos->y());
//     }
//     if (point->y() > 0 && (side & State::CornerFlags::Bottom) == State::CornerFlags::Bottom) {
//         *point = *point + QPointF(0, state->sceneBoundingRect().center().y() - previousCenterPos->y());
//     }
// }


// void CyberiadaSMEditorTransitionItem::slotStateResized(State *state, State::CornerFlags side)
// {
//     if (state != m_source && state != m_target) return;

//     if (state == m_source) {
//         updateCoordinates(state, side, &m_sourcePoint, &m_previousSourceCenterPos);
//         if (!m_points.isEmpty()) {
//             for (int i = 0; i < m_points.size(); ++i) {
//                 updateCoordinates(state, side, &(m_points[i]), &m_previousSourceCenterPos);
//             }
//         }
//         //        updateCoordinates(state, side, &m_textPosition, &m_previousSourceCenterPos);
//     }
//     if (state == m_target) {
//         updateCoordinates(state, side, &m_targetPoint, &m_previousTargetCenterPos);
//         //        updateCoordinates(state, side, &m_textPosition, &m_previousSourceCenterPos);
//     }

//     updatePath();
//     updateTextPosition();
// }

void CyberiadaSMEditorTransitionItem::slotMoveDot(QGraphicsItem *signalOwner, qreal dx, qreal dy)
{
    if (!isSelected()) return;

    QPainterPath linePath = path();
    for(int i = 0; i < linePath.elementCount(); i++){
        if(listDots.at(i) == signalOwner){
            if(i == 0) {
                break;
            }
            if(i == linePath.elementCount() - 1) {
                break;
            }
            // TODO
            // m_points[i - 1] = QPointF(m_points[i - 1].x() + dx, m_points[i - 1].y() + dy);
            //            m_pointForCheck = i;
        }
    }
    // updatePath();
    updateTextPosition();
}

// void CyberiadaSMEditorTransitionItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
// {
//     QPointF clickPos = event->pos();
//     QLineF checkLineFirst(clickPos.x() - 5, clickPos.y() - 5, clickPos.x() + 5, clickPos.y() + 5);
//     QLineF checkLineSecond(clickPos.x() + 5, clickPos.y() - 5, clickPos.x() - 5, clickPos.y() + 5);
//     QPainterPath oldPath = path();
//     for(int i = 0; i < oldPath.elementCount() - 1; i++){
//         QLineF checkableLine(oldPath.elementAt(i), oldPath.elementAt(i+1));
//         if(checkableLine.intersect(checkLineFirst,0) == 1 || checkableLine.intersect(checkLineSecond,0) == 1){
//             m_points.insert(i, clickPos - m_source->sceneBoundingRect().center());
//         }
//     }
//     updatePath();
//     updateTextPosition();
//     updateDots();
//     QGraphicsItem::mouseDoubleClickEvent(event);
// }

void CyberiadaSMEditorTransitionItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (dynamic_cast<CyberiadaSMEditorScene*>(scene())->getCurrentTool() != ToolType::Select) {
        event->ignore();
        return;
    }

    showDots();
    if (event->button() & Qt::LeftButton) {
        isLeftMouseButtonPressed = true;
       //        setPreviousPosition(event->scenePos());
        emit clicked(this);
    }
    QGraphicsItem::mousePressEvent(event);
}

void CyberiadaSMEditorTransitionItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (dynamic_cast<CyberiadaSMEditorScene*>(scene())->getCurrentTool() != ToolType::Select) {
        event->ignore();
        return;
    }

   if (isLeftMouseButtonPressed) {
//        auto dx = event->scenePos().x() - m_previousPosition.x();
//        auto dy = event->scenePos().y() - m_previousPosition.y();
//        moveBy(dx,dy);
//        //        setPreviousPosition(event->scenePos());
//        emit signalMove(this, dx, dy);
   }
   QGraphicsItem::mouseMoveEvent(event);
}

// //void CyberiadaSMEditorTransitionItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
// //{
// //    if (event->button() & Qt::LeftButton) {
// //        m_leftMouseButtonPressed = false;
// //        qDebug() << "release on edge";
// //    }
// //    QGraphicsItem::mouseReleaseEvent(event);
// //}


void CyberiadaSMEditorTransitionItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (dynamic_cast<CyberiadaSMEditorScene*>(scene())->getCurrentTool() != ToolType::Select) {
        event->ignore();
        return;
    }

    if (isSelected()) {
        showDots();
        setDotsPosition();
    }
    QGraphicsItem::hoverEnterEvent(event);
}

void CyberiadaSMEditorTransitionItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (dynamic_cast<CyberiadaSMEditorScene*>(scene())->getCurrentTool() != ToolType::Select) {
        event->ignore();
        return;
    }

    QGraphicsItem::hoverMoveEvent(event);
}

void CyberiadaSMEditorTransitionItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (dynamic_cast<CyberiadaSMEditorScene*>(scene())->getCurrentTool() != ToolType::Select) {
        event->ignore();
        return;
    }

    hideDots();
    QGraphicsItem::hoverLeaveEvent(event);
}

void CyberiadaSMEditorTransitionItem::initializeDots()
{
    QPainterPath linePath = path();
    for(int i = 0; i < linePath.elementCount(); i++) {
        QPointF point = linePath.elementAt(i);
        DotSignal *dot = new DotSignal(point, this);
        connect(dot, &DotSignal::signalMove, this, &CyberiadaSMEditorTransitionItem::slotMoveDot);
        //        connect(dot, &DotSignal::signalMouseRelease, this, &CyberiadaSMEditorTransitionItem::checkForDeletePoints);
        dot->setDotFlags(DotSignal::Movable);
        listDots.append(dot);
    }
}

void CyberiadaSMEditorTransitionItem::updateDots()
{
    if(!listDots.isEmpty()) {
        foreach (DotSignal *dot, listDots) {
            dot->deleteLater();
        }
        listDots.clear();
    }

    QPainterPath linePath = path();
    for(int i = 0; i < linePath.elementCount(); i++) {
        QPointF point = linePath.elementAt(i);
        DotSignal *dot = new DotSignal(point, this);
        connect(dot, &DotSignal::signalMove, this, &CyberiadaSMEditorTransitionItem::slotMoveDot);
        //        connect(dot, &DotSignal::signalMouseRelease, this, &CyberiadaSMEditorTransitionItem::checkForDeletePoints);
        dot->setDotFlags(DotSignal::Movable);
        listDots.append(dot);
    }
}

void CyberiadaSMEditorTransitionItem::showDots()
{
    foreach( DotSignal* dot, listDots ) {
        dot->setVisible(true);
    }
}

void CyberiadaSMEditorTransitionItem::hideDots()
{
    foreach( DotSignal* dot, listDots ) {
        dot->setVisible(false);
    }
}

void CyberiadaSMEditorTransitionItem::setDotsPosition()
{
    QPainterPath linePath = path();
    for(int i = 0; i < linePath.elementCount(); i++) {
        QPointF point = linePath.elementAt(i);
        listDots.at(i)->setPos(point);
    }
}

void CyberiadaSMEditorTransitionItem::setPointPos(int pointIndex, qreal dx, qreal dy)
{
    // TODO
    QModelIndex ind = model->elementToIndex(element);
    // model->setData(&ind, )
}


/* -----------------------------------------------------------------------------
 * Transition Text Item
 * ----------------------------------------------------------------------------- */

TransitionText::TransitionText(const QString &text, QGraphicsItem *parent) :
    EditableTextItem(text, parent)
{
    setTextWidthEnabled(false);
    setTextAlignment(Qt::AlignCenter);
    setTextMargin(0);
}

void TransitionText::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w) {
    if (!toPlainText().isEmpty())
        painter->fillRect(boundingRect(), painter->background());
    QGraphicsTextItem::paint(painter, o, w);
}
