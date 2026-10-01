#include <QImageReader>
#include <QPixmap>

#include "floatimageobj.h"
#include "branchobj.h"

/////////////////////////////////////////////////////////////////
// FloatImageObj
/////////////////////////////////////////////////////////////////

uint FloatImageObj::saveCounter=0;		// make instance 


FloatImageObj::FloatImageObj ():FloatObj()
{
//    cout << "Const FloatImageObj ()\n";
    setParObj (this);	
    init();
    depth=-1;
}

FloatImageObj::FloatImageObj (QGraphicsScene* s):FloatObj(s)
{
 //   cout << "Const FloatImageObj (s)  called from MapCenterObj (s)\n";
    setParObj (this);	
    init();
    depth=-1;
}

FloatImageObj::FloatImageObj (QGraphicsScene *s, OrnamentedObj* p):FloatObj(s,p)
{
 //   cout << "Const FloatImageObj (c,p)\n";
    init();
}

FloatImageObj::~FloatImageObj ()
{
//	cout << "Destr FloatImageObj "<<this<<"\n";
	delete(icon);
}

void FloatImageObj::init () 
{
	icon=new ImageObj (scene);
	icon->setPos (absPos.x(), absPos.y() );
	icon->setVisibility (true);
	setZValue (Z_FLOATIMG);
	bbox.setSize (QSizeF(icon->boundingRect().width(), icon->boundingRect().height()));
	clickBox.setSize (QSizeF(icon->boundingRect().width(), icon->boundingRect().height()));filename="";
	originalFilename="no original name available";
	filetype="";
	saveInMap=true;
	useRelPos=true;

	//Hide flags
	systemFlags->setShowFlags(false);
}

void FloatImageObj::copy (FloatImageObj* other)
{					
	FloatObj::copy (other);
	icon->copy (other->icon);
	filetype=other->filetype;
	filename=other->filename;
	originalFilename=other->originalFilename;
	saveInMap=other->saveInMap;
    positionBBox();
}

void FloatImageObj::setZValue (const int &i)
{
	icon->setZValue (i);
	zPlane=i;
}

int FloatImageObj::z ()
{
	return qRound (icon->zValue());
}

void FloatImageObj::load (const QPixmap &pixmap)
{
	icon->load(pixmap);
	bbox.setSize (QSizeF(icon->boundingRect().width()+8, icon->boundingRect().height()+8));
	clickBox.setSize (QSizeF(icon->boundingRect().width()+8, icon->boundingRect().height()+8));
	positionBBox();
	filetype="PNG";
	filename="No filename given";
}

bool FloatImageObj::load (const QString &fn)
{	
	QImageReader reader (fn);
	QImage img;

	if( reader.read (&img))
	{
		icon->setPixmap(QPixmap::fromImage(img));
		bbox.setSize (QSizeF(icon->boundingRect().width()+8, icon->boundingRect().height()+8));
		positionBBox();
		filename=fn;
		filetype=reader.format();
		setOriginalFilename (fn);
		return true;
	} else
		return false;
	
}

void FloatImageObj::save (const QString &fn, const QString &format)
{	
	icon->save (fn,qPrintable (format));
}

void FloatImageObj::setOriginalFilename(const QString & fn)
{
	originalFilename=fn;

	// Set short name, too. Search from behind:
	int i=originalFilename.findRev("/");
	if (i>=0) originalFilename=originalFilename.remove (0,i+1);
}

QString FloatImageObj::getOriginalFilename()
{
	return originalFilename;
}

void FloatImageObj::setVisibility(bool v)
{
    OrnamentedObj::setVisibility(v);
	if (v)
	    icon->setVisibility(true);
	else
	    icon->setVisibility(false);
}

void FloatImageObj::move (double x, double y)
{
	FloatObj::move(x,y);
	icon->setPos (x+4,y+4);
    positionBBox();
}

void FloatImageObj::move (QPointF p)
{
	OrnamentedObj::move (p.x(),p.y());
}

void FloatImageObj::positionBBox()
{
	clickBox=bbox;
}

void FloatImageObj::calcBBoxSize()
{
	// TODO
}

QString FloatImageObj::saveToDir (const QString &tmpdir,const QString &prefix)
{
	if (hidden) return "";

	saveCounter++;
	
	QString useOrientAttr;
	if (useOrientation)
		useOrientAttr=attribut ("useOrientation","true");
	else	
		useOrientAttr=attribut ("useOrientation","false");
		
	QString saveInMapAttr;
	if (saveInMap)
		saveInMapAttr=attribut ("saveInMap","true");
	else	
		
		saveInMapAttr=attribut ("saveInMap","false");

	QString exportAttr;
	if (floatExport)
		exportAttr=attribut ("floatExport","true");
	else	
		exportAttr=attribut ("floatExport","false");

	QString zAttr=attribut ("zPlane",QString().setNum(zPlane));
	QString url;

	url="images/"+prefix+"image-" + QString().number(saveCounter,10) + ".png" ;

	// And really save the image
	icon->save (tmpdir +"/"+ url, "PNG");
 
	QString nameAttr=attribut ("orgName",originalFilename);

    return singleElement ("floatimage",  
		getOrnXMLAttr() 
		+useOrientAttr 
		+saveInMapAttr 
		+exportAttr  
		+zAttr  
		+attribut ("href",QString ("file:")+url)
		+nameAttr
	);	
}

void FloatImageObj::resetSaveCounter()
{
	saveCounter=0;
}


QRectF FloatImageObj::getTotalBBox()
{
	return bbox;
}

QRectF FloatImageObj::getBBoxSizeWithChilds()
{
	//TODO abstract in linkablemapobj.h, not calculated
	return bboxTotal;
}

void FloatImageObj::calcBBoxSizeWithChilds()
{
	//TODO abstract in linkablemapobj.h
}

/*
QString FloatImageObj::getSelectString()  //FIXME move to selection
{
	QString s;
	if (parObj)
	{
		if (parObj->getDepth()==0)
			s= "fi:" + QString("%1").arg( ((BranchObj*)(parObj))->getFloatImageNum(this));
		else	
			s= ((BranchObj*)(parObj))->getSelectString() + ",fi:" + QString("%1").arg( ((BranchObj*)(parObj))->getFloatImageNum(this));
	} else
		s="mc:";
	
	return s;

}
*/

