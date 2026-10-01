#ifndef EXPORTXHTMLDIALOG_H
#define EXPORTXHTMLDIALOG_H

#include "ui_exportxhtmldialog.h"

/*! \brief Dialog to export a map as XHTML document

This is an overloaded QDialog with various settings needed to call
convert the vym.xml to a XHTML document. 
The conversion itself is done by calling xsltproc.
*/

class ExportXHTMLDialog:public QDialog
{
	Q_OBJECT
public:
    ExportXHTMLDialog(QWidget* parent = 0);

    virtual QString getDir();
    virtual bool warnings();
    virtual bool hasChanged();

public slots:
    virtual void readSettings();
    virtual void setDir (const QString&);
    virtual void dirChanged();
    virtual void browseDirectoryPressed();
    virtual void imageButtonPressed( bool b );
    virtual void textcolorButtonPressed( bool b );
    virtual void saveSettingsInMapButtonPressed( bool b );
    virtual void warningsButtonPressed( bool b );
    virtual void outputButtonPressed( bool b );
    virtual void cssChanged();
    virtual void browseCSSPressed();
    virtual void xslChanged();
    virtual void prescriptChanged();
    virtual void browseXSLPressed();
    virtual void postscriptChanged();
    virtual void browsePreExportButtonPressed();
    virtual void browsePostExportButtonPressed();
    virtual void doExport( const QString & mapname );
    virtual void setFilePath( const QString & s );
    virtual void setMapName( const QString & s );

protected:
    bool useTextColor;
    bool showWarnings;
    QString xsl;
    QString css;
    bool useImage;
    bool showOutput;
    QString dir;
    QString filepath;
    QString prescript;
    QString postscript;
    bool settingsChanged;
    QString mapname;
    bool saveSettingsInMap;
    XSLTProc p;
    Process *scriptProc;

private:
	Ui::ExportXHTMLDialog ui;
    void init();
    void destroy();
    void runScript( QString spath, QString fpath );

};

#endif // EXPORTXHTMLDIALOG_H
