#ifndef MISC_H
#define MISC_H

#include <qpoint.h>
#include <qdir.h>
#include <iostream>

using namespace std;


/////////////////////////////////////////////////////////////////////////////
QString qpointToString (const QPoint &p);
QString qpointfToString (const QPointF &p);
extern ostream &operator<< (ostream &stream, QPoint const &p);
extern ostream &operator<< (ostream &stream, QPointF const &p);
qreal getAngle(const QPointF &);
QPointF normalise (const QPointF &);
qreal max (qreal,qreal);
class BranchObj;
class MapEditor;

#endif
