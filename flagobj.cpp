#include "flagobj.h"

/////////////////////////////////////////////////////////////////
// FlagObj
/////////////////////////////////////////////////////////////////
FlagObj::FlagObj()
{
//	cout << "Const FlagObj ()\n";
    init ();
}

FlagObj::FlagObj(QGraphicsScene* s):MapObj(s) 
{
//	cout << "Const FlagObj  this="<<this<<"  scene="<<s<<endl;
    init ();
}

FlagObj::FlagObj (FlagObj* io)
{
    copy (io);
}

FlagObj::~FlagObj()
{
//   cout << "Destr FlagObj  this="<<this <<"  " << qPrintable(name) << "\n";
	if (icon) delete (icon);
}


void FlagObj::init ()
{
	name="undefined";
	unsetGroup();

	icon=new ImageObj (scene);
	icon->setPos (absPos.x(), absPos.y() );
	state=false;
	used=false;
	avis=true;
	action=NULL;
}

void FlagObj::copy (FlagObj* other)
{
    MapObj::copy(other);
	name=other->name;
	group=other->group;
	tooltip=other->tooltip;
	state=other->state;
	avis=other->avis;
	used=other->used;
	icon->copy(other->icon);
	action=other->action;
	setVisibility (other->isVisibleObj() );
}

void FlagObj::move(double x, double y)
{
    MapObj::move(x,y);
	icon->setPos(x,y);
	positionBBox();
}

void FlagObj::moveBy(double x, double y)
{
    move (x+absPos.x(),y+absPos.y() );
}

void FlagObj::setVisibility (bool v)
{
    MapObj::setVisibility(v);
	if (v && state)
	    icon->setVisibility(true);
	else
	    icon->setVisibility(false);
}

void FlagObj::load (const QString &fn)
{
	icon->load(fn);
	calcBBoxSize();
	positionBBox();
}

void FlagObj::load (const QPixmap &pm)
{
	icon->load(pm);
	calcBBoxSize();
	positionBBox();
}

void FlagObj::setName(const QString &n)
{
	name=n;
}

const QString FlagObj::getName()
{
	return name;
}

void FlagObj::setGroup (const QString &n)
{
	group=n;
}

const QString FlagObj::getGroup()
{
	return group;
}

void FlagObj::unsetGroup()
{
	group="undefined";
}

void FlagObj::setToolTip(const QString &n)
{
	tooltip=n;
}

const QString FlagObj::getToolTip()
{
	return tooltip;
}

QPixmap FlagObj::getPixmap()
{
	return icon->pixmap();
}

void FlagObj::setAction (QAction* a)
{
	action=a;
}

void FlagObj::updateAction ()
{
	if (action) 
	{
		action->setChecked(state);
		if (!avis)
			action->setVisible(state);
	}
}

void FlagObj::setAlwaysVisible(bool b)
{
	avis=b;
}

bool FlagObj::isAlwaysVisible()
{
	return avis;
}

bool FlagObj::isActive()
{
	return state;
}

void FlagObj::toggle()
{
	if (state)
		deactivate();
	else
		activate();
}

void FlagObj::activate()
{
	state=true;
	// only show icon, if flag itself is visible 
	if (visible) 
	{
		icon->setVisibility (true);
		//icon->setPos (100,100);
		//icon->setPos (rand()%100,rand()%130);
		calcBBoxSize();
	}	
}

void FlagObj::deactivate()
{
	state=false;
	// if flag itself is invisible we don't need to call 
	if (visible) 
	{
		icon->setVisibility (false);
		calcBBoxSize();
	}	
}

void FlagObj::setUsed (bool b)
{
	used=b;
}

bool FlagObj::isUsed()
{
	return used;
}

void FlagObj::saveToDir (const QString &tmpdir, const QString &prefix)
{
	QString fn=tmpdir + prefix + name + ".png";
	icon->save (fn,"PNG");
}

void FlagObj::positionBBox()
{
    bbox.moveTopLeft (absPos );
    clickBox.moveTopLeft (absPos );
}

void FlagObj::calcBBoxSize()
{
	if (visible && state)
	{
		bbox.setSize (	QSizeF(
			icon->boundingRect().width(), 
			icon->boundingRect().height() ) );
	} else
	{
		bbox.setSize (QSizeF(0,0));
	}
	clickBox.setSize (bbox.size());
}

