#include "xlinkobj.h"
#include "branchobj.h"


/////////////////////////////////////////////////////////////////
// XLinkObj
/////////////////////////////////////////////////////////////////

int XLinkObj::arrowSize=10;						// make instances 

XLinkObj::XLinkObj ():MapObj() 
{
	//	cout << "Const XLinkObj ()\n";
	init();
}

XLinkObj::XLinkObj (QGraphicsScene* s):MapObj(s)
{
	//	cout << "Const XLinkObj (s)  called from MapCenterObj (s)\n";
	init();
}


XLinkObj::~XLinkObj ()
{
//	cout << "Destr XLinkObj\n";
	if (xLinkState!=undefinedXLink)
		deactivate();
	delete (line);
	delete (poly);
}


void XLinkObj::init () 
{
	beginBranch=NULL;
	endBranch=NULL;
	visBranch=NULL;
	xLinkState=undefinedXLink;

	color=QColor (180,180,180);
	width=1;
	pen.setColor (color);
	pen.setWidth (width);
	pen.setCapStyle (  Qt::RoundCap );
	line=scene->addLine(QLineF(1,1,1,1),pen);
    line->setZValue (Z_XLINK);
	poly=scene->addPolygon(QPolygonF(),pen,color);
    poly->setZValue (Z_XLINK);
	setVisibility (false);
}

void XLinkObj::copy (XLinkObj* other)
{
	// TODO copy not used yet
	MapObj::copy (other);
	setVisibility (other->visible);
	beginBranch=other->beginBranch;
	endBranch=other->endBranch;
	width=other->width;

}

void XLinkObj::setBegin (BranchObj *bo)
{
	if (bo) 
	{
		xLinkState=initXLink;
		beginBranch=bo;
		beginPos=beginBranch->getChildPos();
	}	
}

BranchObj* XLinkObj::getBegin ()
{
	return beginBranch;
}

void XLinkObj::setEnd (BranchObj *bo)
{
	if (bo) 
	{
		xLinkState=initXLink;
		endBranch=bo;
		endPos=endBranch->getChildPos();
	}		
}

BranchObj* XLinkObj::getEnd()
{
	return endBranch;
}

void XLinkObj::setWidth (int w)
{
	width=w;
	pen.setWidth (w);
	setColor (color);
}

int XLinkObj::getWidth()
{
	return pen.width();
}

void XLinkObj::setColor(QColor c)
{
	color=c;
	pen.setColor (c);
	line->setPen (pen);
	poly->setBrush( color );
}

QColor XLinkObj::getColor()
{
	return pen.color();
}

void XLinkObj::setEnd (QPointF p)
{
	endPos=p;
}

bool XLinkObj::activate ()
{
	if (beginBranch && endBranch)
	{
		if (beginBranch==endBranch) return false;
		xLinkState=activeXLink;
		beginBranch->addXLink (this);
		endBranch->addXLink (this);
		setVisibility ();
		return true;
	} else
		return false;
}

void XLinkObj::deactivate ()
{
	if (beginBranch)
		beginBranch->removeXLinkRef (this);
	beginBranch=NULL;	
	if (endBranch)
		endBranch->removeXLinkRef (this);
	endBranch=NULL;	
	visBranch=NULL;
	xLinkState=undefinedXLink;

	line->hide();
}

bool XLinkObj::isUsed()
{
	if (beginBranch || endBranch || xLinkState!=undefinedXLink)
		return true;
	else
		return false;
}

void XLinkObj::updateXLink()
{
	QPointF a,b;
	QPolygonF pa;
	if (visBranch)
	{
		// Only one of the linked branches is visible
		a=b=visBranch->getChildPos();
		if (visBranch->getOrientation()==LinkableMapObj::RightOfCenter)
		{
			b.setX (b.x()+25);
			
			pa.clear();
			pa<< QPointF(b.x(),b.y())<<
				QPointF(b.x()-arrowSize,b.y()-arrowSize)<<
				QPointF(b.x()-arrowSize,b.y()+arrowSize);
			poly->setPolygon(pa);
		} else
		{
			b.setX (b.x()-25);
			pa.clear();
			pa<< QPointF(b.x(),b.y())<<
				QPointF(b.x()+arrowSize,b.y()-arrowSize)<<
				QPointF(b.x()+arrowSize,b.y()+arrowSize);
			poly->setPolygon (pa);
		}	
	} else
	{
		// Both linked branches are visible
		if (beginBranch)
			// If a link is just drawn in the editor,
			// we have already a beginBranch
			a=beginBranch->getChildPos();
		else
			// This shouldn't be reached normally...
			a=beginPos;
		if (xLinkState==activeXLink && endBranch)
			b=endBranch->getChildPos();
		else
			b=endPos;
	}


	if (line->line().p1()==a && line->line().p2()==b && !visBranch)
	{
		// update is called from both branches, so only
		// update if something has changed
		return;
	}	
	else
	{
		beginPos=a;
		endPos=b;
		line->setPen (pen);
		line->setLine(a.x(), a.y(), b.x(), b.y());
	}
}

BranchObj* XLinkObj::otherBranch(BranchObj* thisBranch)
{
	if (!beginBranch && !endBranch)
		return NULL;
	if (thisBranch==beginBranch)
		return endBranch;
	else	
		return beginBranch;
}

void XLinkObj::positionBBox()
{
}

void XLinkObj::calcBBoxSize()
{
}

void XLinkObj::setVisibility (bool b)
{
	MapObj::setVisibility (b);
	if (b)
	{
		line->show();
		if (visBranch) 
			poly->show();
		else	
			poly->hide();
	}	
	else
	{
		line->hide();
		poly->hide();
	}	
}

void XLinkObj::setVisibility ()
{
	if (beginBranch && endBranch)
	{
		if(beginBranch->isVisibleObj() && endBranch->isVisibleObj())
		{	// Both ends are visible
			visBranch=NULL;
			setVisibility (true);
		} else
		{
			if(!beginBranch->isVisibleObj() && !endBranch->isVisibleObj())
			{	//None of the ends is visible
				visBranch=NULL;
				setVisibility (false);
			} else
			{	// Just one end is visible, draw a symbol that shows
				// that there is a link to a scrolled branch
				if (beginBranch->isVisibleObj())
					visBranch=beginBranch;
				else
					visBranch=endBranch;
				setVisibility (true);
			}
		}
	}
}

QString XLinkObj::saveToDir ()
{
	QString s="";
	if (beginBranch && endBranch &&xLinkState==activeXLink)
	{
		if (beginBranch==endBranch && xLinkState)
			s="";
		else
		{
			QString colAttr=attribut ("color",color.name());
			QString widAttr=attribut ("width",QString().setNum(width,10));
			QString begSelAttr=attribut ("beginID",beginBranch->getSelectString());
			QString endSelAttr=attribut ("endID",  endBranch->getSelectString());
			s=beginElement ("xlink", colAttr +widAttr +begSelAttr +endSelAttr);

			s+=endElement ("xlink");
		}
	}
	return s;
}

