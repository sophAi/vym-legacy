#ifndef BRANCHOBJ_H
#define BRANCHOBJ_H

#include "floatimageobj.h"
#include "linkablemapobj.h"
#include "ornamentedobj.h"
#include "xlinkobj.h"


bool isAbove(BranchObj*,BranchObj*);

/*! \brief A branch visible in the map */

/*! If HideExport is used, this branch and its childs will be hidden in export */
enum HideTmpMode {HideNone, HideExport};


/////////////////////////////////////////////////////////////////////////////
class BranchObj:public OrnamentedObj {
public:
	/*! New branches will get use same color for heading as parent */
	enum BranchModification {NewBranch, MovedBranch};


    BranchObj ();
    BranchObj (QGraphicsScene*);
    BranchObj (QGraphicsScene*, LinkableMapObj* parent);
    ~BranchObj ();
	bool operator< ( const BranchObj & );
	bool operator== ( const BranchObj & );
    virtual void init ();
    virtual void copy (BranchObj*);
    void clear();
	virtual int getNum();				// return number of this in parent
	virtual int getNum(BranchObj*);		// return number of this in parent
	virtual int getFloatImageNum(FloatImageObj*);		
	virtual int countBranches();		
	virtual int countFloatImages();		
	virtual int countXLinks();		
    virtual void setParObjTmp (LinkableMapObj*,QPointF,int);// Only for moving Obj around
	virtual void unsetParObjTmp();			// reuse original ParObj

	virtual void unScroll();				
	virtual void toggleScroll();			// scroll or unscroll
	virtual bool isScrolled();				// returns scroll state
	virtual bool hasScrolledParent(BranchObj*);	// true, if any of the parents is scrolled
	virtual void tmpUnscroll();				// unscroll scrolled parents temporary e.g. during "find" process
	virtual void resetTmpUnscroll();		// scroll all tmp scrolled parents again e.g. when unselecting

	virtual void setVisibility(bool,int);	// set visibility
    virtual void setVisibility(bool);	    // set vis. for w
	virtual void setLinkColor();			// set the color of link
	virtual void setColorSubtree(QColor);	// set the color of heading

	BranchObj* first ();				// set Iterator to first LMO
	BranchObj* next ();					// find next LMO after given one
	BranchObj* getLastIterator();		// to interrupt and resume next iteration
	void setLastIterator (BranchObj*);	// needed by next() 

	virtual void positionContents();
    virtual void move (double x,double y);
    virtual void move (QPointF);
    virtual void moveBy (double x,double y);
    virtual void moveBy (QPointF);
    virtual void positionBBox();
    virtual void calcBBoxSize();
	virtual void setDockPos();
    virtual LinkableMapObj* findMapObj(QPointF,LinkableMapObj*);	// find MapObj 
    virtual LinkableMapObj* findID (QString sid);	// find Obj by ID string
    virtual void setHeading (QString);

	virtual void setHideTmp (HideTmpMode);
	virtual bool hasHiddenExportParent ();

	virtual QString saveToDir (const QString&,const QString&, const QPointF&);// Save data recursivly to tempdir
	virtual void addXLink (XLinkObj*);
	virtual void removeXLinkRef (XLinkObj*);// Remove ref in list
	virtual void deleteXLink (XLinkObj*);	// remove references and delete XLinkObj 
	virtual void deleteXLinkAt (int);		// remove references and delete XLinkObj 
	virtual XLinkObj* XLinkAt (int);		// return reference of XLinkObj 
	virtual int countXLink ();
	virtual BranchObj* XLinkTargetAt (int);
	void setIncludeImagesVer(bool);
	bool getIncludeImagesVer();
	void setIncludeImagesHor(bool);
	bool getIncludeImagesHor();
	QString getIncludeImageAttr();
	virtual FloatImageObj* addFloatImage();
	virtual FloatImageObj* addFloatImage(FloatImageObj*);
	virtual void removeFloatImage(FloatImageObj*);
    virtual FloatImageObj* getFirstFloatImage();
    virtual FloatImageObj* getLastFloatImage();
	virtual FloatImageObj* getFloatImageNum(const uint &);
protected:	
	virtual void savePosInAngle();					// write pos in angle for resorting			
	virtual void setDefAttr (BranchModification);	// set default attributes (font, size, ...)
public:	
    virtual BranchObj* addBranch();
    virtual BranchObj* addBranch(BranchObj*);		// makes deep copy of BranchObj
    virtual BranchObj* addBranchPtr(BranchObj*);	// just adds pointer
    virtual BranchObj* insertBranch(int);
    virtual BranchObj* insertBranch(BranchObj*,int);
    virtual BranchObj* insertBranchPtr (BranchObj*,int);
    virtual void removeBranchHere(BranchObj*);  
    virtual void removeChilds();  
    virtual void removeBranch(BranchObj*);  
    virtual void removeBranchPtr (BranchObj*);  
    virtual void setLastSelectedBranch(BranchObj*);
    virtual BranchObj* getLastSelectedBranch();
    virtual BranchObj* getFirstBranch();
    virtual BranchObj* getLastBranch();
	virtual BranchObj* getBranchNum(int);
    virtual bool canMoveBranchUp();
    virtual BranchObj* moveBranchUp(BranchObj*);
    virtual bool canMoveBranchDown();
    virtual BranchObj* moveBranchDown(BranchObj*);
    virtual void sortChildren();
    virtual BranchObj* linkTo (BranchObj*, int);
    virtual void alignRelativeTo(const QPointF ref, bool alignSelf=false );
	virtual void reposition();
	virtual void unsetAllRepositionRequests();

	virtual QPolygonF shape();				//!< Returns arbitrary bounding polygon
	virtual QRectF getTotalBBox();			// return BBox including childs			
	virtual QRectF getBBoxSizeWithChilds();	// return size of BBox including childs  
	virtual void calcBBoxSizeWithChilds();	// calc size of  BBox including childs recursivly

    virtual void select();
    virtual void unselect();
	virtual QString getSelectString();
	virtual void setAnimation(const AnimPoint &ap);
	virtual bool animate();

protected:
	static BranchObj* itLast;		// iterator for first(), next()
	static BranchObj* itFirst;		// first iterator for first(), next()
    QList<BranchObj*> branch;		// all child branches
	QList<FloatImageObj*> floatimage;// child images
	QList<XLinkObj*> xlink;			// xlinks to other branches

	AnimPoint anim;

public:	
	float angle;					// used in mainbranch to reorder mainbranches
protected:	
    int lastSelectedBranch;			// for going deeper into tree
	bool scrolled;					// true if all childs are scrolled and thus invisible
	bool tmpUnscrolled;				// can only be true (temporary) for a scrolled subtree
	bool includeImagesVer;			// include floatimages in bbox vertically
	bool includeImagesHor;			// include floatimages in bbox horizontally

};


#endif

