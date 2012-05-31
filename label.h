#ifndef LABEL_H
#define LABEL_H

#include <QtGui>

class Label : public QGraphicsTextItem
{
    Q_OBJECT
public:
    Label(QGraphicsItem *parent = 0);
    Label(const QString &text, QGraphicsItem *parent = 0);
    
    void setRadius(const int radius);
    const int getRadius();

    void setBorderColor(const QColor &color);
    const QColor getBorderColor();

    void setBackgroundColor(const QColor &color);
    const QColor getBackgroundColor();
signals:
    
public slots:
    
private:
    int m_radius;
    QColor m_borderColor;
    QColor m_backgroundColor;

    void initialize();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
};

#endif // LABEL_H
