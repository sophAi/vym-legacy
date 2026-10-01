#include <typeinfo>

#include "selection.h"

#include "mainwindow.h"
#include "mapeditor.h"
#include "vymmodel.h"



extern Main *mainWindow;

Selection::Selection()
{
	color= QColor(255,255,0);
}

Selection::~Selection()
{
}

void Selection::setModel (VymModel *m)
{
	model=m;
	scene=model->getScene();
}

void Selection::copy(const Selection &other)
{
	selectList=other.selectList;
	lastSelectList=other.lastSelectList;
}

void Selection::clear()
{
	unselect();
	lastSelectList.clear();
}

void Selection::update()
{
	QRectF bbox;
	int w=0;
	for (int i=0; i< selectList.count(); ++i) 
	{
		bbox=selectList.at(i)->getBBox();
		selboxList.at(i)->setRect (
			bbox.x()-w,bbox.y()-w, 
			bbox.width()+2*w, bbox.height()+2*w);
		selboxList.at(i)->setPen (color);	
		selboxList.at(i)->setBrush (color);	
	}	
}

void Selection::setColor (QColor col)
{
	color=col;
	update();
}

QColor Selection::getColor ()
{
	return color;
}

bool Selection::select(LinkableMapObj *lmo)	// TODO no multiselections yet
{
	if (!selectList.isEmpty()) unselect();
	selectList.append (lmo);
	QGraphicsRectItem *sb = scene->addRect(
		QRectF(0,0,0,0), 
		QPen(color),
		color);
	sb->setZValue(Z_SELBOX);
	sb->show();
	selboxList.append (sb);
	lmo->select();
	update();
	mainWindow->updateSatellites (model->getMapEditor() );	
	return true;
}

bool Selection::select (const QString &s)	// TODO no multiselections yet
{
	LinkableMapObj *lmo=model->findObjBySelect(s);

	// Finally select the found object
	if (lmo)
	{
		unselect();
		select (lmo);
		return true;
	} 
	return false;

}

bool Selection::reselect ()	// TODO no multiselections yet
{
	if (!lastSelectList.isEmpty())
	{
		select (lastSelectList.first());
		return true;
	}
	return false;

}

void Selection::unselect()
{
	if (!selectList.isEmpty() )
	{
		for (int i=0; i< selectList.count(); ++i) 
			selectList.at(i)->unselect();
		lastSelectList=selectList;
		selectList.clear();
		while (!selboxList.isEmpty() )
			delete selboxList.takeFirst();

	}	
}

bool Selection::isEmpty()
{
	return selectList.isEmpty();
}

uint Selection::count()
{
	return selectList.count();
}

Selection::Type Selection::type() // TODO no multiselections yet
{
	if (!selectList.isEmpty())
	{
		LinkableMapObj *sel=selectList.first();
		if (typeid (*sel)==typeid (BranchObj)) return Branch;
		if (typeid (*sel)==typeid (MapCenterObj)) return MapCenter;
		if (typeid (*sel)==typeid (FloatImageObj)) return FloatImage;
	}
	return Undefined;
}

LinkableMapObj* Selection::first()
{
	if (!selectList.isEmpty())
		return selectList.first();
	else	
		return NULL;
}

LinkableMapObj* Selection::single()
{
	if (selectList.count() == 1)
		return selectList.first();
	else	
		return NULL;
}

BranchObj* Selection::getBranch()
{
	if (!selectList.isEmpty())
	{
		LinkableMapObj *sel=selectList.first();
		if (typeid (*sel)==typeid (BranchObj) ||
		    typeid (*sel)==typeid (MapCenterObj)) 
			return (BranchObj*)sel;
	}
		return NULL;
}

FloatImageObj* Selection::getFloatImage()
{
	if (!selectList.isEmpty())
	{
		LinkableMapObj *sel=selectList.first();
		if (typeid (*sel)==typeid (FloatImageObj)) 
			return (FloatImageObj*)sel;
	}
		return NULL;
}

QString Selection::getSelectString()// TODO no multiselections yet
{
	if (selectList.count()==1)
	{
		return model->getSelectString (selectList.first() );
	}
	else
		return"";
}


