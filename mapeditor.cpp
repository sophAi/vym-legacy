#include "mapeditor.h"

#include <q3filedialog.h>

#include <iostream>
#include <cstdlib>
#include <typeinfo>

#include "parser.h"
#include "editxlinkdialog.h"
#include "exports.h"
#include "exportxhtmldialog.h"
#include "extrainfodialog.h"
#include "file.h"
#include "linkablemapobj.h"
#include "mainwindow.h"
#include "misc.h"
#include "texteditor.h"
#include "warningdialog.h"
#include "xml-freemind.h"
#include "xml-vym.h"


extern TextEditor *textEditor;
extern int statusbarTime;
extern Main *mainWindow;
extern QString tmpVymDir;
extern QString clipboardDir;
extern QString clipboardFile;
extern bool clipboardEmpty;
extern bool debug;
extern FlagRowObj *standardFlagsDefault;

extern QMenu* branchContextMenu;
extern QMenu* branchAddContextMenu;
extern QMenu* branchRemoveContextMenu;
extern QMenu* branchLinksContextMenu;
extern QMenu* branchXLinksContextMenuEdit;
extern QMenu* branchXLinksContextMenuFollow;
extern QMenu* floatimageContextMenu;
extern QMenu* canvasContextMenu;


extern Settings settings;
extern ImageIO imageIO;

extern QString vymName;
extern QString vymVersion;

extern QString iconPath;
extern QDir vymBaseDir;
extern QDir lastImageDir;
extern QDir lastFileDir;

int MapEditor::mapNum=0;	// make instance

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
MapEditor::MapEditor( QWidget* parent) :
  QGraphicsView(parent)  
{
	setObjectName ("MapEditor");

	//cout << "Constructor ME "<<this<<endl;
	mapNum++;


	mapScene= new QGraphicsScene(parent);
	//mapScene= new QGraphicsScene(QRectF(0,0,width(),height()), parent);
	mapScene->setBackgroundBrush (QBrush(Qt::white, Qt::SolidPattern));

	model=new VymModel();
	model->setScene (mapScene);
	model->setMapEditor (this);

    setScene (mapScene);

    printer=NULL;

	defLinkColor=QColor (0,0,255);
	defXLinkColor=QColor (180,180,180);
	linkcolorhint=LinkableMapObj::DefaultColor;
	linkstyle=LinkableMapObj::PolyParabel;

	// Create bitmap cursors, platform dependant
	HandOpenCursor=QCursor (QPixmap(iconPath+"cursorhandopen.png"),1,1);		
	PickColorCursor=QCursor ( QPixmap(iconPath+"cursorcolorpicker.png"), 5,27 ); 
	CopyCursor=QCursor ( QPixmap(iconPath+"cursorcopy.png"), 1,1 ); 
	XLinkCursor=QCursor ( QPixmap(iconPath+"cursorxlink.png"), 1,7 ); 

	setFocusPolicy (Qt::StrongFocus);

	pickingColor=false;
	drawingLink=false;
	copyingObj=false;

    editingBO=NULL;
    movingObj=NULL;

	xelection.setModel (model);
	xelection.unselect();

	defXLinkWidth=1;
	defXLinkColor=QColor (230,230,230);

    mapChanged=false;
	mapDefault=true;
	mapUnsaved=false;
	
	zipped=true;
	filePath="";
	fileName=tr("unnamed");
	mapName="";

	stepsTotal=settings.readNumEntry("/mapeditor/stepsTotal",100);
	undoSet.setEntry ("/history/stepsTotal",QString::number(stepsTotal));
	mainWindow->updateHistory (undoSet);
	
	// Initialize find routine
	itFind=NULL;				
	EOFind=false;

	printFrame=true;
	printFooter=true;

	blockReposition=false;
	blockSaveState=false;

	hidemode=HideNone;

	// Create temporary files
	makeTmpDirs();

	curStep=0;
	redosAvail=0;
	undosAvail=0;

	setAcceptDrops (true);	

	model->reposition();

	// autosave
	autosaveTimer=new QTimer (this);
	connect(autosaveTimer, SIGNAL(timeout()), this, SLOT(autosave()));

	fileChangedTimer=new QTimer (this);
	fileChangedTimer->start(3000);
	connect(fileChangedTimer, SIGNAL(timeout()), this, SLOT(fileChanged()));

	// Network
	netstate=Offline;

	// Attributes	//FIXME testing only...
	QString k;
	AttributeDef *ad;
	attrTable= new AttributeTable();
	k="A - StringList";
	ad=attrTable->addKey (k,StringList);
	if (ad)
	{
		QStringList sl;
		sl <<"val 1"<<"val 2"<< "val 3";
		ad->setValue (QVariant (sl));
	}
	//attrTable->addValue ("Key A","P 1");
	//attrTable->addValue ("Key A","P 2");
	//attrTable->addValue ("Key A","P 3");
	//attrTable->addValue ("Key A","P 4");
	k="B - FreeString";
	ad=attrTable->addKey (k,FreeString);
	if (ad)
	{
		//attrTable->addValue ("Key B","w1");
		//attrTable->addValue ("Key B","w2");
	}
	k="C - UniqueString";
	ad=attrTable->addKey (k,UniqueString);
	if (ad)
	{
	//attrTable->addKey ("Key Prio");
	//attrTable->addValue ("Key Prio","Prio 1");
	//attrTable->addValue ("Key Prio","Prio 2");
	}
}

MapEditor::~MapEditor()
{
	//cout <<"Destructor MapEditor "<<mapName.toStdString()<<endl;
	fileChangedTimer->stop();
	clear();

	// tmpMapDir is in tmpVymDir, so it gets removed automagically when vym closes
	
	//removeDir(QDir(tmpMapDir));
	delete (model);
}

VymModel* MapEditor::getModel()
{
    return model;
}

QGraphicsScene * MapEditor::getScene()
{
    return mapScene;
}

MapEditor::State MapEditor::getState()
{
    return state;
}

void MapEditor::setStateEditHeading(bool s)
{
	if (s)
	{
		if (state==Idle) state=EditHeading;
	}
	else	
		state=Idle;
}

bool MapEditor::isRepositionBlocked()
{
	return blockReposition;
}

void MapEditor::setSaveStateBlocked(bool b)
{
	blockSaveState=b;
}

bool MapEditor::isSelectBlocked()
{
	if (state==EditHeading)
		return true;
	else
		return false; 
}

QString MapEditor::getName (const LinkableMapObj *lmo)
{
	QString s;
	if (!lmo) return QString("Error: NULL has no name!");

	if ((typeid(*lmo) == typeid(BranchObj) ||
				      typeid(*lmo) == typeid(MapCenterObj))) 
	{
		
		s=(((BranchObj*)lmo)->getHeading());
		if (s=="") s="unnamed";
		return QString("branch (%1)").arg(s);
	}	
	if ((typeid(*lmo) == typeid(FloatImageObj) ))
		return QString ("floatimage [%1]").arg(((FloatImageObj*)lmo)->getOriginalFilename());
	return QString("Unknown type has no name!");
}

void MapEditor::makeTmpDirs()
{
	// Create unique temporary directories
	tmpMapDir = tmpVymDir+QString("/mapeditor-%1").arg(mapNum);
	histPath = tmpMapDir+"/history";
	QDir d;
	d.mkdir (tmpMapDir);
}

QString MapEditor::saveToDir(const QString &tmpdir, const QString &prefix, bool writeflags, const QPointF &offset, LinkableMapObj *saveSel)
{
	// tmpdir		temporary directory to which data will be written
	// prefix		mapname, which will be appended to images etc.
	// writeflags	Only write flags for "real" save of map, not undo
	// offset		offset of bbox of whole map in scene. 
	//				Needed for XML export
	
	// Save Header
	QString ls;
	switch (linkstyle)
	{
		case LinkableMapObj::Line: 
			ls="StyleLine";
			break;
		case LinkableMapObj::Parabel:
			ls="StyleParabel";
			break;
		case LinkableMapObj::PolyLine:	
			ls="StylePolyLine";
			break;
		default:
			ls="StylePolyParabel";
			break;
	}	

	QString s="<?xml version=\"1.0\" encoding=\"utf-8\"?><!DOCTYPE vymmap>\n";
	QString colhint="";
	if (linkcolorhint==LinkableMapObj::HeadingColor) 
		colhint=attribut("linkColorHint","HeadingColor");

	QString mapAttr=attribut("version",vymVersion);
	if (!saveSel)
		mapAttr+= attribut("author",model->getAuthor()) +
				  attribut("comment",model->getComment()) +
			      attribut("date",model->getDate()) +
				  attribut("branchCount", QString().number(model->countBranches())) +

		          attribut("backgroundColor", mapScene->backgroundBrush().color().name() ) +
		          attribut("selectionColor", xelection.getColor().name() ) +
		          attribut("linkStyle", ls ) +
		          attribut("linkColor", defLinkColor.name() ) +
		          attribut("defXLinkColor", defXLinkColor.name() ) +
		          attribut("defXLinkWidth", QString().setNum(defXLinkWidth,10) ) +
		          colhint; 
	s+=beginElement("vymmap",mapAttr);
	incIndent();

	// Find the used flags while traversing the tree
	standardFlagsDefault->resetUsedCounter();
	
	// Reset the counters before saving
	// TODO constr. of FIO creates lots of objects, better do this in some other way...
	FloatImageObj (mapScene).resetSaveCounter();

	// Build xml recursivly
	if (!saveSel || typeid (*saveSel) == typeid (MapCenterObj))
		// Save complete map, if saveSel not set
		s+=model->saveToDir(tmpdir,prefix,writeflags,offset);
	else
	{
		if ( typeid(*saveSel) == typeid(BranchObj) )
			// Save Subtree
			s+=((BranchObj*)saveSel)->saveToDir(tmpdir,prefix,offset);
		else 
		{
			if ( typeid(*saveSel) == typeid(FloatImageObj) )
				// Save image
				s+=((FloatImageObj*)saveSel)->saveToDir(tmpdir,prefix);
		}	
	}

	// Save local settings
	s+=settings.getDataXML (destPath);

	// Save selection
	if (!xelection.isEmpty() && !saveSel ) 
		s+=valueElement("select",xelection.getSelectString());

	decIndent();
	s+=endElement("vymmap");

	if (writeflags)
		standardFlagsDefault->saveToDir (tmpdir+"/flags/","",writeflags);
	return s;
}

QString MapEditor::getHistoryDir()
{
	QString histName(QString("history-%1").arg(curStep));
	return (tmpMapDir+"/"+histName);
}

void MapEditor::saveState(const SaveMode &savemode, const QString &undoSelection, const QString &undoCom, const QString &redoSelection, const QString &redoCom, const QString &comment, LinkableMapObj *saveSel)
{
	sendData(redoCom);	//FIXME testing

	// Main saveState


	if (blockSaveState) return;

	if (debug) cout << "ME::saveState() for  "<<qPrintable (mapName)<<endl;
	
	// Find out current undo directory
	if (undosAvail<stepsTotal) undosAvail++;
	curStep++;
	if (curStep>stepsTotal) curStep=1;
	
	QString backupXML="";
	QString histDir=getHistoryDir();
	QString bakMapPath=histDir+"/map.xml";

	// Create histDir if not available
	QDir d(histDir);
	if (!d.exists()) 
		makeSubDirs (histDir);

	// Save depending on how much needs to be saved	
	if (saveSel)
		backupXML=saveToDir (histDir,mapName+"-",false, QPointF (),saveSel);
		
	QString undoCommand="";
	if (savemode==UndoCommand)
	{
		undoCommand=undoCom;
	}	
	else if (savemode==PartOfMap )
	{
		undoCommand=undoCom;
		undoCommand.replace ("PATH",bakMapPath);
	}

	if (!backupXML.isEmpty())
		// Write XML Data to disk
		saveStringToDisk (bakMapPath,backupXML);

	// We would have to save all actions in a tree, to keep track of 
	// possible redos after a action. Possible, but we are too lazy: forget about redos.
	redosAvail=0;

	// Write the current state to disk
	undoSet.setEntry ("/history/undosAvail",QString::number(undosAvail));
	undoSet.setEntry ("/history/redosAvail",QString::number(redosAvail));
	undoSet.setEntry ("/history/curStep",QString::number(curStep));
	undoSet.setEntry (QString("/history/step-%1/undoCommand").arg(curStep),undoCommand);
	undoSet.setEntry (QString("/history/step-%1/undoSelection").arg(curStep),undoSelection);
	undoSet.setEntry (QString("/history/step-%1/redoCommand").arg(curStep),redoCom);
	undoSet.setEntry (QString("/history/step-%1/redoSelection").arg(curStep),redoSelection);
	undoSet.setEntry (QString("/history/step-%1/comment").arg(curStep),comment);
	undoSet.setEntry (QString("/history/version"),vymVersion);
	undoSet.writeSettings(histPath);

	if (debug)
	{
		// TODO remove after testing
		//cout << "          into="<< histPath.toStdString()<<endl;
		cout << "    stepsTotal="<<stepsTotal<<
		", undosAvail="<<undosAvail<<
		", redosAvail="<<redosAvail<<
		", curStep="<<curStep<<endl;
		cout << "    ---------------------------"<<endl;
		cout << "    comment="<<comment.toStdString()<<endl;
		cout << "    undoCom="<<undoCommand.toStdString()<<endl;
		cout << "    undoSel="<<undoSelection.toStdString()<<endl;
		cout << "    redoCom="<<redoCom.toStdString()<<endl;
		cout << "    redoSel="<<redoSelection.toStdString()<<endl;
		if (saveSel) cout << "    saveSel="<<qPrintable (model->getSelectString(saveSel))<<endl;
		cout << "    ---------------------------"<<endl;
	}

	mainWindow->updateHistory (undoSet);
	setChanged();
	updateActions();
}


void MapEditor::saveStateChangingPart(LinkableMapObj *undoSel, LinkableMapObj* redoSel, const QString &rc, const QString &comment)
{
	// save the selected part of the map, Undo will replace part of map 
	QString undoSelection="";
	if (undoSel)
		undoSelection=model->getSelectString(undoSel);
	else
		qWarning ("MapEditor::saveStateChangingPart  no undoSel given!");
	QString redoSelection="";
	if (redoSel)
		redoSelection=model->getSelectString(undoSel);
	else
		qWarning ("MapEditor::saveStateChangingPart  no redoSel given!");
		

	saveState (PartOfMap,
		undoSelection, "addMapReplace (\"PATH\")",
		redoSelection, rc, 
		comment, 
		undoSel);
}

void MapEditor::saveStateRemovingPart(LinkableMapObj *redoSel, const QString &comment)
{
	if (!redoSel)
	{
		qWarning ("MapEditor::saveStateRemovingPart  no redoSel given!");
		return;
	}
	QString undoSelection=model->getSelectString (redoSel->getParObj());
	QString redoSelection=model->getSelectString(redoSel);
	if (typeid(*redoSel) == typeid(BranchObj)  ) 
	{
		// save the selected branch of the map, Undo will insert part of map 
		saveState (PartOfMap,
			undoSelection, QString("addMapInsert (\"PATH\",%1)").arg(((BranchObj*)redoSel)->getNum()),
			redoSelection, "delete ()", 
			comment, 
			redoSel);
	}
}


void MapEditor::saveState(LinkableMapObj *undoSel, const QString &uc, LinkableMapObj *redoSel, const QString &rc, const QString &comment) 
{
	// "Normal" savestate: save commands, selections and comment
	// so just save commands for undo and redo
	// and use current selection

	QString redoSelection="";
	if (redoSel) redoSelection=model->getSelectString(redoSel);
	QString undoSelection="";
	if (undoSel) undoSelection=model->getSelectString(undoSel);

	saveState (UndoCommand,
		undoSelection, uc,
		redoSelection, rc, 
		comment, 
		NULL);
}

void MapEditor::saveState(const QString &undoSel, const QString &uc, const QString &redoSel, const QString &rc, const QString &comment) 
{
	// "Normal" savestate: save commands, selections and comment
	// so just save commands for undo and redo
	// and use current selection
	saveState (UndoCommand,
		undoSel, uc,
		redoSel, rc, 
		comment, 
		NULL);
}

void MapEditor::saveState(const QString &uc, const QString &rc, const QString &comment) 
{
	// "Normal" savestate applied to model (no selection needed): 
	// save commands  and comment
	saveState (UndoCommand,
		NULL, uc,
		NULL, rc, 
		comment, 
		NULL);
}

		
void MapEditor::parseAtom(const QString &atom)
{
	BranchObj *selb=xelection.getBranch();
	QString s,t;
	double x,y;
	int n;
	bool b,ok;

	// Split string s into command and parameters
	parser.parseAtom (atom);
	QString com=parser.getCommand();
	
	// External commands
	/////////////////////////////////////////////////////////////////////
	if (com=="addBranch")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else 
		{	
			QList <int> pl;
			pl << 0 <<1;
			if (parser.checkParCount(pl))
			{
				if (parser.parCount()==0)
					addNewBranch (0);
				else
				{
					n=parser.parInt (ok,0);
					if (ok ) addNewBranch (n);
				}
			}
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="addBranchBefore")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else 
		{	
			if (parser.parCount()==0)
			{
				addNewBranchBefore ();
			}	
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com==QString("addMapCenter"))
	{
		if (parser.checkParCount(2))
		{
			x=parser.parDouble (ok,0);
			if (ok)
			{
				y=parser.parDouble (ok,1);
				if (ok) model->addMapCenter (QPointF(x,y));
			}
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com==QString("addMapReplace"))
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			//s=parser.parString (ok,0);	// selection
			t=parser.parString (ok,0);	// path to map
			if (QDir::isRelativePath(t)) t=(tmpMapDir + "/"+t);
			addMapReplaceInt(model->getSelectString(selb),t);	
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com==QString("addMapInsert"))
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else 
		{	
			if (parser.checkParCount(2))
			{
				t=parser.parString (ok,0);	// path to map
				n=parser.parInt(ok,1);		// position
				if (QDir::isRelativePath(t)) t=(tmpMapDir + "/"+t);
				addMapInsertInt(t,n);	
			}
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="clearFlags")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{
			selb->clearStandardFlags();	
			selb->updateFlagsToolbar();
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="colorBranch")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{	
			QColor c=parser.parColor (ok,0);
			if (ok) colorBranch (c);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="colorSubtree")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{	
			QColor c=parser.parColor (ok,0);
			if (ok) colorSubtree (c);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="copy")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{	
			//FIXME missing action for copy
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="cut")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if ( xelection.type()!=Selection::Branch  && 
					xelection.type()!=Selection::MapCenter  &&
					xelection.type()!=Selection::FloatImage )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch or floatimage");
		} else if (parser.checkParCount(0))
		{	
			cut();
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="delete")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} 
		/*else if (xelection.type() != Selection::Branch && xelection.type() != Selection::FloatImage )
		{
			parser.setError (Aborted,"Type of selection is wrong.");
		} 
		*/
		else if (parser.checkParCount(0))
		{	
			deleteSelection();
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="deleteKeepChilds")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{	
			deleteKeepChilds();
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="deleteChilds")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb)
		{
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{	
			deleteChilds();
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportASCII")
	{
		QString fname="";
		ok=true;
		if (parser.parCount()>=1)
			// Hey, we even have a filename
			fname=parser.parString(ok,0); 
		if (!ok)
		{
			parser.setError (Aborted,"Could not read filename");
		} else
		{
				exportASCII (fname,false);
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportImage")
	{
		QString fname="";
		ok=true;
		if (parser.parCount()>=2)
			// Hey, we even have a filename
			fname=parser.parString(ok,0); 
		if (!ok)
		{
			parser.setError (Aborted,"Could not read filename");
		} else
		{
			QString format="PNG";
			if (parser.parCount()>=2)
			{
				format=parser.parString(ok,1);
			}
			exportImage (fname,false,format);
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportXHTML")
	{
		QString fname="";
		ok=true;
		if (parser.parCount()>=2)
			// Hey, we even have a filename
			fname=parser.parString(ok,1); 
		if (!ok)
		{
			parser.setError (Aborted,"Could not read filename");
		} else
		{
			exportXHTML (fname,false);
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportXML")
	{
		QString fname="";
		ok=true;
		if (parser.parCount()>=2)
			// Hey, we even have a filename
			fname=parser.parString(ok,1); 
		if (!ok)
		{
			parser.setError (Aborted,"Could not read filename");
		} else
		{
			exportXML (fname,false);
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="importDir")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			s=parser.parString(ok,0);
			if (ok) importDirInt(s);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="linkTo")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if ( selb)
		{
			if (parser.checkParCount(4))
			{
				// 0	selectstring of parent
				// 1	num in parent (for branches)
				// 2,3	x,y of mainbranch or mapcenter
				s=parser.parString(ok,0);
				LinkableMapObj *dst=model->findObjBySelect (s);
				if (dst)
				{	
					if (typeid(*dst) == typeid(BranchObj) ) 
					{
						// Get number in parent
						n=parser.parInt (ok,1);
						if (ok)
						{
							selb->linkTo ((BranchObj*)(dst),n);
							xelection.update();
						}	
					} else if (typeid(*dst) == typeid(MapCenterObj) ) 
					{
						selb->linkTo ((BranchObj*)(dst),-1);
						// Get coordinates of mainbranch
						x=parser.parDouble(ok,2);
						if (ok)
						{
							y=parser.parDouble(ok,3);
							if (ok) 
							{
								selb->move (x,y);
								xelection.update();
							}
						}
					}	
				}	
			}	
		} else if ( xelection.type() == Selection::FloatImage) 
		{
			if (parser.checkParCount(1))
			{
				// 0	selectstring of parent
				s=parser.parString(ok,0);
				LinkableMapObj *dst=model->findObjBySelect (s);
				if (dst)
				{	
					if (typeid(*dst) == typeid(BranchObj) ||
						typeid(*dst) == typeid(MapCenterObj)) 
						linkTo (model->getSelectString(dst));
				} else	
					parser.setError (Aborted,"Destination is not a branch");
			}		
		} else
			parser.setError (Aborted,"Type of selection is not a floatimage or branch");
	/////////////////////////////////////////////////////////////////////
	} else if (com=="loadImage")
	{
		if (xelection.isEmpty())
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			s=parser.parString(ok,0);
			if (ok) loadFloatImageInt (s);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="moveBranchUp")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{
			moveBranchUp();
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="moveBranchDown")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{
			moveBranchDown();
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="move")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if ( xelection.type()!=Selection::Branch  && 
					xelection.type()!=Selection::MapCenter  &&
					xelection.type()!=Selection::FloatImage )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch or floatimage");
		} else if (parser.checkParCount(2))
		{	
			x=parser.parDouble (ok,0);
			if (ok)
			{
				y=parser.parDouble (ok,1);
				if (ok) move (x,y);
			}
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="moveRel")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if ( xelection.type()!=Selection::Branch  && 
					xelection.type()!=Selection::MapCenter  &&
					xelection.type()!=Selection::FloatImage )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch or floatimage");
		} else if (parser.checkParCount(2))
		{	
			x=parser.parDouble (ok,0);
			if (ok)
			{
				y=parser.parDouble (ok,1);
				if (ok) moveRel (x,y);
			}
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="nop")
	{
	/////////////////////////////////////////////////////////////////////
	} else if (com=="paste")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{	
			n=parser.parInt (ok,0);
			if (ok) pasteNoSave(n);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="qa")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(4))
		{	
			QString c,u;
			c=parser.parString (ok,0);
			if (!ok)
			{
				parser.setError (Aborted,"No comment given");
			} else
			{
				s=parser.parString (ok,1);
				if (!ok)
				{
					parser.setError (Aborted,"First parameter is not a string");
				} else
				{
					t=parser.parString (ok,2);
					if (!ok)
					{
						parser.setError (Aborted,"Condition is not a string");
					} else
					{
						u=parser.parString (ok,3);
						if (!ok)
						{
							parser.setError (Aborted,"Third parameter is not a string");
						} else
						{
							if (s!="heading")
							{
								parser.setError (Aborted,"Unknown type: "+s);
							} else
							{
								if (! (t=="eq") ) 
								{
									parser.setError (Aborted,"Unknown operator: "+t);
								} else
								{
									if (! selb    )
									{
										parser.setError (Aborted,"Type of selection is not a branch");
									} else
									{
										if (selb->getHeading() == u)
										{
											cout << "PASSED: " << qPrintable (c)  << endl;
										} else
										{
											cout << "FAILED: " << qPrintable (c)  << endl;
										}
									}
								}
							}
						} 
					} 
				} 
			}
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="saveImage")
	{
		FloatImageObj *fio=xelection.getFloatImage();
		if (!fio)
		{
			parser.setError (Aborted,"Type of selection is not an image");
		} else if (parser.checkParCount(2))
		{
			s=parser.parString(ok,0);
			if (ok)
			{
				t=parser.parString(ok,1);
				if (ok) saveFloatImageInt (fio,t,s);
			}
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="scroll")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{	
			if (!scrollBranch (selb))	
				parser.setError (Aborted,"Could not scroll branch");
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="select")
	{
		if (parser.checkParCount(1))
		{
			s=parser.parString(ok,0);
			if (ok) select (s);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="selectLastBranch")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{	
			BranchObj *bo=selb->getLastBranch();
			if (!bo)
				parser.setError (Aborted,"Could not select last branch");
			selectInt (bo);	
				
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="selectLastImage")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{	
			FloatImageObj *fio=selb->getLastFloatImage();
			if (!fio)
				parser.setError (Aborted,"Could not select last image");
			selectInt (fio);	
				
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="selectLatestAdded")
	{
		if (latestSelection.isEmpty() )
		{
			parser.setError (Aborted,"No latest added object");
		} else
		{	
			if (!select (latestSelection))
				parser.setError (Aborted,"Could not select latest added object "+latestSelection);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFrameType")
	{
		if ( xelection.type()!=Selection::Branch && xelection.type()!= Selection::MapCenter && xelection.type()!=Selection::FloatImage)
		{
			parser.setError (Aborted,"Type of selection does not allow setting frame type");
		}
		else if (parser.checkParCount(1))
		{
			s=parser.parString(ok,0);
			if (ok) setFrameType (s);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFramePenColor")
	{
		if ( xelection.type()!=Selection::Branch && xelection.type()!= Selection::MapCenter && xelection.type()!=Selection::FloatImage)
		{
			parser.setError (Aborted,"Type of selection does not allow setting of pen color");
		}
		else if (parser.checkParCount(1))
		{
			QColor c=parser.parColor(ok,0);
			if (ok) setFramePenColor (c);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFrameBrushColor")
	{
		if ( xelection.type()!=Selection::Branch && xelection.type()!= Selection::MapCenter && xelection.type()!=Selection::FloatImage)
		{
			parser.setError (Aborted,"Type of selection does not allow setting brush color");
		}
		else if (parser.checkParCount(1))
		{
			QColor c=parser.parColor(ok,0);
			if (ok) setFrameBrushColor (c);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFramePadding")
	{
		if ( xelection.type()!=Selection::Branch && xelection.type()!= Selection::MapCenter && xelection.type()!=Selection::FloatImage)
		{
			parser.setError (Aborted,"Type of selection does not allow setting frame padding");
		}
		else if (parser.checkParCount(1))
		{
			n=parser.parInt(ok,0);
			if (ok) setFramePadding(n);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFrameBorderWidth")
	{
		if ( xelection.type()!=Selection::Branch && xelection.type()!= Selection::MapCenter && xelection.type()!=Selection::FloatImage)
		{
			parser.setError (Aborted,"Type of selection does not allow setting frame border width");
		}
		else if (parser.checkParCount(1))
		{
			n=parser.parInt(ok,0);
			if (ok) setFrameBorderWidth (n);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapAuthor")
	{
		if (parser.checkParCount(1))
		{
			s=parser.parString(ok,0);
			if (ok) setMapAuthor (s);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapComment")
	{
		if (parser.checkParCount(1))
		{
			s=parser.parString(ok,0);
			if (ok) setMapComment(s);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapBackgroundColor")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! xelection.getBranch() )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			QColor c=parser.parColor (ok,0);
			if (ok) setMapBackgroundColor (c);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapDefLinkColor")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			QColor c=parser.parColor (ok,0);
			if (ok) setMapDefLinkColor (c);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapLinkStyle")
	{
		if (parser.checkParCount(1))
		{
			s=parser.parString (ok,0);
			if (ok) setMapLinkStyle(s);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setHeading")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			s=parser.parString (ok,0);
			if (ok) 
				setHeading (s);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setHideExport")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (xelection.type()!=Selection::Branch && xelection.type() != Selection::MapCenter &&xelection.type()!=Selection::FloatImage)
		{				  
			parser.setError (Aborted,"Type of selection is not a branch or floatimage");
		} else if (parser.checkParCount(1))
		{
			b=parser.parBool(ok,0);
			if (ok) setHideExport (b);
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setIncludeImagesHorizontally")
	{ 
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb)
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			b=parser.parBool(ok,0);
			if (ok) setIncludeImagesHor(b);
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setIncludeImagesVertically")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb)
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			b=parser.parBool(ok,0);
			if (ok) setIncludeImagesVer(b);
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setHideLinkUnselected")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if ( xelection.type()!=Selection::Branch && xelection.type()!= Selection::MapCenter && xelection.type()!=Selection::FloatImage)
		{				  
			parser.setError (Aborted,"Type of selection does not allow hiding the link");
		} else if (parser.checkParCount(1))
		{
			b=parser.parBool(ok,0);
			if (ok) setHideLinkUnselected(b);
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setSelectionColor")
	{
		if (parser.checkParCount(1))
		{
			QColor c=parser.parColor (ok,0);
			if (ok) setSelectionColorInt (c);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setURL")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			s=parser.parString (ok,0);
			if (ok) setURL(s);
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setVymLink")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			s=parser.parString (ok,0);
			if (ok) setVymLinkInt(s);
		}	
	}
	/////////////////////////////////////////////////////////////////////
	else if (com=="setFlag")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			s=parser.parString(ok,0);
			if (ok) 
			{
				selb->activateStandardFlag(s);
				selb->updateFlagsToolbar();
			}	
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFrameType")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			s=parser.parString(ok,0);
			if (ok) 
				setFrameType (s);
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="sortChildren")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{
			sortChildren();
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="toggleFlag")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			s=parser.parString(ok,0);
			if (ok) 
			{
				selb->toggleStandardFlag(s);	
				selb->updateFlagsToolbar();
			}	
		}
	/////////////////////////////////////////////////////////////////////
	} else if (com=="unscroll")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{	
			if (!unscrollBranch (selb))	
				parser.setError (Aborted,"Could not unscroll branch");
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="unscrollChilds")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(0))
		{	
			unscrollChilds ();
		}	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="unsetFlag")
	{
		if (xelection.isEmpty() )
		{
			parser.setError (Aborted,"Nothing selected");
		} else if (! selb )
		{				  
			parser.setError (Aborted,"Type of selection is not a branch");
		} else if (parser.checkParCount(1))
		{
			s=parser.parString(ok,0);
			if (ok) 
			{
				selb->deactivateStandardFlag(s);
				selb->updateFlagsToolbar();
			}	
		}
	} else
		parser.setError (Aborted,"Unknown command");

	// Any errors?
	if (parser.errorLevel()==NoError)
	{
		// setChanged();  FIXME should not be called e.g. for export?!
		model->reposition();
	}	
	else	
	{
		// TODO Error handling
		qWarning("MapEditor::parseAtom: Error!");
		qWarning(parser.errorMessage());
	} 
}

void MapEditor::runScript (QString script)
{
	parser.setScript (script);
	parser.runScript();
	while (parser.next() ) 
		parseAtom(parser.getAtom());
}

bool MapEditor::isDefault()
{
    return mapDefault;
}

bool MapEditor::hasChanged()
{
    return mapChanged;
}

void MapEditor::setChanged()
{
	if (!mapChanged)
	{
		autosaveTimer->start(settings.value("/mainwindow/autosave/ms/",300000).toInt());
		fileChangedTimer->start (3000);
	}	
	mapChanged=true;
	mapDefault=false;
	mapUnsaved=true;
	findReset();

}

void MapEditor::closeMap()
{
	// Unselect before disabling the toolbar actions
	if (!xelection.isEmpty() ) xelection.unselect();
	xelection.clear();
	updateActions();

    clear();
	// close();  FIXME needed?
}

void MapEditor::setFilePath(QString fpath, QString destname)
{
	if (fpath.isEmpty() || fpath=="")
	{
		filePath="";
		fileName="";
		destPath="";
	} else
	{
		filePath=fpath;		// becomes absolute path
		fileName=fpath;		// gets stripped of path
		destPath=destname;	// needed for vymlinks and during load to reset fileChangedTime

		// If fpath is not an absolute path, complete it
		filePath=QDir(fpath).absPath();
		fileDir=filePath.left (1+filePath.findRev ("/"));

		// Set short name, too. Search from behind:
		int i=fileName.findRev("/");
		if (i>=0) fileName=fileName.remove (0,i+1);

		// Forget the .vym (or .xml) for name of map
		mapName=fileName.left(fileName.findRev(".",-1,true) );
	}
}

void MapEditor::setFilePath(QString fpath)
{
	setFilePath (fpath,fpath);
}

QString MapEditor::getFilePath()
{
	return filePath;
}

QString MapEditor::getFileName()
{
	return fileName;
}

QString MapEditor::getMapName()
{
	return mapName;
}

QString MapEditor::getDestPath()
{
	return destPath;
}

ErrorCode MapEditor::load (QString fname, const LoadMode &lmode, const FileType &ftype)
{
	ErrorCode err=success;

	parseBaseHandler *handler;
	fileType=ftype;
	switch (fileType)
	{
		case VymMap: handler=new parseVYMHandler; break;
		case FreemindMap : handler=new parseFreemindHandler; break;
		default: 
			QMessageBox::critical( 0, tr( "Critical Parse Error" ),
				   "Unknown FileType in MapEditor::load()");
		return aborted;	
	}

	// Save original zip state, important for inserting complete maps
	bool zipped_org=zipped;

	if (lmode==NewMap)
	{
		xelection.clear();
		model->setMapEditor(this);
		// (map state is set later at end of load...)
	} else
	{
		BranchObj *bo=xelection.getBranch();
		if (!bo) return aborted;
		if (lmode==ImportAdd)
			saveStateChangingPart(
				bo,
				bo,
				QString("addMapInsert (%1)").arg(fname),
				QString("Add map %1 to %2").arg(fname).arg(getName(bo)));
		else	
			saveStateChangingPart(
				bo,
				bo,
				QString("addMapReplace(%1)").arg(fname),
				QString("Add map %1 to %2").arg(fname).arg(getName(bo)));
	}	
    

	// Create temporary directory for packing
	bool ok;
	QString tmpZipDir=makeTmpDir (ok,"vym-pack");
	if (!ok)
	{
		QMessageBox::critical( 0, tr( "Critical Load Error" ),
		   tr("Couldn't create temporary directory before load\n"));
		return aborted; 
	}

	// Try to unzip file
	err=unzipDir (tmpZipDir,fname);
	QString xmlfile;
	if (err==nozip)
	{
		xmlfile=fname;
		zipped=false;
	} else
	{
		zipped=true;
		
		// Look for mapname.xml
		xmlfile= fname.left(fname.findRev(".",-1,true));
		xmlfile=xmlfile.section( '/', -1 );
		QFile mfile( tmpZipDir + "/" + xmlfile + ".xml");
		if (!mfile.exists() )
		{
			// mapname.xml does not exist, well, 
			// maybe someone renamed the mapname.vym file...
			// Try to find any .xml in the toplevel 
			// directory of the .vym file
			QStringList flist=QDir (tmpZipDir).entryList("*.xml");
			if (flist.count()==1) 
			{
				// Only one entry, take this one
				xmlfile=tmpZipDir + "/"+flist.first();
			} else
			{
				for ( QStringList::Iterator it = flist.begin(); it != flist.end(); ++it ) 
					*it=tmpZipDir + "/" + *it;
				// TODO Multiple entries, load all (but only the first one into this ME)
				//mainWindow->fileLoadFromTmp (flist);
				//returnCode=1;	// Silently forget this attempt to load
				qWarning ("MainWindow::load (fn)  multimap found...");
			}	
				
			if (flist.isEmpty() )
			{
				QMessageBox::critical( 0, tr( "Critical Load Error" ),
						   tr("Couldn't find a map (*.xml) in .vym archive.\n"));
				err=aborted;				   
			}	
		} //file doesn't exist	
		else
			xmlfile=mfile.name();
	}

	QFile file( xmlfile);

	// I am paranoid: file should exist anyway
	// according to check in mainwindow.
	if (!file.exists() )
	{
		QMessageBox::critical( 0, tr( "Critical Parse Error" ),
				   tr(QString("Couldn't open map %1").arg(file.name())));
		err=aborted;	
	} else
	{
		bool blockSaveStateOrg=blockSaveState;
		blockReposition=true;
		blockSaveState=true;
		QXmlInputSource source( file);
		QXmlSimpleReader reader;
		reader.setContentHandler( handler );
		reader.setErrorHandler( handler );
		handler->setModel ( model);


		// We need to set the tmpDir in order  to load files with rel. path
		QString tmpdir;
		if (zipped)
			tmpdir=tmpZipDir;
		else
			tmpdir=fname.left(fname.findRev("/",-1));	
		handler->setTmpDir (tmpdir);
		handler->setInputFile (file.name());
		handler->setLoadMode (lmode);
		bool ok = reader.parse( source );
		blockReposition=false;
		blockSaveState=blockSaveStateOrg;
		file.close();
		if ( ok ) 
		{
			model->reposition();	// FIXME reposition the view instead...
			xelection.update();
			if (lmode==NewMap)
			{
				mapDefault=false;
				mapChanged=false;
				mapUnsaved=false;
				autosaveTimer->stop();
			}

			// Reset timestamp to check for later updates of file
			fileChangedTime=QFileInfo (destPath).lastModified();
		} else 
		{
			QMessageBox::critical( 0, tr( "Critical Parse Error" ),
					   tr( handler->errorProtocol() ) );
			// returnCode=1;	
			// Still return "success": the map maybe at least
			// partially read by the parser
		}	
	}	

	// Delete tmpZipDir
	removeDir (QDir(tmpZipDir));

	// Restore original zip state
	zipped=zipped_org;

	updateActions();

	return err;
}

ErrorCode MapEditor::save (const SaveMode &savemode)
{
	QString tmpZipDir;
	QString mapFileName;
	QString safeFilePath;

	ErrorCode err=success;

	if (zipped)
		// save as .xml
		mapFileName=mapName+".xml";
	else
		// use name given by user, even if he chooses .doc
		mapFileName=fileName;

	// Look, if we should zip the data:
	if (!zipped)
	{
		QMessageBox mb( vymName,
			tr("The map %1\ndid not use the compressed "
			"vym file format.\nWriting it uncompressed will also write images \n"
			"and flags and thus may overwrite files in the "
			"given directory\n\nDo you want to write the map").arg(filePath),
			QMessageBox::Warning,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::No ,
			QMessageBox::Cancel | QMessageBox::Escape);
		mb.setButtonText( QMessageBox::Yes, tr("compressed (vym default)") );
		mb.setButtonText( QMessageBox::No, tr("uncompressed") );
		mb.setButtonText( QMessageBox::Cancel, tr("Cancel"));
		switch( mb.exec() ) 
		{
			case QMessageBox::Yes:
				// save compressed (default file format)
				zipped=true;
				break;
			case QMessageBox::No:
				// save uncompressed
				zipped=false;
				break;
			case QMessageBox::Cancel:
				// do nothing
				return aborted;
				break;
		}
	}

	// First backup existing file, we 
	// don't want to add to old zip archives
	QFile f(destPath);
	if (f.exists())
	{
		if ( settings.value ("/mapeditor/writeBackupFile").toBool())
		{
			QString backupFileName(destPath + "~");
			QFile backupFile(backupFileName);
			if (backupFile.exists() && !backupFile.remove())
			{
				QMessageBox::warning(0, tr("Save Error"),
									 tr("%1\ncould not be removed before saving").arg(backupFileName));
			}
			else if (!f.rename(backupFileName))
			{
				QMessageBox::warning(0, tr("Save Error"),
									 tr("%1\ncould not be renamed before saving").arg(destPath));
			}
		}
	}

	if (zipped)
	{
		// Create temporary directory for packing
		bool ok;
		tmpZipDir=makeTmpDir (ok,"vym-zip");
		if (!ok)
		{
			QMessageBox::critical( 0, tr( "Critical Load Error" ),
			   tr("Couldn't create temporary directory before save\n"));
			return aborted; 
		}

//	cout <<"ME::save      filePath="<<filePath.toStdString()<<endl;
		safeFilePath=filePath;
		setFilePath (tmpZipDir+"/"+ mapName+ ".xml", safeFilePath);
	} // zipped

	// Create mapName and fileDir
	makeSubDirs (fileDir);

/*
	// FIXME testing
	cout <<"ME::save      filePath="<<filePath.toStdString()<<endl;
	cout <<"ME::save  saveFilePath="<<safeFilePath.toStdString()<<endl;
	cout <<"ME::save      destPath="<<destPath.toStdString()<<endl;
	cout <<"ME::save       mapName="<<mapName.toStdString()<<endl;
	cout <<"ME::save   mapFileName="<<mapFileName.toStdString()<<endl;
*/

	QString saveFile;
	if (savemode==CompleteMap || xelection.isEmpty())
	{
		// Save complete map
		saveFile=saveToDir (fileDir,mapName+"-",true,QPointF(),NULL);
		mapChanged=false;
		mapUnsaved=false;
		autosaveTimer->stop();
	}
	else	
	{
		// Save part of map
		if (xelection.type()==Selection::FloatImage)
			saveFloatImage();
		else	
			saveFile=saveToDir (fileDir,mapName+"-",true,QPointF(),xelection.getBranch());	
		// TODO take care of multiselections
	}	

	// FIXME trying to debug save problem
	if (saveFile.length()<1000)
		QMessageBox::critical (0,"Critical error in MapEditor::save",QString("saveFile is too small, try make a backup NOW\nof your original file\nbefore vym writes to\"%1\":\n%2").arg(mapName).arg(saveFile));
	if (!saveStringToDisk(fileDir+mapFileName,saveFile))
	{
		err=aborted;
		QMessageBox::critical (0,"Critical error in MapEditor::save",QString("could not save %1").arg(fileDir+mapFileName));
	}

	if (zipped)
	{
		// zip
		if (err==success) err=zipDir (tmpZipDir,destPath);

		// Delete tmpDir
		removeDir (QDir(tmpZipDir));

		// Restore original filepath outside of tmp zip dir
		setFilePath (safeFilePath);
	}

	updateActions();
	fileChangedTime=QFileInfo (destPath).lastModified();
	return err;
}


void MapEditor::print()
{
	if ( !printer ) 
	{
		printer = new QPrinter;
		printer->setColorMode (QPrinter::Color);
		printer->setPrinterName (settings.value("/mainwindow/printerName",printer->printerName()).toString());
		printer->setOutputFormat((QPrinter::OutputFormat)settings.value("/mainwindow/printerFormat",printer->outputFormat()).toInt());
		printer->setOutputFileName(settings.value("/mainwindow/printerFileName",printer->outputFileName()).toString());
	}

	QRectF totalBBox=model->getTotalBBox();

	// Try to set orientation automagically
	// Note: Interpretation of generated postscript is amibiguous, if 
	// there are problems with landscape mode, see
	// http://sdb.suse.de/de/sdb/html/jsmeix_print-cups-landscape-81.html

	if (totalBBox.width()>totalBBox.height())
		// recommend landscape
		printer->setOrientation (QPrinter::Landscape);
	else	
		// recommend portrait
		printer->setOrientation (QPrinter::Portrait);

	if ( printer->setup(this) ) 
	// returns false, if printing is canceled
	{
		QPainter pp(printer);

		pp.setRenderHint(QPainter::Antialiasing,true);

		// Don't print the visualisation of selection
		xelection.unselect();

		QRectF mapRect=totalBBox;
		QGraphicsRectItem *frame=NULL;

		if (printFrame) 
		{
			// Print frame around map
			mapRect.setRect (totalBBox.x()-10, totalBBox.y()-10, 
				totalBBox.width()+20, totalBBox.height()+20);
			frame=mapScene->addRect (mapRect, QPen(Qt::black),QBrush(Qt::NoBrush));
			frame->setZValue(0);
			frame->show();    
		}		


		double paperAspect = (double)printer->width()   / (double)printer->height();
		double   mapAspect = (double)mapRect.width() / (double)mapRect.height();
		int viewBottom;
		if (mapAspect>=paperAspect)
		{
			// Fit horizontally to paper width
			//pp.setViewport(0,0, printer->width(),(int)(printer->width()/mapAspect) );	
			viewBottom=(int)(printer->width()/mapAspect);	
		}	else
		{
			// Fit vertically to paper height
			//pp.setViewport(0,0,(int)(printer->height()*mapAspect),printer->height());	
			viewBottom=printer->height();	
		}	
		
		if (printFooter) 
		{
			// Print footer below map
			QFont font;		
			font.setPointSize(10);
			pp.setFont (font);
			QRectF footerBox(0,viewBottom,printer->width(),15);
			pp.drawText ( footerBox,Qt::AlignLeft,"VYM - " +fileName);
			pp.drawText ( footerBox, Qt::AlignRight, QDate::currentDate().toString(Qt::TextDate));
		}
		mapScene->render (
			&pp, 
			QRectF (0,0,printer->width(),printer->height()-15),
			QRectF(mapRect.x(),mapRect.y(),mapRect.width(),mapRect.height())
		);
		
		// Viewport has paper dimension
		if (frame)  delete (frame);

		// Restore selection
		xelection.reselect();

		// Save settings in vymrc
		settings.writeEntry("/mainwindow/printerName",printer->printerName());
		settings.writeEntry("/mainwindow/printerFormat",printer->outputFormat());
		settings.writeEntry("/mainwindow/printerFileName",printer->outputFileName());
	}
}

void MapEditor::setAntiAlias (bool b)
{
	setRenderHint(QPainter::Antialiasing,b);
}

void MapEditor::setSmoothPixmap(bool b)
{
	setRenderHint(QPainter::SmoothPixmapTransform,b);
}

QPixmap MapEditor::getPixmap()
{
	QRectF mapRect=model->getTotalBBox();
	QPixmap pix((int)mapRect.width()+2,(int)mapRect.height()+1);
	QPainter pp (&pix);
	
	pp.setRenderHints(renderHints());

	// Don't print the visualisation of selection
	xelection.unselect();

	mapScene->render (	&pp, 
		QRectF(0,0,mapRect.width()+1,mapRect.height()+1),
		QRectF(mapRect.x(),mapRect.y(),mapRect.width(),mapRect.height() ));

	// Restore selection
	xelection.reselect();
	
	return pix;
}

void MapEditor::setHideTmpMode (HideTmpMode mode)
{
	hidemode=mode;
	model->setHideTmp (hidemode);
	model->reposition();
	scene()->update();
}

HideTmpMode MapEditor::getHideTmpMode()
{
	return hidemode;
}

void MapEditor::setExportMode (bool b)
{
	// should be called before and after exports
	// depending on the settings
	if (b && settings.value("/export/useHideExport","true")=="true")
		setHideTmpMode (HideExport);
	else	
		setHideTmpMode (HideNone);
}

void MapEditor::exportASCII(QString fname,bool askName)
{
	ExportASCII ex;
	ex.setModel (model);
	if (fname=="") 
		ex.setFile (mapName+".txt");	
	else
		ex.setFile (fname);

	if (askName)
	{
		//ex.addFilter ("TXT (*.txt)");
		ex.setDir(lastImageDir);
		//ex.setCaption(vymName+ " -" +tr("Export as ASCII")+" "+tr("(still experimental)"));
		ex.execDialog() ; 
	} 
	if (!ex.canceled())
	{
		setExportMode(true);
		ex.doExport();
		setExportMode(false);
	}
}

void MapEditor::exportImage(QString fname, bool askName, QString format)
{
	if (fname=="")
	{
		fname=mapName+".png";
		format="PNG";
	} 	

	if (askName)
	{
		QStringList fl;
		QFileDialog *fd=new QFileDialog (this);
		fd->setCaption (tr("Export map as image"));
		fd->setDirectory (lastImageDir);
		fd->setFileMode(QFileDialog::AnyFile);
		fd->setFilters  (imageIO.getFilters() );
		if (fd->exec())
		{
			fl=fd->selectedFiles();
			fname=fl.first();
			format=imageIO.getType(fd->selectedFilter());
		} 
	}

	setExportMode (true);
	QPixmap pix (getPixmap());
	pix.save(fname, format);
	setExportMode (false);
}

void MapEditor::exportOOPresentation(const QString &fn, const QString &cf)
{
	ExportOO ex;
	ex.setFile (fn);
	ex.setModel (model);
	if (ex.setConfigFile(cf)) 
	{
		setExportMode (true);
		ex.exportPresentation();
		setExportMode (false);
	}
}

void MapEditor::exportXHTML (const QString &dir, bool askForName)
{
			ExportXHTMLDialog dia(this);
			dia.setFilePath (filePath );
			dia.setMapName (mapName );
			dia.readSettings();
			if (dir!="") dia.setDir (dir);

			bool ok=true;
			
			if (askForName)
			{
				if (dia.exec()!=QDialog::Accepted) 
					ok=false;
				else	
				{
					QDir d (dia.getDir());
					// Check, if warnings should be used before overwriting
					// the output directory
					if (d.exists() && d.count()>0)
					{
						WarningDialog warn;
						warn.showCancelButton (true);
						warn.setText(QString(
							"The directory %1 is not empty.\n"
							"Do you risk to overwrite some of its contents?").arg(d.path() ));
						warn.setCaption("Warning: Directory not empty");
						warn.setShowAgainName("mainwindow/overwrite-dir-xhtml");

						if (warn.exec()!=QDialog::Accepted) ok=false;
					}
				}	
			}

			if (ok)
			{
				exportXML (dia.getDir(),false );
				dia.doExport(mapName );
				//if (dia.hasChanged()) setChanged();
			}
}

void MapEditor::exportXML(QString dir, bool askForName)
{
	if (askForName)
	{
		dir=browseDirectory(this,tr("Export XML to directory"));
		if (dir =="" && !reallyWriteDirectory(dir) )
		return;
	}

	// Hide stuff during export, if settings want this
	setExportMode (true);

	// Create subdirectories
	makeSubDirs (dir);

	// write to directory
	QString saveFile=saveToDir (dir,mapName+"-",true,model->getTotalBBox().topLeft() ,NULL);
	QFile file;

	file.setName ( dir + "/"+mapName+".xml");
	if ( !file.open( QIODevice::WriteOnly ) )
	{
		// This should neverever happen
		QMessageBox::critical (0,tr("Critical Export Error"),tr("MapEditor::exportXML couldn't open %1").arg(file.name()));
		return;
	}	

	// Write it finally, and write in UTF8, no matter what 
	QTextStream ts( &file );
	ts.setEncoding (QTextStream::UnicodeUTF8);
	ts << saveFile;
	file.close();

	// Now write image, too
	exportImage (dir+"/images/"+mapName+".png",false,"PNG");

	setExportMode (false);
}

void MapEditor::clear()
{
	//cout << "ME::clear() "<<mapName.toStdString()<<endl;
	xelection.clear();
	autosaveTimer->stop();
	fileChangedTimer->stop();
	model->clear();
}

void MapEditor::copy()
{
	LinkableMapObj *sel=xelection.single();
	if (sel)
	{
		if (redosAvail == 0)
		{
			// Copy to history
			QString s=model->getSelectString(sel);
			saveState (PartOfMap, s, "nop ()", s, "copy ()","Copy selection to clipboard",sel  );
			curClipboard=curStep;
		}

		// Copy also to global clipboard, because we are at last step in history
		QString bakMapName(QString("history-%1").arg(curStep));
		QString bakMapDir(tmpMapDir +"/"+bakMapName);
		copyDir (bakMapDir,clipboardDir );

		clipboardEmpty=false;
		updateActions();
	}	    
}

void MapEditor::redo()
{
	// Can we undo at all?
	if (redosAvail<1) return;

	bool blockSaveStateOrg=blockSaveState;
	blockSaveState=true;
	
	redosAvail--;

	if (undosAvail<stepsTotal) undosAvail++;
	curStep++;
	if (curStep>stepsTotal) curStep=1;
	QString undoCommand=  undoSet.readEntry (QString("/history/step-%1/undoCommand").arg(curStep));
	QString undoSelection=undoSet.readEntry (QString("/history/step-%1/undoSelection").arg(curStep));
	QString redoCommand=  undoSet.readEntry (QString("/history/step-%1/redoCommand").arg(curStep));
	QString redoSelection=undoSet.readEntry (QString("/history/step-%1/redoSelection").arg(curStep));
	QString comment=undoSet.readEntry (QString("/history/step-%1/comment").arg(curStep));
	QString version=undoSet.readEntry ("/history/version");

	/* TODO Maybe check for version, if we save the history
	if (!checkVersion(version))
		QMessageBox::warning(0,tr("Warning"),
			tr("Version %1 of saved undo/redo data\ndoes not match current vym version %2.").arg(version).arg(vymVersion));
	*/ 

	// Find out current undo directory
	QString bakMapDir(QString(tmpMapDir+"/undo-%1").arg(curStep));

	if (debug)
	{
		cout << "ME::redo() begin\n";
		cout << "    undosAvail="<<undosAvail<<endl;
		cout << "    redosAvail="<<redosAvail<<endl;
		cout << "       curStep="<<curStep<<endl;
		cout << "    ---------------------------"<<endl;
		cout << "    comment="<<comment.toStdString()<<endl;
		cout << "    undoCom="<<undoCommand.toStdString()<<endl;
		cout << "    undoSel="<<undoSelection.toStdString()<<endl;
		cout << "    redoCom="<<redoCommand.toStdString()<<endl;
		cout << "    redoSel="<<redoSelection.toStdString()<<endl;
		cout << "    ---------------------------"<<endl<<endl;
	}

	// select  object before redo
	if (!redoSelection.isEmpty())
		select (redoSelection);


	parseAtom (redoCommand);
	model->reposition();

	blockSaveState=blockSaveStateOrg;

	undoSet.setEntry ("/history/undosAvail",QString::number(undosAvail));
	undoSet.setEntry ("/history/redosAvail",QString::number(redosAvail));
	undoSet.setEntry ("/history/curStep",QString::number(curStep));
	undoSet.writeSettings(histPath);

	mainWindow->updateHistory (undoSet);
	updateActions();

	/* TODO remove testing
	cout << "ME::redo() end\n";
	cout << "    undosAvail="<<undosAvail<<endl;
	cout << "    redosAvail="<<redosAvail<<endl;
	cout << "       curStep="<<curStep<<endl;
	cout << "    ---------------------------"<<endl<<endl;
	*/


}

bool MapEditor::isRedoAvailable()
{
	if (undoSet.readNumEntry("/history/redosAvail",0)>0)
		return true;
	else	
		return false;
}

void MapEditor::undo()
{
	// Can we undo at all?
	if (undosAvail<1) return;

	mainWindow->statusMessage (tr("Autosave disabled during undo."));

	bool blockSaveStateOrg=blockSaveState;
	blockSaveState=true;
	
	QString undoCommand=  undoSet.readEntry (QString("/history/step-%1/undoCommand").arg(curStep));
	QString undoSelection=undoSet.readEntry (QString("/history/step-%1/undoSelection").arg(curStep));
	QString redoCommand=  undoSet.readEntry (QString("/history/step-%1/redoCommand").arg(curStep));
	QString redoSelection=undoSet.readEntry (QString("/history/step-%1/redoSelection").arg(curStep));
	QString comment=undoSet.readEntry (QString("/history/step-%1/comment").arg(curStep));
	QString version=undoSet.readEntry ("/history/version");

	/* TODO Maybe check for version, if we save the history
	if (!checkVersion(version))
		QMessageBox::warning(0,tr("Warning"),
			tr("Version %1 of saved undo/redo data\ndoes not match current vym version %2.").arg(version).arg(vymVersion));
	*/

	// Find out current undo directory
	QString bakMapDir(QString(tmpMapDir+"/undo-%1").arg(curStep));

	// select  object before undo
	if (!undoSelection.isEmpty())
		select (undoSelection);

	if (debug)
	{
		cout << "ME::undo() begin\n";
		cout << "    undosAvail="<<undosAvail<<endl;
		cout << "    redosAvail="<<redosAvail<<endl;
		cout << "       curStep="<<curStep<<endl;
		cout << "    ---------------------------"<<endl;
		cout << "    comment="<<comment.toStdString()<<endl;
		cout << "    undoCom="<<undoCommand.toStdString()<<endl;
		cout << "    undoSel="<<undoSelection.toStdString()<<endl;
		cout << "    redoCom="<<redoCommand.toStdString()<<endl;
		cout << "    redoSel="<<redoSelection.toStdString()<<endl;
		cout << "    ---------------------------"<<endl<<endl;
	}	
	parseAtom (undoCommand);
	model->reposition();

	undosAvail--;
	curStep--; 
	if (curStep<1) curStep=stepsTotal;

	redosAvail++;

	blockSaveState=blockSaveStateOrg;
/* TODO remove testing
	cout << "ME::undo() end\n";
	cout << "    undosAvail="<<undosAvail<<endl;
	cout << "    redosAvail="<<redosAvail<<endl;
	cout << "       curStep="<<curStep<<endl;
	cout << "    ---------------------------"<<endl<<endl;
*/

	undoSet.setEntry ("/history/undosAvail",QString::number(undosAvail));
	undoSet.setEntry ("/history/redosAvail",QString::number(redosAvail));
	undoSet.setEntry ("/history/curStep",QString::number(curStep));
	undoSet.writeSettings(histPath);

	mainWindow->updateHistory (undoSet);
	updateActions();
	xelection.update();
	ensureSelectionVisible();
}

bool MapEditor::isUndoAvailable()
{
	if (undoSet.readNumEntry("/history/undosAvail",0)>0)
		return true;
	else	
		return false;
}

void MapEditor::gotoHistoryStep (int i)
{
	// Restore variables
	int undosAvail=undoSet.readNumEntry (QString("/history/undosAvail"));
	int redosAvail=undoSet.readNumEntry (QString("/history/redosAvail"));

	if (i<0) i=undosAvail+redosAvail;

	// Clicking above current step makes us undo things
	if (i<undosAvail) 
	{	
		for (int j=0; j<undosAvail-i; j++) undo();
		return;
	}	
	// Clicking below current step makes us redo things
	if (i>undosAvail) 
		for (int j=undosAvail; j<i; j++) 
		{
			if (debug) cout << "ME::gotoHistoryStep redo "<<j<<"/"<<undosAvail<<" i="<<i<<endl;
			redo();
		}

	// And ignore clicking the current row ;-)	
}

void MapEditor::addMapReplaceInt(const QString &undoSel, const QString &path)
{
	QString pathDir=path.left(path.findRev("/"));
	QDir d(pathDir);
	QFile file (path);

	if (d.exists() )
	{
		// We need to parse saved XML data
		parseVYMHandler handler;
		QXmlInputSource source( file);
		QXmlSimpleReader reader;
		reader.setContentHandler( &handler );
		reader.setErrorHandler( &handler );
		handler.setModel ( model);
		handler.setTmpDir ( pathDir );	// needed to load files with rel. path
		if (undoSel.isEmpty())
		{
			unselect();
			model->clear();
			handler.setLoadMode (NewMap);
		} else	
		{
			select (undoSel);
			handler.setLoadMode (ImportReplace);
		}	
		blockReposition=true;
		bool ok = reader.parse( source );
		blockReposition=false;
		if (! ok ) 
		{	
			// This should never ever happen
			QMessageBox::critical( 0, tr( "Critical Parse Error while reading %1").arg(path),
								    handler.errorProtocol());
		}
	} else	
		QMessageBox::critical( 0, tr( "Critical Error" ), tr("Could not read %1").arg(path));
}

void MapEditor::addMapInsertInt (const QString &path, int pos)
{
	BranchObj *sel=xelection.getBranch();
	if (sel)
	{
		QString pathDir=path.left(path.findRev("/"));
		QDir d(pathDir);
		QFile file (path);

		if (d.exists() )
		{
			// We need to parse saved XML data
			parseVYMHandler handler;
			QXmlInputSource source( file);
			QXmlSimpleReader reader;
			reader.setContentHandler( &handler );
			reader.setErrorHandler( &handler );
			handler.setModel (model);
			handler.setTmpDir ( pathDir );	// needed to load files with rel. path
			handler.setLoadMode (ImportAdd);
			blockReposition=true;
			bool ok = reader.parse( source );
			blockReposition=false;
			if (! ok ) 
			{	
				// This should never ever happen
				QMessageBox::critical( 0, tr( "Critical Parse Error while reading %1").arg(path),
										handler.errorProtocol());
			}
			if (sel->getDepth()>0)
				sel->getLastBranch()->linkTo (sel,pos);
		} else	
			QMessageBox::critical( 0, tr( "Critical Error" ), tr("Could not read %1").arg(path));
	}		
}

void MapEditor::pasteNoSave(const int &n)
{
	bool old=blockSaveState;
	blockSaveState=true;
	bool zippedOrg=zipped;
	if (redosAvail > 0 || n!=0)
	{
		// Use the "historical" buffer
		QString bakMapName(QString("history-%1").arg(n));
		QString bakMapDir(tmpMapDir +"/"+bakMapName);
		load (bakMapDir+"/"+clipboardFile,ImportAdd, VymMap);
	} else
		// Use the global buffer
		load (clipboardDir+"/"+clipboardFile,ImportAdd, VymMap);
	zipped=zippedOrg;
	blockSaveState=old;
}

void MapEditor::paste()		
{   
	BranchObj *sel=xelection.getBranch();
	if (sel)
	{
		saveStateChangingPart(
			sel,
			sel,
			QString ("paste (%1)").arg(curClipboard),
			QString("Paste to %1").arg( getName(sel))
		);
		pasteNoSave(0);
		model->reposition();
	}
}

void MapEditor::cut()
{
	LinkableMapObj *sel=xelection.single();
	if ( sel && (xelection.type() == Selection::Branch ||
		xelection.type()==Selection::MapCenter ||
		xelection.type()==Selection::FloatImage))
	{
	/* No savestate! savestate is called in cutNoSave
		saveStateChangingPart(
			sel->getParObj(),
			sel,
			"cut ()",
			QString("Cut %1").arg(getName(sel ))
		);
	*/	
		copy();
		deleteSelection();
		model->reposition();
	}
}

void MapEditor::move(const double &x, const double &y)
{
	LinkableMapObj *sel=xelection.single();
	if (sel)
	{
        QPointF ap(sel->getAbsPos());
        QPointF to(x, y);
        if (ap != to)
        {
            QString ps=qpointfToString(ap);
            QString s=xelection.getSelectString();
            saveState(
                s, "move "+ps, 
                s, "move "+qpointfToString(to), 
                QString("Move %1 to %2").arg(getName(sel)).arg(ps));
            sel->move(x,y);
            model->reposition();
            xelection.update();
        }
	}
}

void MapEditor::moveRel (const double &x, const double &y)
{
	LinkableMapObj *sel=xelection.single();
	if (sel)
	{
        QPointF rp(sel->getRelPos());
        QPointF to(x, y);
        if (rp != to)
        {
            QString ps=qpointfToString (sel->getRelPos());
            QString s=model->getSelectString(sel);
            saveState(
                s, "moveRel "+ps, 
                s, "moveRel "+qpointfToString(to), 
                QString("Move %1 to relative position %2").arg(getName(sel)).arg(ps));
            ((OrnamentedObj*)sel)->move2RelPos (x,y);
            model->reposition();
            sel->updateLink();
            xelection.update();
        }
	}
}

void MapEditor::moveBranchUp()
{
	BranchObj* bo=xelection.getBranch();
	BranchObj* par;
	if (bo)
	{
		if (!bo->canMoveBranchUp()) return;
		par=(BranchObj*)(bo->getParObj());
		BranchObj *obo=par->moveBranchUp (bo);	// bo will be the one below selection
		saveState (model->getSelectString(bo),"moveBranchDown ()",model->getSelectString(obo),"moveBranchUp ()",QString("Move up %1").arg(getName(bo)));
		model->reposition();
		scene()->update();
		xelection.update();
		ensureSelectionVisible();
	}
}

void MapEditor::moveBranchDown()
{
	BranchObj* bo=xelection.getBranch();
	BranchObj* par;
	if (bo)
	{
		if (!bo->canMoveBranchDown()) return;
		par=(BranchObj*)(bo->getParObj());
		BranchObj *obo=par->moveBranchDown(bo);	// bo will be the one above selection
		saveState(model->getSelectString(bo),"moveBranchUp ()",model->getSelectString(obo),"moveBranchDown ()",QString("Move down %1").arg(getName(bo)));
		model->reposition();
		scene()->update();
		xelection.update();
		ensureSelectionVisible();
	}	
}

void MapEditor::sortChildren()
{
	BranchObj* bo=xelection.getBranch();
	if (bo)
	{
		if(bo->countBranches()>1)
		{
			saveStateChangingPart(bo,bo, "sortChildren ()",QString("Sort children of %1").arg(getName(bo)));
			bo->sortChildren();
			model->reposition();
			ensureSelectionVisible();
		}
	}
}

void MapEditor::linkTo(const QString &dstString)	
{
	FloatImageObj *fio=xelection.getFloatImage();
	if (fio)
	{
		BranchObj *dst=(BranchObj*)(model->findObjBySelect(dstString));
		if (dst && (typeid(*dst)==typeid (BranchObj) || 
					typeid(*dst)==typeid (MapCenterObj)))
		{			
			LinkableMapObj *dstPar=dst->getParObj();
			QString parString=model->getSelectString(dstPar);
			QString fioPreSelectString=model->getSelectString(fio);
			QString fioPreParentSelectString=model->getSelectString (fio->getParObj());
			((BranchObj*)(dst))->addFloatImage (fio);
			xelection.unselect();
			((BranchObj*)(fio->getParObj()))->removeFloatImage (fio);
			fio=((BranchObj*)(dst))->getLastFloatImage();
			fio->setRelPos();
			fio->reposition();
			xelection.select(fio);
			saveState(
				model->getSelectString(fio),
				QString("linkTo (\"%1\")").arg(fioPreParentSelectString), 
				fioPreSelectString, 
				QString ("linkTo (\"%1\")").arg(dstString),
				QString ("Link floatimage to %1").arg(getName(dst)));
		}
	}
}

QString MapEditor::getHeading(bool &ok, QPoint &p)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		ok=true;
		p=mapFromScene(bo->getAbsPos());
		return bo->getHeading();
	}
	ok=false;
	return QString();
}

void MapEditor::setHeading(const QString &s)
{
	BranchObj *sel=xelection.getBranch();
	if (sel)
	{
		saveState(
			sel,
			"setHeading (\""+sel->getHeading()+"\")", 
			sel,
			"setHeading (\""+s+"\")", 
			QString("Set heading of %1 to \"%2\"").arg(getName(sel)).arg(s) );
		sel->setHeading(s );
		model->reposition();
		xelection.update();
		ensureSelectionVisible();
	}
}

void MapEditor::setHeadingInt(const QString &s)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		bo->setHeading(s);
		model->reposition();
		xelection.update();
		ensureSelectionVisible();
	}
}

void MapEditor::setVymLinkInt (const QString &s)
{
	// Internal function, no saveState needed
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		bo->setVymLink(s);
		model->reposition();
		updateActions();
		xelection.update();
		ensureSelectionVisible();
	}
}

BranchObj* MapEditor::addMapCenter ()
{
	MapCenterObj *mco= model->addMapCenter(contextMenuPos);
	xelection.select (mco);
	updateActions();
	ensureSelectionVisible();
	saveState (
		mco,
		"delete()",
		NULL,
		QString ("addMapCenter (%1,%2)").arg (contextMenuPos.x()).arg(contextMenuPos.y()),
		QString ("Adding MapCenter to (%1,%2").arg (contextMenuPos.x()).arg(contextMenuPos.y())
	);	
	return mco;	
}

BranchObj* MapEditor::addNewBranchInt(int num)
{
	// Depending on pos:
	// -3		insert in childs of parent  above selection 
	// -2		add branch to selection 
	// -1		insert in childs of parent below selection 
	// 0..n		insert in childs of parent at pos
	BranchObj *newbo=NULL;
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		if (num==-2)
		{
			// save scroll state. If scrolled, automatically select
			// new branch in order to tmp unscroll parent...
			newbo=bo->addBranch();
			
		}else if (num==-1)
		{
			num=bo->getNum()+1;
			bo=(BranchObj*)bo->getParObj();
			if (bo) newbo=bo->insertBranch(num);
		}else if (num==-3)
		{
			num=bo->getNum();
			bo=(BranchObj*)bo->getParObj();
			if (bo) newbo=bo->insertBranch(num);
		}
		if (!newbo) return NULL;
	}	
	return newbo;
}	

BranchObj* MapEditor::addNewBranch(int pos)
{
	// Different meaning than num in addNewBranchInt!
	// -1	add above
	//  0	add as child
	// +1	add below
	BranchObj *bo = xelection.getBranch();
	BranchObj *newbo=NULL;

	if (bo)
	{
		setCursor (Qt::ArrowCursor);

		newbo=addNewBranchInt (pos-2);

		if (newbo)
		{
			saveState(
				newbo,		
				"delete ()",
				bo,
				QString ("addBranch (%1)").arg(pos),
				QString ("Add new branch to %1").arg(getName(bo)));	

			model->reposition();
			xelection.update();
			latestSelection=model->getSelectString(newbo);
			// In Network mode, the client needs to know where the new branch is,
			// so we have to pass on this information via saveState.
			// TODO: Get rid of this positioning workaround
			QString ps=qpointfToString (newbo->getAbsPos());
			sendData ("selectLatestAdded ()");
			sendData (QString("move %1").arg(ps));
			sendSelection();
		}
	}	
	return newbo;
}


BranchObj* MapEditor::addNewBranchBefore()
{
	BranchObj *newbo=NULL;
	BranchObj *bo = xelection.getBranch();
	if (bo && xelection.type()==Selection::Branch)
		 // We accept no MapCenterObj here, so we _have_ a parent
	{
		QPointF p=bo->getRelPos();


		BranchObj *parbo=(BranchObj*)(bo->getParObj());

		// add below selection
		newbo=parbo->insertBranch(bo->getNum()+1);
		if (newbo)
		{
			newbo->move2RelPos (p);

			// Move selection to new branch
			bo->linkTo (newbo,-1);

			saveState (newbo, "deleteKeepChilds ()", newbo, "addBranchBefore ()", 
				QString ("Add branch before %1").arg(getName(bo)));

			model->reposition();
			xelection.update();
		}
	}	
	latestSelection=xelection.getSelectString();
	return newbo;
}

void MapEditor::deleteSelection()
{
	BranchObj *bo = xelection.getBranch();
	if (bo && xelection.type()==Selection::MapCenter)
	{
	//	BranchObj* par=(BranchObj*)(bo->getParObj());
		xelection.unselect();
	/* FIXME Note:  does saveStateRemovingPart work for MCO? (No parent!)
		saveStateRemovingPart (bo, QString ("Delete %1").arg(getName(bo)));
		*/
		bo=model->removeMapCenter ((MapCenterObj*)bo);
		if (bo) 
		{
			xelection.select (bo);
			ensureSelectionVisible();
			xelection.update();
		}	
		model->reposition();
		return;
	}
	if (bo && xelection.type()==Selection::Branch)
	{
		BranchObj* par=(BranchObj*)bo->getParObj();
		xelection.unselect();
		saveStateRemovingPart (bo, QString ("Delete %1").arg(getName(bo)));
		par->removeBranch(bo);
		xelection.select (par);
		ensureSelectionVisible();
		model->reposition();
//		xelection.update();
		xelection.update();
		return;
	}
	FloatImageObj *fio=xelection.getFloatImage();
	if (fio)
	{
		BranchObj* par=(BranchObj*)fio->getParObj();
		saveStateChangingPart(
			par, 
			fio,
			"delete ()",
			QString("Delete %1").arg(getName(fio))
		);
		xelection.unselect();
		par->removeFloatImage(fio);
		xelection.select (par);
		model->reposition();
		xelection.update();
		ensureSelectionVisible();
		return;
	}
}

LinkableMapObj* MapEditor::getSelection()
{
	return xelection.single();
}

BranchObj* MapEditor::getSelectedBranch()
{
	return xelection.getBranch();
}

FloatImageObj* MapEditor::getSelectedFloatImage()
{
	return xelection.getFloatImage();
}

void MapEditor::unselect()
{
	xelection.unselect();
}	

void MapEditor::reselect()
{
	xelection.reselect();
}	

bool MapEditor::select (const QString &s)
{
	if (xelection.select(s))
	{
		xelection.update();
		ensureSelectionVisible();
		sendSelection ();
		return true;
	}
	return false;
}

bool MapEditor::select (LinkableMapObj *lmo)
{
	if (xelection.select(lmo))
	{
		xelection.update();
		ensureSelectionVisible();
		sendSelection ();
		return true;
	}
	return false;
}

QString MapEditor::getSelectString()
{
	return xelection.getSelectString();
}

void MapEditor::selectInt (LinkableMapObj *lmo)
{
	if (lmo && xelection.single()!= lmo && isSelectBlocked()==false )
	{
		xelection.select(lmo);
		xelection.update();
		sendSelection ();
	}	
}

void MapEditor::selectNextBranchInt()
{
	// Increase number of branch
	LinkableMapObj *sel=xelection.single();
	if (sel)
	{
		QString s=xelection.getSelectString();
		QString part;
		QString typ;
		QString num;

		// Where am I? 
		part=s.section(",",-1);
		typ=part.left (3);
		num=part.right(part.length() - 3);

		s=s.left (s.length() -num.length());

		// Go to next lmo
		num=QString ("%1").arg(num.toUInt()+1);

		s=s+num;
		
		// Try to select this one
		if (select (s)) return;

		// We have no direct successor, 
		// try to increase the parental number in order to
		// find a successor with same depth

		int d=xelection.single()->getDepth();
		int oldDepth=d;
		int i;
		bool found=false;
		bool b;
		while (!found && d>0)
		{
			s=s.section (",",0,d-1);
			// replace substring of current depth in s with "1"
			part=s.section(",",-1);
			typ=part.left (3);
			num=part.right(part.length() - 3);

			if (d>1)
			{	
				// increase number of parent
				num=QString ("%1").arg(num.toUInt()+1);
				s=s.section (",",0,d-2) + ","+ typ+num;
			} else
			{
				// Special case, look at orientation
				if (xelection.single()->getOrientation()==LinkableMapObj::RightOfCenter)
					num=QString ("%1").arg(num.toUInt()+1);
				else	
					num=QString ("%1").arg(num.toUInt()-1);
				s=typ+num;
			}	

			if (select (s))
				// pad to oldDepth, select the first branch for each depth
				for (i=d;i<oldDepth;i++)
				{
					b=select (s);
					if (b)
					{	
						if ( xelection.getBranch()->countBranches()>0)
							s+=",bo:0";
						else	
							break;
					} else
						break;
				}	

			// try to select the freshly built string
			found=select(s);
			d--;
		}
		return;
	}	
}

void MapEditor::selectPrevBranchInt()
{
	// Decrease number of branch
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		QString s=xelection.getSelectString();
		QString part;
		QString typ;
		QString num;

		// Where am I? 
		part=s.section(",",-1);
		typ=part.left (3);
		num=part.right(part.length() - 3);

		s=s.left (s.length() -num.length());

		int n=num.toInt()-1;
		
		// Go to next lmo
		num=QString ("%1").arg(n);
		s=s+num;
		
		// Try to select this one
		if (n>=0 && select (s)) return;

		// We have no direct precessor, 
		// try to decrease the parental number in order to
		// find a precessor with same depth

		int d=xelection.single()->getDepth();
		int oldDepth=d;
		int i;
		bool found=false;
		bool b;
		while (!found && d>0)
		{
			s=s.section (",",0,d-1);
			// replace substring of current depth in s with "1"
			part=s.section(",",-1);
			typ=part.left (3);
			num=part.right(part.length() - 3);

			if (d>1)
			{
				// decrease number of parent
				num=QString ("%1").arg(num.toInt()-1);
				s=s.section (",",0,d-2) + ","+ typ+num;
			} else
			{
				// Special case, look at orientation
				if (xelection.single()->getOrientation()==LinkableMapObj::RightOfCenter)
					num=QString ("%1").arg(num.toInt()-1);
				else	
					num=QString ("%1").arg(num.toInt()+1);
				s=typ+num;
			}	

			if (select(s))
				// pad to oldDepth, select the last branch for each depth
				for (i=d;i<oldDepth;i++)
				{
					b=select (s);
					if (b)
						if ( xelection.getBranch()->countBranches()>0)
							s+=",bo:"+ QString ("%1").arg( xelection.getBranch()->countBranches()-1 );
						else	
							break;
					else
						break;
				}	
			
			// try to select the freshly built string
			found=select(s);
			d--;
		}
		return;
	}	
}

void MapEditor::selectUpperBranch()
{
	if (isSelectBlocked() ) return;

	BranchObj *bo=xelection.getBranch();
	if (bo && xelection.type()==Selection::Branch)
	{
		if (bo->getOrientation()==LinkableMapObj::RightOfCenter)
			selectPrevBranchInt();
		else
			if (bo->getDepth()==1)
				selectNextBranchInt();
			else
				selectPrevBranchInt();
	}
}

void MapEditor::selectLowerBranch()
{
	if (isSelectBlocked() ) return;

	BranchObj *bo=xelection.getBranch();
	if (bo && xelection.type()==Selection::Branch)
	{
		if (bo->getOrientation()==LinkableMapObj::RightOfCenter)
			selectNextBranchInt();
		else
			if (bo->getDepth()==1)
				selectPrevBranchInt();
			else
				selectNextBranchInt();
	}			
}


void MapEditor::selectLeftBranch()
{
	if (isSelectBlocked() ) return;

	BranchObj* bo;
	BranchObj* par;
	LinkableMapObj *sel=xelection.single();
	if (sel)
	{
		if (xelection.type()== Selection::MapCenter)
		{
			par=xelection.getBranch();
			bo=par->getLastSelectedBranch();
			if (bo)
			{
				// Workaround for reselecting on left and right side
				if (bo->getOrientation()==LinkableMapObj::RightOfCenter)
					bo=par->getLastBranch();
				if (bo)
				{
					bo=par->getLastBranch();
					xelection.select(bo);
					xelection.update();
					ensureSelectionVisible();
					sendSelection();
				}
			}	
		} else
		{
			par=(BranchObj*)(sel->getParObj());
			if (sel->getOrientation()==LinkableMapObj::RightOfCenter)
			{
				if (xelection.type() == Selection::Branch ||
					xelection.type() == Selection::FloatImage)
				{
					xelection.select(par);
					xelection.update();
					ensureSelectionVisible();
					sendSelection();
				}
			} else
			{
				if (xelection.type() == Selection::Branch )
				{
					bo=xelection.getBranch()->getLastSelectedBranch();
					if (bo) 
					{
						xelection.select(bo);
						xelection.update();
						ensureSelectionVisible();
					sendSelection();
					}
				}
			}
		}	
	}
}

void MapEditor::selectRightBranch()
{
	if (isSelectBlocked() ) return;

	BranchObj* bo;
	BranchObj* par;
	LinkableMapObj *sel=xelection.single();
	if (sel)
	{
		if (xelection.type()==Selection::MapCenter) 
		{
			par=xelection.getBranch();
			bo=par->getLastSelectedBranch();
			if (bo)
			{
				// Workaround for reselecting on left and right side
				if (bo->getOrientation()==LinkableMapObj::LeftOfCenter)
					bo=par->getFirstBranch();
				if (bo)
				{
					xelection.select(bo);
					xelection.update();
					ensureSelectionVisible();
					sendSelection();
				}
			}
		} else
		{
			par=(BranchObj*)(xelection.single()->getParObj());
			if (xelection.single()->getOrientation()==LinkableMapObj::LeftOfCenter)
			{
				if (xelection.type() == Selection::Branch ||
					xelection.type() == Selection::FloatImage)
				{
					xelection.select(par);
					xelection.update();
					ensureSelectionVisible();
					sendSelection();
				}
			} else
			{
				if (xelection.type()  == Selection::Branch) 
				{
					bo=xelection.getBranch()->getLastSelectedBranch();
					if (bo) 
					{
						xelection.select(bo);
						xelection.update();
						ensureSelectionVisible();
					sendSelection();
					}
				}
			}
		}
	}
}

void MapEditor::selectFirstBranch()
{
	BranchObj *bo1=xelection.getBranch();
	BranchObj *bo2;
	BranchObj* par;
	if (bo1)
	{
		par=(BranchObj*)(bo1->getParObj());
		if (!par) return;
		bo2=par->getFirstBranch();
		if (bo2) {
			xelection.select(bo2);
			xelection.update();
			ensureSelectionVisible();
			sendSelection();
		}
	}		
}

void MapEditor::selectLastBranch()
{
	BranchObj *bo1=xelection.getBranch();
	BranchObj *bo2;
	BranchObj* par;
	if (bo1)
	{
		par=(BranchObj*)(bo1->getParObj());
		if (!par) return;
		bo2=par->getLastBranch();
		if (bo2) 
		{
			xelection.select(bo2);
			xelection.update();
			ensureSelectionVisible();
			sendSelection();
		}
	}		
}

void MapEditor::selectMapBackgroundImage ()
{
	Q3FileDialog *fd=new Q3FileDialog( this);
	fd->setMode (Q3FileDialog::ExistingFile);
	fd->addFilter (QString (tr("Images") + " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm)"));
	ImagePreview *p =new ImagePreview (fd);
	fd->setContentsPreviewEnabled( TRUE );
	fd->setContentsPreview( p, p );
	fd->setPreviewMode( Q3FileDialog::Contents );
	fd->setCaption(vymName+" - " +tr("Load background image"));
	fd->setDir (lastImageDir);
	fd->show();

	if ( fd->exec() == QDialog::Accepted )
	{
		// TODO selectMapBackgroundImg in QT4 use:	lastImageDir=fd->directory();
		lastImageDir=QDir (fd->dirPath());
		setMapBackgroundImage (fd->selectedFile());
	}
}	

void MapEditor::setMapBackgroundImage (const QString &fn)	//FIXME missing savestate
{
	QColor oldcol=mapScene->backgroundBrush().color();
	/*
	saveState(
		selection,
		QString ("setMapBackgroundImage (%1)").arg(oldcol.name()),
		selection,
		QString ("setMapBackgroundImage (%1)").arg(col.name()),
		QString("Set background color of map to %1").arg(col.name()));
	*/	
	QBrush brush;
	brush.setTextureImage (QPixmap (fn));
	mapScene->setBackgroundBrush(brush);
}

void MapEditor::selectMapBackgroundColor()
{
	QColor col = QColorDialog::getColor( mapScene->backgroundBrush().color(), this );
	if ( !col.isValid() ) return;
	setMapBackgroundColor( col );
}


void MapEditor::setMapBackgroundColor(QColor col)
{
	QColor oldcol=mapScene->backgroundBrush().color();
	saveState(
		QString ("setMapBackgroundColor (\"%1\")").arg(oldcol.name()),
		QString ("setMapBackgroundColor (\"%1\")").arg(col.name()),
		QString("Set background color of map to %1").arg(col.name()));
	mapScene->setBackgroundBrush(col);
}

QColor MapEditor::getMapBackgroundColor()
{
    return mapScene->backgroundBrush().color();
}

QColor MapEditor::getCurrentHeadingColor()
{
	BranchObj *bo=xelection.getBranch();
	if (bo) return bo->getColor(); 
	
	QMessageBox::warning(0,tr("Warning"),tr("Can't get color of heading,\nthere's no branch selected"));
	return Qt::black;
}

void MapEditor::colorBranch (QColor c)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		saveState(
			bo, 
			QString ("colorBranch (\"%1\")").arg(bo->getColor().name()),
			bo,
			QString ("colorBranch (\"%1\")").arg(c.name()),
			QString("Set color of %1 to %2").arg(getName(bo)).arg(c.name())
		);	
		bo->setColor(c); // color branch
	}
}

void MapEditor::colorSubtree (QColor c)
{
	BranchObj *bo=xelection.getBranch();
	if (bo) 
	{
		saveStateChangingPart(
			bo, 
			bo,
			QString ("colorSubtree (\"%1\")").arg(c.name()),
			QString ("Set color of %1 and childs to %2").arg(getName(bo)).arg(c.name())
		);	
		bo->setColorSubtree (c); // color links, color childs
	}
}


void MapEditor::toggleStandardFlag(QString f)
{
	BranchObj *bo=xelection.getBranch();
	if (bo) 
	{
		QString u,r;
		if (bo->isSetStandardFlag(f))
		{
			r="unsetFlag";
			u="setFlag";
		}	
		else
		{
			u="unsetFlag";
			r="setFlag";
		}	
		saveState(
			bo,
			QString("%1 (\"%2\")").arg(u).arg(f), 
			bo,
			QString("%1 (\"%2\")").arg(r).arg(f),
			QString("Toggling standard flag \"%1\" of %2").arg(f).arg(getName(bo)));
		bo->toggleStandardFlag (f,mainWindow->useFlagGroups());
		xelection.update();
	}
}


BranchObj* MapEditor::findText (QString s, bool cs)
{
	QTextDocument::FindFlags flags=0;
	if (cs) flags=QTextDocument::FindCaseSensitively;

	if (!itFind) 
	{	// Nothing found or new find process
		if (EOFind)
			// nothing found, start again
			EOFind=false;
		itFind=model->first();
	}	
	bool searching=true;
	bool foundNote=false;
	while (searching && !EOFind)
	{
		if (itFind)
		{
			// Searching in Note
			if (itFind->getNote().contains(s,cs))
			{
				if (xelection.single()!=itFind) 
				{
					xelection.select(itFind);
					ensureSelectionVisible();
				}
				if (textEditor->findText(s,flags)) 
				{
					searching=false;
					foundNote=true;
				}	
			}
			// Searching in Heading
			if (searching && itFind->getHeading().contains (s,cs) ) 
			{
				xelection.select(itFind);
				ensureSelectionVisible();
				searching=false;
			}
		}	
		if (!foundNote)
		{
			itFind=model->next(itFind);
			if (!itFind) EOFind=true;
		}
	//cout <<"still searching...  "<<qPrintable( itFind->getHeading())<<endl;
	}	
	if (!searching)
		return xelection.getBranch();
	else
		return NULL;
}

void MapEditor::findReset()
{	// Necessary if text to find changes during a find process
	itFind=NULL;
	EOFind=false;
}
void MapEditor::setURL(const QString &url)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		QString oldurl=bo->getURL();
		bo->setURL (url);
		saveState (
			bo,
			QString ("setURL (\"%1\")").arg(oldurl),
			bo,
			QString ("setURL (\"%1\")").arg(url),
			QString ("set URL of %1 to %2").arg(getName(bo)).arg(url)
		);
		updateActions();
		model->reposition();
		xelection.update();
		ensureSelectionVisible();
	}
}	

void MapEditor::editURL()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{		
		bool ok;
		QString text = QInputDialog::getText(
				"VYM", tr("Enter URL:"), QLineEdit::Normal,
				bo->getURL(), &ok, this );
		if ( ok) 
			// user entered something and pressed OK
			setURL (text);
	}
}

void MapEditor::editLocalURL()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{		
		QStringList filters;
		filters <<"All files (*)";
		filters << tr("Text","Filedialog") + " (*.txt)";
		filters << tr("Spreadsheet","Filedialog") + " (*.odp,*.sxc)";
		filters << tr("Textdocument","Filedialog") +" (*.odw,*.sxw)";
		filters << tr("Images","Filedialog") + " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm)";
		QFileDialog *fd=new QFileDialog( this,vymName+" - " +tr("Set URL to a local file"));
		fd->setFilters (filters);
		fd->setCaption(vymName+" - " +tr("Set URL to a local file"));
		fd->setDirectory (lastFileDir);
		if (! bo->getVymLink().isEmpty() )
			fd->selectFile( bo->getURL() );
		fd->show();

		if ( fd->exec() == QDialog::Accepted )
		{
			lastFileDir=QDir (fd->directory().path());
			setURL (fd->selectedFile() );
		}
	}
}

QString MapEditor::getURL()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
		return bo->getURL();
	else
		return "";
}

QStringList MapEditor::getURLs()
{
	QStringList urls;
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{		
		bo=bo->first();	
		while (bo) 
		{
			if (!bo->getURL().isEmpty()) urls.append( bo->getURL());
			bo=bo->next();
		}	
	}	
	return urls;
}


void MapEditor::editHeading2URL()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
		setURL (bo->getHeading());
}	

void MapEditor::editBugzilla2URL()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{		
		QString url= "https://bugzilla.novell.com/show_bug.cgi?id="+bo->getHeading();
		setURL (url);
	}
}	

void MapEditor::editFATE2URL()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{		
		QString url= "http://keeper.suse.de:8080/webfate/match/id?value=ID"+bo->getHeading();
		saveState(
			bo,
			"setURL (\""+bo->getURL()+"\")",
			bo,
			"setURL (\""+url+"\")",
			QString("Use heading of %1 as link to FATE").arg(getName(bo))
		);	
		bo->setURL (url);
		updateActions();
	}
}	

void MapEditor::editVymLink()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{		
		QStringList filters;
		filters <<"VYM map (*.vym)";
		QFileDialog *fd=new QFileDialog( this,vymName+" - " +tr("Link to another map"));
		fd->setFilters (filters);
		fd->setCaption(vymName+" - " +tr("Link to another map"));
		fd->setDirectory (lastFileDir);
		if (! bo->getVymLink().isEmpty() )
			fd->selectFile( bo->getVymLink() );
		fd->show();

		QString fn;
		if ( fd->exec() == QDialog::Accepted )
		{
			lastFileDir=QDir (fd->directory().path());
			saveState(
				bo,
				"setVymLink (\""+bo->getVymLink()+"\")",
				bo,
				"setVymLink (\""+fd->selectedFile()+"\")",
				QString("Set vymlink of %1 to %2").arg(getName(bo)).arg(fd->selectedFile())
			);	
			setVymLinkInt (fd->selectedFile() );
		}
	}
}

void MapEditor::deleteVymLink()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{		
		saveState(
			bo,
			"setVymLink (\""+bo->getVymLink()+"\")",
			bo,
			"setVymLink (\"\")",
			QString("Unset vymlink of %1").arg(getName(bo))
		);	
		bo->setVymLink ("" );
		updateActions();
		model->reposition();
		scene()->update();
	}
}

void MapEditor::setHideExport(bool b)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		bo->setHideInExport (b);
		QString u= b ? "false" : "true";
		QString r=!b ? "false" : "true";
		
		saveState(
			bo,
			QString ("setHideExport (%1)").arg(u),
			bo,
			QString ("setHideExport (%1)").arg(r),
			QString ("Set HideExport flag of %1 to %2").arg(getName(bo)).arg (r)
		);	
		updateActions();
		model->reposition();
		xelection.update();
		scene()->update();
	}
}

void MapEditor::toggleHideExport()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
		setHideExport ( !bo->hideInExport() );
}

QString MapEditor::getVymLink()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
		return bo->getVymLink();
	else	
		return "";
	
}

QStringList MapEditor::getVymLinks()
{
	QStringList links;
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{		
		bo=bo->first();	
		while (bo) 
		{
			if (!bo->getVymLink().isEmpty()) links.append( bo->getVymLink());
			bo=bo->next();
		}	
	}	
	return links;
}


void MapEditor::deleteKeepChilds()
{
	BranchObj *bo=xelection.getBranch();
	BranchObj *par;
	if (bo)
	{
		par=(BranchObj*)(bo->getParObj());

		// Don't use this on mapcenter
		if (!par) return;

		// Check if we have childs at all to keep
		if (bo->countBranches()==0) 
		{
			deleteSelection();
			return;
		}

		QPointF p=bo->getRelPos();
		saveStateChangingPart(
			bo->getParObj(),
			bo,
			"deleteKeepChilds ()",
			QString("Remove %1 and keep its childs").arg(getName(bo))
		);

		QString sel=model->getSelectString(bo);
		unselect();
		par->removeBranchHere(bo);
		model->reposition();
		select (sel);
		xelection.getBranch()->move2RelPos (p);
		model->reposition();
	}	
}

void MapEditor::deleteChilds()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{		
		saveStateChangingPart(
			bo, 
			bo,
			"deleteChilds ()",
			QString( "Remove childs of branch %1").arg(getName(bo))
		);
		bo->removeChilds();
		model->reposition();
	}	
}

void MapEditor::editMapInfo()
{
	ExtraInfoDialog dia;
	dia.setMapName (getFileName() );
	dia.setAuthor (model->getAuthor() );
	dia.setComment(model->getComment() );

	// Calc some stats
	QString stats;
    stats+=tr("%1 items on map\n","Info about map").arg (mapScene->items().size(),6);

	uint b=0;
	uint f=0;
	uint n=0;
	uint xl=0;
	BranchObj *bo;
	bo=model->first();
	while (bo) 
	{
		if (!bo->getNote().isEmpty() ) n++;
		f+= bo->countFloatImages();
		b++;
		xl+=bo->countXLinks();
		bo=model->next(bo);
	}
    stats+=QString ("%1 branches\n").arg (b-1,6);
    stats+=QString ("%1 xLinks \n").arg (xl,6);
    stats+=QString ("%1 notes\n").arg (n,6);
    stats+=QString ("%1 images\n").arg (f,6);
	dia.setStats (stats);

	// Finally show dialog
	if (dia.exec() == QDialog::Accepted)
	{
		setMapAuthor (dia.getAuthor() );
		setMapComment (dia.getComment() );
	}
}

void MapEditor::ensureSelectionVisible()
{
	LinkableMapObj *lmo=xelection.single();
	if (lmo) ensureVisible (lmo->getBBox() );
	
}

void MapEditor::updateSelection()
{
	// Tell selection to update geometries
	xelection.update();
}

void MapEditor::updateActions()
{
	// Tell mainwindow to update states of actions
	mainWindow->updateActions();
	// TODO maybe don't update if blockReposition is set
}

void MapEditor::updateNoteFlag()
{
	setChanged();
	BranchObj *bo=xelection.getBranch();
	if (bo) 
	{
		bo->updateNoteFlag();
		mainWindow->updateActions();
	}	
}

void MapEditor::setMapAuthor (const QString &s)
{
	saveState (
		QString ("setMapAuthor (\"%1\")").arg(model->getAuthor()),
		QString ("setMapAuthor (\"%1\")").arg(s),
		QString ("Set author of map to \"%1\"").arg(s)
	);
	model->setAuthor (s);
}

void MapEditor::setMapComment (const QString &s)
{
	saveState (
		QString ("setMapComment (\"%1\")").arg(model->getComment()),
		QString ("setMapComment (\"%1\")").arg(s),
		QString ("Set comment of map")
	);
	model->setComment (s);
}

void MapEditor::setMapLinkStyle (const QString & s)
{
	QString snow;
	if (linkstyle==LinkableMapObj::Line)
		snow="StyleLine";
	else if (linkstyle==LinkableMapObj::Parabel)
		snow="StyleParabel";
	else if (linkstyle==LinkableMapObj::PolyLine)
		snow="StylePolyLine";
	else if (linkstyle==LinkableMapObj::PolyParabel)
		snow="StyleParabel";

	saveState (
		QString("setMapLinkStyle (\"%1\")").arg(s),
		QString("setMapLinkStyle (\"%1\")").arg(snow),
		QString("Set map link style (\"%1\")").arg(s)
	);	

	if (s=="StyleLine")
		linkstyle=LinkableMapObj::Line;
	else if (s=="StyleParabel")
		linkstyle=LinkableMapObj::Parabel;
	else if (s=="StylePolyLine")
		linkstyle=LinkableMapObj::PolyLine;
	else	
		linkstyle=LinkableMapObj::PolyParabel;

	BranchObj *bo;
	bo=model->first();
	bo=model->next(bo);
	while (bo) 
	{
		bo->setLinkStyle(bo->getDefLinkStyle());
		bo=model->next(bo);
	}
	model->reposition();
}

LinkableMapObj::Style MapEditor::getMapLinkStyle ()
{
	return linkstyle;
}	

void MapEditor::setMapDefLinkColor(QColor c)
{
	defLinkColor=c;
	BranchObj *bo;
	bo=model->first();
	while (bo) 
	{
		bo->setLinkColor();
		bo=model->next(bo);
	}
	updateActions();
}

void MapEditor::setMapLinkColorHintInt()
{
	// called from setMapLinkColorHint(lch) or at end of parse
	BranchObj *bo;
	bo=model->first();
	while (bo) 
	{
		bo->setLinkColor();
		bo=model->next(bo);
	}
}

void MapEditor::setMapLinkColorHint(LinkableMapObj::ColorHint lch)
{
	linkcolorhint=lch;
	setMapLinkColorHintInt();
}

void MapEditor::toggleMapLinkColorHint()
{
	if (linkcolorhint==LinkableMapObj::HeadingColor)
		linkcolorhint=LinkableMapObj::DefaultColor;
	else	
		linkcolorhint=LinkableMapObj::HeadingColor;
	BranchObj *bo;
	bo=model->first();
	while (bo) 
	{
		bo->setLinkColor();
		bo=model->next(bo);
	}
}

LinkableMapObj::ColorHint MapEditor::getMapLinkColorHint()
{
	return linkcolorhint;
}

QColor MapEditor::getMapDefLinkColor()
{
	return defLinkColor;
}

void MapEditor::setMapDefXLinkColor(QColor col)
{
	defXLinkColor=col;
}

QColor MapEditor::getMapDefXLinkColor()
{
	return defXLinkColor;
}

void MapEditor::setMapDefXLinkWidth (int w)
{
	defXLinkWidth=w;
}

int MapEditor::getMapDefXLinkWidth()
{
	return defXLinkWidth;
}

void MapEditor::selectMapLinkColor()
{
	QColor col = QColorDialog::getColor( defLinkColor, this );
	if ( !col.isValid() ) return;
	saveState (
		QString("setMapDefLinkColor (\"%1\")").arg(getMapDefLinkColor().name()),
		QString("setMapDefLinkColor (\"%1\")").arg(col.name()),
		QString("Set map link color to %1").arg(col.name())
	);
	setMapDefLinkColor( col );
}

void MapEditor::selectMapSelectionColor()
{
	QColor col = QColorDialog::getColor( defLinkColor, this );
	setSelectionColor (col);
}

void MapEditor::setSelectionColorInt (QColor col)
{
	if ( !col.isValid() ) return;
	xelection.setColor (col);
}

void MapEditor::setSelectionColor(QColor col)
{
	if ( !col.isValid() ) return;
	saveState (
		QString("setSelectionColor (%1)").arg(xelection.getColor().name()),
		QString("setSelectionColor (%1)").arg(col.name()),
		QString("Set color of selection box to %1").arg(col.name())
	);
	setSelectionColorInt (col);
}

QColor MapEditor::getSelectionColor()
{
	return xelection.getColor();
}

bool MapEditor::scrollBranch(BranchObj *bo)
{
	if (bo)
	{
		if (bo->isScrolled()) return false;
		if (bo->countBranches()==0) return false;
		if (bo->getDepth()==0) return false;
		QString u,r;
		r="scroll";
		u="unscroll";
		saveState(
			bo,
			QString ("%1 ()").arg(u),
			bo,
			QString ("%1 ()").arg(r),
			QString ("%1 %2").arg(r).arg(getName(bo))
		);
		bo->toggleScroll();
		xelection.update();
		scene()->update();
		return true;
	}	
	return false;
}

bool MapEditor::unscrollBranch(BranchObj *bo)
{
	if (bo)
	{
		if (!bo->isScrolled()) return false;
		if (bo->countBranches()==0) return false;
		if (bo->getDepth()==0) return false;
		QString u,r;
		u="scroll";
		r="unscroll";
		saveState(
			bo,
			QString ("%1 ()").arg(u),
			bo,
			QString ("%1 ()").arg(r),
			QString ("%1 %2").arg(r).arg(getName(bo))
		);
		bo->toggleScroll();
		xelection.update();
		scene()->update();
		return true;
	}	
	return false;
}

void MapEditor::toggleScroll()
{
	BranchObj *bo=xelection.getBranch();
	if (xelection.type()==Selection::Branch )
	{
		if (bo->isScrolled())
			unscrollBranch (bo);
		else
			scrollBranch (bo);
	}
}

void MapEditor::unscrollChilds() 
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		bo->first();
		while (bo) 
		{
			if (bo->isScrolled()) unscrollBranch (bo);
			bo=bo->next();
		}
	}	
}

FloatImageObj* MapEditor::loadFloatImageInt (QString fn)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		FloatImageObj *fio;
		bo->addFloatImage();
		fio=bo->getLastFloatImage();
		fio->load(fn);
		model->reposition();
		scene()->update();
		return fio;
	}
	return NULL;
}	

void MapEditor::loadFloatImage ()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{

		Q3FileDialog *fd=new Q3FileDialog( this);
		fd->setMode (Q3FileDialog::ExistingFiles);
		fd->addFilter (QString (tr("Images") + " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm)"));
		ImagePreview *p =new ImagePreview (fd);
		fd->setContentsPreviewEnabled( TRUE );
		fd->setContentsPreview( p, p );
		fd->setPreviewMode( Q3FileDialog::Contents );
		fd->setCaption(vymName+" - " +tr("Load image"));
		fd->setDir (lastImageDir);
		fd->show();

		if ( fd->exec() == QDialog::Accepted )
		{
			// TODO loadFIO in QT4 use:	lastImageDir=fd->directory();
			lastImageDir=QDir (fd->dirPath());
			QString s;
			FloatImageObj *fio;
			for (int j=0; j<fd->selectedFiles().count(); j++)
			{
				s=fd->selectedFiles().at(j);
				fio=loadFloatImageInt (s);
				if (fio)
					saveState(
						(LinkableMapObj*)fio,
						"delete ()",
						bo, 
						QString ("loadImage (%1)").arg(s ),
						QString("Add image %1 to %2").arg(s).arg(getName(bo))
					);
				else
					// TODO loadFIO error handling
					qWarning ("Failed to load "+s);
			}
		}
		delete (p);
		delete (fd);
	}
}

void MapEditor::saveFloatImageInt  (FloatImageObj *fio, const QString &type, const QString &fn)
{
	fio->save (fn,type);
}

void MapEditor::saveFloatImage ()
{
	FloatImageObj *fio=xelection.getFloatImage();
	if (fio)
	{
		QFileDialog *fd=new QFileDialog( this);
		fd->setFilters (imageIO.getFilters());
		fd->setCaption(vymName+" - " +tr("Save image"));
		fd->setFileMode( QFileDialog::AnyFile );
		fd->setDirectory (lastImageDir);
//		fd->setSelection (fio->getOriginalFilename());
		fd->show();

		QString fn;
		if ( fd->exec() == QDialog::Accepted && fd->selectedFiles().count()==1)
		{
			fn=fd->selectedFiles().at(0);
			if (QFile (fn).exists() )
			{
				QMessageBox mb( vymName,
					tr("The file %1 exists already.\n"
					"Do you want to overwrite it?").arg(fn),
				QMessageBox::Warning,
				QMessageBox::Yes | QMessageBox::Default,
				QMessageBox::Cancel | QMessageBox::Escape,
				QMessageBox::NoButton );

				mb.setButtonText( QMessageBox::Yes, tr("Overwrite") );
				mb.setButtonText( QMessageBox::No, tr("Cancel"));
				switch( mb.exec() ) 
				{
					case QMessageBox::Yes:
						// save 
						break;
					case QMessageBox::Cancel:
						// do nothing
						delete (fd);
						return;
						break;
				}
			}
			saveFloatImageInt (fio,fd->selectedFilter(),fn );
		}
		delete (fd);
	}
}

void MapEditor::setFrameType(const FrameObj::FrameType &t)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		QString s=bo->getFrameTypeName();
		bo->setFrameType (t);
		saveState (bo, QString("setFrameType (\"%1\")").arg(s),
			bo, QString ("setFrameType (\"%1\")").arg(bo->getFrameTypeName()),QString ("set type of frame to %1").arg(s));
		model->reposition();
		bo->updateLink();
	}
}

void MapEditor::setFrameType(const QString &s)	
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		saveState (bo, QString("setFrameType (\"%1\")").arg(bo->getFrameTypeName()),
			bo, QString ("setFrameType (\"%1\")").arg(s),QString ("set type of frame to %1").arg(s));
		bo->setFrameType (s);
		model->reposition();
		bo->updateLink();
	}
}

void MapEditor::setFramePenColor(const QColor &c)	
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		saveState (bo, QString("setFramePenColor (\"%1\")").arg(bo->getFramePenColor().name() ),
			bo, QString ("setFramePenColor (\"%1\")").arg(c.name() ),QString ("set pen color of frame to %1").arg(c.name() ));
		bo->setFramePenColor (c);
	}	
}

void MapEditor::setFrameBrushColor(const QColor &c)	
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		saveState (bo, QString("setFrameBrushColor (\"%1\")").arg(bo->getFrameBrushColor().name() ),
			bo, QString ("setFrameBrushColor (\"%1\")").arg(c.name() ),QString ("set brush color of frame to %1").arg(c.name() ));
		bo->setFrameBrushColor (c);
	}	
}

void MapEditor::setFramePadding (const int &i)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		saveState (bo, QString("setFramePadding (\"%1\")").arg(bo->getFramePadding() ),
			bo, QString ("setFramePadding (\"%1\")").arg(i),QString ("set brush color of frame to %1").arg(i));
		bo->setFramePadding (i);
		model->reposition();
		bo->updateLink();
	}	
}

void MapEditor::setFrameBorderWidth(const int &i)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		saveState (bo, QString("setFrameBorderWidth (\"%1\")").arg(bo->getFrameBorderWidth() ),
			bo, QString ("setFrameBorderWidth (\"%1\")").arg(i),QString ("set border width of frame to %1").arg(i));
		bo->setFrameBorderWidth (i);
		model->reposition();
		bo->updateLink();
	}	
}

void MapEditor::setIncludeImagesVer(bool b)	
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		QString u= b ? "false" : "true";
		QString r=!b ? "false" : "true";
		
		saveState(
			bo,
			QString("setIncludeImagesVertically (%1)").arg(u),
			bo, 
			QString("setIncludeImagesVertically (%1)").arg(r),
			QString("Include images vertically in %1").arg(getName(bo))
		);	
		bo->setIncludeImagesVer(b);
		model->reposition();
	}	
}

void MapEditor::setIncludeImagesHor(bool b)	
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		QString u= b ? "false" : "true";
		QString r=!b ? "false" : "true";
		
		saveState(
			bo,
			QString("setIncludeImagesHorizontally (%1)").arg(u),
			bo, 
			QString("setIncludeImagesHorizontally (%1)").arg(r),
			QString("Include images horizontally in %1").arg(getName(bo))
		);	
		bo->setIncludeImagesHor(b);
		model->reposition();
	}	
}

void MapEditor::setHideLinkUnselected (bool b)
{
	LinkableMapObj *sel=xelection.single();
	if (sel &&
		(xelection.type() == Selection::Branch || 
		xelection.type() == Selection::MapCenter  ||
		xelection.type() == Selection::FloatImage ))
	{
		QString u= b ? "false" : "true";
		QString r=!b ? "false" : "true";
		
		saveState(
			sel,
			QString("setHideLinkUnselected (%1)").arg(u),
			sel, 
			QString("setHideLinkUnselected (%1)").arg(r),
			QString("Hide link of %1 if unselected").arg(getName(sel))
		);	
		sel->setHideLinkUnselected(b);
	}
}

void MapEditor::importDirInt(BranchObj *dst, QDir d)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		// Traverse directories
		d.setFilter( QDir::Dirs| QDir::Hidden | QDir::NoSymLinks );
		QFileInfoList list = d.entryInfoList();
		QFileInfo fi;

		for (int i = 0; i < list.size(); ++i) 
		{
			fi=list.at(i);
			if (fi.fileName() != "." && fi.fileName() != ".." )
			{
				dst->addBranch();
				bo=dst->getLastBranch();
				bo->setHeading (fi.fileName() );
				bo->setColor (QColor("blue"));
				bo->toggleScroll();
				if ( !d.cd(fi.fileName()) ) 
					QMessageBox::critical (0,tr("Critical Import Error"),tr("Cannot find the directory %1").arg(fi.fileName()));
				else 
				{
					// Recursively add subdirs
					importDirInt (bo,d);
					d.cdUp();
				}
			}	
		}		
		// Traverse files
		d.setFilter( QDir::Files| QDir::Hidden | QDir::NoSymLinks );
		list = d.entryInfoList();

		for (int i = 0; i < list.size(); ++i) 
		{
			fi=list.at(i);
			dst->addBranch();
			bo=dst->getLastBranch();
			bo->setHeading (fi.fileName() );
			bo->setColor (QColor("black"));
			if (fi.fileName().right(4) == ".vym" )
				bo->setVymLink (fi.filePath());
		}	
	}		
}

void MapEditor::importDirInt (const QString &s)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		saveStateChangingPart (bo,bo,QString ("importDir (\"%1\")").arg(s),QString("Import directory structure from %1").arg(s));

		QDir d(s);
		importDirInt (bo,d);
	}
}	

void MapEditor::importDir()
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		QStringList filters;
		filters <<"VYM map (*.vym)";
		QFileDialog *fd=new QFileDialog( this,vymName+ " - " +tr("Choose directory structure to import"));
		fd->setMode (QFileDialog::DirectoryOnly);
		fd->setFilters (filters);
		fd->setCaption(vymName+" - " +tr("Choose directory structure to import"));
		fd->show();

		QString fn;
		if ( fd->exec() == QDialog::Accepted )
		{
			importDirInt (fd->selectedFile() );
			model->reposition();
			scene()->update();
		}
	}	
}

void MapEditor::followXLink(int i)
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		bo=bo->XLinkTargetAt(i);
		if (bo) 
		{
			xelection.select(bo);
			ensureSelectionVisible();
		}
	}
}

void MapEditor::editXLink(int i)	// FIXME missing saveState
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
	{
		XLinkObj *xlo=bo->XLinkAt(i);
		if (xlo) 
		{
			EditXLinkDialog dia;
			dia.setXLink (xlo);
			dia.setSelection(bo);
			if (dia.exec() == QDialog::Accepted)
			{
				if (dia.useSettingsGlobal() )
				{
					setMapDefXLinkColor (xlo->getColor() );
					setMapDefXLinkWidth (xlo->getWidth() );
				}
				if (dia.deleteXLink())
					bo->deleteXLinkAt(i);
			}
		}	
	}
}

AttributeTable* MapEditor::attributeTable()
{
	return attrTable;
}

void MapEditor::testFunction1()
{
	BranchObj *bo=xelection.getBranch();
	if (bo) model->moveAway (bo);
	
/* TODO Hide hidden stuff temporary, maybe add this as regular function somewhere
	if (hidemode==HideNone)
	{
		setHideTmpMode (HideExport);
		mapCenter->calcBBoxSizeWithChilds();
		QRectF totalBBox=mapCenter->getTotalBBox();
		QRectF mapRect=totalBBox;
		QCanvasRectangle *frame=NULL;

		cout << "  map has =("<<totalBBox.x()<<","<<totalBBox.y()<<","<<totalBBox.width()<<","<<totalBBox.height()<<")\n";
	
		mapRect.setRect (totalBBox.x(), totalBBox.y(), 
			totalBBox.width(), totalBBox.height());
		frame=new QCanvasRectangle (mapRect,mapScene);
		frame->setBrush (QColor(white));
		frame->setPen (QColor(black));
		frame->setZValue(0);
		frame->show();    
	}	
	else	
	{
		setHideTmpMode (HideNone);
	}	
	cout <<"  hidemode="<<hidemode<<endl;
	*/
}	
	
void MapEditor::testFunction2()
{

/*
	// Toggle hidemode
	if (hidemode==HideExport)
		setHideTmpMode (HideNone);
	else	
		setHideTmpMode (HideExport);
*/		
}

void MapEditor::contextMenuEvent ( QContextMenuEvent * e )
{
	// Lineedits are already closed by preceding
	// mouseEvent, we don't need to close here.

    QPointF p = mapToScene(e->pos());
    LinkableMapObj* lmo=model->findMapObj(p, NULL);
	
    if (lmo) 
	{	// MapObj was found
		if (xelection.single() != lmo)
		{
			// select the MapObj
			xelection.select(lmo);
		}
		// Context Menu 
		if (xelection.getBranch() ) 
		{
			// Context Menu on branch or mapcenter
			updateActions();
			branchContextMenu->popup(e->globalPos() );
		} else
		{
			if (xelection.getFloatImage() )
			{
				// Context Menu on floatimage
				updateActions();
				floatimageContextMenu->popup(e->globalPos() );
			}	
		}	
	} else 
	{ // No MapObj found, we are on the Canvas itself
		// Context Menu on scene
		updateActions();
		contextMenuPos=p;
		canvasContextMenu->popup(e->globalPos() );
    } 
	e->accept();
}

void MapEditor::keyPressEvent(QKeyEvent* e)
{
	if (e->modifiers() & Qt::ControlModifier)
	{
		switch (mainWindow->getModMode())
		{
			case Main::ModModeColor: 
				setCursor (PickColorCursor);
				break;
			case Main::ModModeCopy: 
				setCursor (CopyCursor);
				break;
			case Main::ModModeXLink: 
				setCursor (XLinkCursor);
				break;
			default :
				setCursor (Qt::ArrowCursor);
				break;
		} 
	}	
}

void MapEditor::keyReleaseEvent(QKeyEvent* e)
{
	if (!(e->modifiers() & Qt::ControlModifier))
		setCursor (Qt::ArrowCursor);
}

void MapEditor::mousePressEvent(QMouseEvent* e)
{
	// Ignore right clicks, these will go to context menus
	if (e->button() == Qt::RightButton )
	{
		e->ignore();
		return;
	}

	//Ignore clicks while editing heading
	if (isSelectBlocked() ) 
	{
		e->ignore();
		return;
	}

    QPointF p = mapToScene(e->pos());
    LinkableMapObj* lmo=model->findMapObj(p, NULL);
	
	e->accept();

	//Take care of  system flags _or_ modifier modes
	//
	if (lmo && (typeid(*lmo)==typeid(BranchObj) ||
		typeid(*lmo)==typeid(MapCenterObj) ))
	{
		QString foname=((BranchObj*)lmo)->getSystemFlagName(p);
		if (!foname.isEmpty())
		{
			// systemFlag clicked
			selectInt (lmo);
			if (foname=="url") 
			{
				if (e->state() & Qt::ControlModifier)
					mainWindow->editOpenURLTab();
				else	
					mainWindow->editOpenURL();
			}	
			else if (foname=="vymLink")
			{
				mainWindow->editOpenVymLink();
				// tabWidget may change, better return now
				// before segfaulting...
			} else if (foname=="note")
				mainWindow->windowToggleNoteEditor();
			else if (foname=="hideInExport")		
				toggleHideExport();
			xelection.update();	
			return;	
		} 
	} 

	// No system flag clicked, take care of modmodes (CTRL-Click)
	if (e->state() & Qt::ControlModifier)
	{
		if (mainWindow->getModMode()==Main::ModModeColor)
		{
			pickingColor=true;
			setCursor (PickColorCursor);
			return;
		} 
		if (mainWindow->getModMode()==Main::ModModeCopy)
		{	
			if (lmo && (typeid(*lmo)==typeid(BranchObj) ||
				typeid(*lmo)==typeid(MapCenterObj) ))
			{

				selectInt (lmo);
				copy();
				if (lmo->getDepth()>0) select (lmo->getParObj() );
				paste();
				cout << "sel 1="<<getSelectedBranch()->getHeading().toStdString()<<endl;
				lmo=getSelectedBranch()->getLastBranch();
				cout << "sel 2="<<getSelectedBranch()->getHeading().toStdString()<<endl;
				setCursor (Qt::ArrowCursor);
			} else	
				return;
		} 
		if (mainWindow->getModMode()==Main::ModModeXLink)
		{	
			BranchObj *bo_begin=NULL;
			if (lmo)
				bo_begin=(BranchObj*)(lmo);
			else	
				if (xelection.getBranch() ) 
					bo_begin=xelection.getBranch();
			if (bo_begin)	
			{
				drawingLink=true;
				linkingObj_src=bo_begin;
				tmpXLink=new XLinkObj (mapScene);
				tmpXLink->setBegin (bo_begin);
				tmpXLink->setEnd   (p);
				tmpXLink->setColor(defXLinkColor);
				tmpXLink->setWidth(defXLinkWidth);
				tmpXLink->updateXLink();
				tmpXLink->setVisibility (true);
				return;
			} 
		}
	}	// End of modmodes

    if (lmo) 
	{	
		// Select the clicked object
		selectInt (lmo);

		// Left Button	    Move Branches
		if (e->button() == Qt::LeftButton )
		{
			//movingObj_start.setX( p.x() - selection->x() );// TODO replaced selection->lmo here	
			//movingObj_start.setY( p.y() - selection->y() );	
			movingObj_start.setX( p.x() - lmo->x() );	
			movingObj_start.setY( p.y() - lmo->y() );	
			movingObj_orgPos.setX (lmo->x() );
			movingObj_orgPos.setY (lmo->y() );
			movingObj_orgRelPos=lmo->getRelPos();

			// If modMode==copy, then we want to "move" the _new_ object around
			// then we need the offset from p to the _old_ selection, because of tmp
			/*
			if (mainWindow->getModMode()==Main::ModModeCopy &&
				e->state() & Qt::ControlModifier)
			{
				BranchObj *bo=xelection.getBranch();
				if (bo)
				{
					copyingObj=true;
					bo->addBranch ((BranchObj*)xelection.single());
					unselect();
					xelection.select(bo->getLastBranch());
					model->reposition();
				}
			} 
			*/

			movingObj=xelection.single();	
		} else
			// Middle Button    Toggle Scroll
			// (On Mac OS X this won't work, but we still have 
			// a button in the toolbar)
			if (e->button() == Qt::MidButton )
				toggleScroll();
		updateActions();
		xelection.update();
	} else 
	{ // No MapObj found, we are on the scene itself
		// Left Button	    move Pos of sceneView
		if (e->button() == Qt::LeftButton )
		{
			movingObj=NULL;	// move Content not Obj
			movingObj_start=e->globalPos();
			movingCont_start=QPointF (
				horizontalScrollBar()->value(),
				verticalScrollBar()->value());
			movingVec=QPointF(0,0);
			setCursor(HandOpenCursor);
		} 
    } 
}

void MapEditor::mouseMoveEvent(QMouseEvent* e)
{
    QPointF p = mapToScene(e->pos());
	LinkableMapObj *lmosel=xelection.single();

    // Move the selected MapObj
    if ( lmosel && movingObj) 
    {	
		// reset cursor if we are moving and don't copy
		if (mainWindow->getModMode()!=Main::ModModeCopy)
			setCursor (Qt::ArrowCursor);

		// To avoid jumping of the sceneView, only 
		// ensureSelectionVisible, if not tmp linked
		if (!lmosel->hasParObjTmp())
			ensureSelectionVisible ();
		
		// Now move the selection, but add relative position 
		// (movingObj_start) where selection was chosen with 
		// mousepointer. (This avoids flickering resp. jumping 
		// of selection back to absPos)
		
		// Check if we could link 
		LinkableMapObj* lmo=model->findMapObj(p, lmosel);
		

		FloatObj *fio=xelection.getFloatImage();
		if (fio)
		{
			fio->move   (p.x() -movingObj_start.x(), p.y()-movingObj_start.y() );		
			fio->setRelPos();
			fio->updateLink(); //no need for reposition, if we update link here
			xelection.update();

			// Relink float to new mapcenter or branch, if shift is pressed	
			// Only relink, if selection really has a new parent
			if ( (e->modifiers()==Qt::ShiftModifier) && lmo &&
				( (typeid(*lmo)==typeid(BranchObj)) ||
				  (typeid(*lmo)==typeid(MapCenterObj)) ) &&
				( lmo != fio->getParObj())  
				)
			{
				if (typeid(*fio) == typeid(FloatImageObj) && 
				( (typeid(*lmo)==typeid(BranchObj) ||
				  typeid(*lmo)==typeid(MapCenterObj)) ))  
				{

					// Also save the move which was done so far
					QString pold=qpointfToString(movingObj_orgRelPos);
					QString pnow=qpointfToString(fio->getRelPos());
					saveState(
						fio,
						"moveRel "+pold,
						fio,
						"moveRel "+pnow,
						QString("Move %1 to relative position %2").arg(getName(fio)).arg(pnow));
					fio->getParObj()->requestReposition();
					model->reposition();

					linkTo (model->getSelectString(lmo));
					//movingObj=lmosel;
					//movingObj_orgRelPos=lmosel->getRelPos();	

					model->reposition();
				}	
			}
		} else	
		{	// selection != a FloatObj
			if (lmosel->getDepth()==0)
			{
				// Move MapCenter
				if (e->buttons()== Qt::LeftButton && e->modifiers()==Qt::ShiftModifier) 
					((MapCenterObj*)lmosel)->moveAll(p.x() -movingObj_start.x(), p.y()-movingObj_start.y() );		
				else	
					lmosel->move   (p.x() -movingObj_start.x(), p.y()-movingObj_start.y() );		
				model->updateRelPositions();
			} else
			{	
				if (lmosel->getDepth()==1)
				{
					// Move mainbranch
					lmosel->move(p.x() -movingObj_start.x(), p.y()-movingObj_start.y() );		
					lmosel->setRelPos();
				} else
				{
					// Move ordinary branch
					lmosel->move(p.x() -movingObj_start.x(), p.y()-movingObj_start.y()  );		
				} 

				// Maybe we can relink temporary?
				if (lmo && (lmo!=lmosel) && xelection.getBranch() && 
					 (typeid(*lmo)==typeid(BranchObj) ||
					  typeid(*lmo)==typeid(MapCenterObj)) ) 

				{
					if (e->modifiers()==Qt::ControlModifier)
					{
						// Special case: CTRL to link below lmo
						lmosel->setParObjTmp (lmo,p,+1);
					}
					else if (e->modifiers()==Qt::ShiftModifier)
						lmosel->setParObjTmp (lmo,p,-1);
					else
						lmosel->setParObjTmp (lmo,p,0);
				} else	
				{
					lmosel->unsetParObjTmp();
				}		
				// reposition subbranch
				lmosel->reposition();	
			} // depth>0

			xelection.update();
		} // no FloatImageObj

		scene()->update();
		return;
	} // selection && moving_obj
		
	// Draw a link from one branch to another
	if (drawingLink)
	{
		 tmpXLink->setEnd (p);
		 tmpXLink->updateXLink();
	}	 
	
    // Move sceneView 
    if (!movingObj && !pickingColor &&!drawingLink && e->buttons() == Qt::LeftButton ) 
	{
		QPointF p=e->globalPos();
		movingVec.setX(-p.x() + movingObj_start.x() );
		movingVec.setY(-p.y() + movingObj_start.y() );
		horizontalScrollBar()->setSliderPosition((int)( movingCont_start.x()+movingVec.x() ));
		verticalScrollBar()->setSliderPosition((int)( movingCont_start.y()+movingVec.y() ) );
    }
}


void MapEditor::mouseReleaseEvent(QMouseEvent* e)
{
    QPointF p = mapToScene(e->pos());
	LinkableMapObj *dst;
	LinkableMapObj *lmosel=xelection.single();
	// Have we been picking color?
	if (pickingColor)
	{
		pickingColor=false;
		setCursor (Qt::ArrowCursor);
		// Check if we are over another branch
		dst=model->findMapObj(p, NULL);
		if (dst && lmosel) 
		{	
			if (e->state() & Qt::ShiftModifier)
				colorBranch (((BranchObj*)dst)->getColor());
			else	
				colorSubtree (((BranchObj*)dst)->getColor());
		} 
		return;
	}

	// Have we been drawing a link?
	if (drawingLink)	
	{
		drawingLink=false;
		// Check if we are over another branch
		dst=model->findMapObj(p, NULL);
		if (dst && lmosel) 
		{	
			tmpXLink->setEnd ( ((BranchObj*)(dst)) );
			tmpXLink->updateXLink();
			tmpXLink->activate(); //FIXME savestate missing
			//saveStateComplete(QString("Activate xLink from %1 to %2").arg(getName(tmpXLink->getBegin())).arg(getName(tmpXLink->getEnd())) );	
		} else
		{
			delete(tmpXLink);
			tmpXLink=NULL;
		}
		return;
	}
	
    // Have we been moving something?
    if ( lmosel && movingObj ) 
    {	
		FloatImageObj *fo=xelection.getFloatImage();
		if(fo)
		{
			// Moved FloatObj. Maybe we need to reposition
		    QString pold=qpointfToString(movingObj_orgRelPos);
		    QString pnow=qpointfToString(fo->getRelPos());
			saveState(
				fo,
				"moveRel "+pold,
				fo,
				"moveRel "+pnow,
				QString("Move %1 to relative position %2").arg(getName(fo)).arg(pnow));

			fo->getParObj()->requestReposition();
			model->reposition();
		}	

		// Check if we are over another branch, but ignore 
		// any found LMOs, which are FloatObjs
		dst=model->findMapObj(mapToScene(e->pos() ), lmosel);

		if (dst && (typeid(*dst)!=typeid(BranchObj) && typeid(*dst)!=typeid(MapCenterObj))) 
			dst=NULL;
		
		BranchObj *bo=xelection.getBranch();
		if (bo && bo->getDepth()==0)
		{	
            if (movingObj_orgPos != bo->getAbsPos())
            {
                QString pold=qpointfToString(movingObj_orgPos);
                QString pnow=qpointfToString(bo->getAbsPos());
                saveState(
                    fo,
                    "move "+pold,
                    fo,
                    "move "+pnow,
                    QString("Move mapcenter %1 to position %2").arg(getName(bo)).arg(pnow));
            }
		}
	
		if (xelection.type() == Selection::Branch )
		{	// A branch was moved
			
			// save the position in case we link to mapcenter
			QPointF savePos=QPointF (lmosel->getAbsPos()  );

			// Reset the temporary drawn link to the original one
			lmosel->unsetParObjTmp();

			// For Redo we may need to save original selection
			QString preSelStr=model->getSelectString(lmosel);

			copyingObj=false;	
			if (dst ) 
			{
				// We have a destination, relink to that

				BranchObj* bsel=xelection.getBranch();
				BranchObj* bdst=(BranchObj*)dst;

				QString preParStr=model->getSelectString (bsel->getParObj());
				QString preNum=QString::number (bsel->getNum(),10);
				QString preDstParStr;

				if (e->state() & Qt::ShiftModifier && dst->getParObj())
				{	// Link above dst
					preDstParStr=model->getSelectString (dst->getParObj());
					bsel->linkTo ( (BranchObj*)(bdst->getParObj()), bdst->getNum());
				} else 
				if (e->state() & Qt::ControlModifier && dst->getParObj())
				{
					// Link below dst
					preDstParStr=model->getSelectString (dst->getParObj());
					bsel->linkTo ( (BranchObj*)(bdst->getParObj()), bdst->getNum()+1);
				} else	
				{	// Append to dst
					preDstParStr=model->getSelectString(dst);
					bsel->linkTo (bdst,-1);
					if (dst->getDepth()==0) bsel->move (savePos);
				} 
				QString postSelStr=model->getSelectString(lmosel);
				QString postNum=QString::number (bsel->getNum(),10);

				QString undoCom="linkTo (\""+ 
					preParStr+ "\"," + preNum  +"," + 
					QString ("%1,%2").arg(movingObj_orgPos.x()).arg(movingObj_orgPos.y())+ ")";

				QString redoCom="linkTo (\""+ 
					preDstParStr + "\"," + postNum + "," +
					QString ("%1,%2").arg(savePos.x()).arg(savePos.y())+ ")";

				saveState (
					postSelStr,undoCom,
					preSelStr, redoCom,
					QString("Relink %1 to %2").arg(getName(bsel)).arg(getName(dst)) );

				model->reposition();	// not necessary if we undo temporary move  below
			} else
			{
				// No destination, undo  temporary move

				if (lmosel->getDepth()==1)
				{
					// The select string might be different _after_ moving around.
					// Therefor reposition and then use string of old selection, too
					model->reposition();

                    QPointF rp(lmosel->getRelPos());
                    if (rp != movingObj_orgRelPos)
                    {
                        QString ps=qpointfToString(rp);
                        saveState(
                            model->getSelectString(lmosel), "moveRel "+qpointfToString(movingObj_orgRelPos), 
                            preSelStr, "moveRel "+ps, 
                            QString("Move %1 to relative position %2").arg(getName(lmosel)).arg(ps));
                    }
				}

				// Draw the original link, before selection was moved around
				if (settings.value("/animation/use",false).toBool() && lmosel->getDepth()>1) 
				{
					QPointF p=bo->getParObj()->getChildPos();
					lmosel->setRelPos();	// calc relPos first
					model->startAnimation(
						lmosel->getRelPos(),
						QPointF (movingObj_orgPos.x() - p.x(), movingObj_orgPos.y() - p.y() )
					);	
				} else	
					model->reposition();
			}
		}
		 xelection.update();
		// Finally resize scene, if needed
		scene()->update();
		movingObj=NULL;		

		// Just make sure, that actions are still ok,e.g. the move branch up/down buttons...
		updateActions();
	} else 
		// maybe we moved View: set old cursor
		setCursor (Qt::ArrowCursor);
    
}

void MapEditor::mouseDoubleClickEvent(QMouseEvent* e)
{
	if (isSelectBlocked() ) 
	{
		e->ignore();
		return;
	}

	if (e->button() == Qt::LeftButton )
	{
		QPointF p = mapToScene(e->pos());
		LinkableMapObj *lmo=model->findMapObj(p, NULL);
		if (lmo) {	// MapObj was found
			// First select the MapObj than edit heading
			xelection.select(lmo);
			mainWindow->editHeading();
		}
	}
}

void MapEditor::resizeEvent (QResizeEvent* e)
{
	QGraphicsView::resizeEvent( e );
}

void MapEditor::dragEnterEvent(QDragEnterEvent *event)
{
	//for (unsigned int i=0;event->format(i);i++) // Debug mime type
	//	cerr << event->format(i) << endl;

	if (event->mimeData()->hasImage())
		event->acceptProposedAction();
	else	
		if (event->mimeData()->hasUrls())
			event->acceptProposedAction();
}

void MapEditor::dragMoveEvent(QDragMoveEvent *)
{
}

void MapEditor::dragLeaveEvent(QDragLeaveEvent *event)
{
	event->accept();
}

void MapEditor::dropEvent(QDropEvent *event)
{
	BranchObj *sel=xelection.getBranch();
	if (sel)
	{
		if (debug)
			foreach (QString format,event->mimeData()->formats()) 
				cout << "MapEditor: Dropped format: "<<qPrintable (format)<<endl;


		QList <QUrl> uris;
		if (event->mimeData()->hasImage()) 
		{
			 QVariant imageData = event->mimeData()->imageData();
			 addFloatImageInt (qvariant_cast<QPixmap>(imageData));
		} else
		if (event->mimeData()->hasUrls())
			uris=event->mimeData()->urls();

		if (uris.count()>0)
		{
			QStringList files;
			QString s;
			QString heading;
			BranchObj *bo;
			for (int i=0; i<uris.count();i++)
			{
				// Workaround to avoid adding empty branches
				if (!uris.at(i).toString().isEmpty())
				{
					bo=sel->addBranch();
					if (bo)
					{
						s=uris.at(i).toLocalFile();
						if (!s.isEmpty()) 
						{
						   QString file = QDir::fromNativeSeparators(s);
						   heading = QFileInfo(file).baseName();
						   files.append(file);
						   if (file.endsWith(".vym", false))
							   bo->setVymLink(file);
						   else
							   bo->setURL(uris.at(i).toString());
					   } else 
					   {
						   bo->setURL(uris.at(i).toString());
					   }

					   if (!heading.isEmpty())
						   bo->setHeading(heading);
					   else
						   bo->setHeading(uris.at(i).toString());
					}
				}
			}
			model->reposition();
		}
	}	
	event->acceptProposedAction();
}


void MapEditor::sendSelection()
{
	if (netstate!=Server) return;
	sendData (QString("select (\"%1\")").arg(xelection.getSelectString()) );
}

void MapEditor::newServer()
{
	port=54321;
	sendCounter=0;
    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any,port)) {
        QMessageBox::critical(this, "vym server",
                              QString("Unable to start the server: %1.").arg(tcpServer->errorString()));
        close();
        return;
    }
	connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newClient()));
	netstate=Server;
	cout<<"Server is running on port "<<tcpServer->serverPort()<<endl;
}

void MapEditor::connectToServer()
{
	port=54321;
	server="salam.suse.de";
	server="localhost";
	clientSocket = new QTcpSocket (this);
	clientSocket->abort();
    clientSocket->connectToHost(server ,port);
	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayNetworkError(QAbstractSocket::SocketError)));
	netstate=Client;		
	cout<<"connected to "<<qPrintable (server)<<" port "<<port<<endl;

	
}

void MapEditor::newClient()
{
    QTcpSocket *newClient = tcpServer->nextPendingConnection();
    connect(newClient, SIGNAL(disconnected()),
            newClient, SLOT(deleteLater()));

	cout <<"ME::newClient  at "<<qPrintable( newClient->peerAddress().toString() )<<endl;

	clientList.append (newClient);
}


void MapEditor::sendData(const QString &s)
{
	if (clientList.size()==0) return;

	// Create bytearray to send
	QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);

	// Reserve some space for blocksize
    out << (quint16)0;

	// Write sendCounter
    out << sendCounter++;

	// Write data
    out << s;

	// Go back and write blocksize so far
    out.device()->seek(0);
    quint16 bs=(quint16)(block.size() - 2*sizeof(quint16));
	out << bs;

	if (debug)
		cout << "ME::sendData  bs="<<bs<<"  counter="<<sendCounter<<"  s="<<qPrintable(s)<<endl;

	for (int i=0; i<clientList.size(); ++i)
	{
		//cout << "Sending \""<<qPrintable (s)<<"\" to "<<qPrintable (clientList.at(i)->peerAddress().toString())<<endl;
		clientList.at(i)->write (block);
	}
}

void MapEditor::readData ()
{
	while (clientSocket->bytesAvailable() >=(int)sizeof(quint16) )
	{
		if (debug)
			cout <<"readData  bytesAvail="<<clientSocket->bytesAvailable();
		quint16 recCounter;
		quint16 blockSize;

		QDataStream in(clientSocket);
		in.setVersion(QDataStream::Qt_4_0);

		in >> blockSize;
		in >> recCounter;
		
		QString t;
		in >>t;
		if (debug)
			cout << "  t="<<qPrintable (t)<<endl;
		parseAtom (t);
	}
	return;
}

void MapEditor::displayNetworkError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, vymName +" Network client",
                                 "The host was not found. Please check the "
                                    "host name and port settings.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, vymName + " Network client",
                                 "The connection was refused by the peer. "
                                    "Make sure the fortune server is running, "
                                    "and check that the host name and port "
                                    "settings are correct.");
        break;
    default:
        QMessageBox::information(this, vymName + " Network client",
                                 QString("The following error occurred: %1.")
                                 .arg(clientSocket->errorString()));
    }
}

void MapEditor::autosave()
{
	if (filePath=="")
	{
		if (debug) cout <<"  ME::autosave  rejected, no filepath available\n";
		return;
	}

	QDateTime now=QDateTime().currentDateTime();
	/* FIXME debug
	cout << "ME::autosave checking "<<qPrintable(filePath)<<"...\n"; 
	cout << "fsaved: "<<qPrintable (fileChangedTime.toString())<<endl;
	cout << "  fnow: "<<qPrintable (QFileInfo(filePath).lastModified().toString())<<endl;
	cout << "  time: "<<qPrintable (now.toString())<<endl;
	cout << " zipped="<<zipped<<endl;
	*/

	// Disable autosave, while we have gone back in history
	int redosAvail=undoSet.readNumEntry (QString("/history/redosAvail"));
	if (redosAvail>0) return;

	// Also disable autosave for new map without filename
	if (filePath.isEmpty()) return;


	if (mapUnsaved &&mapChanged && settings.value ("/mainwindow/autosave/use",true).toBool() )
	{
		if (QFileInfo(filePath).lastModified()<=fileChangedTime) 
			mainWindow->fileSave (this);
		else
			if (debug)
				cout <<"  ME::autosave  rejected, file on disk is newer than last save.\n"; 
	}	
}

void MapEditor::fileChanged()
{
	// Check if file on disk has changed meanwhile
	if (!filePath.isEmpty())
	{
		QDateTime tmod=QFileInfo (filePath).lastModified();
		if (tmod>fileChangedTime)
		{
			
			/* FIXME debug message, sometimes there's a glitch in the metrics...
			cout << "ME::fileChanged()\n" 
			     << "  last saved:     "<<qPrintable (fileChangedTime.toString())<<endl
				 << "  last modififed: "<<qPrintable (tmod.toString())<<endl;
			*/	 
			// FIXME switch to current mapeditor and finish lineedits...
			QMessageBox mb( vymName,
				tr("The file of the map  on disk has changed:\n\n"  
				   "   %1\n\nDo you want to reload that map with the new file?").arg(filePath),
				QMessageBox::Question,
				QMessageBox::Yes ,
				QMessageBox::Cancel | QMessageBox::Default,
				QMessageBox::NoButton );

			mb.setButtonText( QMessageBox::Yes, tr("Reload"));
			mb.setButtonText( QMessageBox::No, tr("Ignore"));
			switch( mb.exec() ) 
			{
				case QMessageBox::Yes:
					// Reload map
					load (filePath,NewMap,fileType);
		        case QMessageBox::Cancel:
					fileChangedTime=tmod; // allow autosave to overwrite newer file!
			}
		}
	}	

}


/*TODO not needed? void MapEditor::contentsDropEvent(QDropEvent *event) 
{

		} else if (event->provides("application/x-moz-file-promise-url") && 
			 event->provides("application/x-moz-nativeimage")) 
		{
			// Contains url to the img src in unicode16
			QByteArray d = event->encodedData("application/x-moz-file-promise-url");
			QString url = QString((const QChar*)d.data(),d.size()/2);
			fetchImage(url);
			event->accept();
			update=true;
		} else if (event->provides ("text/uri-list"))
		{	// Uris provided e.g. by konqueror
			Q3UriDrag::decode (event,uris);
		} else if (event->provides ("_NETSCAPE_URL"))
		{	// Uris provided by Mozilla
		  QStringList l = QStringList::split("\n", event->encodedData("_NETSCAPE_URL"));
		  uris.append(l[0]);
		  heading = l[1];
		} else if (event->provides("text/html")) {

		  // Handels text mime types
		  // Look like firefox allways handle text as unicode16 (2 bytes per char.)
		  QByteArray d = event->encodedData("text/html");
		  QString text;
		  if (isUnicode16(d)) 
		    text = QString((const QChar*)d.data(),d.size()/2);
		  else 
		    text = QString(d);

		  textEditor->setText(text);

		  event->accept();
		  update=true;
		} else if (event->provides("text/plain")) {
		  QByteArray d = event->encodedData("text/plain");
		  QString text;
		  if (isUnicode16(d))
		    text = QString((const QChar*)d.data(),d.size()/2);
		  else 
		    text = QString(d);

		  textEditor->setText(text);
		  
		  event->accept();
		  update= true;
		}

		*/



bool isUnicode16(const QByteArray &d) 
{
  // TODO: make more precise check for unicode 16.
  // Guess unicode16 if any of second bytes are zero
  unsigned int length = max(0,d.size()-2)/2;
  for (unsigned int i = 0; i<length ; i++)
    if (d.at(i*2+1)==0) return true;
  return false;
}
      
void MapEditor::addFloatImageInt (const QPixmap &img) 
{
	BranchObj *bo=xelection.getBranch();
	if (bo)
  {
	FloatImageObj *fio=bo->addFloatImage();
    fio->load(img);
    fio->setOriginalFilename("No original filename (image added by dropevent)");	
	QString s=model->getSelectString(bo);
	saveState (PartOfMap, s, "nop ()", s, "copy ()","Copy dropped image to clipboard",fio  );
	saveState (fio,"delete ()", bo,QString("paste(%1)").arg(curStep),"Pasting dropped image");
    model->reposition();
    scene()->update();
  }
}

/*

void MapEditor::imageDataFetched(const QByteArray &a, Q3NetworkOperation * / *nop* /) 
{
  if (!imageBuffer) imageBuffer = new QBuffer();
  if (!imageBuffer->isOpen()) {
    imageBuffer->open(QIODevice::WriteOnly | QIODevice::Append);
  }
  imageBuffer->at(imageBuffer->at()+imageBuffer->writeBlock(a));
}


void MapEditor::imageDataFinished(Q3NetworkOperation *nop) 
{
	if (nop->state()==Q3NetworkProtocol::StDone) {
		QPixmap img(imageBuffer->buffer());
		addFloatImageInt (img);
	}

	if (imageBuffer) {
		imageBuffer->close();
		if (imageBuffer) {
			imageBuffer->close();
			delete imageBuffer;
			imageBuffer = 0;
		}
	}
}

void MapEditor::fetchImage(const QString &url) 
{
  if (urlOperator) {
    urlOperator->stop();
    disconnect(urlOperator);
    delete urlOperator;
  }
  
  urlOperator = new Q3UrlOperator(url);
  connect(urlOperator, SIGNAL(finished(Q3NetworkOperation *)), 
	  this, SLOT(imageDataFinished(Q3NetworkOperation*)));

  connect(urlOperator, SIGNAL(data(const QByteArray &, Q3NetworkOperation *)),
	  this, SLOT(imageDataFetched(const QByteArray &, Q3NetworkOperation *)));
  urlOperator->get();
}
*/

