#include "exportxhtmldialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "options.h"
#include "settings.h"


extern Options options;
extern QDir vymBaseDir;
extern Settings settings;
extern bool debug;

ExportXHTMLDialog::ExportXHTMLDialog(QWidget* parent) : QDialog(parent)
{
    ui.setupUi(this);

	filepath="";
	settingsChanged=false;
	scriptProc=new Process;

    // signals and slots connections
    connect(ui.browseExportDirButton, SIGNAL(pressed()), this, SLOT(browseDirectoryPressed()));
    connect(ui.outputButton, SIGNAL(toggled(bool)), this, SLOT(outputButtonPressed(bool)));
    connect(ui.browseXSLButton, SIGNAL(pressed()), this, SLOT(browseXSLPressed()));
    connect(ui.browseCSSButton, SIGNAL(pressed()), this, SLOT(browseCSSPressed()));
    connect(ui.imageButton, SIGNAL(toggled(bool)), this, SLOT(imageButtonPressed(bool)));
    connect(ui.textColorButton, SIGNAL(toggled(bool)), this, SLOT(textcolorButtonPressed(bool)));
    connect(ui.lineEditDir, SIGNAL(textChanged(const QString&)), this, SLOT(dirChanged()));
    connect(ui.lineEditCSS, SIGNAL(textChanged(const QString&)), this, SLOT(cssChanged()));
    connect(ui.lineEditXSL, SIGNAL(textChanged(const QString&)), this, SLOT(xslChanged()));
    connect(ui.warningsButton, SIGNAL(toggled(bool)), this, SLOT(warningsButtonPressed(bool)));
    connect(ui.saveSettingsInMapButton, SIGNAL(toggled(bool)), this, SLOT(saveSettingsInMapButtonPressed(bool)));
    connect(ui.browsePreExportButton, SIGNAL(pressed()), this, SLOT(browsePreExportButtonPressed()));
    connect(ui.lineEditPreScript, SIGNAL(textChanged(const QString&)), this, SLOT(prescriptChanged()));
    connect(ui.lineEditPostScript, SIGNAL(textChanged(const QString&)), this, SLOT(postscriptChanged()));
    connect(ui.browsePostExportButton, SIGNAL(pressed()), this, SLOT(browsePostExportButtonPressed()));
}	


void ExportXHTMLDialog::readSettings()
{

	dir=settings.readLocalEntry (filepath,"/export/xhtml/exportDir",vymBaseDir.currentDirPath() );
	ui.lineEditDir->setText(dir);
	
    if ( settings.readLocalEntry (filepath,"/export/xhtml/useImage","yes")=="yes")
		useImage=true;
	else	
		useImage=false;
	ui.imageButton->setChecked(useImage);
		
	if ( settings.readLocalEntry (filepath,"/export/xhtml/useTextColor","no")=="yes")
		useTextColor=true;
	else	
		useTextColor=false;
	ui.textColorButton->setChecked(useTextColor);
	
/* TODO this was used in old html export, is not yet in new stylesheet
	if ( settings.readEntry ("/export/html/useHeading","no")=="yes")
		useHeading=true;
	else	
		useHeading=false;
	checkBox4_2->setChecked(useHeading);
*/		

	if ( settings.readLocalEntry (filepath,"/export/xhtml/saveSettingsInMap","no")=="yes")
		saveSettingsInMap=true;
	else	
		saveSettingsInMap=false;
	ui.saveSettingsInMapButton->setChecked(saveSettingsInMap);

	if ( settings.readEntry ("/export/xhtml/showWarnings","yes")=="yes")
		showWarnings=true;
	else	
		showWarnings=false;
	ui.warningsButton->setChecked(showWarnings);
	
	if ( settings.readEntry ("/export/xhtml/showOutput","no")=="yes")
		showOutput=true;
	else	
		showOutput=false;
	ui.outputButton->setChecked(showOutput);

	// For testing better use local styles
    const QString defxsl(vymBaseDir.path() + "/styles/vym2xhtml.xsl");
    const QString defcss(vymBaseDir.path() + "/styles/vym.css");
	if (options.isOn ("local"))
	{
		xsl=defxsl;
		css=defcss;
	} else
	{
		xsl=settings.readLocalEntry 
			(filepath,"/export/xhtml/xsl",defxsl);
		css=settings.readLocalEntry 
			(filepath,"/export/xhtml/css",defcss);	
	}
	ui.lineEditXSL->setText(xsl);
	ui.lineEditCSS->setText(css);
	
	prescript=settings.readLocalEntry
		(filepath,"/export/xhtml/prescript","");
	ui.lineEditPreScript->setText (prescript);	
	
	postscript=settings.readLocalEntry
		(filepath,"/export/xhtml/postscript","");
	ui.lineEditPostScript->setText (postscript);	

	if (!prescript.isEmpty() || !postscript.isEmpty())
	{
		QMessageBox::warning( 0, tr( "Warning" ),tr(
		"The settings saved in the map "
		"would like to run scripts:\n\n"
		"%1\n\n"
		"Please check, if you really\n"
		"want to allow this in your system!").arg(prescript+"  "+postscript));
		
	}
}

void ExportXHTMLDialog::setDir(const QString &d)
{
	dir=d;
	if (dir.right(1)!="/") dir+="/";
}

void ExportXHTMLDialog::dirChanged()
{
	setDir (ui.lineEditDir->text());
	settingsChanged=true;
}

void ExportXHTMLDialog::browseDirectoryPressed()
{
   	QFileDialog fd( this);
	fd.setMode (QFileDialog::DirectoryOnly);
	fd.setCaption(tr("VYM - Export HTML to directory"));
	fd.setModal (true);
	fd.setDirectory (QDir::current());
	fd.show();

	if ( fd.exec() == QDialog::Accepted )
	{
		dir=fd.selectedFile();
		ui.lineEditDir->setText (dir );
		settingsChanged=true;
	}
}

void ExportXHTMLDialog::imageButtonPressed(bool b)
{
	useImage=b;
	settingsChanged=true;
}

void ExportXHTMLDialog::textcolorButtonPressed(bool b)
{
	useTextColor=b;	
	settingsChanged=true;
}

void ExportXHTMLDialog::saveSettingsInMapButtonPressed(bool b)
{
	saveSettingsInMap=b;	
	settingsChanged=true;
}

void ExportXHTMLDialog::warningsButtonPressed(bool b)
{
	showWarnings=b;
	settingsChanged=true;
}


void ExportXHTMLDialog::outputButtonPressed(bool b)
{
	showOutput=b;
	settingsChanged=true;
}

void ExportXHTMLDialog::cssChanged()
{
	css=ui.lineEditCSS->text();
	settingsChanged=true;
}

void ExportXHTMLDialog::browseCSSPressed()
{
   	QFileDialog fd( this);
	fd.setModal (true);
	fd.setFilter ("Cascading Stylesheet (*.css)");
	fd.setDirectory (QDir::current());
	fd.show();

	if ( fd.exec() == QDialog::Accepted )
	{
		css=fd.selectedFile();
		ui.lineEditCSS->setText (css );
		settingsChanged=true;
	}
}

void ExportXHTMLDialog::xslChanged()
{
	xsl=ui.lineEditXSL->text();
	settingsChanged=true;
}

void ExportXHTMLDialog::prescriptChanged()
{
	prescript=ui.lineEditPreScript->text();
	settingsChanged=true;
}

void ExportXHTMLDialog::browseXSLPressed()
{
   	QFileDialog fd( this);
	fd.setModal (true);
	fd.setFilter ("Extensible Stylesheet Language (*.xsl)");
	fd.setDirectory (QDir::current());
	fd.show();

	if ( fd.exec() == QDialog::Accepted )
	{
		xsl=fd.selectedFile();
		ui.lineEditXSL->setText (xsl );
		settingsChanged=true;
	}
}

void ExportXHTMLDialog::postscriptChanged()
{
	postscript=ui.lineEditPostScript->text();
	settingsChanged=true;
}

void ExportXHTMLDialog::browsePreExportButtonPressed()
{
	QFileDialog fd( this);
	fd.setModal (true);
	fd.setFilter ("Scripts (*.sh *.pl *.py *.php)");
	fd.setDirectory (QDir::current());
	fd.show();

	if ( fd.exec() == QDialog::Accepted )
	{
		prescript=fd.selectedFile();
		ui.lineEditPreScript->setText (prescript );
		settingsChanged=true;
	}

}

void ExportXHTMLDialog::browsePostExportButtonPressed()
{
	QFileDialog fd( this);
	fd.setModal (true);
	fd.setFilter ("Scripts (*.sh *.pl *.py *.php)");
	fd.setDirectory (QDir::current());
	fd.show();

	if ( fd.exec() == QDialog::Accepted )
	{
		postscript=fd.selectedFile();
		ui.lineEditPostScript->setText (postscript );
		settingsChanged=true;
	}
}


void ExportXHTMLDialog::doExport (const QString &mapname)
{
	// Save options to settings file 
	// (but don't save at destructor, which
	// is called for "cancel", too)
	settings.setLocalEntry (filepath,"/export/xhtml/exportDir",dir);
	settings.setLocalEntry (filepath,"/export/xhtml/prescript",prescript);
	settings.setLocalEntry (filepath,"/export/xhtml/postscript",postscript);

    if (useImage)
		settings.setLocalEntry (filepath,"/export/xhtml/useImage","yes");
    else
		settings.setLocalEntry (filepath,"/export/xhtml/useImage","no");	
	
  if (useTextColor)
		settings.setLocalEntry (filepath,"/export/xhtml/useTextColor","yes");
    else
		settings.setLocalEntry (filepath,"/export/xhtml/useTextColor","no");	
	
   if (showWarnings)
		settings.writeEntry ("/export/xhtml/showWarnings","yes");
    else
		settings.writeEntry ("/export/xhtml/showWarnings","no");	
			
	if (showOutput)
		settings.writeEntry ("/export/xhtml/showOutput","yes");
	else
		settings.writeEntry ("/export/xhtml/showOutput","no");	

	QString ipath;	
	ipath=vymBaseDir.path()+"/flags/flag-url-16x16.png";
	if (!options.isOn ("local"))
	{
		settings.setLocalEntry 
			(filepath,"/export/xhtml/xsl",xsl);
		settings.setLocalEntry 
			(filepath,"/export/xhtml/css",css);	
	}

	// Provide a smaller URL-icon to improve Layout
	QPixmap pm;
	if (!pm.load(ipath,"PNG") )
		QMessageBox::warning( 0, tr( "Warning" ),tr("Could not open %1").arg(ipath));
		
		
	if(!pm.save (dir + "flags/flag-url-16x16.png","PNG"))
		QMessageBox::warning( 0, tr( "Warning" ),tr("Could not write %1").arg(ipath));
	if (!saveSettingsInMap)
		settings.clearLocal("/export/xhtml");
	else	
		settings.setLocalEntry 
			(filepath,"/export/xhtml/saveSettingsInMap","yes");

	// Copy CSS file
	QFile css_src (css);
	QFile css_dst (dir+"vym.css");
	if (!css_src.open ( QIODevice::ReadOnly))
		QMessageBox::warning( 0, tr( "Warning" ),tr("Could not open %1").arg(css));
	else
	{
		if (!css_dst.open( QIODevice::WriteOnly))
			QMessageBox::warning( 0, tr( "Warning" ), tr("Could not open %1").arg(dir+"vym.css"));
		else
		{	
		
			QTextStream tsout( &css_dst);
			QTextStream tsin ( &css_src);
			QString s= tsin.read();
			tsout << s;
			css_dst.close();
		}	
		css_src.close();
	}

	if (!prescript.isEmpty()) runScript (prescript,dir+mapname+".xml");
	
	if (useImage)
		p.addStringParam ("imagemap","images/"+mapname+".png");
	if (useTextColor)
		p.addStringParam ("use.textcolor","1");
	p.addStringParam ("mapname",mapname+".vym");
	
	p.setOutputFile (dir+mapname+".html");
	p.setInputFile (dir+mapname+".xml");
	p.setXSLFile (xsl);
	p.process();

	if (!postscript.isEmpty()) runScript (postscript,dir+mapname+".html");

}

void ExportXHTMLDialog::setFilePath(const QString &s)
{
	filepath=s;
}

void ExportXHTMLDialog::setMapName(const QString &s)
{
	mapname=s;
}

QString ExportXHTMLDialog::getDir()
{
	return dir;
}

bool ExportXHTMLDialog::warnings()
{
	return showWarnings;
}

bool ExportXHTMLDialog::hasChanged()
{
	return settingsChanged;
}


void ExportXHTMLDialog::runScript(QString spath, QString fpath)
{
	spath.replace ("%f",fpath);
	QStringList args=QStringList::split (' ',spath,false);
		
	p.addOutput ("vym is executing: \n" + spath+" "+args.join(" ") );	
	scriptProc->start (spath,args);
	if (!scriptProc->waitForStarted() )
	{
		QMessageBox::critical( 0, tr( "Critical Error" ),
					   tr("Could not start %1").arg(spath) );
	} else
	{
		if (!scriptProc->waitForFinished())
			QMessageBox::critical( 0, tr( "Critical Error" ),
			   tr("%1 didn't exit normally").arg(spath) +
			   scriptProc->getErrout() );
		else
			if (scriptProc->exitStatus()>0) showOutput=true;
			
	}	
	p.addOutput ("\n");
	p.addOutput (scriptProc->getErrout());
	p.addOutput (scriptProc->getStdout());
}

