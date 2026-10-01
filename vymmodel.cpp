#include <QApplication>
#include <typeinfo>

#include "geometry.h"		// for addBBox
#include "vymmodel.h"


extern Settings settings;

VymModel::VymModel() 
{
//    cout << "Const VymModel\n";
}


VymModel::~VymModel() 
{
    //cout << "Destr VymModel\n";
	clear();
}	

void VymModel::clear() 
{
	while (!mapCenters.isEmpty())
		delete mapCenters.takeFirst();
}

void VymModel::init () 
{
	addMapCenter();

	// animations
	animationUse=settings.readBoolEntry("/animation/use",false);
	animationTicks=settings.readNumEntry("/animation/ticks",10);
	animationInterval=settings.readNumEntry("/animation/interval",50);
	animObjList.clear();
	animationTimer=new QTimer (this);
	connect(animationTimer, SIGNAL(timeout()), this, SLOT(animate()));

}

void VymModel::setMapEditor(MapEditor *me)
{
	mapEditor=me;
	for (int i=0; i<mapCenters.count(); i++)
		mapCenters.at(i)->setMapEditor(mapEditor);
}

MapEditor* VymModel::getMapEditor()
{
	return mapEditor;
}

void VymModel::setVersion (const QString &s)
{
	version=s;
}

void VymModel::setAuthor (const QString &s)
{
	author=s;
}

QString VymModel::getAuthor()
{
	return author;
}

void VymModel::setComment (const QString &s)
{
	comment=s;
}

QString VymModel::getComment ()
{
	return comment;
}

QString VymModel::getDate ()
{
	return QDate::currentDate().toString ("yyyy-MM-dd");
}

void VymModel::setScene (QGraphicsScene *s)
{
	mapScene=s;
    init();	// Here we have a mapScene set, 
			// which is (still) needed to create MapCenters
}

QGraphicsScene* VymModel::getScene ()
{
	return mapScene;
}

MapCenterObj* VymModel::addMapCenter()
{
	return addMapCenter (QPointF(0,0));
}

MapCenterObj* VymModel::addMapCenter(QPointF absPos)
{
	MapCenterObj *mapCenter = new MapCenterObj(mapScene);
	mapCenter->move (absPos);
    mapCenter->setVisibility (true);
	mapCenter->setHeading (QApplication::translate("Heading of mapcenter in new map", "New map"));
	mapCenter->setMapEditor(mapEditor);		//FIXME needed to get defLinkStyle, mapLinkColorHint ... for later added objects
	mapCenters.append(mapCenter);
	return mapCenter;
}

MapCenterObj *VymModel::removeMapCenter(MapCenterObj* mco)
{
	int i=mapCenters.indexOf (mco);
	if (i>=0)
	{
		mapCenters.removeAt (i);
		delete (mco);
		if (i>0) return mapCenters.at(i-1);	// Return previous MCO
	}
	return NULL;
}

MapCenterObj *VymModel::getMapCenterNum (int i)
{
	if (i>mapCenters.count()-1 || i<0)
		return NULL;
	else
		return mapCenters.at(i);
}

int VymModel::countMapCenters()
{
	return mapCenters.count();
}

int VymModel::countBranches()
{
	uint b=0;
	BranchObj *bo=first();
	while (bo) 
	{
		b++;
		bo=next(bo);
	}
	return b;
}

BranchObj* VymModel::first()
{
	if (mapCenters.count()>0) 
		return mapCenters.first();
	else	
		return NULL;
}
	
BranchObj* VymModel::next(BranchObj *bo_start)
{
	BranchObj *rbo;
	BranchObj *bo=bo_start;
	if (bo)
	{
		// Try to find next branch in current MapCenter
		rbo=bo->next();
		if (rbo) return rbo;

		// Try to find MapCenter of bo
		while (bo->getDepth()>0) bo=(BranchObj*)bo->getParObj();

		// Try to find next MapCenter
		int i=mapCenters.indexOf ((MapCenterObj*)bo);
		if (i+2 > mapCenters.count() || i<0) return NULL;
		if (mapCenters.at(i+1)!=bo_start)
			return mapCenters.at(i+1);
	} 
	return NULL;
}

LinkableMapObj* VymModel::findMapObj(QPointF p, LinkableMapObj *excludeLMO)
{
	LinkableMapObj *lmo;

	for (int i=0;i<mapCenters.count(); i++)
	{
		lmo=mapCenters.at(i)->findMapObj (p,excludeLMO);
		if (lmo) return lmo;
	}
	return NULL;
}

LinkableMapObj* VymModel::findObjBySelect(const QString &s)
{
	LinkableMapObj *lmo;
	if (!s.isEmpty() )
	{
		QString part;
		QString typ;
		QString num;
		part=s.section(",",0,0);
		typ=part.left (2);
		num=part.right(part.length() - 3);
		if (typ=="mc" && num.toInt()>=0 && num.toInt() <mapCenters.count() )
			return mapCenters.at(num.toInt() );
	}		

	for (int i=0; i<mapCenters.count(); i++)
	{
		lmo=mapCenters.at(i)->findObjBySelect(s);
		if (lmo) return lmo;
	}	
	return NULL;
}

LinkableMapObj* VymModel::findID (const QString &s)
{
	LinkableMapObj *lmo;
	for (int i=0; i<mapCenters.count(); i++)
	{
		lmo=mapCenters.at(i)->findID (s);
		if (lmo) return lmo;
	}	
	return NULL;
}

QString VymModel::saveToDir (const QString &tmpdir,const QString &prefix, int verbose, const QPointF &offset)
{
    QString s;

	for (int i=0; i<mapCenters.count(); i++)
		s+=mapCenters.at(i)->saveToDir (tmpdir,prefix,verbose,offset);
    return s;
}


//////////////////////////////////////////////
// View related
//////////////////////////////////////////////

void VymModel::updateRelPositions()
{
	for (int i=0; i<mapCenters.count(); i++)
		mapCenters.at(i)->updateRelPositions();
}

void VymModel::reposition()
{
	for (int i=0;i<mapCenters.count(); i++)
		mapCenters.at(i)->reposition();	//	for positioning heading
}

QPolygonF VymModel::shape(BranchObj *bo)
{
	// Creating (arbitrary) shapes

	QPolygonF p;
	QRectF rb=bo->getBBox();
	if (bo->getDepth()==0)
	{
		// Just take BBox of this mapCenter
		p<<rb.topLeft()<<rb.topRight()<<rb.bottomRight()<<rb.bottomLeft();
		return p;
	}

	// Take union of BBox and TotalBBox 

	QRectF ra=bo->getTotalBBox();
	if (bo->getOrientation()==LinkableMapObj::LeftOfCenter)
		p   <<ra.bottomLeft()
			<<ra.topLeft()
			<<QPointF (rb.topLeft().x(), ra.topLeft().y() )
			<<rb.topRight()
			<<rb.bottomRight()
			<<QPointF (rb.bottomLeft().x(), ra.bottomLeft().y() ) ;
	else		
		p   <<ra.bottomRight()
			<<ra.topRight()
			<<QPointF (rb.topRight().x(), ra.topRight().y() )
			<<rb.topLeft()
			<<rb.bottomLeft()
			<<QPointF (rb.bottomRight().x(), ra.bottomRight().y() ) ;
	return p;		

}

void VymModel::moveAway(LinkableMapObj *lmo)
{
	// Autolayout:
	//
	// Move all branches and MapCenters away from lmo 
	// to avoid collisions 

	QPolygonF pA;
	QPolygonF pB;

	BranchObj *boA=(BranchObj*)lmo;
	BranchObj *boB;
	for (int i=0; i<mapCenters.count(); i++)
	{
		boB=mapCenters.at(i);
		pA=shape (boA);
		pB=shape (boB);
		PolygonCollisionResult r = PolygonCollision(pA, pB, QPoint(0,0));
		cout <<"------->"
			<<"="<<r.intersect
			<<"  ("<<qPrintable(boA->getHeading() )<<")"
			<<"  with ("<< qPrintable (boB->getHeading() )
			<<")  willIntersect"
			<<r.willIntersect 
			<<"  minT="<<r.minTranslation<<endl<<endl;
	}
}

void VymModel::animate()
{
	animationTimer->stop();
	BranchObj *bo;
	int i=0;
	while (i<animObjList.size() )
	{
		bo=(BranchObj*)animObjList.at(i);
		if (!bo->animate())
		{
			if (i>=0) animObjList.removeAt(i);
			i--;
		}
		bo->reposition();
		i++;
	} 
	mapEditor->updateSelection();
	mapScene->update();
	if (!animObjList.isEmpty() ) animationTimer->start();
}


void VymModel::startAnimation(const QPointF &start, const QPointF &dest)
{
	BranchObj *bo=getSelectedBranch();
	if (bo && bo->getDepth()>0) 
	{
		AnimPoint ap;
		ap.setStart (start);
		ap.setDest  (dest);
		ap.setTicks (animationTicks);
		ap.setAnimated (true);
		bo->setAnimation (ap);
		animObjList.append( bo );
		animationTimer->setSingleShot (true);
		animationTimer->start(animationInterval);
	}
}

void VymModel::stopAnimation(MapObj *mo)
{
	int i=animObjList.indexOf(mo);
    if (i>=0)
		animObjList.removeAt (i);
}

//////////////////////////////////////////////
// Selection related
//////////////////////////////////////////////


// Only as long as we dont have Model/View yet
LinkableMapObj* VymModel::getSelection()
{
	return mapEditor->getSelection();
}
BranchObj* VymModel::getSelectedBranch()
{
	return mapEditor->getSelectedBranch();
}


bool VymModel::select (const QString &s)
{
	return mapEditor->select (s);
}

QString VymModel::getSelectString (LinkableMapObj *lmo)
{
	QString s;
	if (!lmo) return s;
	if (typeid(*lmo)==typeid(BranchObj) ||
		typeid(*lmo)==typeid(MapCenterObj) )
	{	
		LinkableMapObj *par=lmo->getParObj();
		if (par)
		{
			if (lmo->getDepth() ==1)
				// Mainbranch, return 
				s= "bo:" + QString("%1").arg(((BranchObj*)lmo)->getNum());
			else	
				// Branch, call myself recursively
				s= getSelectString(par) + ",bo:" + QString("%1").arg(((BranchObj*)lmo)->getNum());
		} else
		{
			// MapCenter
			int i=mapCenters.indexOf ((MapCenterObj*)lmo);
			if (i>=0) s=QString("mc:%1").arg(i);
		}	
	}	
	return s;

}

	
void VymModel::setHideTmp (HideTmpMode mode)
{
	for (int i=0;i<mapCenters.count(); i++)
		mapCenters.at(i)->setHideTmp (mode);	
}

QRectF VymModel::getTotalBBox()
{
	QRectF r;
	for (int i=0;i<mapCenters.count(); i++)
		r=addBBox (mapCenters.at(i)->getTotalBBox(), r);
	return r;	
}

