#ifndef LINKABLEMAPOBJ_H
#define LINKABLEMAPOBJ_H

#include "animpoint.h"
#include "noteobj.h"
#include "headingobj.h"
#include "flagrowobj.h"

#define MAX_DEPTH 999

class MapEditor;



/*! \brief This class adds links to MapObj 

The links are connecting the branches (BranchObj) and images (FloatImageObj) in the map.
*/

class LinkableMapObj:public QObject, public MapObj {
	Q_OBJECT
public:
	/*! Orientation of an object depends on the position relative to the parent */
	enum Orientation {
		UndefinedOrientation, //!< Undefined
		LeftOfCenter,			//!< Object is left of center
		RightOfCenter			//!< Object is right of center
	};

	/*! Various drawing styles for links */
	enum Style {
		UndefinedStyle,	//!< Undefined
		Line,			//!< Straight line
		Parabel,		//!< Parabel
		PolyLine,		//!< Polygon (thick line)
		PolyParabel		//!< Thick parabel
	};

	/*! Vertical position of link in object */
	enum Position {
		Middle, //!< Link is drawn in the middle of object
		Bottom  //!< Link is drawn at bottom of object
	};

	/*! Hint if link should use the default link color or the color of heading */
	enum ColorHint {
		DefaultColor,	//!< Link uses the default color
		HeadingColor	//!< Link uses the color of heading
	};

    LinkableMapObj ();
    LinkableMapObj (QGraphicsScene*);
    LinkableMapObj (LinkableMapObj*);
    ~LinkableMapObj ();
	virtual void delLink();
    virtual void init ();
    virtual void copy (LinkableMapObj*);
    void setChildObj (LinkableMapObj*);
    virtual void setParObj (LinkableMapObj*);
    virtual void setParObjTmp (LinkableMapObj*,QPointF,int);	// Only for moving Obj around
	virtual void unsetParObjTmp();						// reuse original ParObj
	virtual bool hasParObjTmp();

	virtual void setUseRelPos (const bool&);
	virtual void setRelPos();				// set relPos to current parentPos
	virtual void setRelPos(const QPointF&);	
	virtual QPointF getRelPos();
	virtual void setUseOrientation (const bool &);


	virtual qreal getTopPad();
	virtual qreal getLeftPad();
	virtual qreal getRightPad();
	Style getDefLinkStyle();
    void setLinkStyle(Style);            
	Style getLinkStyle();
	void setHideLinkUnselected(bool);
	bool getHideLinkUnselected();
	void setLinkPos (Position);
	Position getLinkPos ();

	virtual void setID (const QString &s);
	virtual QString getID ();

	virtual void setLinkColor();					// sets color according to colorhint, overloaded
	virtual void setLinkColor(QColor);
	QColor getLinkColor();
	virtual void setVisibility (bool);
	virtual void setOrientation();
    virtual void updateLink();				// update parPos and childPos
											// depending on pos
											// redraw link with given style
    LinkableMapObj* getChildObj();			// returns pointer to fromObj
    LinkableMapObj* getParObj();			// returns pointer to toObj
    virtual LinkableMapObj* findObjBySelect(QString s);	// find obj by selectstring
	virtual void setDockPos()=0;				// sets childPos and parPos
    QPointF getChildPos();					// returns pos where childs dock
    QPointF getParPos();						// returns pos where parents dock
    Orientation getOrientation();			// get orientation
    virtual int getDepth();					// return depth
	virtual void setMapEditor(MapEditor*);	// set MapEditor (needed in LMO::updateNoteFlag)
	virtual MapEditor* getMapEditor();		// get MapEditor (usually from parent);
	virtual QPointF getRandPos();			// make randomised position

	virtual void reposition();
	virtual void requestReposition();		// do reposition after next user event
	virtual void forceReposition();			// to force a reposition now (outside
											// of mapeditor e.g. in noteeditor
	virtual bool repositionRequested();

	virtual void calcBBoxSizeWithChilds()=0;// calc size of  BBox including childs recursivly

    virtual void select();
    virtual void unselect();

protected:
	void parabel(QPolygonF &,double,double,double,double);	// Create Parabel connecting two points
	QString getLinkAttr();

    QPointF childPos;
    QPointF parPos;
	bool link2ParPos;				// While moving around, sometimes link to parent
	MapEditor* mapEditor;			// for and toggleScroll(), get default styles
									// and mapEditor->updateActions()
									// and mapEditor->updateSelection()
    Orientation orientation;     
    qreal linkwidth;				// width of a link
    int depth;						// depth: undef=-1 mapCenter=0 branch=1..n
	QRectF bboxTotal;				// bounding box including childs

    LinkableMapObj* childObj;
    LinkableMapObj* parObj;
    LinkableMapObj* parObjTmpBuf;	// temporary buffer the original parent
    qreal bottomlineY;              // vertical offset of dockpos to pos

	int thickness_start;			// for StylePoly*	
    Style style;					// Current style
	Position linkpos;				// Link at bottom of object or middle of height
    QColor linkcolor;               // Link color
	QPen pen;
    QGraphicsLineItem* l;           // line style
	QGraphicsPolygonItem* p;		// poly styles
    int arcsegs;                    // arc: number of segments
    QList <QGraphicsLineItem*> segment; // a part of e.g. the parabel
	QPolygonF pa0;					// For drawing of PolyParabel and PolyLine
	QPolygonF pa1;					// For drawing of PolyParabel 
	QPolygonF pa2;					// For drawing of PolyParabel	
    QGraphicsLineItem* bottomline;  // on bottom of BBox
	bool repositionRequest;			// 

	bool selected;					// Used for marking the selection
	bool hideLinkUnselected;		// to hide links if unselected
	qreal topPad, botPad,
		leftPad, rightPad;          // padding within bbox

	QPointF	 relPos;				// position relative to childPos of parent
	//AnimPoint relPos;				// position relative to childPos of parent
	bool useRelPos;
	bool useOrientation;

	QString objID;					// id set during load/save currently used for xLinks
};
#endif
