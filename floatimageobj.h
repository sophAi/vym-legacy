#ifndef FLOATIMAGEOBJ_H
#define FLOATIMAGEOBJ_H

#include "floatobj.h"
//Added by qt3to4:
#include <QPixmap>

/*! \brief A pixmap which can be positioned freely as FlagObj  on the map.  */


/////////////////////////////////////////////////////////////////////////////
class FloatImageObj:public FloatObj {
public:
    FloatImageObj ();
    FloatImageObj (QGraphicsScene*);
    FloatImageObj (QGraphicsScene*, OrnamentedObj* parent);
    ~FloatImageObj ();
    virtual void init ();
    virtual void copy (FloatImageObj*);
	virtual void setZValue (const int&);
	virtual int z();

	virtual void load (const QPixmap &);
	virtual bool load (const QString &);
	virtual void save (const QString &, const QString&);
	virtual void setOriginalFilename(const QString &);
	virtual QString getOriginalFilename();
    virtual void setVisibility(bool);	    // set vis. for w
    virtual void move (double x,double y);
    virtual void move (QPointF);
	virtual void positionBBox();
	virtual void calcBBoxSize();
	virtual QRectF getTotalBBox();			// return BBox including childs			
	virtual QRectF getBBoxSizeWithChilds();	// return size of BBox including childs  
	virtual void calcBBoxSizeWithChilds();	// calc size of  BBox including childs recursivly
	virtual QString saveToDir(const QString &,const QString&);
	virtual void resetSaveCounter();


protected:
	ImageObj *icon;
	bool saveInMap;
	static uint saveCounter;	// numerate the files during saveToDir
	QString filetype;
	QString filename;
	QString originalFilename;
};

#endif
