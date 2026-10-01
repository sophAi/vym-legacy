#ifndef XLINKOBJ_H
#define XLINKOBJ_H

#include "linkablemapobj.h"

class BranchObj;

enum XLinkState {undefinedXLink,initXLink,activeXLink,deleteXLink};

/*! \brief xlinks are used to draw arbitrary connections between branches (BranchObj) in the map. */

/////////////////////////////////////////////////////////////////////////////
class XLinkObj:public MapObj {
public:
    XLinkObj ();
    XLinkObj (QGraphicsScene*);
    ~XLinkObj ();
    virtual void init ();
    virtual void copy (XLinkObj*);
	void setBegin (BranchObj*);
	BranchObj* getBegin();
	void setEnd   (BranchObj*);
	void setEnd   (QPointF);
	BranchObj* getEnd();
	void setColor(QColor);
	QColor getColor();
	void setWidth (int);
	int getWidth ();
	bool activate ();			// Sets pointers in branchObjects
	void deactivate();			// removes those pointers
	bool isUsed();				// true, if at least on branch uses it
	void updateXLink();
	BranchObj* otherBranch (BranchObj*);
	void positionBBox();
	void calcBBoxSize();
	void setVisibility (bool);
	void setVisibility ();
	QString saveToDir ();

private:
	static int arrowSize;
	QPen pen;
	QColor color;
	int width;
	QGraphicsLineItem *line;
	QGraphicsPolygonItem *poly;
	BranchObj *beginBranch;
	BranchObj *endBranch;
	BranchObj *visBranch;	// the "visible" part of a partially scrolled link
	XLinkState xLinkState;	// init during drawing or active
	QPointF beginPos;
	QPointF   endPos;
};

#endif
