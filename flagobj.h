#ifndef FLAGOBJ_H
#define FLAGOBJ_H


#include <QAction>
#include <QPixmap>

#include "mapobj.h"
#include "imageobj.h"

/*! \brief One flag which is visible in the map. 

    Flags are aligned in a row. 
*/


/////////////////////////////////////////////////////////////////////////////
class FlagObj:public MapObj {
public:
    FlagObj ();
    FlagObj (QGraphicsScene*);
    FlagObj (FlagObj*);
    ~FlagObj ();
    virtual void init ();
    virtual void copy (FlagObj*);
    virtual void move (double x,double y);      // move to absolute Position
    virtual void moveBy (double x,double y);    // move to relative Position
    virtual void setVisibility(bool);
	void load (const QString&);
	void load (const QPixmap&);
	void setName (const QString&);
	const QString getName ();
	void setGroup (const QString&);
	const QString getGroup();
	void unsetGroup ();
	void setToolTip(const QString&);
	const QString getToolTip();
	QPixmap getPixmap();
	void setAction(QAction*);
	void updateAction ();
	void setAlwaysVisible (bool b);
	bool isAlwaysVisible ();
	bool isActive();
	void toggle();
	void activate();
	void deactivate();
	void setUsed (bool);
	bool isUsed();
	void saveToDir (const QString&, const QString&);
	
protected:	
	QString name;
	QString group;
	QString tooltip;
	bool state;
	bool avis;
	bool used;
    virtual void positionBBox();
    virtual void calcBBoxSize();
private:
	ImageObj* icon;
	QAction* action;
};

#endif
