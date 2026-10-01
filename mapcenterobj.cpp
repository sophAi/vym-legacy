#include <QDateTime>

#include "floatimageobj.h"
#include "geometry.h"
#include "mapcenterobj.h"
#include "mapeditor.h"


/////////////////////////////////////////////////////////////////
// MapCenterObj
/////////////////////////////////////////////////////////////////
MapCenterObj::MapCenterObj() : BranchObj ()
{
//    cout << "Const MapCenterObj\n";
    init();
}

MapCenterObj::MapCenterObj(QGraphicsScene* s) : BranchObj (s)
{
//    cout << "Const MapCenterObj   canvas="<<s<<"\n";
    init();
}

MapCenterObj::~MapCenterObj() 
{
//    cout << "Destr MapCenterObj\n";
	clear();
}	

void MapCenterObj::clear() 
{
	BranchObj::clear();
}

void MapCenterObj::init () 
{
	BranchObj::init();
    orientation=LinkableMapObj::UndefinedOrientation;

	// TODO this should be done in TextObj later...
	//QFont font ("Sans Serif,16,-1,5,50,0,0,0,0,0");		
	//heading->setFont(font);
	depth=0;
	setDefAttr(MovedBranch);

	frame->setFrameType (FrameObj::Rectangle);
}

void MapCenterObj::move (double x, double y)
{
	BranchObj::move(x,y);
}

void MapCenterObj::move (QPointF absPos)
{
	BranchObj::move(absPos);
}

void MapCenterObj::moveBy (double x, double y)
{
	BranchObj::moveBy(x,y);
}

void MapCenterObj::moveAll (double x, double y)
{
	// Get rel. position
	double dx=x-absPos.x();
	double dy=y-absPos.y();

	// Move myself and branches
	moveAllBy (dx,dy);
}

void MapCenterObj::moveAllBy (double dx, double dy)
{
	// Move myself and childs
	BranchObj::moveBy(dx,dy);
}

void MapCenterObj::updateLink()
{
	// set childPos to middle of MapCenterObj
	childPos.setX( clickBox.topLeft().x() + (int)(clickBox.width())/2 );
	childPos.setY( clickBox.topLeft().y() + (int)(clickBox.height())/2 );
	parPos=childPos;		
	for (int i=0; i<branch.size(); ++i)
		branch.at(i)->updateLink();
}

void MapCenterObj::updateRelPositions()
{
	if (repositionRequest) unsetAllRepositionRequests();

	// update relative Positions of branches and floats
	for (int i=0; i<branch.size(); ++i)
	{
		branch.at(i)->setRelPos();
		branch.at(i)->setOrientation();
	}
	
	for (int i=0; i<floatimage.size(); ++i)
		floatimage.at(i)->setRelPos();

	if (repositionRequest) reposition();
}

LinkableMapObj* MapCenterObj::findMapObj(QPointF p, LinkableMapObj *excludeLMO)
	{
	LinkableMapObj *lmo;

	// Search through child branches
	for (int i=0; i<branch.size(); ++i)
	{	
		lmo = branch.at(i)->findMapObj(p, excludeLMO);
		if (lmo!= NULL) return lmo;
	}
	// is p in MapCenter?
	if (inBox (p,clickBox) && (this != excludeLMO) ) return this;

	// Search float images
	for (int i=0; i<floatimage.size(); ++i)
		if (inBox(p,floatimage.at(i)->getClickBox()) && (floatimage.at(i) != excludeLMO) && floatimage.at(i)->getParObj()!= excludeLMO) return floatimage.at(i);

	// nothing found
	return NULL;
}

QString MapCenterObj::saveToDir (const QString &tmpdir,const QString &prefix, int verbose, const QPointF &offset)
{
    QString s,a;

	// save area, if not scrolled
	QString areaAttr=
		attribut("x1",QString().setNum(absPos.x()-offset.x())) +
		attribut("y1",QString().setNum(absPos.y()-offset.y())) +
		attribut("x2",QString().setNum(absPos.x()+width()-offset.x())) +
		attribut("y2",QString().setNum(absPos.y()+height()-offset.y()));
	
	// Providing an ID for a branch makes export to XHTML easier
	QString idAttr;
	if (countXLinks()>0)
		idAttr=attribut ("id",mapEditor->getModel()->getSelectString(this)); //TODO directly access model

	else
		idAttr="";

	QString linkAttr=getLinkAttr();

    s=beginElement ("mapcenter" 
		+getOrnXMLAttr() 
		+areaAttr 
		+idAttr 
		+getIncludeImageAttr() );
    incIndent();
    if (heading->getColor()!=QColor("black"))
		a=attribut ("textColor",QColor(heading->getColor()).name() );
    else	
		a="";
    
	// Save flags. If verbose is set (export to xml dir), also write
	// the flags as picture
	s+=standardFlags->saveToDir(tmpdir+"/flags", "/standardFlag-", verbose);

	// Save heading
    s+=valueElement("heading", getHeading(),a);

	// Save frame
	s+=frame->saveToDir ();

	// Update of note is usually done while unselecting a branch
	if (isNoteInEditor) getNoteFromTextEditor();

	// save note
	if (!note.isEmpty() )
		s+=note.saveToDir();
	
	// Save branches
	for (int i=0; i<branch.size(); ++i)
		s+=branch.at(i)->saveToDir(tmpdir,prefix, offset);

	// Save FloatImages
	for (int i=0; i<floatimage.size(); ++i)
		s+=floatimage.at(i)->saveToDir (tmpdir,prefix);

	// Save XLinks
	for (int i=0;i<xlink.size(); ++i)
		s+=xlink.at(i)->saveToDir();

    decIndent();
    s+=endElement   ("mapcenter");
    return s;
}

void MapCenterObj::setVersion (const QString &s)
{
	version=s;
}

void MapCenterObj::setAuthor (const QString &s)
{
	author=s;
}

QString MapCenterObj::getAuthor()
{
	return author;
}

void MapCenterObj::setComment (const QString &s)
{
	comment=s;
}

QString MapCenterObj::getComment ()
{
	return comment;
}

QString MapCenterObj::getDate ()
{
	return QDate::currentDate().toString ("yyyy-MM-dd");
}

