#ifndef FLAGROWOBJ_H
#define FLAGROWOBJ_H

#include <QMainWindow>

#include "mapobj.h"
#include "flagobj.h"

/*! \brief A collection of flags (FlagObj). 

   The flags are aligned horizontally  in a row on the map. 
   A toolbar can be created from the flags in this row.
 */

class FlagRowObj:public MapObj {
public:
    FlagRowObj ();
    FlagRowObj (QGraphicsScene *);
    ~FlagRowObj ();
    virtual void init ();
    virtual void copy (FlagRowObj*);
    virtual void clone(FlagRowObj*);
    virtual void move   (double,double);
    virtual void moveBy (double,double);
    virtual void setVisibility(bool);
    virtual FlagObj* addFlag (FlagObj *fo);			// make deep copy of FlagObj
    virtual void positionBBox();
    virtual void calcBBoxSize();
	virtual QString getFlagName (const QPointF &p);	// Find flag by position
	bool isActive(const QString&);
	void toggle (const QString&,bool);
	void activate(const QString&);
	void deactivate(const QString&);
	void deactivateAll();
	void deactivateGroup(FlagObj *);
	void setToolBar (QToolBar *);
	void setEnabled (bool);
	void setShowFlags (bool);
	void resetUsedCounter();
	QString saveToDir (const QString &,const QString &,bool);
	void setName (const QString&);			// prefix for exporting flags to dir
	void makeToolbar (QMainWindow*, const QString &);	// Create Toolbar buttons
	void updateToolbar();					// Update Toolbar buttons	
private:	
	FlagRowObj* parentRow;					// look for flags in this row
	FlagObj* findFlag (const QString&);
    QList <FlagObj*> flag; 
	QToolBar *toolbar;
	QString name;
	bool showFlags;							// FloatObjects want to hide their flags
};
#endif
