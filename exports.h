#ifndef EXPORTS_H
#define EXPORTS_H

#include <qdir.h>
#include <qstring.h>
#include <iostream>

#include "mapcenterobj.h"
#include "settings.h"
#include "vymmodel.h"


/*! \brief Base class for all exports
*/

///////////////////////////////////////////////////////////////////////

class ExportBase
{
public:
	ExportBase();
	virtual ~ExportBase();
	virtual void setDir(const QDir&);
	virtual void setFile(const QString &);
	virtual QString getFile ();
	virtual void setModel (VymModel *m);
	virtual void setCaption(const QString &);
	virtual void addFilter (const QString &);
	virtual bool execDialog();
	virtual bool canceled();
protected:  
	VymModel *model;
	virtual QString getSectionString (BranchObj*);

	QDir tmpDir;
	QDir outDir;
	QString outputFile;
	QString	indentPerDepth;
	QString caption;
	QString filter;
	bool cancelFlag;
};

///////////////////////////////////////////////////////////////////////
class ExportASCII:public ExportBase
{
public:
	ExportASCII();
	virtual void doExport();
	virtual QString underline (const QString &text, const QString &line);
};

///////////////////////////////////////////////////////////////////////
class ExportCSV:public ExportBase
{
public:
	virtual void doExport();
};

///////////////////////////////////////////////////////////////////////
class ExportXMLBase:public ExportBase
{
};

///////////////////////////////////////////////////////////////////////
class ExportKDE3Bookmarks:public ExportXMLBase
{
public:
	virtual void doExport();
};	

///////////////////////////////////////////////////////////////////////
class ExportKDE4Bookmarks:public ExportXMLBase
{
public:
	virtual void doExport();
};	

///////////////////////////////////////////////////////////////////////
class ExportFirefoxBookmarks:public ExportXMLBase
{
public:
	virtual void doExport();
};	

///////////////////////////////////////////////////////////////////////
class ExportTaskjuggler:public ExportXMLBase
{
public:
	virtual void doExport();
};	

///////////////////////////////////////////////////////////////////////
class ExportLaTeX:public ExportBase
{
public:
	virtual void doExport();
};	

///////////////////////////////////////////////////////////////////////
class ExportOO:public ExportBase
{
public:
	ExportOO();
	~ExportOO();
	void exportPresentation();
	bool setConfigFile (const QString &);
private:
	QString buildList (BranchObj*);
	bool useSections;
	QString configFile;
	QString configDir;
	QString templateDir;
	QString content;
	QString contentTemplate;
	QString contentTemplateFile;
	QString contentFile;
	QString pageTemplate;
	QString pageTemplateFile;
	QString sectionTemplate;
	QString sectionTemplateFile;
};
#endif
