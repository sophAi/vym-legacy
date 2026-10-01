#ifndef NOTEOBJ_H
#define NOTEOBJ_H

#include <qstring.h>

class NoteObj;

#include "xmlobj.h"

/*! \brief The text note belonging to one OrnamentedObj */


class NoteObj:public XMLObj
{
public:
	NoteObj();
	NoteObj(const QString&);
	void copy (NoteObj);
	void clear();
	void setNote (const QString&);
	QString getNote();
	QString getNoteASCII();
	QString getNoteASCII(const QString &indent, const int &width);
	QString getNoteOpenDoc();
	void setFontHint (const QString&);
	QString getFontHint ();
	void setFilenameHint (const QString&);
	QString getFilenameHint ();
	bool isEmpty();
	QString	saveToDir();

private:
	QString note;
	QString fonthint;
	QString filenamehint;
};
#endif
