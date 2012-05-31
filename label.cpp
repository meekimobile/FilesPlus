#include "label.h"

Label::Label(QGraphicsItem *parent) :
    QGraphicsTextItem(parent)
{
    initialize();
}

Label::Label(const QString &text, QGraphicsItem *parent) :
    QGraphicsTextItem(text, parent)
{
    initialize();
}

void Label::initialize() {
    m_borderColor = QColor(0,0,0,64);
    m_backgroundColor = QColor(0,0,0,64);
}

void Label::setRadius(const int radius)
{
    m_radius = radius;
}

const int Label::getRadius()
{
    return m_radius;
}

void Label::setBorderColor(const QColor &color)
{
    m_borderColor = color;
}

const QColor Label::getBorderColor()
{
    return m_borderColor;
}

void Label::setBackgroundColor(const QColor &color)
{
    m_backgroundColor = color;
}

const QColor Label::getBackgroundColor()
{
    return m_backgroundColor;
}

void Label::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Draw text rect.
    painter->setPen(QPen(getBorderColor()));
    painter->setBrush(QBrush(getBackgroundColor()));
    painter->drawRoundedRect(boundingRect(), getRadius(), getRadius());

    // Invoke superclass method.
    QGraphicsTextItem::paint(painter, option, widget);
}
