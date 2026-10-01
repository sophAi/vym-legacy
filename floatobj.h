#ifndef FLOATOBJ_H
#define FLOATOBJ_H

#include "ornamentedobj.h"

/*! \brief Base class for objects floating in the map, which means they can be positioned freely. */


/////////////////////////////////////////////////////////////////////////////
class FloatObj:public OrnamentedObj {
public:
    FloatObj ();
    FloatObj (QGraphicsScene*);
    FloatObj (QGraphicsScene*, LinkableMapObj* parent);
    ~FloatObj ();
    virtual void init ();
    virtual void copy (FloatObj*);
	virtual bool load (const QString&)=0;
	virtual void setZValue(const int&);		// set zPlane
	virtual int zValue();
	virtual void setFloatExport (const bool &);
	virtual bool getFloatExport ();

	virtual void move (double,double);
	virtual void move (QPointF);
	virtual void setDockPos();
	virtual void reposition();
											
	virtual QRectF getTotalBBox();			// return BBox including childs			
	virtual QRectF getBBoxSizeWithChilds();	// return size of BBox including childs  

	virtual void resetSaveCounter()=0;

	virtual void select();
	virtual void unselect();

protected:
	bool floatExport;
	int zPlane;
};

#endif
