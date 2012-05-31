#include "pieslice.h"
#include "piechart.h"
#include <QDebug>

const int PieSlice::MaxAngleSpan = 360;
const int PieSlice::ShowOptionalTextAngleSpan = 60;
const int PieSlice::ShowLabelAngleSpan = 30;
const int PieSlice::MaxLabelLength = 15;

PieSlice::PieSlice(QGraphicsItem *parent) : QGraphicsPolygonItem(parent)
{
    QRectF rect = parentItem()->boundingRect();
    m_radius = rect.width() / 2;

}

int PieSlice::modelIndex() const
{
    return m_index;
}

void PieSlice::setModelIndex(const int index)
{
    m_index = index;
}

QColor PieSlice::color() const
{
    return m_color;
}

void PieSlice::setColor(const QColor color)
{
    m_color = color;
}

int PieSlice::fromAngle() const
{
    return m_fromAngle;
}

void PieSlice::setFromAngle(const int fromAngle)
{
    m_fromAngle = fromAngle;
}

int PieSlice::angleSpan() const
{
    return m_angleSpan;
}

void PieSlice::setAngleSpan(const int angleSpan)
{
    m_angleSpan = angleSpan;
}

int PieSlice::radius() const
{
    return m_radius;
}

void PieSlice::setRadius(const int radius)
{
    m_radius = radius;
}

QString PieSlice::text() const
{
    return m_text;
}

void PieSlice::setText(const QString text)
{
    m_text = text;
}

QString PieSlice::subText() const
{
    return m_subText;
}

void PieSlice::setSubText(const QString text)
{
    m_subText = text;
}

QString PieSlice::optionalText() const
{
    return m_optionalText;
}

void PieSlice::setOptionalText(const QString text)
{
    m_optionalText = text;
}

void PieSlice::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
//    qDebug() << "PieSlice::mousePressEvent event.pos " << event->pos();
    m_tempBrush = brush();

    QRadialGradient grad(polygon().first().toPoint(), radius());
    grad.setColorAt(0, Qt::white);
    grad.setColorAt(1, PieChart::highlightSliceColor);
    setBrush(QBrush(grad));
}

void PieSlice::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
//    qDebug() << "PieSlice::mouseReleaseEvent event.pos " << event->pos();
    setBrush(m_tempBrush);
}

void PieSlice::paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w)
{
//    qDebug() << "PieSlice::paint";

    painter->setRenderHint(QPainter::Antialiasing);

    QGraphicsPolygonItem::paint(painter, o, w);
}

int PieSlice::getTextRadius() {
    int rText;

    if (m_angleSpan >= MaxAngleSpan)
        rText = 0;
    else if (m_angleSpan >= ShowOptionalTextAngleSpan)
        rText = m_radius * 0.6;
    else if (m_angleSpan >= ShowLabelAngleSpan)
        rText = m_radius * 0.8;
    else
        rText = m_radius;

    return rText;
}

bool PieSlice::shouldShowLabel() {
    return (m_angleSpan >= ShowLabelAngleSpan);
}

QPoint PieSlice::getTextCenterPoint() {
    qreal r = PieChart::radians(m_fromAngle + (m_angleSpan / 2));
    int x1 = getTextRadius() * qCos(r);
    int y1 = getTextRadius() * qSin(r);
    return QPoint(x1, y1);
}

QRect PieSlice::getTextRect() {
    qreal r = PieChart::radians(m_fromAngle + (m_angleSpan / 2));
    int x1 = getTextRadius() * qCos(r);
    int y1 = getTextRadius() * qSin(r);
    int x2 = (radius()) * qCos(r);
    int y2 = (radius()) * qSin(r);
    QRect textRect = QRect(QPoint(x1, y1), QPoint(x2, y2));

    return textRect;
}

QString PieSlice::getTextLabel() {
    QString label;
    label += "<b>" + ( (text().size() > MaxLabelLength) ? (text().left(MaxLabelLength) + "...") : text() ) + "</b>";
    label += "<br>" + ((subText().size() > MaxLabelLength) ? (subText().left(MaxLabelLength) + "...") : subText());
    if (m_angleSpan >= ShowOptionalTextAngleSpan) {
        label += (optionalText().isEmpty()) ? "" : ("<br>" + optionalText());
    }

    return label;
}

QPolygonF PieSlice::createPiePolygon()
{
    QRectF rect = parentItem()->boundingRect();

    int r = radius();
    int cx = rect.center().x();
    int cy = rect.center().y();

    qreal rStart = PieChart::radians(m_fromAngle);
    qreal rEnd = PieChart::radians(m_fromAngle + m_angleSpan);
    qreal rStep = PieChart::radians(5);
    QPolygonF polygon;
    polygon << rect.center();

    while (rStart <= rEnd) {
        qreal x2 = cx + r * qCos(rStart);
        qreal y2 = cy + r * qSin(rStart);
        polygon << QPointF(x2, y2);

        rStart += rStep;
    }
    qreal x2 = cx + r * qCos(rEnd);
    qreal y2 = cy + r * qSin(rEnd);
    polygon << QPointF(x2, y2);

    return polygon;
}
