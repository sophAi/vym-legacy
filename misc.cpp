#include <math.h>

#include <qregexp.h>
#include <qpoint.h>
#include <stdlib.h>

#include "misc.h"

QString qpointToString (const QPoint &p)
{
	return "(" + QString("%1").arg(p.x()) +","+ QString ("%1").arg (p.y()) +")";
}

QString qpointfToString (const QPointF &p)
{
	return "(" + QString("%1").arg(p.x()) +","+ QString ("%1").arg (p.y()) +")";
}

ostream &operator<< (ostream &stream, QPoint const &p)
{ 
	stream << "("<<p.x()<<","<<p.y()<<")";
	return stream;
}

ostream &operator<< (ostream &stream, QPointF const &p)
{ 
	stream << "("<<p.x()<<","<<p.y()<<")";
	return stream;
}

qreal getAngle(const QPointF &p)
{	
	// Calculate angle of vector to y-axis
	if (p.y()==0)
	{
		if (p.x()>=0)
			return M_PI_2;
		else
			return 3* M_PI_2;
	} else
	{
		if (p.y()>0) 
			return (qreal)(M_PI  - atan ( (qreal)(p.x()) / (qreal)(p.y()) ) );
		else	
			if (p.x()<0)
				return (qreal)( 2*M_PI - atan ( (qreal)(p.x()) / (qreal)(p.y()) ) );
			else	
				return (qreal)( - atan ( (qreal)(p.x()) / (qreal)(p.y()) ) );
	}	
}

QPointF normalise(const QPointF &p)
{	
	// Calculate normalised position (fixed length) 

	qreal px=p.x();
	qreal py=p.y();
	qreal x;
	qreal y;
	qreal r=150;

	if (px==0)
	{
		x=0;
		if (py>=0)
			y=r;
		else
			y=-r;
	} else
	{
		qreal sign;
		qreal a;
		if (px>0) 
			sign=1; 
		else 
			sign=-1;
		
		a=atan (py / px);
		x=cos (a) * r *sign;
		y=sin (a) * r *sign;
	}	
	return QPoint ((int) (x),(int) (y));
}


qreal max(qreal a, qreal b)
{
	if (a>b)
		return a;
	return b;
}

