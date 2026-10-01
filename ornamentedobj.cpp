#include <typeinfo> 

#include "ornamentedobj.h"
#include "texteditor.h"
#include "mapeditor.h"
#include "linkablemapobj.h"

extern TextEditor *textEditor;
extern FlagRowObj *systemFlagsDefault;
extern FlagRowObj *standardFlagsDefault;


/////////////////////////////////////////////////////////////////
// OrnamentedObj
/////////////////////////////////////////////////////////////////

OrnamentedObj::OrnamentedObj():LinkableMapObj()
{
  //  cout << "Const OrnamentedObj ()\n";
    init ();
}

OrnamentedObj::OrnamentedObj(QGraphicsScene* s) :LinkableMapObj(s)
{
//    cout << "Const OrnamentedObj (s)\n";
    init ();
}

OrnamentedObj::OrnamentedObj (OrnamentedObj* lmo) : LinkableMapObj (lmo->scene)
{
    copy (lmo);
}

OrnamentedObj::~OrnamentedObj()
{
    delete heading;
	delete systemFlags;
	delete standardFlags;
	delete frame;
}


void OrnamentedObj::init ()
{
	heading = new HeadingObj(scene);
	heading->move (absPos.x(), absPos.y());

	note.setNote("");
	note.setFontHint (textEditor->getFontHintDefault() );
	isNoteInEditor=false;

	systemFlags=new FlagRowObj(scene);
	systemFlags->clone(systemFlagsDefault);
	systemFlags->setName ("systemFlags");
	
	standardFlags=new FlagRowObj(scene);
	standardFlags->clone(standardFlagsDefault);
	standardFlags->setName ("standardFlags");

	attributes.clear();

	frame = new FrameObj (scene);

	hideExport=false;
	hidden=false;

	url="";
	vymLink="";
	
}

void OrnamentedObj::copy (OrnamentedObj* other)
{
    LinkableMapObj::copy(other);
	heading->copy(other->heading);
    setColor   (other->heading->getColor());	

	note.copy (other->note);
	systemFlags->copy (other->systemFlags);
	standardFlags->copy (other->standardFlags);

	ornamentsBBox=other->ornamentsBBox;

	hideExport=other->hideExport;
	url=other->url;
	vymLink=other->vymLink;
}

QString OrnamentedObj::getHeading()
{
    return heading->text();
}

void OrnamentedObj::setLinkColor()
{
	if (mapEditor->getMapLinkColorHint()==HeadingColor)
		LinkableMapObj::setLinkColor (heading->getColor());
	else	
		LinkableMapObj::setLinkColor (mapEditor->getMapDefLinkColor());
}

void OrnamentedObj::setColor (QColor col)
{
    heading->setColor(col);
	setLinkColor();
}

QColor OrnamentedObj::getColor ()
{
    return heading->getColor();
}

FrameObj::FrameType OrnamentedObj::getFrameType()
{
	return frame->getFrameType();
}

QString OrnamentedObj::getFrameTypeName()
{
	return frame->getFrameTypeName();
}

void OrnamentedObj::setFrameType(const FrameObj::FrameType &t)
{
	frame->setFrameType(t);
	if (t == FrameObj::NoFrame)
		linkpos=LinkableMapObj::Bottom;
	else	
		linkpos=LinkableMapObj::Middle;

	calcBBoxSize();
	positionBBox();
	requestReposition();
}

void OrnamentedObj::setFrameType(const QString &t)
{
	frame->setFrameType(t);
	if (frame->getFrameType() == FrameObj::NoFrame)
		linkpos=LinkableMapObj::Bottom;
	else	
		linkpos=LinkableMapObj::Middle;

	calcBBoxSize();
	positionBBox();
	requestReposition();
}

void OrnamentedObj::setFramePadding (const int &i)
{
	frame->setPadding (i);
	calcBBoxSize();
	positionBBox();
	requestReposition();
}

int OrnamentedObj::getFramePadding ()
{
	return frame->getPadding();
}

void OrnamentedObj::setFrameBorderWidth (const int &i)
{
	frame->setBorderWidth(i);
	calcBBoxSize();
	positionBBox();
	requestReposition();
}

int OrnamentedObj::getFrameBorderWidth()
{
	return frame->getBorderWidth();
}

void OrnamentedObj::setFramePenColor(QColor col)
{
	frame->setPenColor (col);
}

QColor OrnamentedObj::getFramePenColor()
{
	return frame->getPenColor ();
}

void OrnamentedObj::setFrameBrushColor(QColor col)
{
	frame->setBrushColor (col);
}

QColor OrnamentedObj::getFrameBrushColor()
{
	return frame->getBrushColor ();
}

void OrnamentedObj::positionContents()
{
	double d=frame->getPadding()/2;
	double x=absPos.x();
	double y=absPos.y();

	double ox,oy;	// Offset due to padding

	ox=leftPad + d;
	oy=topPad + d;
	
	systemFlags-> move (ox +x , oy + y );

	// vertical align heading to bottom
    heading->move (ox + x + systemFlags->getBBox().width(),
				   oy + y + ornamentsBBox.height() - heading->getHeight() 
					);
	standardFlags->move (ox +x + heading->getWidth() + systemFlags->getBBox().width() , oy + y );

	ornamentsBBox.moveTopLeft ( QPointF ((int)(ox+x),(int)(oy+y)));
	clickBox.moveTopLeft (QPointF ((int)(ox + x), (int)(oy + y)));
}

void OrnamentedObj::move (double x, double y)
{
	MapObj::move (x,y);
	positionContents();
	updateLink();
	requestReposition();
}

void OrnamentedObj::move (QPointF p)
{
	move (p.x(), p.y());
}	

void OrnamentedObj::moveBy (double x, double y)
{

	MapObj::moveBy (x,y);
    frame->moveBy (x,y);
    systemFlags->moveBy (x,y);
    standardFlags->moveBy (x,y);
    heading->moveBy (x,y);
	updateLink();
	requestReposition();
}

void OrnamentedObj::moveBy (QPointF p)
{
	moveBy (p.x(), p.y());
}	

void OrnamentedObj::move2RelPos(double x, double y)
{
	setRelPos (QPointF(x,y));
	if (parObj)
	{
		QPointF p=parObj->getChildPos();
		move (p.x()+x, p.y() +y);
	}
}

void OrnamentedObj::move2RelPos(QPointF p)
{
	move2RelPos (p.x(),p.y());
}

void OrnamentedObj::setNote(QString s)
{
	note.setNote(s);
	updateNoteFlag();
}

void OrnamentedObj::setNote(NoteObj n)
{
	note=n;
	updateNoteFlag();
}

QString OrnamentedObj::getNote()
{
    return note.getNote();
}

QString OrnamentedObj::getNoteASCII(const QString &indent, const int &width)
{
    return note.getNoteASCII(indent,width);
}

QString OrnamentedObj::getNoteASCII()
{
    return note.getNoteASCII();
}

QString OrnamentedObj::getNoteOpenDoc()
{
    return note.getNoteOpenDoc();
}

void OrnamentedObj::setURL(QString s)
{
	url=s;
	if (!url.isEmpty())
		systemFlags->activate("url");
	else	
		systemFlags->deactivate("url");
	calcBBoxSize();			// recalculate bbox
    positionBBox();			// rearrange contents
	forceReposition();
}

QString OrnamentedObj::getURL()
{
	return url;
}

void OrnamentedObj::setVymLink(QString s)
{
	if (!s.isEmpty())
	{
		// We need the relative (from loading) 
		// or absolute path (from User event)
		// and build the absolute path.
		// Note: If we have relative, use path of
		// current map to build absolute path
		QDir d(s);
		if (!d.path().startsWith ("/"))
		{
			QString p=mapEditor->getDestPath();
			int i=p.findRev("/",-1);
			d.setPath(p.left(i)+"/"+s);
			d.convertToAbs();
		}
		vymLink=d.path();
		systemFlags->activate("vymLink");
	}	
	else	
	{
		systemFlags->deactivate("vymLink");
		vymLink="";
	}	
	calcBBoxSize();			// recalculate bbox
    positionBBox();			// rearrange contents
	forceReposition();
}

QString OrnamentedObj::getVymLink()
{
	return vymLink;
}


void OrnamentedObj::setAttributes (const QList <Attribute> &al)
{
	attributes=al;
}

QList <Attribute> OrnamentedObj::getAttributes()
{
	return attributes;
}

void OrnamentedObj::clearStandardFlags()
{
	standardFlags->deactivateAll();
	calcBBoxSize();
	positionBBox();
	move (absPos.x(), absPos.y() );
	forceReposition();
}

void OrnamentedObj::toggleStandardFlag(QString f, bool exclusive)
{
	standardFlags->toggle(f,exclusive);
	calcBBoxSize();
	positionBBox();
	move (absPos.x(), absPos.y() );
	forceReposition();
}

void OrnamentedObj::activateStandardFlag(QString f)
{
	standardFlags->activate(f);
	calcBBoxSize();
	positionBBox();
	move (absPos.x(), absPos.y() );
	forceReposition();
}

void OrnamentedObj::deactivateStandardFlag(QString f)
{
	standardFlags->deactivate(f);
	calcBBoxSize();
	positionBBox();
	move (absPos.x(), absPos.y() );
	forceReposition();
}

bool OrnamentedObj::isSetStandardFlag (QString f)
{
	return standardFlags->isActive(f);
}

QString OrnamentedObj::getSystemFlagName(const QPointF &p)
{
	return systemFlags->getFlagName(p);	
}

bool OrnamentedObj::isActiveFlag (const QString & fname)
{
	if (standardFlags->isActive (fname) ) return true;
	return false;
}

void OrnamentedObj::getNoteFromTextEditor ()
{
	note.setFilenameHint (textEditor->getFilename());
	note.setFontHint (textEditor->getFontHint() );
	setNote( textEditor->getText() );
}

void OrnamentedObj::updateNoteFlag()
{
	bool noteEmpty;
	if (isNoteInEditor)
		noteEmpty=textEditor->isEmpty();
	else	
		noteEmpty=note.isEmpty();

	if (!noteEmpty)
	{	
		if (systemFlags->isActive ("note")) return;
		systemFlags->activate("note");
	}	
	else		
	{	
		if (!systemFlags->isActive ("note")) return;
		systemFlags->deactivate("note");
	}	
	mapEditor->setChanged();
	calcBBoxSize();
	positionBBox();	
	move (absPos.x(), absPos.y() );
	forceReposition();
	
}	

void OrnamentedObj::updateFlagsToolbar()
{
	standardFlags->updateToolbar();
}

void OrnamentedObj::setHideInExport(bool b)
{
	if (parObj)
	{
		// Don't allow to MapCenter to be hidden
		hideExport=b;
		if (b)
			systemFlags->activate("hideInExport");
		else	
			systemFlags->deactivate("hideInExport");
		calcBBoxSize();
		positionBBox();
		requestReposition();	
	}
}	

bool OrnamentedObj::hideInExport()
{
	return hideExport;
}	

bool OrnamentedObj::isHidden()
{
	return hidden;
}	

QString OrnamentedObj::getOrnXMLAttr()
{
	QString posAttr;

	if (depth==0)
		posAttr=		
			attribut("absPosX",QString().setNum(absPos.x())) +
			attribut("absPosY",QString().setNum(absPos.y())); 
	else
	{
		if (depth==1 || typeid (*this)==typeid (FloatImageObj))
		{
			if (relPos.x()==0 && relPos.y()==0)
				setRelPos();
			posAttr=
				attribut("relPosX",QString().setNum(relPos.x())) +
				attribut("relPosY",QString().setNum(relPos.y())); 
		} else
			posAttr="";
	}	

	QString urlAttr;
	if (!url.isEmpty())
		urlAttr=attribut ("url",url);

	QString vymLinkAttr;
	if (!vymLink.isEmpty())
		vymLinkAttr=attribut ("vymLink",convertToRel(mapEditor->getDestPath(),vymLink) );

	QString hideExpAttr;
	if (hideExport)
		hideExpAttr= attribut("hideInExport","true");
	else	
		hideExpAttr="";

	return posAttr +urlAttr +vymLinkAttr +getLinkAttr() +hideExpAttr;
}

