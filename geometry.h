#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <QPointF>
#include <QRectF>
#include <QPolygonF>

QRectF addBBox(QRectF r1, QRectF r2);
bool inBox(const QPointF &p, const QRectF &box);

QPointF normalize (const QPointF &p);


qreal dotProduct (const QPointF &a, const QPointF &b);

class PolygonCollisionResult {
public:
    // Are the polygons going to intersect forward in time?
    bool willIntersect;

    // Are the polygons currently intersecting?
    bool intersect;

    // The translation to apply to the first polygon to push the polygons apart.
    QPointF minTranslation;
};


void ProjectPolygon(QPointF axis, QPolygonF polygon, qreal &min, qreal &max) ;
qreal intervalDistance(qreal minA, qreal maxA, qreal minB, qreal maxB);
PolygonCollisionResult PolygonCollision(QPolygonF polygonA, 
                              QPolygonF polygonB, QPointF velocity);

#endif
