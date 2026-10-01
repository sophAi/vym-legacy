#ifndef MAPOBJ_H
#define MAPOBJ_H

#include <QGraphicsScene>
#include <QGraphicsItem>

#include "xmlobj.h"

#define Z_BBOX      0
#define Z_XLINK    10
#define Z_LINK     20
#define Z_FRAME    50
#define Z_SELBOX   60
#define Z_FLOATIMG 65
#define Z_ICON     80
#define Z_TEXT    100

/*! \brief Base class for all objects visible on a map
*/

class MapObj:public XMLObj {
public:
    MapObj ();
    MapObj (QGraphicsScene*);
    MapObj (MapObj*);
    virtual ~MapObj ();
    virtual void init ();
    virtual void copy (MapObj*);
	virtual QGraphicsScene* getScene();
    virtual qreal x();
    virtual qreal y();
	virtual qreal width();
	virtual qreal height();
	virtual QPointF getAbsPos();
	virtual QString getPos();					// Return position as string (x,y)
    virtual void move (double x,double y);      // move to absolute Position
    virtual void move (QPointF p);
    virtual void moveBy (double x,double y);    // move to relative Position
    virtual QRectF getBBox();					// returns bounding box
    virtual QRectF getClickBox();				// returns box to click
    virtual QSizeF getSize();					// returns size of bounding box
    virtual bool isVisibleObj();
    virtual void setVisibility(bool);
    virtual void positionBBox()=0;       
	virtual void calcBBoxSize()=0;
protected:  
    QGraphicsScene* scene;
    QRectF bbox;									// bounding box of MO itself
	QRectF clickBox;								// area where mouseclicks are found
    QPointF absPos;							    // Position on canvas
    bool visible;
};

#endif
