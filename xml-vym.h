#ifndef XML_H
#define XML_H

#include "xml-base.h"


/*! \brief Parsing VYM maps from XML documents */

class parseVYMHandler  : public parseBaseHandler
{
public:
	bool startDocument();
	bool startElement ( const QString&, const QString&,
                        const QString& eName, const QXmlAttributes& atts ); 
	bool   endElement ( const QString&, const QString&, const QString& ); 
	bool characters   ( const QString&);
	QString errorString();
	bool readBranchAttr (const QXmlAttributes&);
	bool readFrameAttr (const QXmlAttributes&);
	bool readOOAttr (const QXmlAttributes&);
	bool readNoteAttr (const QXmlAttributes&);
	bool readFloatImageAttr (const QXmlAttributes&);
	bool readXLinkAttr (const QXmlAttributes&);
	bool readHtmlAttr (const QXmlAttributes&);
	bool readSettingAttr (const QXmlAttributes&);

private:
	enum State 
	{ 
		StateInit, 
		StateMap, 
		StateMapSelect, 
		StateMapSetting,
		StateMapCenter, 
		StateBranch, 
		StateBranchXLink,			//
		StateHtmlNote, 
		StateHtml, 
		StateFrame,
		StateStandardFlag,
		StateNote,
		StateFloatImage,
		StateHeading
	 };
				 


	State state;			 
	State laststate;
	QList <State> stateStack;
	QString htmldata;
	NoteObj no;
}; 
#endif
