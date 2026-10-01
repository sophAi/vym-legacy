#ifndef ORNAMENTEDOBJ_H
#define ORNAMENTEDOBJ_H

#include "attribute.h"
#include "frameobj.h"
#include "linkablemapobj.h"

/*! \brief Adds various ornaments and data to the class LinkableMapObj

The ornaments are:
	- frame
	- note
	- references
		- URL
		-vymLink
	- flags
		- standard flags
		- system flags
	- attributes (key/value pairs)	
 */

class OrnamentedObj:public LinkableMapObj {
public:	
    OrnamentedObj ();
    OrnamentedObj (QGraphicsScene*);
    OrnamentedObj (OrnamentedObj*);
    ~OrnamentedObj ();
    virtual void init ();
    virtual void copy (OrnamentedObj*);

    virtual QString getHeading();			//!< returns the heading	
	virtual void setLinkColor();			// sets color according to colorhint, overloaded
	virtual void setColor(QColor);			// set the color of text and link
	QColor getColor ();						// get color of heading

	virtual FrameObj::FrameType getFrameType ();
	virtual QString getFrameTypeName ();
	virtual void setFrameType (const FrameObj::FrameType &);
	virtual void setFrameType (const QString &);
	virtual void setFramePadding (const int &);
	virtual int  getFramePadding ();
	virtual void setFrameBorderWidth(const int &);
	virtual int  getFrameBorderWidth ();
	virtual void setFramePenColor (QColor);
	virtual QColor getFramePenColor ();
	virtual void setFrameBrushColor (QColor);
	virtual QColor getFrameBrushColor ();

    virtual void positionContents();
    virtual void move   (double,double);
    virtual void move   (QPointF);
    virtual void moveBy (double,double);
    virtual void moveBy (QPointF);
    virtual void move2RelPos (QPointF);		// move relativly to parent^
    virtual void move2RelPos (double,double);
    virtual void setNote(QString);			// set note
    virtual void setNote(NoteObj);			// set note
    virtual QString getNote();				// returns note	(HTML)
    virtual QString getNoteASCII(const QString &indent, const int &width); // returns note	(ASCII)
    virtual QString getNoteASCII();			// returns note	(ASCII)
    virtual QString getNoteOpenDoc();		// returns note	(OpenDoc)

    virtual void setURL (QString);
    virtual QString getURL ();

    virtual void setVymLink (QString);
    virtual QString getVymLink ();

	virtual void setAttributes (const QList <Attribute> &al);
	virtual QList <Attribute> getAttributes ();

	virtual void clearStandardFlags();
	virtual void toggleStandardFlag(QString, bool exclusive=false);
	virtual void activateStandardFlag(QString);
	virtual void deactivateStandardFlag(QString);
	virtual bool isSetStandardFlag(QString);
	virtual QString getSystemFlagName (const QPointF &p);
	virtual bool isActiveFlag(const QString&);	// check if flag is set
	virtual void updateNoteFlag();
	virtual void getNoteFromTextEditor ();
	virtual void updateFlagsToolbar();
	virtual void setHideInExport(bool);		// set export of object (and childs)
	virtual bool hideInExport();
	virtual bool isHidden ();
	virtual QString getOrnXMLAttr();		// get attributes for saveToDir

protected:
    HeadingObj *heading;			// Heading
	NoteObj note;					// Notes
	bool isNoteInEditor;			// true if TextEditor has this note
	FlagRowObj *systemFlags;		// System Flags
	FlagRowObj *standardFlags;		// Standard Flags
	FrameObj *frame;				// frame around object
	QRectF ornamentsBBox;			// bbox of flags and heading
	QString url;					// url to external doc
	QString vymLink;				// path to another map
	QList <Attribute> attributes;	// List with attributes

	bool hideExport;				// hide in exports if set
	bool hidden;					// true if temporary hidden
};

#endif
