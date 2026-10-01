#include "floatobj.h"

extern QAction* actionEditToggleFloatExport;

/////////////////////////////////////////////////////////////////
// FloatObj
/////////////////////////////////////////////////////////////////

FloatObj::FloatObj ():OrnamentedObj() 
{
//    cout << "Const FloatObj ()\n";
    setParObj (this);	
    init();
    depth=-1;
}

FloatObj::FloatObj (QGraphicsScene* s):OrnamentedObj(s)
{
//	cout << "Const FloatObj (s)  called from MapCenterObj (s)\n";
    setParObj (this);	
    init();
    depth=-1;
}

FloatObj::FloatObj (QGraphicsScene* s, LinkableMapObj* p):OrnamentedObj (s)
{
//    cout << "Const FloatObj (s,p)\n";
    setParObj (p);	
    depth=p->getDepth()+1;
    init();
}

FloatObj::~FloatObj ()
{
//   cout << "Destr FloatObj\n";
}

void FloatObj::init () 
{
    setChildObj(this);
	floatExport=true;
	zPlane=Z_ICON;
	setLinkStyle (LinkableMapObj::Parabel);
	setHideLinkUnselected(true);
}

void FloatObj::copy (FloatObj* other)
{
	LinkableMapObj::copy (other);
	setVisibility (other->visible);
}

void FloatObj::setZValue(const int &i)
{
	zPlane=i;
}

int FloatObj::zValue()
{
	return zPlane;
}

void FloatObj::setFloatExport(const bool& b)
{
	floatExport=b;
}

bool FloatObj::getFloatExport()
{
	return floatExport;
}

void FloatObj::move (double x, double y)
{
	MapObj::move(x,y);
}

void FloatObj::move (QPointF p)
{
	MapObj::move (p);
}

void FloatObj::setDockPos()
{
	parPos=absPos;
	childPos=absPos;
}

void FloatObj::reposition()
{
	move2RelPos (relPos);
	updateLink();	
}

QRectF FloatObj::getTotalBBox()
{
	return bbox;
}

QRectF FloatObj::getBBoxSizeWithChilds()
{
	return bboxTotal;
}

void FloatObj::select()
{
    LinkableMapObj::select();
	LinkableMapObj::setLinkColor(parObj->getLinkColor());
}

void FloatObj::unselect()
{
	LinkableMapObj::unselect();
}

