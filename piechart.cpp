#include "piechart.h"
#include "pieslice.h"
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QDeclarativeItem>

//const QColor PieChart::sliceColors[8] = {Qt::cyan, Qt::magenta, Qt::yellow, Qt::red, Qt::green, Qt::blue, QColor("orchid"), QColor("orange")};
const QColor PieChart::highlightSliceColor = QColor("#0084ff");
const QString PieChart::sliceColorsCSV = "cyan,magenta,yellow,red,green,blue,orchid,orange";

qreal PieChart::radians(qreal degrees) {
    return float(degrees) * 3.1415927 / 180;
}

PieChart::PieChart(QDeclarativeItem *parent)
    : QDeclarativeItem(parent)
{
    // need to disable this QGraphicsItem flag to draw inside a QDeclarativeItem
//    setFlag(QGraphicsItem::ItemHasNoContents, false);

    // Make PieChart item to accept mouse event.
    setAcceptedMouseButtons(Qt::LeftButton);

    // Set polling timer for checking if scene is active.
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(checkSceneIsActive()));
    timer->start(1000);

    // Initialize default colors.
    setSliceColorsCSV(sliceColorsCSV);
}

void PieChart::checkSceneIsActive() {
    if (scene()) {
        if (scene()->isActive()) {
            emit sceneActivated();
            timer->stop();
        }
    }
}

QString PieChart::name() const
{
    return m_name;
}

void PieChart::setName(const QString &name)
{
    m_name = name;
}

int PieChart::getRole(const QAbstractListModel *model, QString roleName) {
    int sizeRole;
    foreach (int key, model->roleNames().keys()) {
        if (model->roleNames().value(key) == roleName) {
            sizeRole = key;
            break;
        }
    }

    return sizeRole;
}

QString PieChart::formatFileSize(double size, int len) {
    const uint KB = 1024;
    const uint MB = 1048576;
    const uint GB = 1073741824;

    QString fileSize;
    int pow = qPow(10, len);

    if (size >= GB) {
        fileSize = QString("%1 GB").arg( round(pow * size/GB)/pow );
    } else if (size >= MB) {
        fileSize = QString("%1 MB").arg( round(pow * size/MB)/pow );
    } else if (size >= 1024) {
        fileSize = QString("%1 KB").arg( round(pow * size/KB)/pow );
    } else {
        fileSize = QString("%1").arg( size );
    }

    return fileSize;
}

void PieChart::refreshItems(bool forceRefresh)
{
//    qDebug() << "PieChart::refreshItems isActive()" << isActive() << "visible()" << visible() << "forceRefresh" << forceRefresh;
    if ((isActive() && visible()) || forceRefresh) {
        removeAllExistingItems();
        createItemFromModel();
        createLabelFromSlices();
        update();
    } else {
        removeAllExistingItems();
        update();
    }
}

void PieChart::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
//    qDebug() << QTime::currentTime() << "PieChart::mousePressEvent event.pos " << event->pos();
    emit chartClicked(event);
}

bool PieChart::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    // Emit signal once it filter required events.
    int isDirRole = getRole(model(), "isDir");
    int nameRole = getRole(model(), "name");

    if (event->type() == QEvent::GraphicsSceneMousePress) {
        qDebug() << QTime::currentTime() << "PieChart::sceneEventFilter GraphicsSceneMousePress begin " << pressTime;

        pressTime.start();
        pressEvent = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
    } else if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        qDebug() << QTime::currentTime() << "PieChart::sceneEventFilter GraphicsSceneMouseRelease begin " << pressTime << ", elapsed " << pressTime.elapsed();

        QGraphicsSceneMouseEvent *ge = dynamic_cast<QGraphicsSceneMouseEvent *>(event);
        PieSlice *slice = dynamic_cast<PieSlice *>(watched);

        // Files always have modelIndex = -1. Respond only if modelIndex >= 0.
        if (slice->modelIndex() >= 0) {
            // If release in slice, invoke slice's release.
            if (slice->contains(ge->pos())) {
                // Can't use index from childItems() for refering as model index.
                QVariant vIsDir = model()->data(model()->index(slice->modelIndex(), 0), isDirRole);
                QVariant vName = model()->data(model()->index(slice->modelIndex(), 0), nameRole);
                qDebug() << "PieChart::sceneEventFilter watched slice modelIndex" << slice->modelIndex() << "label" << slice->getTextLabel() << "name" << vName.toString() << "isDir" << vIsDir.toBool();

                slice->mouseReleaseEvent(ge);

                emit sliceClicked(slice->text(), slice->modelIndex(), vIsDir.toBool());
            } else {
                // TODO below code causes crash on device.

    //            qDebug() << QTime::currentTime() << "PieChart::sceneEventFilter swipe " << pressEvent->pos() << " -> " << ge->pos();

    //            qreal dx = pressEvent->pos().x() - ge->pos().x();
    //            qreal dy = pressEvent->pos().y() - ge->pos().y();

    //            qreal angle = qAtan(dy / dx) / 3.1416 * 180;

                slice->mouseReleaseEvent(ge);

    //            emit swipe(angle);
            }
        } else {
            qDebug() << "PieChart::sceneEventFilter watched slice modelIndex" << slice->modelIndex() << "label" << slice->getTextLabel() << " is for files. Suppress event.";

            slice->mouseReleaseEvent(ge);

            // Emit click for files's pie slice.
            emit sliceClicked(slice->text(), slice->modelIndex(), false);
        }

        // Stop propagating.
        return true;
    }

    // return false to indicate that the event should be propagated further.
    // Issues: return false to propagate to below items will cause unexpected errors.
    return false;
}

void PieChart::modelDataChanged(QModelIndex topLeft, QModelIndex bottomRight)
{
//    qDebug() << QTime::currentTime() << "PieChart::modelDataChanged " << topLeft << ", " << bottomRight;

    refreshItems();
}

void PieChart::classBegin()
{
//    qDebug() <<  QTime::currentTime() << "PieChart::classBegin";
}

void PieChart::componentComplete()
{
//    qDebug() <<  QTime::currentTime() << "PieChart::componentComplete scene = " << scene();
}

QAbstractListModel *PieChart::model() const
{
    return m_model;
}

void PieChart::setModel(QAbstractListModel *model)
{
    if (model) {
        qDebug() << QTime::currentTime() << "PieChart::setModel rowCount " << model->rowCount();

        m_model = model;

        connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                this, SLOT(modelDataChanged(QModelIndex,QModelIndex)));

        refreshItems();
    } else {
        qDebug() << QTime::currentTime() << "PieChart::setModel model is not available.";
    }
}

bool PieChart::visible() const
{
    return m_visible;
}

void PieChart::setVisible(const bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        refreshItems();
    }
}

QString PieChart::labelFontDesc() const
{
    return m_labelFont.toString();
}

bool PieChart::setLabelFontDesc(const QString fontDesc)
{
//    qDebug() << "PieChart::setLabelFontDesc m_labelFont" << m_labelFont.toString();
    bool res = m_labelFont.fromString(fontDesc);
//    qDebug() << "PieChart::setLabelFontDesc m_labelFont" << m_labelFont.toString();

    return res;
}

QString PieChart::getSliceColorsCSV()
{
    return m_sliceColorsCSV;
}

void PieChart::setSliceColorsCSV(QString colorNamesCSV)
{
    qDebug() << "PieChart::setSliceColorsCSV colorNamesCSV" << colorNamesCSV << "m_sliceColorsCSV" << m_sliceColorsCSV;

    if (colorNamesCSV == "") return;
    if (m_sliceColorsCSV == colorNamesCSV) return;

    // Set and parse colors.
    m_sliceColorsCSV = colorNamesCSV;
    m_sliceColorList.clear();
    foreach (QString colorCode, m_sliceColorsCSV.split(",")) {
        m_sliceColorList.append(QColor(colorCode.trimmed()));
    }

    qDebug() << "PieChart::setSliceColorsCSV m_sliceColorList" << m_sliceColorList;
    emit sliceColorsChanged();
}

void PieChart::paint(QPainter *painter, const QStyleOptionGraphicsItem * o, QWidget * w)
{
    // Paint every frames while flipping. The longer animation invoke more paint.
}

void PieChart::removeAllExistingItems() {
    // Remove existing slices from scene and list.
    if (!childItems().empty()) {
        qDebug() << QTime::currentTime() << "PieChart::removeAllExistingItems childItems.count " << childItems().count() << " scene item count " << scene()->items().count();
        foreach (QGraphicsItem *slice, childItems()) {
            slice->removeSceneEventFilter(this);
            scene()->removeItem(slice);
        }
        update();

        qDebug() << QTime::currentTime() << "PieChart::removeAllExistingItems removed childItems.count " << childItems().count() << " scene item count " << scene()->items().count();
    }
}

void PieChart::createItemFromModel()
{
    qDebug() << QTime::currentTime() << "PieChart::createItemFromModel started ";

    int radius = (boundingRect().width() < boundingRect().height()) ? (boundingRect().width() / 2) : (boundingRect().height() / 2);
    qDebug() << QTime::currentTime() << "PieChart::createItemFromModel radius " << radius;

    // Create pieSlice from model
    int sizeRole = getRole(model(), "size");
    int nameRole = getRole(model(), "name");
    int isDirRole = getRole(model(), "isDir");
    int subDirCountRole = getRole(model(), "subDirCount");
    int subFileCountRole = getRole(model(), "subFileCount");
    int lastModifiedRole = getRole(model(), "lastModified");

    double totalSize = 0;
    int i = 0;
    qreal z = 0;
    for (i=0; i<model()->rowCount(); i++) {
        QVariant v = model()->data(model()->index(i,0), sizeRole);
        totalSize += v.toDouble();
    }

    // Create slice for each dir.
    double currentAngle = 0;
    double currentAngleSpan;
    i=0;
    while (i<model()->rowCount()) {
        double size = (model()->data(model()->index(i,0), sizeRole)).toDouble();
        QString name = (model()->data(model()->index(i,0), nameRole)).toString();
        bool isDir = (model()->data(model()->index(i,0), isDirRole)).toBool();
        double subDirCount = (model()->data(model()->index(i,0), subDirCountRole)).toDouble();
        double subFileCount = (model()->data(model()->index(i,0), subFileCountRole)).toDouble();
        QDateTime lastModified = (model()->data(model()->index(i,0), lastModifiedRole)).toDateTime();

        if (!isDir) break;

        currentAngleSpan = (size / totalSize) * 360.0;

        PieSlice *slice;
//        qDebug() << QTime::currentTime() << "PieChart::createItemFromModel creating PieSlice " << QString("name %1 size %2").arg(name).arg(size) ;

        // To avoid unexpected exit with error code, MUST create item with default parent.
        // Then assign parent after it's constructed.
        slice = new PieSlice(this);
        slice->setModelIndex(i);
        slice->setColor(m_sliceColorList.at(i % m_sliceColorList.length()));
        slice->setText(name);
        slice->setSubText(formatFileSize(size, 1));
        QString oText = "";
        if (subDirCount > 0) oText += tr("%n dir(s)", "", subDirCount);
        if (subFileCount > 0) oText += ((oText == "") ? "" : " ") + tr("%n file(s)", "", subFileCount);
        oText += ((oText == "") ? "" : "<br>") + lastModified.toString("d MMM yyyy h:mm:ss ap");
        slice->setOptionalText(oText);
        slice->setFromAngle(int(currentAngle));
        slice->setAngleSpan(int(currentAngleSpan));
        slice->setRadius(radius);

        // Set QGraphicsPolygonItem properties.
        slice->setPolygon(slice->createPiePolygon());
        slice->setPen(QPen(slice->color().darker(150.0), 2));
        QRadialGradient grad(slice->polygon().first().toPoint() , slice->radius());
        grad.setColorAt(0, slice->color());
        grad.setColorAt(1, slice->pen().color());
        slice->setBrush(QBrush(grad));
        slice->setZValue(z);
        slice->setParentItem(this);

        // PieSlice's event will be filtered by PieChart sceneEventFilter.
        slice->installSceneEventFilter(this);

        currentAngle += currentAngleSpan;
        i++;
    }

    // Merge all files into 1 slice.
    double totalfileSize = 0;
    int totalFiles = 0;
    while (i<model()->rowCount()) {
        double size = (model()->data(model()->index(i,0), sizeRole)).toDouble();
        bool isDir = (model()->data(model()->index(i,0), isDirRole)).toBool();

        if (!isDir) {
            totalfileSize += size;
            totalFiles++;
        }

        i++;
    }
    PieSlice *slice;
    slice = new PieSlice(this);
    slice->setModelIndex(-1); // always use -1 for files.
    slice->setColor(Qt::gray);
    slice->setText(tr("%n file(s)", "disambiguation", totalFiles));
    slice->setSubText(formatFileSize(totalfileSize, 1));
    slice->setOptionalText("");
    slice->setFromAngle(int(currentAngle));
    slice->setAngleSpan(int(totalfileSize / totalSize * 360));
    slice->setRadius(radius);
    slice->setPolygon(slice->createPiePolygon());
    slice->setPen(QPen(slice->color().darker(150.0), 2));
    QRadialGradient grad(slice->polygon().first().toPoint() , slice->radius());
    grad.setColorAt(0, slice->color());
    grad.setColorAt(1, slice->pen().color());
    slice->setBrush(QBrush(grad));
    slice->setZValue(z);
    slice->setParentItem(this);

    // PieSlice's event will be filtered by PieChart sceneEventFilter.
//        qDebug() << QTime::currentTime() << "PieChart::createItemFromModel installSceneEventFilter";
    slice->installSceneEventFilter(this);

    qDebug() << QTime::currentTime() << "PieChart::createItemFromModel childItems count = " << childItems().count();
}

void PieChart::createLabelFromSlices() {
    foreach (QGraphicsItem *item, childItems()) {
        PieSlice *slice = dynamic_cast<PieSlice *>(item);
        createLabelFromSlice(slice);
    }
}

void PieChart::createLabelFromSlice(PieSlice *slice)
{
    if (!slice->shouldShowLabel()) return;

    qreal cx = boundingRect().center().x();
    qreal cy = boundingRect().center().y();

    // Create label for slice.
    QPoint textCenter = slice->getTextCenterPoint();
    QString textLabel = slice->getTextLabel();
    QRectF sliceRect = slice->boundingRect();

    // Draw text rect.
    Label *label = new Label();
    label->setHtml(textLabel);
    label->setFont(m_labelFont);
    label->setDefaultTextColor(Qt::white);
    label->setRadius(5);
    label->setZValue(1);
    label->setParentItem(this);

    // Calculate topleft position
    qreal px = cx + textCenter.x() - (label->boundingRect().width() / 2);
    qreal py = cy + textCenter.y() - (label->boundingRect().height() / 2);

    QRectF labelRect = label->boundingRect();
    if (sliceRect.width() < labelRect.width())  {
        if (textCenter.x() < 0) {
            // Move to left of position
            px = cx + textCenter.x() - label->boundingRect().width();
        } else {
            // Move to left of position
            px = cx + textCenter.x();
        }
    }

    // Adjust pos to be in scene.
    px = (px < 0) ? 0 : px;
    px = (px + label->boundingRect().width() > scene()->sceneRect().right()) ? (scene()->sceneRect().right() - label->boundingRect().width()) : px;
    py = (py < 0) ? 0 : py;
    py = (py + label->boundingRect().height() > scene()->sceneRect().bottom()) ? (scene()->sceneRect().bottom() - label->boundingRect().height()) : py;

    label->setPos(px, py);
}
