#include "exports.h"
#include "file.h"
#include "linkablemapobj.h"
#include "misc.h"
#include "mainwindow.h"
#include "warningdialog.h"
#include "xsltproc.h"

extern Main *mainWindow;
extern QDir vymBaseDir;
extern QString vymName;

ExportBase::ExportBase()
{
	indentPerDepth="  ";
	bool ok;
    tmpDir.setPath (makeTmpDir(ok,"vym-export"));
	if (!tmpDir.exists() || !ok)
		QMessageBox::critical( 0, QObject::tr( "Error" ),
					   QObject::tr("Couldn't access temporary directory\n"));
	cancelFlag=false;				   
}

ExportBase::~ExportBase()
{
	// Cleanup tmpdir
	removeDir (tmpDir);
}

void ExportBase::setDir(const QDir &d)
{
	outDir=d;
}

void ExportBase::setFile (const QString &p)
{
	outputFile=p;
}

QString ExportBase::getFile ()
{
	return outputFile;
}

void ExportBase::setModel(VymModel *m)
{
	model=m;
}

void ExportBase::setCaption (const QString &s)
{
	caption=s;
}

void ExportBase::addFilter(const QString &s)
{
	filter=s;
}

bool ExportBase::execDialog()
{
	//MapEditor *me=model.getMapEditor(); FIXME needed?
	// if (model->mapCenters.count() && me)
	{
		QFileDialog *fd=new QFileDialog( 0, caption);
		fd->setFilter (filter);
		fd->setCaption(caption);
		fd->setMode( QFileDialog::AnyFile );
		fd->setDir (outDir);
		fd->show();

		if ( fd->exec() == QDialog::Accepted )
		{
			if (QFile (fd->selectedFile()).exists() )
			{
				QMessageBox mb( vymName,
					QObject::tr("The file %1 exists already.\nDo you want to overwrite it?").arg(fd->selectedFile()), 
				QMessageBox::Warning,
				QMessageBox::Yes | QMessageBox::Default,
				QMessageBox::Cancel | QMessageBox::Escape,
				Qt::NoButton );
				mb.setButtonText( QMessageBox::Yes, QObject::tr("Overwrite") );
				mb.setButtonText( QMessageBox::No, QObject::tr("Cancel"));
				ExportBase ex;
				switch( mb.exec() ) 
				{
					case QMessageBox::Yes:
						// save 
						break;;
					case QMessageBox::Cancel:
						cancelFlag=true;
						return false;
						break;
				}
			}
			outputFile=fd->selectedFile();
			cancelFlag=false;
			return true;
		}
	}
	return false;
}

bool ExportBase::canceled()
{
	return cancelFlag;
}

QString ExportBase::getSectionString(BranchObj *bostart)
{
	// Make prefix like "2.5.3" for "bo:2,bo:5,bo:3"
	QString r;
	BranchObj *bo=bostart;
	int depth=bo->getDepth();
	while (depth>0)
	{
		r=QString("%1").arg(1+bo->getNum(),0,10)+"." + r;
		bo=(BranchObj*)(bo->getParObj());
		depth=bo->getDepth();
	}	
	if (r.isEmpty())
		return r;
	else	
		return r + " ";
}

////////////////////////////////////////////////////////////////////////
ExportASCII::ExportASCII()
{
	filter="TXT (*.txt)";
	caption=vymName+ " -" +QObject::tr("Export as ASCII")+" "+QObject::tr("(still experimental)");
}

void ExportASCII::doExport()
{
	QFile file (outputFile);
	if ( !file.open( QIODevice::WriteOnly ) )
	{
		qWarning ("ExportBase::exportXML  couldn't open "+outputFile);
		return;
	}
	QTextStream ts( &file );	// use LANG decoding here...

	// Main loop over all branches
	QString s;
	QString curIndent;
	int i;
	BranchObj *bo;
	bo=model->first();
	while (bo) 
	{
		// Make indentstring
		curIndent="";
		for (i=0;i<bo->getDepth()-1;i++) curIndent+= indentPerDepth;

		if (!bo->hasHiddenExportParent() )
		{
			switch (bo->getDepth())
			{
				case 0:
					ts << underline (bo->getHeading(),QString("="));
					ts << "\n";
					break;
				case 1:
					ts << "\n";
					ts << (underline (getSectionString(bo) + bo->getHeading(), QString("-") ) );
					ts << "\n";
					break;
				case 2:
					ts << "\n";
					ts << (curIndent + "* " + bo->getHeading());
					ts << "\n";
					break;
				case 3:
					ts << (curIndent + "- " + bo->getHeading());
					ts << "\n";
					break;
				default:
					ts << (curIndent + "- " + bo->getHeading());
					ts << "\n";
					break;
			}

			// If necessary, write note
			if (!bo->getNote().isEmpty())
			{
				curIndent +="  | ";
				s=bo->getNoteASCII( curIndent, 80);
				ts << s;
			}
		}
		bo=model->next(bo);
	}
	file.close();
}

QString ExportASCII::underline (const QString &text, const QString &line)
{
	QString r=text + "\n";
	for (int j=0;j<text.length();j++) r+=line;
	return r;
}


////////////////////////////////////////////////////////////////////////
void ExportCSV::doExport()
{
	QFile file (outputFile);
	if ( !file.open( QIODevice::WriteOnly ) )
	{
		qWarning ("ExportBase::exportXML  couldn't open "+outputFile);
		return;
	}
	QTextStream ts( &file );	// use LANG decoding here...

	// Write header
	ts << "\"Note\""  <<endl;

	// Main loop over all branches
	QString s;
	QString curIndent("");
	int i;
	BranchObj *bo;
	bo=model->first();
	while (bo) 
	{
		if (!bo->hasHiddenExportParent() )
		{
			// If necessary, write note
			if (!bo->getNote().isEmpty())
			{
				s =bo->getNoteASCII();
				s=s.replace ("\n","\n"+curIndent);
				ts << ("\""+s+"\",");
			} else
				ts <<"\"\",";

			// Make indentstring
			for (i=0;i<bo->getDepth();i++) curIndent+= "\"\",";

			// Write heading
			ts << curIndent << "\"" << bo->getHeading()<<"\""<<endl;
		}
		
		bo=model->next(bo);
		curIndent="";
	}
	file.close();
}

////////////////////////////////////////////////////////////////////////
void ExportKDE3Bookmarks::doExport() 
{
	MapEditor *me=model->getMapEditor();
	if (me)
	{
		WarningDialog dia;
		dia.showCancelButton (true);
		dia.setText(QObject::tr("Exporting the %1 bookmarks will overwrite\nyour existing bookmarks file.").arg("KDE"));
		dia.setCaption(QObject::tr("Warning: Overwriting %1 bookmarks").arg("KDE 3"));
		dia.setShowAgainName("/exports/KDE/overwriteKDEBookmarks");
		if (dia.exec()==QDialog::Accepted)
		{
			me->exportXML(tmpDir.path(),false);

			XSLTProc p;
			p.setInputFile (tmpDir.path()+"/"+me->getMapName()+".xml");
			p.setOutputFile (tmpDir.home().path()+"/.kde/share/apps/konqueror/bookmarks.xml");
			p.setXSLFile (vymBaseDir.path()+"/styles/vym2kdebookmarks.xsl");
			p.process();

			QString ub=vymBaseDir.path()+"/scripts/update-bookmarks";
			QProcess *proc= new QProcess ;
			proc->start( ub);
			if (!proc->waitForStarted())
			{
				QMessageBox::warning(0, 
					QObject::tr("Warning"),
					QObject::tr("Couldn't find script %1\nto notifiy Browsers of changed bookmarks.").arg(ub));
			}	
		}
	}
}

////////////////////////////////////////////////////////////////////////
void ExportKDE4Bookmarks::doExport() 
{
	MapEditor *me=model->getMapEditor();
	if (me)
	{
		WarningDialog dia;
		dia.showCancelButton (true);
		dia.setText(QObject::tr("Exporting the %1 bookmarks will overwrite\nyour existing bookmarks file.").arg("KDE"));
		dia.setCaption(QObject::tr("Warning: Overwriting %1 bookmarks").arg("KDE 4"));
		dia.setShowAgainName("/exports/KDE/overwriteKDEBookmarks");
		if (dia.exec()==QDialog::Accepted)
		{
			me->exportXML(tmpDir.path(),false);

			XSLTProc p;
			p.setInputFile (tmpDir.path()+"/"+me->getMapName()+".xml");
			p.setOutputFile (tmpDir.home().path()+"/.kde4/share/apps/konqueror/bookmarks.xml");
			p.setXSLFile (vymBaseDir.path()+"/styles/vym2kdebookmarks.xsl");
			p.process();

			QString ub=vymBaseDir.path()+"/scripts/update-bookmarks";
			QProcess *proc= new QProcess ;
			proc->start( ub);
			if (!proc->waitForStarted())
			{
				QMessageBox::warning(0, 
					QObject::tr("Warning"),
					QObject::tr("Couldn't find script %1\nto notifiy Browsers of changed bookmarks.").arg(ub));
			}	
		}
	}

}

////////////////////////////////////////////////////////////////////////
void ExportFirefoxBookmarks::doExport() 
{
	MapEditor *me=model->getMapEditor();
	if (me)
	{
		WarningDialog dia;
		dia.showCancelButton (true);
		dia.setText(QObject::tr("Exporting the %1 bookmarks will overwrite\nyour existing bookmarks file.").arg("Firefox"));
		dia.setCaption(QObject::tr("Warning: Overwriting %1 bookmarks").arg("Firefox"));
		dia.setShowAgainName("/vym/warnings/overwriteImportBookmarks");
		if (dia.exec()==QDialog::Accepted)
		{
			me->exportXML(tmpDir.path(),false);

/*
			XSLTProc p;
			p.setInputFile (tmpDir.path()+"/"+me->getMapName()+".xml");
			p.setOutputFile (tmpDir.home().path()+"/.kde/share/apps/konqueror/bookmarks.xml");
			p.setXSLFile (vymBaseDir.path()+"/styles/vym2kdebookmarks.xsl");
			p.process();

			QString ub=vymBaseDir.path()+"/scripts/update-bookmarks";
			QProcess *proc = new QProcess( );
			proc->addArgument(ub);

			if ( !proc->start() ) 
			{
				QMessageBox::warning(0, 
					QObject::tr("Warning"),
					QObject::tr("Couldn't find script %1\nto notifiy Browsers of changed bookmarks.").arg(ub));
			}	

*/

		}
	}
}

////////////////////////////////////////////////////////////////////////
void ExportTaskjuggler::doExport() 
{
	MapEditor *me=model->getMapEditor();
	if (me)
	{
		me->exportXML(tmpDir.path(),false);

		XSLTProc p;
		p.setInputFile (tmpDir.path()+"/"+me->getMapName()+".xml");
		p.setOutputFile (outputFile);
		p.setXSLFile (vymBaseDir.path()+"/styles/vym2taskjuggler.xsl");
		p.process();
	}

}

////////////////////////////////////////////////////////////////////////
void ExportLaTeX::doExport() 
{
	// Exports a map to a LaTex file.  
	// This file needs to be included 
	// or inported into a LaTex document
	// it will not add a preamble, or anything 
	// that makes a full LaTex document.
  QFile file (outputFile);
  if ( !file.open( QIODevice::WriteOnly ) ) {
	QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not write %1").arg(outputFile));
	mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
    return;
  }
  QTextStream ts( &file );	// use LANG decoding here...
  ts.setEncoding (QTextStream::UnicodeUTF8); // Force UTF8
  
  // Main loop over all branches
  QString s;
  // QString curIndent("");
  // int i;
  BranchObj *bo;
  bo=model->first();
  while (bo) {
	if (!bo->hasHiddenExportParent() )
	{
		if (bo->getDepth()==0);
		else if (bo->getDepth()==1) {
		  ts << ("\\chapter{" + bo->getHeading()+ "}\n");
		}
		else if (bo->getDepth()==2) {
		  ts << ("\\section{" + bo->getHeading()+ "}\n");
		}
		else if (bo->getDepth()==3) {
		  ts << ("\\subsection{" + bo->getHeading()+ "}\n");
		}
		else if (bo->getDepth()==4) {
		  ts << ("\\subsubsection{" + bo->getHeading()+ "}\n");
		}
		else {
		  ts << ("\\paragraph*{" + bo->getHeading()+ "}\n");
		}
		
		// If necessary, write note
		if (!bo->getNote().isEmpty()) {
		  ts << (bo->getNoteASCII());
		  ts << ("\n");
		}
	}
    bo=model->next(bo);
   }
  file.close();
}

////////////////////////////////////////////////////////////////////////
ExportOO::ExportOO()
{
	useSections=false;
}

ExportOO::~ExportOO()
{
}	

QString ExportOO::buildList (BranchObj *current)
{
    QString r;
    BranchObj *bo;

    uint i=0;
    bo=current->getFirstBranch();
    if (bo)
    {
		if (!bo->hasHiddenExportParent() )
		{
			// Start list
			r+="<text:list text:style-name=\"vym-list\">\n";
			while (bo)
			{
				r+="<text:list-item><text:p >";
				r+=quotemeta(bo->getHeading());
				// If necessary, write note
				if (!bo->getNote().isEmpty())
					r+=bo->getNoteOpenDoc();
				r+="</text:p>";
				r+=buildList (bo);	// recursivly add deeper branches
				r+="</text:list-item>\n";
				i++;
				bo=current->getBranchNum(i);
			}
			r+="</text:list>\n";
		}
    }
    return r;
}


void ExportOO::exportPresentation()
{
	QString allPages;

	MapCenterObj *firstMCO=(MapCenterObj*)model->first();
	if (!firstMCO) 
	{
		QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("No objects in map!"));
		return;
	}

	// Insert new content
	// FIXME add extra title in mapinfo for vym 1.13.x
	content.replace ("<!-- INSERT TITLE -->",quotemeta(firstMCO->getHeading()));
	content.replace ("<!-- INSERT AUTHOR -->",quotemeta(model->getAuthor()));

	QString	onePage;
	QString list;
	
	BranchObj *sectionBO;	
    int i=0;
	BranchObj *pagesBO;
    int j=0;

	int mapcenters=model->countMapCenters();

	// useSections already has been set in setConfigFile 
	if (mapcenters>1)	
		sectionBO=firstMCO;
	else
		sectionBO=firstMCO->getFirstBranch();

	// Walk sections
	while (sectionBO && !sectionBO->hasHiddenExportParent() )
	{
		if (useSections)
		{
			// Add page with section title
			onePage=sectionTemplate;
			onePage.replace ("<!-- INSERT PAGE HEADING -->", quotemeta(sectionBO->getHeading() ) );
			allPages+=onePage;
			pagesBO=sectionBO->getFirstBranch();
		} else
		{
			//i=-2;	// only use inner loop to 
			        // turn mainbranches into pages
			//sectionBO=firstMCO;
			pagesBO=sectionBO;
		}

		j=0;
		while (pagesBO && !pagesBO->hasHiddenExportParent() )
		{
			// Add page with list of items
			onePage=pageTemplate;
			onePage.replace ("<!-- INSERT PAGE HEADING -->", quotemeta (pagesBO->getHeading() ) );
			list=buildList (pagesBO);
			onePage.replace ("<!-- INSERT LIST -->", list);
			allPages+=onePage;
			if (pagesBO!=sectionBO)
			{
				j++;
				pagesBO=((BranchObj*)pagesBO->getParObj())->getBranchNum(j);
			} else
				pagesBO=NULL;	// We are already iterating over the sectionBOs
		}
		i++;
		if (mapcenters>1 )
			sectionBO=model->getMapCenterNum (i);
		else
			sectionBO=firstMCO->getBranchNum (i);
	}
	
	content.replace ("<!-- INSERT PAGES -->",allPages);

	// Write modified content
	QFile f (contentFile);
    if ( !f.open( QIODevice::WriteOnly ) ) 
	{
		QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not write %1").arg(contentFile));
		mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
		return;
    }

    QTextStream t( &f );
    t << content;
    f.close();

	// zip tmpdir to destination
	zipDir (tmpDir,outputFile);	
}

bool ExportOO::setConfigFile (const QString &cf)
{
	configFile=cf;
	int i=cf.findRev ("/");
	if (i>=0) configDir=cf.left(i);
	SimpleSettings set;
	set.readSettings(configFile);

	// set paths
	templateDir=configDir+"/"+set.readEntry ("Template");

	QDir d (templateDir);
	if (!d.exists())
	{
		QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Check \"%1\" in\n%2").arg("Template="+set.readEntry ("Template")).arg(configFile));
		return false;

	}

	contentTemplateFile=templateDir+"content-template.xml";
	contentFile=tmpDir.path()+"/content.xml";
	pageTemplateFile=templateDir+"page-template.xml";
	sectionTemplateFile=templateDir+"section-template.xml";

	if (model->countMapCenters()>1 ||set.readEntry("useSections").contains("yes"))
		useSections=true;

	// Copy template to tmpdir
	system ("cp -r "+templateDir+"* "+tmpDir.path());

	// Read content-template
	if (!loadStringFromDisk (contentTemplateFile,content))
	{
		QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not read %1").arg(contentTemplateFile));
		return false;
	}

	// Read page-template
	if (!loadStringFromDisk (pageTemplateFile,pageTemplate))
	{
		QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not read %1").arg(pageTemplateFile));
		return false;
	}
	
	// Read section-template
	if (useSections && !loadStringFromDisk (sectionTemplateFile,sectionTemplate))
	{
		QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not read %1").arg(sectionTemplateFile));
		return false;
	}
	return true;
}

