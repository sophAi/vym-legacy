#include "geometry.h"

#include <math.h>
#include <iostream>
#include "misc.h"

using namespace std;

QRectF addBBox(QRectF r1, QRectF r2)
{	
	// Find smallest QRectF containing given rectangles

	QRectF n;
	// Set left border
	if (r1.left() <= r2.left() )
		n.setLeft(r1.left() );
	else
		n.setLeft(r2.left() );
		
	// Set top border		
	if (r1.top() <= r2.top() )
		n.setTop(r1.top() );
	else
		n.setTop(r2.top() );
		
	// Set right border
	if (r1.right() <= r2.right() )
		n.setRight(r2.right() );
	else
		n.setRight(r1.right() );
		
	// Set bottom 
	if (r1.bottom() <= r2.bottom() )
		n.setBottom(r2.bottom() );
	else
		n.setBottom(r1.bottom() );
	return n;
}

bool inBox(const QPointF &p, const QRectF &box)
{
    if (p.x() >= box.left() && p.x() <= box.right()  
	&& p.y() <= box.bottom() && p.y() >= box.top() )
		return true;
    return false;	
}

QPointF normalize (const QPointF &p)
{
	if (p==QPointF(0,0)) return p;
	qreal l=sqrt ( p.x()*p.x() + p.y()*p.y() );
	return QPointF (p.x()/l,p.y()/l);
}

// Dot product of two vectors
qreal dotProduct (const QPointF &a, const QPointF &b)
{
	return a.x()*b.x() + a.y()*b.y();
}


/* Calculate the projection of a polygon on an axis
   and returns it as a [min, max] interval
*/
void ProjectPolygon(QPointF axis, QPolygonF polygon, qreal &min, qreal &max) 
{
    // To project a point on an axis use the dot product

    qreal d = dotProduct(axis,polygon.at(0));
    min = d;
    max = d;
    for (int i = 0; i < polygon.size(); i++) {
        d= dotProduct (polygon.at(i),axis);
        if (d < min) 
            min = d;
        else 
		{
            if (d> max) max = d;
        }
    }
}

/* Calculate the signed distance between [minA, maxA] and [minB, maxB]
   The distance will be negative if the intervals overlap
*/


qreal intervalDistance(qreal minA, qreal maxA, qreal minB, qreal maxB) {
    if (minA < minB) {
        return minB - maxA;
    } else {
        return minA - maxB;
    }
}
/*
 Check if polygon A is going to collide with polygon B.
 The last parameter is the *relative* velocity 
 of the polygons (i.e. velocityA - velocityB)

*/
PolygonCollisionResult PolygonCollision(QPolygonF polygonA, 
                              QPolygonF polygonB, QPointF velocity) {
    PolygonCollisionResult result;
    result.intersect = true;
    result.willIntersect = true;

    int edgeCountA = polygonA.size();
    int edgeCountB = polygonB.size();
    qreal minIntervalDistance = 1000000000;
    QPointF translationAxis;
    QPointF edge;

	cout << "\nA: ";
	for (int k=0; k<edgeCountA;k++)
		cout <<polygonA.at(k);
	cout << "\nB: ";
	for (int k=0; k<edgeCountB;k++)
		cout <<polygonB.at(k);
		
		
    // Loop through all the edges of both polygons
	int i=0;
	int j=0;
	while (i<edgeCountA && j<edgeCountB)
	{
        if (i< edgeCountA) 
		{
			if (i<edgeCountA - 1)
				edge = QPointF (
					polygonA.at(i+1).x()-polygonA.at(i).x(), 
					polygonA.at(i+1).y()-polygonA.at(i).y());
			else		
				edge = QPointF (
					polygonA.at(0).x()-polygonA.at(i).x(), 
					polygonA.at(0).y()-polygonA.at(i).y());
			i++;		
        } else 
		{
			if (i < edgeCountB -1)
				edge = QPointF (
					polygonB.at(j+1).x() - polygonA.at(i).x(), 
					polygonB.at(j+1).y() - polygonA.at(i).y());
			else	
				edge = QPointF (
					polygonB.at(0).x() - polygonA.at(i).x(), 
					polygonB.at(0).y() - polygonA.at(i).y());
			j++;
		}

        // ===== 1. Find if the polygons are currently intersecting =====


        // Find the axis perpendicular to the current edge

        QPointF axis (-edge.y(), edge.x());
        axis=normalize(axis);

        // Find the projection of the polygon on the current axis

        qreal minA = 0; qreal minB = 0; qreal maxA = 0; qreal maxB = 0;
        ProjectPolygon(axis, polygonA, minA, maxA);
        ProjectPolygon(axis, polygonB, minB, maxB);

        // Check if the polygon projections are currentlty intersecting

        if (intervalDistance(minA, maxA, minB, maxB) > 0)
            result.intersect = false;
		else	
            result.intersect = true;

        // ===== 2. Now find if the polygons *will* intersect =====


        // Project the velocity on the current axis

        qreal velocityProjection = dotProduct(axis,velocity);

        // Get the projection of polygon A during the movement

        if (velocityProjection < 0) 
            minA += velocityProjection;
        else 
            maxA += velocityProjection;
        

        // Do the same test as above for the new projection

        qreal d = intervalDistance(minA, maxA, minB, maxB);
        if (d > 0) result.willIntersect = false;
		/*
		*/
		cout <<"   ";
		cout <<"minA="<<minA<<"  ";
		cout <<"maxA="<<maxA<<"  ";
		cout <<"minB="<<minB<<"  ";
		cout <<"maxB="<<maxB<<"  ";
		cout <<"  d="<<d<<"   ";
		cout <<"minD="<<minIntervalDistance<<"  ";
		cout <<"axis="<<axis<<"  ";
		cout <<"int="<<result.intersect<<"  ";
		cout <<"wint="<<result.willIntersect<<"  ";
		//cout <<"velProj="<<velocityProjection<<"  ";
		cout <<endl;


	
        if (result.intersect || result.willIntersect) 
		{
			// Check if the current interval distance is the minimum one. If so
			// store the interval distance and the current distance.  This will
			// be used to calculate the minimum translation vector

			if (d<0) d=-d;
			if (d < minIntervalDistance) {
				minIntervalDistance = d;
				translationAxis = axis;
				cout << "tAxix="<<translationAxis<<endl;

				//QPointF t = polygonA.Center - polygonB.Center;
				QPointF t = polygonA.at(0) - polygonB.at(0);
				if (dotProduct(t,translationAxis) < 0)
					translationAxis = -translationAxis;
			}
		}
    }

    // The minimum translation vector
    // can be used to push the polygons appart.

    if (result.willIntersect)
        result.minTranslation = 
               translationAxis * minIntervalDistance;
    
    return result;
}

/* The function can be used this way: 
   QPointF polygonATranslation = new QPointF();
*/   


/*
PolygonCollisionResult r = PolygonCollision(polygonA, polygonB, velocity);

if (r.WillIntersect) 
  // Move the polygon by its velocity, then move
  // the polygons appart using the Minimum Translation Vector
  polygonATranslation = velocity + r.minTranslation;
else 
  // Just move the polygon by its velocity
  polygonATranslation = velocity;

polygonA.Offset(polygonATranslation);

*/


