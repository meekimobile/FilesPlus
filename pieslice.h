#ifndef PIESLICE_H
#define PIESLICE_H

#include <QtGui>

class PieSlice : public QGraphicsPolygonItem
{
public:
    static const int MaxAngleSpan;
    static const int ShowOptionalTextAngleSpan;
    static const int ShowLabelAngleSpan;
    static const int MaxLabelLength;

    PieSlice(QGraphicsItem *parent = 0);

    int modelIndex() const;
    void setModelIndex(const int index);

    QColor color() const;
    void setColor(const QColor color);

    int fromAngle() const;
    void setFromAngle(const int fromAngle);

    int angleSpan() const;
    void setAngleSpan(const int angleSpan);

    int radius() const;
    void setRadius(const int radius);

    QString text() const;
    void setText(const QString text);

    QString subText() const;
    void setSubText(const QString text);

    QString optionalText() const;
    void setOptionalText(const QString text);

    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    QPolygonF createPiePolygon();
    QRect getTextRect();
    QString getTextLabel();
    QPoint getTextCenterPoint();
    bool shouldShowLabel();
private:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w);
    QColor m_color;
    int m_fromAngle;
    int m_angleSpan;
    int m_radius;
    QString m_text;
    QString m_subText;
    QString m_optionalText;
    int m_index;

    QBrush m_tempBrush;

    int getTextRadius();
};

#endif

