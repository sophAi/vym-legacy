#include "xml-vym.h"

#include <QMessageBox>
#include <QColor>
#include <QTextStream>
#include <iostream>
#include <typeinfo>

#include "misc.h"
#include "settings.h"
#include "linkablemapobj.h"
#include "version.h"

static BranchObj *lastBranch;
static FloatObj *lastFloat;
static OrnamentedObj *lastOO;

extern Settings settings;
extern QString vymVersion;

/*
parseVYMHandler::parseVYMHandler() {}

parseVYMHandler::~parseVYMHandler() {}

QString parseVYMHandler::errorProtocol() { return errorProt; }

*/

bool parseVYMHandler::startDocument()
{
    errorProt = "";
    state = StateInit;
    laststate = StateInit;
	stateStack.clear();
	stateStack.append(StateInit);
	htmldata="";
	isVymPart=false;
    return true;
}


/*
QString parseVYMHandler::parseHREF(QString href)
{
	QString type=href.section(":",0,0);
	QString path=href.section(":",1,1);
	if (!tmpDir.endsWith("/"))
		return tmpDir + "/" + path;
	else	
		return tmpDir + path;
}
*/
bool parseVYMHandler::startElement  ( const QString&, const QString&,
                    const QString& eName, const QXmlAttributes& atts ) 
{
    QColor col;
	/* Testing
	cout << "startElement <"<< qPrintable(eName)
		<<">  state="<<state 
		<<"  laststate="<<stateStack.last()
		<<"   loadMode="<<loadMode
		<<"       line="<<QXmlDefaultHandler::lineNumber()
		<<endl;
	*/	
	stateStack.append (state);	
    if ( state == StateInit && (eName == "vymmap")  ) 
	{
        state = StateMap;

		if (loadMode==NewMap)
			model->clear();	// remove existing mapCenter

		// Check version
		if (!atts.value( "version").isEmpty() ) 
		{
			if (!checkVersion(atts.value("version")))
				QMessageBox::warning( 0, "Warning: Version Problem" ,
				   "<h3>Map is newer than VYM</h3>"
				   "<p>The map you are just trying to load was "
				   "saved using vym " +atts.value("version")+". "
				   "The version of this vym is " + vymVersion + 
				   ". If you run into problems after pressing "
				   "the ok-button below, updating vym should help.");
			else	   
				model->setVersion(atts.value( "version" ));

		}

		if (loadMode==NewMap )
		{
			// Create mapCenter
			model->clear();
			lastBranch=model->first();	// avoid empty pointer

			if (!atts.value( "author").isEmpty() )
				model->setAuthor(atts.value( "author" ) );
			if (!atts.value( "comment").isEmpty() )
				model->setComment (atts.value( "comment" ) );
			if (!atts.value( "backgroundColor").isEmpty() )
			{
				col.setNamedColor(atts.value("backgroundColor"));
				model->getScene()->setBackgroundBrush(col);
			}	    
			if (!atts.value( "selectionColor").isEmpty() )
			{
				col.setNamedColor(atts.value("selectionColor"));
				model->getMapEditor()->setSelectionColor(col);
			}	    
			if (!atts.value( "linkColorHint").isEmpty() ) 
			{
				if (atts.value("linkColorHint")=="HeadingColor")
					model->getMapEditor()->setMapLinkColorHint(LinkableMapObj::HeadingColor);
				else
					model->getMapEditor()->setMapLinkColorHint(LinkableMapObj::DefaultColor);
			}
			if (!atts.value( "linkStyle").isEmpty() ) 
				model->getMapEditor()->setMapLinkStyle(atts.value("linkStyle"));
			if (!atts.value( "linkColor").isEmpty() ) 
			{
				col.setNamedColor(atts.value("linkColor"));
				model->getMapEditor()->setMapDefLinkColor(col);
			}	
			if (!atts.value( "defXLinkColor").isEmpty() ) 
			{
				col.setNamedColor(atts.value("defXLinkColor"));
				model->getMapEditor()->setMapDefXLinkColor(col);
			}	
			if (!atts.value( "defXLinkWidth").isEmpty() ) 
				model->getMapEditor()->setMapDefXLinkWidth(atts.value("defXLinkWidth").toInt ());
		}	
	} else if ( eName == "select" && state == StateMap ) 
	{
		state=StateMapSelect;
	} else if ( eName == "setting" && state == StateMap ) 
	{
		state=StateMapSetting;
		if (loadMode==NewMap)
			readSettingAttr (atts);
	} else if ( eName == "mapcenter" && state == StateMap ) 
	{
		state=StateMapCenter;
		if (loadMode==NewMap)
		{	
			// Really use the found mapcenter as MCO in a new map

			lastBranch=model->addMapCenter(); 
		} else
		{
			// Treat the found mapcenter as a branch 
			// in an existing map
			LinkableMapObj* lmo=model->getSelection();
			if (lmo && ((typeid(*lmo) == typeid(BranchObj) ) 
			        ||  typeid(*lmo) == typeid(MapCenterObj) ) )
			{
				lastBranch=(BranchObj*)lmo;
				if (loadMode==ImportAdd)
				{
					lastBranch->addBranch();
					lastBranch=lastBranch->getLastBranch();
				} else
					lastBranch->clear();
			} else
				return false;
		}
		readBranchAttr (atts);
	} else if ( 
		(eName == "standardflag" ||eName == "standardFlag") && 
		(state == StateMapCenter || state==StateBranch)) 
	{
		state=StateStandardFlag;
	} else if ( eName == "heading" && (state == StateMapCenter||state==StateBranch)) 
	{
		laststate=state;
		state=StateHeading;
		if (!atts.value( "textColor").isEmpty() ) 
		{
			col.setNamedColor(atts.value("textColor"));
			lastBranch->setColor(col );
		}	    
	} else if ( eName == "note" && 
				(state == StateMapCenter ||state==StateBranch))
	{	// only for backward compatibility (<1.4.6). Use htmlnote now.
		state=StateNote;
		if (!readNoteAttr (atts) ) return false;
	} else if ( eName == "htmlnote" && state == StateMapCenter) 
	{
		laststate=state;
		state=StateHtmlNote;
    } else if ( eName == "floatimage" && 
				(state == StateMapCenter ||state==StateBranch)) 
	{
		state=StateFloatImage;
        lastBranch->addFloatImage();
		lastFloat=lastBranch->getLastFloatImage();
		if (!readFloatImageAttr(atts)) return false;
	} else if ( (eName == "branch"||eName=="floatimage") && state == StateMap) 
	{
		// This is used in vymparts, which have no mapcenter!
		isVymPart=true;
		LinkableMapObj* lmo=model->getSelection();
		if (!lmo)
		{
			// If a vym part is _loaded_ (not imported), 
			// selection==lmo==NULL
			// Treat it like ImportAdd then...
			loadMode=ImportAdd;
			lmo=model->first();		// FIXME this used to be lmo=mc before
		}	
		if (lmo && ((typeid(*lmo) == typeid(BranchObj) ) 
				||  typeid(*lmo) == typeid(MapCenterObj) ) )
		{
			lastBranch=(BranchObj*)(lmo);
			if (eName=="branch")
			{
				state=StateBranch;
				if (loadMode==ImportAdd)
				{
					lastBranch->addBranch();
					lastBranch=lastBranch->getLastBranch();
					
				} else
					lastBranch->clear();
				readBranchAttr (atts);
			} else if (eName=="floatimage")
			{
				state=StateFloatImage;
				lastBranch->addFloatImage();
				lastFloat=lastBranch->getLastFloatImage();
				if (!readFloatImageAttr(atts)) return false;
			} else return false;
		} else return false;
	} else if ( eName == "branch" && state == StateMapCenter) 
	{
		state=StateBranch;
		lastBranch->addBranch();
		lastBranch=lastBranch->getLastBranch();
		readBranchAttr (atts);
	} else if ( eName == "htmlnote" && state == StateBranch) 
	{
		laststate=state;
		state=StateHtmlNote;
		no.clear();
		if (!atts.value( "fonthint").isEmpty() ) 
			no.setFontHint(atts.value ("fonthint") );
	} else if ( eName == "frame" && (state == StateBranch||state==StateMapCenter)) 
	{
		laststate=state;
		state=StateFrame;
		if (!readFrameAttr(atts)) return false;
    } else if ( eName == "xlink" && state == StateBranch ) 
	{
		state=StateBranchXLink;
		if (!readXLinkAttr (atts)) return false;
    } else if ( eName == "branch" && state == StateBranch ) 
	{
        lastBranch->addBranch();
		lastBranch=lastBranch->getLastBranch();		
		readBranchAttr (atts);
    } else if ( eName == "html" && state == StateHtmlNote ) 
	{
		state=StateHtml;
		htmldata="<"+eName;
		readHtmlAttr(atts);
		htmldata+=">";
    } else if ( state == StateHtml ) 
	{
		// accept all while in html mode,
		htmldata+="<"+eName;
		readHtmlAttr(atts);
		htmldata+=">";
    } else
        return false;   // Error
    return true;
}

bool parseVYMHandler::endElement  ( const QString&, const QString&, const QString &eName)
{
	/* Testing
	cout << "endElement </" <<qPrintable(eName)
		<<">  state=" <<state 
		<<"  laststate=" <<laststate
		<<"  stateStack="<<stateStack.last() 
		<<endl;
	*/
    switch ( state ) 
	{
        case StateBranch: 
			lastBranch=(BranchObj*)(lastBranch->getParObj());
            break;
        case StateHtml: 
			htmldata+="</"+eName+">";
			if (eName=="html")
			{
				state=StateHtmlNote;  
				htmldata.replace ("<br></br>","<br />");
				no.setNote (htmldata);
				lastBranch->setNote (no);
			}	
			break;
		default: 
			break;
    }  
	state=stateStack.takeLast();	
	return true;
}

bool parseVYMHandler::characters   ( const QString& ch)
{
	//cout << "characters \""<<ch<<"\"  state="<<state <<"  laststate="<<laststate<<endl;

	QString ch_org=quotemeta (ch);
    QString ch_simplified=ch.simplifyWhiteSpace();
    if ( ch_simplified.isEmpty() ) return true;

    switch ( state ) 
    {
        case StateInit: break;
        case StateMap: break; 
		case StateMapSelect:
			model->select(ch_simplified);
			break;
		case StateMapSetting:break;
        case StateMapCenter: break;
        case StateNote:
			lastBranch->setNote(ch_simplified);
			break;
        case StateBranch: break;
        case StateStandardFlag: 
            lastBranch->activateStandardFlag(ch_simplified); 
            break;
        case StateFloatImage: break;
        case StateHtmlNote: break;
        case StateHtml:
			htmldata+=ch_org;
			break;
        case StateHeading: 
            lastBranch->setHeading(ch_simplified);
            break;
        default: 
			return false;
    }
    return true;
}

QString parseVYMHandler::errorString() 
{
    return "the document is not in the VYM file format";
}

bool parseVYMHandler::readBranchAttr (const QXmlAttributes& a)
{
	lastOO=lastBranch;
	if (!readOOAttr(a)) return false;

	if (!a.value( "scrolled").isEmpty() )
		lastBranch->toggleScroll();
	if (!a.value( "frameType").isEmpty() ) 
		lastOO->setFrameType (a.value("frameType")); //Compatibility 1.8.1

	if (!a.value( "incImgV").isEmpty() ) 
	{	
		if (a.value("incImgV")=="true")
			lastBranch->setIncludeImagesVer(true);
		else	
			lastBranch->setIncludeImagesVer(false);
	}	
	if (!a.value( "incImgH").isEmpty() ) 
	{	
		if (a.value("incImgH")=="true")
			lastBranch->setIncludeImagesHor(true);
		else	
			lastBranch->setIncludeImagesHor(false);
	}	
	return true;	
}

bool parseVYMHandler::readFrameAttr (const QXmlAttributes& a)
{
	bool ok;
	int x;
	if (lastOO)
	{
		if (!a.value( "frameType").isEmpty() ) 
			lastOO->setFrameType (a.value("frameType"));
		if (!a.value( "penColor").isEmpty() ) 
			lastOO->setFramePenColor (a.value("penColor"));
		if (!a.value( "brushColor").isEmpty() ) 
			lastOO->setFrameBrushColor (a.value("brushColor"));
		if (!a.value( "padding").isEmpty() ) 
		{
			x=a.value("padding").toInt(&ok);
			if (ok) lastOO->setFramePadding(x);
		}	
		if (!a.value( "borderWidth").isEmpty() ) 
		{
			x=a.value("borderWidth").toInt(&ok);
			if (ok) lastOO->setFrameBorderWidth(x);
		}	
	}		
	return true;
}

bool parseVYMHandler::readOOAttr (const QXmlAttributes& a)
{
	if (lastOO)
	{
		bool okx,oky;
		float x,y;
		if (!a.value( "relPosX").isEmpty() ) 
		{
			if (!a.value( "relPosY").isEmpty() ) 
			{
				x=a.value("relPosX").toFloat (&okx);
				y=a.value("relPosY").toFloat (&oky);
				if (okx && oky  )
				{
					lastOO->setUseRelPos (true);
					lastOO->move2RelPos (x,y);
				}	
				else
					return false;   // Couldn't read relPos
			}           
		}           
		if (!a.value( "absPosX").isEmpty() && loadMode==NewMap ) 
		{
			if (!a.value( "absPosY").isEmpty() ) 
			{
				x=a.value("absPosX").toFloat (&okx);
				y=a.value("absPosY").toFloat (&oky);
				if (okx && oky  )
					lastOO->move(x,y);
				else
					return false;   // Couldn't read absPos
			}           
		}           
		if (!a.value( "id").isEmpty() ) 
			lastOO->setID (a.value ("id"));
		if (!a.value( "url").isEmpty() ) 
			lastOO->setURL (a.value ("url"));
		if (!a.value( "vymLink").isEmpty() ) 
			lastOO->setVymLink (a.value ("vymLink"));
		if (!a.value( "hideInExport").isEmpty() ) 
			if (a.value("hideInExport")=="true")
				lastOO->setHideInExport(true);

		if (!a.value( "hideLink").isEmpty()) 
		{
			if (a.value ("hideLink") =="true")
				lastOO->setHideLinkUnselected(true);
			else	
				lastOO->setHideLinkUnselected(false);
		}	
	}
	return true;	
}

bool parseVYMHandler::readNoteAttr (const QXmlAttributes& a)
{	// only for backward compatibility (<1.4.6). Use htmlnote now.
	no.clear();
	QString fn;
	if (!a.value( "href").isEmpty() ) 
	{
		// Load note
		fn=parseHREF(a.value ("href") );
		QFile file (fn);
		QString s;						// Reading a note

		if ( !file.open( QIODevice::ReadOnly) )
		{
			qWarning ("parseVYMHandler::readNoteAttr:  Couldn't load "+fn);
			return false;
		}	
		QTextStream stream( &file );
		QString lines;
		while ( !stream.atEnd() ) {
			lines += stream.readLine()+"\n"; 
		}
		file.close();

		lines ="<html><head><meta name=\"qrichtext\" content=\"1\" /></head><body>"+lines + "</p></body></html>";
		no.setNote (lines);
	}		
	if (!a.value( "fonthint").isEmpty() ) 
		no.setFontHint(a.value ("fonthint") );
	lastBranch->setNote(no);
	return true;
}

bool parseVYMHandler::readFloatImageAttr (const QXmlAttributes& a)
{
	lastOO=lastFloat;
	
	//if (!readOOAttr(a)) return false;

	if (!a.value( "useOrientation").isEmpty() ) 
	{
		if (a.value ("useOrientation") =="true")
			lastFloat->setUseOrientation (true);
		else	
			lastFloat->setUseOrientation (false);
	}	
	if (!a.value( "href").isEmpty() )
	{
		// Load FloatImage
		if (!lastFloat->load (parseHREF(a.value ("href") ) ))
		{
			QMessageBox::warning( 0, "Warning: " ,
				"Couldn't load float image\n"+parseHREF(a.value ("href") ));
			lastBranch->removeFloatImage(((FloatImageObj*)(lastFloat)));
			lastFloat=NULL;
			return true;
		}
		
	}	
	if (!a.value( "floatExport").isEmpty() ) 
	{
		// Only for compatibility. THis is not used since 1.7.11 
		if (a.value ("floatExport") =="true")
			lastFloat->setFloatExport(true);
		else	
			lastFloat->setFloatExport (false);
	}	
	if (!a.value( "zPlane").isEmpty() ) 
		lastFloat->setZValue (a.value("zPlane").toInt ());
    float x,y;
    bool okx,oky;
	if (!a.value( "relPosX").isEmpty() ) 
	{
		if (!a.value( "relPosY").isEmpty() ) 
		{
			// read relPos
			x=a.value("relPosX").toFloat (&okx);
			y=a.value("relPosY").toFloat (&oky);
			if (okx && oky) 
				
				{
					lastFloat->setRelPos (QPointF (x,y) );
					// make sure floats in mapcenter are repositioned to relative pos
					if (lastBranch->getDepth()==0) lastBranch->positionContents();
				}
			else
				// Couldn't read relPos
				return false;  
		}           
	}	
	
	if (!readOOAttr(a)) return false;

	if (!a.value ("orgName").isEmpty() )
	{
		((FloatImageObj*)(lastFloat))->setOriginalFilename (a.value("orgName"));
	}
	return true;
}

bool parseVYMHandler::readXLinkAttr (const QXmlAttributes& a)
{
	QColor col;
	bool okx;
	bool success=false;
	XLinkObj *xlo=new XLinkObj (model->getScene());
	if (!a.value( "color").isEmpty() ) 
	{
		col.setNamedColor(a.value("color"));
		xlo->setColor (col);
	}

	if (!a.value( "width").isEmpty() ) 
	{
		xlo->setWidth(a.value ("width").toInt (&okx, 10));
	}

	// Connecting by select string for compatibility with version < 1.8.76
	if (!a.value( "beginBranch").isEmpty() ) 
	{ 
		if (!a.value( "endBranch").isEmpty() ) 
		{
			LinkableMapObj *lmo=model->findObjBySelect (a.value( "beginBranch"));
			if (lmo && typeid (*lmo)==typeid (BranchObj))
			{
				xlo->setBegin ((BranchObj*)lmo);
				lmo=model->findObjBySelect (a.value( "endBranch"));
				if (lmo && typeid (*lmo)==typeid (BranchObj))
				{
					xlo->setEnd ((BranchObj*)(lmo));
					xlo->activate();
					success=true;
				}
			}
		}           
	}	

	// object ID is used starting in version 1.8.76
	if (!a.value( "beginID").isEmpty() ) 
	{ 
		if (!a.value( "endID").isEmpty() ) 
		{
			LinkableMapObj *lmo=model->findID (a.value( "beginID"));
			if (lmo && typeid (*lmo)==typeid (BranchObj))
			{
				xlo->setBegin ((BranchObj*)lmo);
				lmo=model->findID (a.value( "endID"));
				if (lmo && typeid (*lmo)==typeid (BranchObj))
				{
					xlo->setEnd ((BranchObj*)(lmo));
					xlo->activate();
					success=true;
				}
			}
		}           
	}	
	if (!success) delete (xlo);
	return true;	// xLinks can only be established at the "end branch", return true
}

bool parseVYMHandler::readHtmlAttr (const QXmlAttributes& a)
{
	for (int i=1; i<=a.count(); i++)
		htmldata+=" "+a.localName(i-1)+"=\""+a.value(i-1)+"\"";
	return true;
}

bool parseVYMHandler::readSettingAttr (const QXmlAttributes& a)
{
	if (!a.value( "key").isEmpty() ) 
	{
		if (!a.value( "value").isEmpty() ) 
			settings.setLocalEntry (model->getMapEditor()->getDestPath(), a.value ("key"), a.value ("value"));
		else
			return false;
		
	} else
		return false;
	
	return true;
}
