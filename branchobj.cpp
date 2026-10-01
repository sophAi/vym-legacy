#include "branchobj.h"

// #include "texteditor.h"
#include "geometry.h"
#include "mapeditor.h"
#include "mainwindow.h"
#include "misc.h"

class TextEditor;

extern TextEditor *textEditor;
extern Main *mainWindow;
extern FlagRowObj *standardFlagsDefault;


/////////////////////////////////////////////////////////////////
// BranchObj
/////////////////////////////////////////////////////////////////

BranchObj* BranchObj::itLast=NULL;
BranchObj* BranchObj::itFirst=NULL;


BranchObj::BranchObj () :OrnamentedObj()
{
//    cout << "Const BranchObj ()\n";
    setParObj (this);	
    init();
    depth=-1;
}

BranchObj::BranchObj (QGraphicsScene* s):OrnamentedObj (s)
{
//    cout << "Const BranchObj (s)  called from MapCenterObj (s)\n";
	parObj=NULL;
    scene=s;
}

BranchObj::BranchObj (QGraphicsScene* s, LinkableMapObj* p):OrnamentedObj (s)
{
//    cout << "Const BranchObj (s,p)\n";
    scene=s;
    setParObj (p);	
    depth=p->getDepth()+1;
	if (depth==1)
		// Calc angle to mapCenter if I am a mainbranch
		// needed for reordering the mainbranches clockwise 
		// around mapcenter 
		angle=getAngle (QPointF (x() - parObj->getChildPos().x() , 
								(y() - parObj->getChildPos().y() ) ) );
    init();
}

BranchObj::~BranchObj ()
{
	// If I'm animated, I need to un-animate myself first
	if (anim.isAnimated() )
	{
		anim.setAnimated (false);
		mapEditor->getModel()->stopAnimation (this);
	}


	//cout << "Destr BranchObj of "<<this<<endl;
	// Check, if this branch was the last child to be deleted
	// If so, unset the scrolled flags

	BranchObj *po=(BranchObj*)parObj;
	BranchObj *bo;
	if (po)
	{
		bo=((BranchObj*)parObj)->getLastBranch();
		if (bo) po->unScroll();
	}
	clear();
}

bool BranchObj::operator< ( const BranchObj & other )
{
    return  angle < other.angle;
}

bool BranchObj::operator== ( const BranchObj & other )
{
    return angle == other.angle;
}

void BranchObj::init () 
{
	if (parObj)
	{
		absPos=getRandPos();
		absPos+=parObj->getChildPos();
	}

    lastSelectedBranch=0;

    setChildObj(this);

	scrolled=false;
	tmpUnscrolled=false;

	includeImagesVer=false;
	includeImagesHor=false;
}

void BranchObj::copy (BranchObj* other)
{
    OrnamentedObj::copy(other);

	branch.clear();
	for (int i=0; i<other->branch.size(); ++i)
		// Make deep copy of b
		// Because addBranch again calls copy for the childs,
		// Those will get a deep copy, too
		addBranch(other->branch.at(i) );	

	for (int i=0; i<other->floatimage.size(); ++i)
		addFloatImage  (other->floatimage.at(i));
	scrolled=other->scrolled;
	tmpUnscrolled=other->tmpUnscrolled;
	setVisibility (other->visible);

	angle=other->angle;

    positionBBox();
}

void BranchObj::clear() 
{
	setVisibility (true);

	while (!floatimage.isEmpty())
		delete floatimage.takeFirst();

	while (!xlink.isEmpty())
		delete xlink.takeFirst();

	while (!branch.isEmpty())
		delete branch.takeFirst();
}

bool isAbove (BranchObj* a, BranchObj *b)
{
	if (a->angle < b->angle)
		return true;
	else	
		return false;
}

int BranchObj::getNum()
{
	if (parObj)
		return ((BranchObj*)parObj)->getNum (this);
	else
		return 0;
}

int BranchObj::getNum(BranchObj *bo)
{
	return branch.indexOf (bo);
}

int BranchObj::getFloatImageNum(FloatImageObj *fio)
{
	return floatimage.indexOf(fio);
}

int BranchObj::countBranches()
{
	return branch.count();
}

int BranchObj::countFloatImages()
{
	return floatimage.count();
}

int BranchObj::countXLinks()
{
	return xlink.count();
}

void BranchObj::setParObjTmp(LinkableMapObj* lmo, QPointF m, int off)
{
	// Temporary link to lmo
	// m is position of mouse pointer 
	// offset 0: default 1: below lmo   -1 above lmo  (if possible)


	BranchObj* o=(BranchObj*)(lmo);
	if (!parObjTmpBuf) 
		parObjTmpBuf=parObj;

	// ignore mapcenter and mainbranch
	if (lmo->getDepth()<2) off=0;
	if (off==0)
		link2ParPos=false;
	else
		link2ParPos=true;
	parObj=o;

	depth=parObj->getDepth()+1;

	// setLinkStyle calls updateLink, only set it once
	if (style!=getDefLinkStyle() ) setLinkStyle (getDefLinkStyle());

	// Move temporary to new position at destination
	// Usually the positioning would be done by reposition(),
	// but then also the destination branch would "Jump" around...
	// Better just do it approximately
	if (depth==1)
	{	// new parent is the mapcenter itself

		QPointF p= normalise ( QPointF (m.x() - o->getChildPos().x(),
									  m.y() - o->getChildPos().y() ));
		if (p.x()<0) p.setX( p.x()-bbox.width() );
		move2RelPos (p);
	} else
	{	
		qreal y;
		if (off==0)
		{
			// new parent is just a branch, link to it
			QRectF t=o->getBBoxSizeWithChilds();
			if (o->getLastBranch())
				y=t.y() + t.height() ;
			else
				y=t.y();

		} else
		{
			if (off<0)
				// we want to link above lmo
				y=o->y() - height() + 5;
			else	
				// we want to link below lmo
				// Bottom of sel should be 5 pixels above
				// the bottom of the branch _below_ the target:
				// Don't try to find that branch, guess 12 pixels
				y=o->getChildPos().y()  -height() + 12; 
		}	
		if (o->getOrientation()==LinkableMapObj::LeftOfCenter)
			move ( o->getChildPos().x() - linkwidth, y );
		else	
			move (o->getChildPos().x() + linkwidth, y );
	}	

	// updateLink is called implicitly in move
	requestReposition();	
}

void BranchObj::unsetParObjTmp()
{
	if (parObjTmpBuf) 
	{
		link2ParPos=false;
		parObj=parObjTmpBuf;
		parObjTmpBuf=NULL;
		depth=parObj->getDepth()+1;
		setLinkStyle (getDefLinkStyle() );
		updateLink();
	}		
}

void BranchObj::unScroll()
{
	if (tmpUnscrolled) resetTmpUnscroll();
	if (scrolled) toggleScroll();
}

void BranchObj::toggleScroll()
{
	if (scrolled)
	{
		scrolled=false;
		systemFlags->deactivate("scrolledright");
		for (int i=0; i<branch.size(); ++i)
			branch.at(i)->setVisibility(true);
	} else
	{
		scrolled=true;
		systemFlags->activate("scrolledright");
		for (int i=0; i<branch.size(); ++i)
			branch.at(i)->setVisibility(false);
	}
	calcBBoxSize();
	positionBBox();	
	move (absPos.x(), absPos.y() );
	forceReposition();
}

bool BranchObj::isScrolled()
{
	return scrolled;
}

bool BranchObj::hasScrolledParent(BranchObj *start)
{
	// Calls parents recursivly to
	// find out, if we are scrolled at all.
	// But ignore myself, just look at parents.

	if (this !=start && scrolled) return true;

	BranchObj* bo=(BranchObj*)(parObj);
	if (bo) 
		return bo->hasScrolledParent(start);
	else
		return false;
}

void BranchObj::tmpUnscroll()
{
	// Unscroll parent (recursivly)
	BranchObj* bo=(BranchObj*)(parObj);
	if (bo) bo->tmpUnscroll();
		
	// Unscroll myself
	if (scrolled)
	{
		tmpUnscrolled=true;
		systemFlags->activate("tmpUnscrolledright");
		toggleScroll();
	}	
}

void BranchObj::resetTmpUnscroll()
{
	// Unscroll parent (recursivly)
	BranchObj* bo=(BranchObj*)(parObj);
	if (bo)
		bo->resetTmpUnscroll();
		
	// Unscroll myself
	if (tmpUnscrolled)
	{
		tmpUnscrolled=false;
		systemFlags->deactivate("tmpUnscrolledright");
		toggleScroll();
	}	
}

void BranchObj::setVisibility(bool v, int toDepth)
{
    if (depth <= toDepth)
    {
		frame->setVisibility(v);
		heading->setVisibility(v);
		systemFlags->setVisibility(v);
		standardFlags->setVisibility(v);
		LinkableMapObj::setVisibility (v);
		
		int i;
		for (i=0; i<floatimage.size(); ++i)
			floatimage.at(i)->setVisibility (v);
		for (i=0; i<xlink.size(); ++i)	
			xlink.at(i)->setVisibility ();	

		// Only change childs, if I am not scrolled
		if (!scrolled && (depth < toDepth))
		{
			// Now go recursivly through all childs
			for (i=0; i<branch.size(); ++i)
				branch.at(i)->setVisibility (v,toDepth);	
		}
    } // depth <= toDepth	
	requestReposition();
}	

void BranchObj::setVisibility(bool v)
{
    setVisibility (v,MAX_DEPTH);
}


void BranchObj::setLinkColor ()
{
	// Overloaded from LinkableMapObj
	// BranchObj can use color of heading

	if (mapEditor)
	{
		if (mapEditor->getMapLinkColorHint()==HeadingColor)
			LinkableMapObj::setLinkColor (heading->getColor() );
		else	
			LinkableMapObj::setLinkColor ();
	}		
}

void BranchObj::setColorSubtree(QColor col)
{
	setColor (col);
	for (int i=0; i<branch.size(); ++i)
		branch.at(i)->setColorSubtree(col);
}

BranchObj* BranchObj::first()
{
	itLast=NULL;	
	itFirst=this;
	return this; 
}
	
BranchObj* BranchObj::next()
{
	BranchObj *bo;
	BranchObj *lmo;
	BranchObj *po=(BranchObj*)parObj;

	if (branch.isEmpty())
		bo=NULL;
	else
		bo=branch.first();

	if (!itLast)
	{
		// no itLast, we are just beginning
		if (bo) 
		{
			// we have childs, return first one
			itLast=this;
			return bo;
		}	
		else
		{
			// No childs, so there is no next
			itLast=this;
			return NULL;
		}	
	}

	// We have an itLast
	if (itLast==po)
	{	// We come from parent
		if (bo)
		{
			// there are childs, go there
			itLast=this;
			return bo;
		}	
		else
		{	// no childs, try to go up again
			if (po)
			{
				// go back to parent and try to find next there
				itLast=this;
				lmo=po->next();
				itLast=this;
				return lmo;

			}	
			else
			{
				// can't go up, I am mapCenter, no next
				itLast=NULL;
				return NULL;
			}	
		}
	}

	// We don't come from parent, but from brother or childs

	// Try to find last child, where we came from, in my own childs
	bool searching=true;
	int i=0;
	while (i<branch.size())
	{
		// Try to find itLast in my own childs
		if (itLast==branch.at(i))
		{
			// ok, we come from my own childs
			if (i<branch.size()-1)
				bo=branch.at(i+1);
			 else
				bo=NULL;
			searching=false;
			i=branch.size();
		} 	
		++i;	
	}
	if (!searching)
	{	// found itLast in my childs
		if (bo)
		{
			// found a brother of lastLMO 
			itLast=this;
			return bo;
		}	
		else
		{
			if (po)
			{
				if (this==itFirst) return NULL;	// Stop at starting point
				// go up
				itLast=this;
				lmo=po->next();
				itLast=this;
				return lmo;
			}
			else
			{
				// can't go up, I am mapCenter
				itLast=NULL;
				return NULL;
			}	
		}
	}

	// couldn't find last child, it must be a nephew of mine
	if (branch.size()>0)
	{
		// proceed with my first child
		itLast=this;	
		return branch.first();
	}	
	else
	{
		// or go back to my parents
		if (po)
		{
			// go up
			itLast=this;
			lmo=po->next();
			itLast=this;
			return lmo;
		}	
		else
		{
			// can't go up, I am mapCenter
			itLast=NULL;
			return NULL;
		}	
	}	
}

BranchObj* BranchObj::getLastIterator()
{
	return itLast;
}

void BranchObj::setLastIterator(BranchObj* it)
{
	itLast=it;
}

void BranchObj::positionContents()
{
    for (int i=0; i<floatimage.size(); ++i )
		floatimage.at(i)->reposition();
	OrnamentedObj::positionContents();
}

void BranchObj::move (double x, double y)
{
	OrnamentedObj::move (x,y);
    for (int i=0; i<floatimage.size(); ++i )
		floatimage.at(i)->reposition();
    positionBBox();
}

void BranchObj::move (QPointF p)
{
	move (p.x(), p.y());
}

void BranchObj::moveBy (double x, double y)
{
	OrnamentedObj::moveBy (x,y);
	for (int i=0; i<branch.size(); ++i)
		branch.at(i)->moveBy (x,y);
    positionBBox();
}
	
void BranchObj::moveBy (QPointF p)
{
	moveBy (p.x(), p.y());
}


void BranchObj::positionBBox()
{
	QPointF ap=getAbsPos();
	bbox.moveTopLeft (ap);
	positionContents();

	// set the frame
	frame->setRect(QRectF(bbox.x(),bbox.y(),bbox.width(),bbox.height() ) );

	// Update links to other branches
	for (int i=0; i<xlink.size(); ++i)
		xlink.at(i)->updateXLink();
}

void BranchObj::calcBBoxSize()
{
    QSizeF heading_r=heading->getSize();
    qreal heading_w=(qreal) heading_r.width() ;
    qreal heading_h=(qreal) heading_r.height() ;
    QSizeF sysflags_r=systemFlags->getSize();
	qreal sysflags_h=sysflags_r.height();
	qreal sysflags_w=sysflags_r.width();
    QSizeF stanflags_r=standardFlags->getSize();
	qreal stanflags_h=stanflags_r.height();
	qreal stanflags_w=stanflags_r.width();
    qreal w;
    qreal h;

	// set width to sum of all widths
	w=heading_w + sysflags_w + stanflags_w;
	// set height to maximum needed height
	h=max (sysflags_h,stanflags_h);
	h=max (h,heading_h);

	// Save the dimension of flags and heading
	ornamentsBBox.setSize ( QSizeF(w,h));

	// clickBox includes Flags and Heading
    clickBox.setSize (ornamentsBBox.size() );

	// Floatimages 
	QPointF rp;

	topPad=botPad=leftPad=rightPad=0;
	if (includeImagesVer || includeImagesHor)
	{
		if (countFloatImages()>0)
		{
			for (int i=0; i<floatimage.size(); ++i )
			{
				rp=floatimage.at(i)->getRelPos();
				if (includeImagesVer)
				{
					if (rp.y() < 0) 
						topPad=max (topPad,-rp.y()-h);
					if (rp.y()+floatimage.at(i)->height() > 0)
						botPad=max (botPad,rp.y()+floatimage.at(i)->height());
				}		
				if (includeImagesHor)
				{
					if (orientation==LinkableMapObj::RightOfCenter)
					{
						if (-rp.x()-w > 0) 
							leftPad=max (leftPad,-rp.x()-w);
						if (rp.x()+floatimage.at(i)->width() > 0)
							rightPad=max (rightPad,rp.x()+floatimage.at(i)->width());
					} else
					{
						if (rp.x()< 0) 
							leftPad=max (leftPad,-rp.x());
						if (rp.x()+floatimage.at(i)->width() > w)
							rightPad=max (rightPad,rp.x()+floatimage.at(i)->width()-w);
					}
				}		
			}	
		}	
		h+=topPad+botPad;
		w+=leftPad+rightPad;
	}

	// Frame thickness
    w+=frame->getPadding();
    h+=frame->getPadding();
	
	// Finally set size
    bbox.setSize (QSizeF (w,h));
}

void BranchObj::setDockPos()
{
	// Sets childpos and parpos depending on orientation
	if (getOrientation()==LinkableMapObj::LeftOfCenter )
    {
		childPos=QPointF (
			ornamentsBBox.bottomLeft().x(), 
			bottomlineY);
		parPos=QPointF (
			ornamentsBBox.bottomRight().x(),
			bottomlineY);
    } else
    {
		childPos=QPointF (
			ornamentsBBox.bottomRight().x(), 
			bottomlineY);
		parPos=QPointF (
			ornamentsBBox.bottomLeft().x(),
			bottomlineY);
    }
}

LinkableMapObj* BranchObj::findMapObj(QPointF p, LinkableMapObj* excludeLMO)
{
	// Search branches
    LinkableMapObj *lmo;
	for (int i=0; i<branch.size(); ++i)
    {	
		lmo=branch.at(i)->findMapObj(p, excludeLMO);
		if (lmo != NULL) return lmo;
    }
	

	// Search myself
    if (inBox (p,clickBox) && (this != excludeLMO) && isVisibleObj() ) 
		return this;

	// Search float images
    for (int i=0; i<floatimage.size(); ++i )
		if (inBox(p,floatimage.at(i)->getClickBox()) && 
			(floatimage.at(i) != excludeLMO) && 
			floatimage.at(i)->getParObj()!= excludeLMO &&
			floatimage.at(i)->isVisibleObj() 
		) return floatimage.at(i);

    return NULL;
}

LinkableMapObj* BranchObj::findID (QString sid)
{
	// Search branches
    LinkableMapObj *lmo;
	for (int i=0; i<branch.size(); ++i)
    {	
		lmo=branch.at(i)->findID (sid);
		if (lmo != NULL) return lmo;
    }
	
	// Search myself
	if (sid==objID) return this;


/*
	// Search float images
    for (int i=0; i<floatimage.size(); ++i )
		if (floatimage.at(i)->inBox(p) && 
			(floatimage.at(i) != excludeLMO) && 
			floatimage.at(i)->getParObj()!= excludeLMO &&
			floatimage.at(i)->isVisibleObj() 
		) return floatimage.at(i);
*/
    return NULL;
}

void BranchObj::setHeading(QString s)
{
    heading->setText(s);	// set new heading
	calcBBoxSize();			// recalculate bbox
    positionBBox();			// rearrange contents
	requestReposition();
}

void BranchObj::setHideTmp (HideTmpMode mode)
{
	if (mode==HideExport && (hideExport|| hasHiddenExportParent() ) )
	{
		// Hide stuff according to hideExport flag and parents
		setVisibility (false);
		hidden=true;
	}else
	{
		// Do not hide, but still take care of scrolled status
		if (hasScrolledParent(this))
			setVisibility (false);
		else
			setVisibility (true);
		hidden=false;
	}	

	// And take care of my childs
	for (int i=0; i<branch.size(); ++i)
		branch.at(i)->setHideTmp (mode);
}

bool BranchObj::hasHiddenExportParent()
{
	// Calls parents recursivly to
	// find out, if we or parents are temp. hidden

	if (hidden || hideExport) return true;

	BranchObj* bo=(BranchObj*)parObj;
	if (bo) 
		return bo->hasHiddenExportParent();
	else
		return false;
}

QString BranchObj::saveToDir (const QString &tmpdir,const QString &prefix, const QPointF& offset)
{
	// Cloudy stuff can be hidden during exports
	if (hidden) return "";

	// Update of note is usually done while unselecting a branch
	if (isNoteInEditor) getNoteFromTextEditor();
	
    QString s,a;
	QString scrolledAttr;
	if (scrolled) 
		scrolledAttr=attribut ("scrolled","yes");
	else
		scrolledAttr="";

	// save area, if not scrolled
	QString areaAttr;
	if (!((BranchObj*)(parObj))->isScrolled() )
	{
		areaAttr=
			attribut("x1",QString().setNum(absPos.x()-offset.x())) +
			attribut("y1",QString().setNum(absPos.y()-offset.y())) +
			attribut("x2",QString().setNum(absPos.x()+width()-offset.x())) +
			attribut("y2",QString().setNum(absPos.y()+height()-offset.y()));

	} else
		areaAttr="";
	
	// Providing an ID for a branch makes export to XHTML easier
	QString idAttr;
	if (countXLinks()>0)
		idAttr=attribut ("id",mapEditor->getModel()->getSelectString(this)); //TODO directly access model
	else
		idAttr="";

    s=beginElement ("branch" 
		+getOrnXMLAttr() 
		+scrolledAttr 
		+areaAttr 
		+idAttr 
		+getIncludeImageAttr() );
    incIndent();

	// save heading
    s+=valueElement("heading", getHeading(),
		attribut ("textColor",QColor(heading->getColor()).name()));

	// Save frame
	if (frame->getFrameType()!=FrameObj::NoFrame) 
		s+=frame->saveToDir ();

	// save names of flags set
	s+=standardFlags->saveToDir(tmpdir,prefix,0);
	
	// Save FloatImages
	for (int i=0; i<floatimage.size(); ++i)
		s+=floatimage.at(i)->saveToDir (tmpdir,prefix);

	// save note
	if (!note.isEmpty() )
		s+=note.saveToDir();
	
	// Save branches
	for (int i=0; i<branch.size(); ++i)
		s+=branch.at(i)->saveToDir(tmpdir,prefix,offset);

	// Save XLinks
	QString ol;	// old link
	QString cl;	// current link
	for (int i=0; i<xlink.size(); ++i)
	{
		cl=xlink.at(i)->saveToDir();
		if (cl!=ol)
		{
			s+=cl;
			ol=cl;
		} else
		{
			qWarning (QString("Ignoring of duplicate xLink in %1").arg(getHeading()));
		}
	}	

    decIndent();
    s+=endElement   ("branch");
    return s;
}

void BranchObj::addXLink (XLinkObj *xlo)
{
	xlink.append (xlo);
	
}

void BranchObj::removeXLinkRef (XLinkObj *xlo)
{
	xlink.removeAt (xlink.indexOf(xlo));
}

void BranchObj::deleteXLink(XLinkObj *xlo)
{
	xlo->deactivate();
	if (!xlo->isUsed()) delete (xlo);
}

void BranchObj::deleteXLinkAt (int i)
{
	XLinkObj *xlo=xlink.at(i);
	xlo->deactivate();
	if (!xlo->isUsed()) delete(xlo);
}

XLinkObj* BranchObj::XLinkAt (int i)
{
	return xlink.at(i);
}

int BranchObj::countXLink()
{
	return xlink.count();
}


BranchObj* BranchObj::XLinkTargetAt (int i)
{
	if (i>=0 && i<xlink.size())
	{
		if (xlink.at(i))
			return xlink.at(i)->otherBranch (this);
	}
	return NULL;
}

void BranchObj::setIncludeImagesVer(bool b)
{
	includeImagesVer=b;
	calcBBoxSize();
	positionBBox();
	requestReposition();
}

bool BranchObj::getIncludeImagesVer()
{
	return includeImagesVer;
}

void BranchObj::setIncludeImagesHor(bool b)
{
	includeImagesHor=b;
	calcBBoxSize();
	positionBBox();
	requestReposition();
}

bool BranchObj::getIncludeImagesHor()
{
	return includeImagesHor;
}

QString BranchObj::getIncludeImageAttr()
{
	QString a;
	if (includeImagesVer)
		a=attribut ("incImgV","true");
	else
		a=attribut ("incImgV","false");
	if (includeImagesHor)
		a+=attribut ("incImgH","true");
	else
		a+=attribut ("incImgH","false");
	return a;	
}

FloatImageObj* BranchObj::addFloatImage ()
{
	FloatImageObj *newfi=new FloatImageObj (scene,this);
	floatimage.append (newfi);
	if (hasScrolledParent(this) )
		newfi->setVisibility (false);
	else	
		newfi->setVisibility(visible);
		/*
	calcBBoxSize();
	positionBBox();
	*/
	requestReposition();
	return newfi;
}

FloatImageObj* BranchObj::addFloatImage (FloatImageObj *fio)
{
	FloatImageObj *newfi=new FloatImageObj (scene,this);
	floatimage.append (newfi);
	newfi->copy (fio);
	if (hasScrolledParent(this) )
		newfi->setVisibility (false);
	else	
		newfi->setVisibility(visible);
		/*
	calcBBoxSize();
	positionBBox();
	*/
	requestReposition();
	return newfi;
}

FloatImageObj* BranchObj::getFirstFloatImage ()
{
    return floatimage.first();
}

FloatImageObj* BranchObj::getLastFloatImage ()
{
    return floatimage.last();
}

FloatImageObj* BranchObj::getFloatImageNum (const uint &i)
{
    return floatimage.at(i);
}

void BranchObj::removeFloatImage (FloatImageObj *fio)
{
	int i=floatimage.indexOf (fio);
	if (i>-1) delete (floatimage.takeAt (i));
	calcBBoxSize();
	positionBBox();
	requestReposition();
}

void BranchObj::savePosInAngle ()
{
	// Save position in angle
	for (int i=0; i<branch.size(); ++i)
		branch.at(i)->angle=i;
}

void BranchObj::setDefAttr (BranchModification mod)
{
	int fontsize;
	int baseNodeSize = 0;
	if (getenv("VYM_NODE_FONT_SIZE"))
		baseNodeSize = QString(getenv("VYM_NODE_FONT_SIZE")).toInt();

	if (baseNodeSize > 0)
	{
		switch (depth)
		{
			case 0: fontsize = baseNodeSize + 6; break;
			case 1: fontsize = baseNodeSize + 2; break;
			default: fontsize = baseNodeSize; break;
		}
	}
	else
	{
		switch (depth)
		{
			case 0: fontsize=18; break;
			case 1: fontsize=14; break;
			default: fontsize=12; break;
		}
	}	

	setLinkColor ();
	setLinkStyle(getDefLinkStyle());
	QFont font("Sans Serif,8,-1,5,50,0,0,0,0,0");
	font.setPointSize(fontsize);
	heading->setFont(font );

	if (mod==NewBranch)
		setColor (((BranchObj*)(parObj))->getColor());
	
	calcBBoxSize();
}

BranchObj* BranchObj::addBranch()
{
    BranchObj* newbo=new BranchObj(scene,this);
    branch.append (newbo);
    newbo->setParObj(this);
	newbo->setDefAttr(NewBranch);
    newbo->setHeading ("new");
	if (scrolled)
		newbo->setVisibility (false);
	else	
		newbo->setVisibility(visible);
	newbo->updateLink();	
	requestReposition();
	return newbo;
}

BranchObj* BranchObj::addBranch(BranchObj* bo)
{
    BranchObj* newbo=new BranchObj(scene,this);
    branch.append (newbo);
    newbo->copy(bo);
    newbo->setParObj(this);
	newbo->setDefAttr(MovedBranch);
	if (scrolled)
		newbo->setVisibility (false);
	else	
		newbo->setVisibility(bo->visible);
	newbo->updateLink();	
	requestReposition();
	return newbo;
}

BranchObj* BranchObj::addBranchPtr(BranchObj* bo)
{
	branch.append (bo);
	bo->setParObj (this);
	bo->depth=depth+1;
	bo->setDefAttr(MovedBranch);
	if (scrolled) tmpUnscroll();
	setLastSelectedBranch (bo);
	return bo;
}

BranchObj* BranchObj::insertBranch(int pos)
{
	savePosInAngle();
	// Add new bo and resort branches
	BranchObj *newbo=addBranch ();
	newbo->angle=pos-0.5;
	qSort (branch.begin(),branch.end(), isAbove);
	return newbo;
}

BranchObj* BranchObj::insertBranch(BranchObj* bo, int pos)
{
	savePosInAngle();
	// Add new bo and resort branches
	bo->angle=pos-0.5;
	BranchObj *newbo=addBranch (bo);
	qSort (branch.begin(),branch.end(), isAbove);
	return newbo;
}

BranchObj* BranchObj::insertBranchPtr (BranchObj* bo, int pos)
{
	savePosInAngle();
	// Add new bo and resort branches
	bo->angle=pos-0.5;
	branch.append (bo);
	bo->setParObj (this);
	bo->depth=depth+1;
	bo->setDefAttr (MovedBranch);
	if (scrolled) tmpUnscroll();
	setLastSelectedBranch (bo);
	qSort (branch.begin(),branch.end(), isAbove);
	return bo;
}

void BranchObj::removeBranchHere(BranchObj* borem)
{
	// This removes the branch bo from list, but 
	// inserts its childs at the place of bo
	BranchObj *bo;
	bo=borem->getLastBranch();
	int pos=borem->getNum();
	while (bo)
	{
		bo->linkTo (this,pos+1);
		bo=borem->getLastBranch();
	}	
	removeBranch (borem);
}

void BranchObj::removeChilds()
{
	clear();
}

void BranchObj::removeBranch(BranchObj* bo)
{
    // if bo is not in branch remove returns false, we
    // don't care...
	
	int i=branch.indexOf(bo);
    if (i>=0)
	{
		delete (bo);
		branch.removeAt (i);
	} else
		qWarning ("BranchObj::removeBranch tried to remove non existing branch?!\n");
	requestReposition();
}

void BranchObj::removeBranchPtr(BranchObj* bo)
{
	int i=branch.indexOf(bo);
	
	if (i>=0)
		branch.removeAt (i);
	else	
		qWarning ("BranchObj::removeBranchPtr tried to remove non existing branch?!\n");
	requestReposition();
}

void BranchObj::setLastSelectedBranch (BranchObj* bo)
{
    lastSelectedBranch=branch.indexOf(bo);
}

BranchObj* BranchObj::getLastSelectedBranch ()
{
    if (lastSelectedBranch>=0)
	{
		if ( branch.size()>lastSelectedBranch) 
			return branch.at(lastSelectedBranch);
		if (branch.size()>0)
			return branch.last();
	}	
    return NULL;
}

BranchObj* BranchObj::getFirstBranch ()
{
	if (branch.size()>0)
		return branch.first();
	else
		return NULL;
}

BranchObj* BranchObj::getLastBranch ()
{
	if (branch.size()>0)
		return branch.last();
	else
		return NULL;
}

BranchObj* BranchObj::getBranchNum (int i)
{
	if (i>=0 && i<branch.size())
		return branch.at(i);
	else	
		return	NULL;
}

bool BranchObj::canMoveBranchUp() 
{
	if (!parObj || depth==1) return false;
	BranchObj* par=(BranchObj*)parObj;
	if (this==par->getFirstBranch())
		return false;
	else
		return true;
}

BranchObj* BranchObj::moveBranchUp(BranchObj* bo1) // modify my childlist
{
	savePosInAngle();
    int i=branch.indexOf(bo1);
    if (i>0) 
	{	// -1 if bo1 not found 
		branch.at(i)->angle--;
		branch.at(i-1)->angle++;
		qSort (branch.begin(),branch.end(), isAbove);
		return branch.at(i);
	} else
		return NULL;
}

bool BranchObj::canMoveBranchDown() 
{
	if (!parObj|| depth==1) return false;
	BranchObj* par=(BranchObj*)parObj;
	if (this==par->getLastBranch())
		return false;
	else
		return true;
}

BranchObj* BranchObj::moveBranchDown(BranchObj* bo1)// modify my childlist
{
	savePosInAngle();
    int i=branch.indexOf(bo1);
	int j;
	if (i <branch.size())
	{
		j = i+1;
		branch.at(i)->angle++;
		branch.at(j)->angle--;
		qSort (branch.begin(),branch.end(), isAbove);
		return branch.at(i);
	} else
		return NULL;
}

void BranchObj::sortChildren()
{
	int childCount=branch.count();
	int curChildIndex;
	bool madeChanges=false;
	do
	{
		madeChanges=false;
		for(curChildIndex=1;curChildIndex<childCount;curChildIndex++){
			BranchObj* curChild=(BranchObj*)branch.at(curChildIndex);
			BranchObj* prevChild=(BranchObj*)branch.at(curChildIndex-1);
			if(prevChild->heading->text().compare(curChild->heading->text())>0)
			{
				this->moveBranchUp(curChild);
				madeChanges=true;
			}
		}
	}while(madeChanges);
}


BranchObj* BranchObj::linkTo (BranchObj* dst, int pos)
{
	// Find current parent and 
	// remove pointer to myself there
	if (!dst) return NULL;
	BranchObj *par=(BranchObj*)parObj;
	if (par)
		par->removeBranchPtr (this);
	else
		return NULL;

	// Create new pointer to myself at dst
	if (pos<0||dst->getDepth()==0)
	{	
		// links myself as last branch at dst
		dst->addBranchPtr (this);
		updateLink();
		return this;
	} else
	{
		// inserts me at pos in parent of dst
		if (par)
		{
			BranchObj *bo=dst->insertBranchPtr (this,pos);
			bo->setDefAttr(MovedBranch);
			updateLink();
			return bo;

		} else
			return NULL;
	}	
}

void BranchObj::alignRelativeTo (QPointF ref,bool alignSelf)
{
	qreal th = bboxTotal.height();	
// TODO testing
/*
	QPointF pp; if (parObj) pp=parObj->getChildPos();
	cout << "BO::alignRelTo "<<qPrintable (getHeading());
	cout << "    d="<<depth<<
		"  ref="<<ref<<
//		"  bbox.topLeft="<<bboxTotal.topLeft()<<
		"  absPos="<<absPos<<
		"  relPos="<<relPos<<
		"  parPos="<<pp<<
		"  orient="<<orientation<<
//		"  pad="<<topPad<<","<<botPad<<","<<leftPad<<","<<rightPad<<
//		"  hidden="<<hidden<<
//		"  th="<<th<<
		endl;
*/

	setOrientation();
	//updateLink();

	if (depth<2)
	{
		if (depth==1)
		{
			// Position relatively, if needed
			//if (useRelPos) move2RelPos (relPos.x(), relPos.y());

			// Calc angle to mapCenter if I am a mainbranch
			// needed for reordering the mainbranches clockwise 
			// around mapcenter 
			angle=getAngle (QPointF ((int)(x() - parObj->getChildPos().x() ), 
									(int)(y() - parObj->getChildPos().y() ) ) );
		}							
	} 
	else
    {
		// Align myself depending on orientation and parent, but
		// only if I am not a mainbranch or mapcenter itself

		if (anim.isAnimated())
		{
			move2RelPos(anim);
		} else
		{
			LinkableMapObj::Orientation o;
			o=parObj->getOrientation();
			if (alignSelf)
				switch (orientation) 
				{
					case LinkableMapObj::LeftOfCenter:
						move (ref.x() - bbox.width(), ref.y() + (th-bbox.height())/2 );
					break;
					case LinkableMapObj::RightOfCenter:	
						move (ref.x() , ref.y() + (th-bbox.height())/2  );
					break;
					default:
						qWarning ("LMO::alignRelativeTo: oops, no orientation given...");
					break;
				}	
		}
    }		

	if (scrolled) return;

    // Set reference point for alignment of childs
    QPointF ref2;
    if (orientation==LinkableMapObj::LeftOfCenter)
		ref2.setX(bbox.topLeft().x() - linkwidth);
    else	
		ref2.setX(bbox.topRight().x() + linkwidth);

	if (depth==1)
		ref2.setY(absPos.y()-(bboxTotal.height()-bbox.height())/2);
	else	
		ref2.setY(ref.y() );	

    // Align the childs depending on reference point 
	for (int i=0; i<branch.size(); ++i)
    {	
		if (!branch.at(i)->isHidden())
		{
			branch.at(i)->alignRelativeTo (ref2,true);
			ref2.setY(ref2.y() + branch.at(i)->getBBoxSizeWithChilds().height() );
		}
    }
}


void BranchObj::reposition()
{	
/* TODO testing only
	if (!getHeading().isEmpty())
		cout << "BO::reposition  "<<qPrintable(getHeading())<<endl;
	else	
		cout << "BO::reposition  ???"<<endl;

	cout << "  orient="<<orientation<<endl;
*/		

	if (depth==0)
	{
		// only calculate the sizes once. If the deepest LMO 
		// changes its height,
		// all upper LMOs have to change, too.
		calcBBoxSizeWithChilds();
		updateLink();	// This update is needed if the scene is resized 
						// due to excessive moving of a FIO

	    alignRelativeTo ( QPointF (absPos.x(),
			absPos.y()-(bboxTotal.height()-bbox.height())/2) );
		qSort (branch.begin(),branch.end(), isAbove);
		positionBBox();	// Reposition bbox and contents
	} else
	{
		// This is only important for moving branches:
		// For editing a branch it isn't called...
	    alignRelativeTo ( QPointF (absPos.x(),
							absPos.y()-(bboxTotal.height()-bbox.height())/2) );
	}
}

void BranchObj::unsetAllRepositionRequests()
{
	repositionRequest=false;
	for (int i=0; i<branch.size(); ++i)
		branch.at(i)->unsetAllRepositionRequests();
}


QPolygonF BranchObj::shape()
{
	QPolygonF p;

	QRectF r=getTotalBBox();
	if (orientation==LinkableMapObj::LeftOfCenter)
		p   <<r.bottomLeft()
			<<r.topLeft()
			<<QPointF (bbox.topLeft().x(), r.topLeft().y() )
			<<bbox.topRight()
			<<bbox.bottomRight()
			<<QPointF (bbox.bottomLeft().x(), r.bottomLeft().y() ) ;
	else		
		p   <<r.bottomRight()
			<<r.topRight()
			<<QPointF (bbox.topRight().x(), r.topRight().y() )
			<<bbox.topLeft()
			<<bbox.bottomLeft()
			<<QPointF (bbox.bottomRight().x(), r.bottomRight().y() ) ;
	return p;
}

QRectF BranchObj::getTotalBBox()
{
	QRectF r=bbox;

	if (scrolled) return r;

	for (int i=0; i<branch.size(); ++i)
		if (!branch.at(i)->isHidden())
			r=addBBox(branch.at(i)->getTotalBBox(),r);

	for (int i=0; i<floatimage.size(); ++i)
		if (!floatimage.at(i)->isHidden())
			r=addBBox(floatimage.at(i)->getTotalBBox(),r);
		
	return r;
}

QRectF BranchObj::getBBoxSizeWithChilds()
{
	return bboxTotal;
}

void BranchObj::calcBBoxSizeWithChilds()
{	
	// This is initially called only from reposition and
	// and only for mapcenter. So it won't be
	// called more than once for a single user 
	// action
	

	// Calculate size of LMO including all childs (to align them later)
	bboxTotal.setX(bbox.x() );
	bboxTotal.setY(bbox.y() );

	// if branch is scrolled, ignore childs, but still consider floatimages
	if (scrolled)
	{
		bboxTotal.setWidth (bbox.width());
		bboxTotal.setHeight(bbox.height());
		return;
	}
	
	if (hidden)
	{
		bboxTotal.setWidth (0);
		bboxTotal.setHeight(0);
		if (parObj)
		{
			bboxTotal.setX (parObj->x());
			bboxTotal.setY (parObj->y());
		} else
		{
			bboxTotal.setX (bbox.x());
			bboxTotal.setY (bbox.y());
		}
		return;
	}
	
	QRectF r(0,0,0,0);
	QRectF br;
	// Now calculate recursivly
	// sum of heights 
	// maximum of widths 
	// minimum of y
	for (int i=0; i<branch.size(); ++i)
	{
		if (!branch.at(i)->isHidden())
		{
			branch.at(i)->calcBBoxSizeWithChilds();
			br=branch.at(i)->getBBoxSizeWithChilds();
			r.setWidth( max (br.width(), r.width() ));
			r.setHeight(br.height() + r.height() );
			if (br.y()<bboxTotal.y()) bboxTotal.setY(br.y());
		}
	}
	// Add myself and also
	// add width of link to sum if necessary
	if (branch.isEmpty())
		bboxTotal.setWidth (bbox.width() + r.width() );
	else	
		bboxTotal.setWidth (bbox.width() + r.width() + linkwidth);
	
	bboxTotal.setHeight(max (r.height(),  bbox.height()));
}

void BranchObj::select()
{
	// update NoteEditor
	textEditor->setText(note.getNote() );
	QString fnh=note.getFilenameHint();
	if (fnh!="")
		textEditor->setFilenameHint(note.getFilenameHint() );
	else	
		textEditor->setFilenameHint(getHeading() );
	textEditor->setFontHint (note.getFontHint() );
	isNoteInEditor=true;

	// set selected and visible
    LinkableMapObj::select();

	// Tell parent that I am selected now:
	BranchObj* po=(BranchObj*)(parObj);
    if (po)	// TODO	    Try to get rid of this cast...
        po->setLastSelectedBranch(this);
		
	// temporary unscroll, if we have scrolled parents somewhere
	if (parObj) ((BranchObj*)(parObj))->tmpUnscroll();

	// Show URL and link in statusbar
	QString status;
	if (!url.isEmpty()) status+="URL: "+url+"  ";
	if (!vymLink.isEmpty()) status+="Link: "+vymLink;
	if (!status.isEmpty()) mainWindow->statusMessage (status);

	// Update Toolbar
	updateFlagsToolbar();

	// Update actions
	mapEditor->updateActions();
}

void BranchObj::unselect()
{
	LinkableMapObj::unselect();
	// Delete any messages like vymLink in StatusBar
	mainWindow->statusMessage ("");

	// Save current note
	if (isNoteInEditor) getNoteFromTextEditor();
	isNoteInEditor=false;

	// reset temporary unscroll, if we have scrolled parents somewhere
	if (parObj) ((BranchObj*)(parObj))->resetTmpUnscroll();

	// Erase content of editor 
	textEditor->setInactive();

	// unselect all buttons in toolbar
	standardFlagsDefault->updateToolbar();
}

QString BranchObj::getSelectString()
{
	return mapEditor->getModel()->getSelectString (this);
}

void BranchObj::setAnimation(const AnimPoint &ap)
{
	anim=ap;
}

bool BranchObj::animate()
{
	anim.animate ();
	if ( anim.isAnimated() )
	{
		setRelPos (anim);
		return true;
	}
	parObj->reposition();	// object might have been relinked meanwhile
	return false;
}

