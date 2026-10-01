#include "flagrowobj.h"
#include "geometry.h"

#include <QToolBar>

/////////////////////////////////////////////////////////////////
// FlagRowObj
/////////////////////////////////////////////////////////////////
FlagRowObj::FlagRowObj()
{
//    cout << "Const FlagRowObj ()\n";
    init ();
}

FlagRowObj::FlagRowObj(QGraphicsScene* s):MapObj(s) 
{
//    cout << "Const FlagRowObj (s)\n";
    init ();
}

FlagRowObj::~FlagRowObj()
{
	//cout << "Destr FlagRowObj\n";
	while (!flag.isEmpty())
		delete (flag.takeFirst() );
}

void FlagRowObj::init ()
{
	parentRow=NULL;
	showFlags=true;
}

void FlagRowObj::copy (FlagRowObj* other)
{
    MapObj::copy(other);
	parentRow=other->parentRow;
	flag.clear();
	for (int i=0; i<flag.size(); ++i)
		addFlag (flag.at(i));
}

void FlagRowObj::clone (FlagRowObj* pr)
{
	// Difference to copy:
	// We don't copy the flags here, they
	// are created on the fly by toggle and activate
	// This saves lots of canvas objects.
	MapObj::copy(pr);
	flag.clear();
	parentRow=pr;
}

void FlagRowObj::move(double x, double y)
{
    MapObj::move(x,y);
	qreal dx=0;
	for (int i=0; i<flag.size(); ++i)
	{
		flag.at(i)->move(x+dx,y);
		dx+=QSizeF(flag.at(i)->getSize() ).width();
	}
}

void FlagRowObj::moveBy(double x, double y)
{
    move (x+absPos.x(),y+absPos.y() );
}

void FlagRowObj::setVisibility (bool v)
{
	MapObj::setVisibility(v);
	for (int i=0; i<flag.size(); ++i)
		flag.at(i)->setVisibility (v);
}

FlagObj* FlagRowObj::addFlag (FlagObj *fo)
{
	FlagObj *newfo=new FlagObj (scene);
	newfo->copy (fo);	// create a deep copy of fo
	newfo->move (absPos.x() + bbox.width(), absPos.y() );
	flag.append(newfo);
	calcBBoxSize();
	positionBBox();
	return newfo;
}

void FlagRowObj::positionBBox()
{
    bbox.moveTopLeft(absPos );
    clickBox.moveTopLeft(absPos );
}

void FlagRowObj::calcBBoxSize()
{
	QSizeF size(0,0);
	QSizeF boxsize(0,0);
	for (int i=0; i<flag.size(); ++i)
	{
		size=flag.at(i)->getSize();
		// add widths
		boxsize.setWidth(boxsize.width() + size.width() );
		// maximize height
		if (size.height() > boxsize.height() ) 
			boxsize.setHeight(size.height() );
	}
	bbox.setSize (boxsize);
	clickBox.setSize (boxsize);
}

QString FlagRowObj::getFlagName (const QPointF &p)
{
	if (!inBox (p,clickBox)) return "";
	for (int i=0; i<flag.size(); ++i)
		if (inBox (p,flag.at(i)->getClickBox ())) return flag.at(i)->getName();
	return "";	

	
}

bool FlagRowObj::isActive (const QString &foname)
{
	FlagObj *fo=findFlag (foname);
	if (parentRow && fo)
		return fo->isActive();
	else
		if (fo) return true;
	return false;
}

void FlagRowObj::toggle (const QString &foname, bool exclusive)
{
	FlagObj *fo=findFlag (foname);
	if (fo)
	{
		// FlagObj is here, it will be active, too.
		// Deactivate it by removing it from this row.
		flag.remove (fo);
		delete (fo);
	} else
	{
		// FlagObj is not present in this row.
		// Copy it from parentRow
		fo=parentRow->findFlag (foname);
		if (fo)
		{
			fo=addFlag (fo);
			fo->activate();
			if (exclusive) 
			{
				deactivateGroup (fo);
				updateToolbar();
			}
		} else
			qWarning ("FlagRowObj ("+name+")::toggle ("+foname+")  failed - could not find it in parentRow");
	}	
	calcBBoxSize();
	positionBBox();	
}

void FlagRowObj::activate (const QString &foname)
{
	// Note: "activate" is also called during loading of a map
	// Here we do not check for exclusive flags!
	FlagObj *fo=findFlag (foname);
	if (parentRow)
	{
		if (!fo)
		{
			// FlagObj is not present in this row.
			// Copy it from parentRow and activate there
			fo=parentRow->findFlag (foname);
			if (fo)
			{
				fo=addFlag (fo);
				fo->activate();
				if (showFlags) 
					fo->setVisibility (visible);
				else
					fo->setVisibility (false);
				calcBBoxSize();
			} else
				qWarning ("FlagRowObj ("+name+")::activate ("+foname+")  failed - could not find it in parentRow");
		}	
	} else
	{
		// I am the parentRow, mark flag as used
		if (fo)
		{
			fo->setUsed(true);
			fo->activate();
		}	
		else
			qWarning ("FlagRowObj::activate no FlagObj \""+foname+"\" found in parentRow");
	}
}


void FlagRowObj::deactivate (const QString &foname)
{
	FlagObj *fo=findFlag (foname);
	if (fo) 
	{
		flag.remove(fo);
		delete (fo);
	}	
	calcBBoxSize();
	positionBBox();
}

void FlagRowObj::deactivateAll ()
{
	if (!parentRow)
	{
		for (int i=0; i<flag.size(); ++i)
			if (flag.at(i)->isActive()) flag.at(i)->deactivate();
	} else
	{
		while (!flag.isEmpty())
			delete flag.takeFirst();
		calcBBoxSize();
		positionBBox();
	}
}

void FlagRowObj::deactivateGroup (FlagObj *keepfo)
{
	// deactivate all flags in keepof, but keep keepfo [sic!]
	if (keepfo)
	{
		QString g=keepfo->getGroup();
		if (g!="undefined")
		{
			for (int i=0; i<flag.size(); ++i)
				if (g==flag.at(i)->getGroup() && keepfo!=flag.at(i)) 
				{
					FlagObj *fo=flag.at(i);
					flag.remove (fo);
					delete (fo);
				}	
		}		
	}	
}

void FlagRowObj::setToolBar(QToolBar *tb)
{
	toolbar=tb;
}

void FlagRowObj::setEnabled (bool b)
{
	if (toolbar)
	{
		toolbar->setEnabled (b);
	}
}

void FlagRowObj::setShowFlags (bool b)
{
	showFlags=b;
}

void FlagRowObj::resetUsedCounter()
{
	for (int i=0; i<flag.size(); ++i)
		flag.at(i)->setUsed (false);
}

QString FlagRowObj::saveToDir (const QString &tmpdir,const QString &prefix, bool writeflags)
{
	// Build xml string
	QString s;
	if (parentRow)
		for (int i=0; i<flag.size(); ++i)
		{
			// save flag to xml, if flag is set 
			s+=valueElement("standardflag",flag.at(i)->getName() );

			// and tell parentRow, that this flag is used
			parentRow->activate(flag.at(i)->getName() );
		}	
	else
		// Save icons to dir, if verbose is set (xml export)
		// and I am a parentRow 
		// and this flag is really used somewhere
		if (writeflags)
			for (int i=0; i<flag.size(); ++i)
				if (flag.at(i)->isUsed()) flag.at(i)->saveToDir (tmpdir,prefix);
	return s;		

}

void FlagRowObj::setName (const QString &n)
{
	name=n;
}

void  FlagRowObj::updateToolbar()
{
	if (parentRow)
	{
		// We are just a branch, not the toolbar default
		// but state has to be copied from ourselves to parentrow!
		parentRow->deactivateAll();
		// In parentRow activate all existing (==active) flags
		for (int i=0; i<flag.size(); ++i)
			parentRow->activate(flag.at(i)->getName());
		parentRow->updateToolbar();	
	} else
	{
		// We are the toolbar default
		if (toolbar)
		{
			// Update state of actions in toolbar
			for (int i=0; i<flag.size(); ++i)
				flag.at(i)->updateAction();
		}	
	}
}

FlagObj* FlagRowObj::findFlag (const QString &name)
{
	for (int i=0; i<flag.size(); ++i)
		if (flag.at(i)->getName()==name) return flag.at(i);
	return NULL;
}

