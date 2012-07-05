#ifndef PIECHART_H
#define PIECHART_H

#include <QDeclarativeItem>
#include <QDeclarativeListProperty>
#include <QAbstractListModel>
#include <QPainter>
#include <QGraphicsScene>
#include "pieslice.h"
#include "label.h"

class PieChart : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QAbstractListModel * model READ model WRITE setModel)
    Q_PROPERTY(bool visible READ visible WRITE setVisible)
    Q_PROPERTY(QString labelFont READ labelFontDesc WRITE setLabelFontDesc)
public:
    static const QColor sliceColors[];
    static const QColor highlightSliceColor;
    static qreal radians(qreal degrees);

    PieChart(QDeclarativeItem *parent = 0);

    QString name() const;
    void setName(const QString &name);

    QAbstractListModel *model() const;
    void setModel(QAbstractListModel *model);

    bool visible() const;
    void setVisible(const bool visible);

    QString labelFontDesc() const;
    bool setLabelFontDesc(const QString fontDesc);

    int getRole(const QAbstractListModel *model, QString sizeRoleName);

    void classBegin();
    void componentComplete();

    QString formatFileSize(double size, int len = 0);

    Q_INVOKABLE void refreshItems();
protected slots:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);
    void modelDataChanged(QModelIndex topLeft,QModelIndex bottomRight);
    void checkSceneIsActive();
signals:
    void chartClicked(QGraphicsSceneMouseEvent *event);
    void sliceClicked(QString text, int index, bool isDir);
    void sceneActivated();
    void swipe(qreal swipeAngle);
private:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *o, QWidget *w);
    void removeAllExistingItems();
    void createItemFromModel();
    void createLabelFromSlice(PieSlice *slice);
    void createLabelFromSlices();

    QGraphicsSceneMouseEvent *pressEvent;
    QTime pressTime;
    QTimer *timer;

    QString m_name;
    QAbstractListModel *m_model;
    bool m_visible;
    QFont m_labelFont;
};

#endif

