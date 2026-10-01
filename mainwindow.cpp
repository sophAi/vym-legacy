#include "mainwindow.h"

#include <QtGui>

#include <iostream>
#include <typeinfo>

#include "aboutdialog.h"
#include "branchpropwindow.h"
#include "exportoofiledialog.h"
#include "exports.h"
#include "file.h"
#include "flagrowobj.h"
#include "historywindow.h"
#include "imports.h"
#include "mapeditor.h"
#include "misc.h"
#include "options.h"
#include "process.h"
#include "settings.h"
#include "texteditor.h"
#include "warningdialog.h"

#if !defined(Q_OS_WIN32)
#include <unistd.h>
#endif

#if defined(Q_OS_WIN32)
// Define only this structure as opposed to
// including full 'windows.h'. FindWindow
// clashes with the one in Win32 API.
typedef struct _PROCESS_INFORMATION
{
  long hProcess;
  long hThread;
  long dwProcessId;
  long dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;
#endif

extern TextEditor *textEditor;
extern Main *mainWindow;
extern QString tmpVymDir;
extern QString clipboardDir;
extern QString clipboardFile;
extern bool clipboardEmpty;
extern int statusbarTime;
extern FlagRowObj* standardFlagsDefault;
extern FlagRowObj* systemFlagsDefault;
extern QString vymName;
extern QString vymVersion;
extern QString vymBuildDate;
extern bool debug;

QMenu* branchContextMenu;
QMenu* branchAddContextMenu;
QMenu* branchRemoveContextMenu;
QMenu* branchLinksContextMenu;
QMenu* branchXLinksContextMenuEdit;
QMenu* branchXLinksContextMenuFollow;
QMenu* floatimageContextMenu;
QMenu* canvasContextMenu;
QMenu* fileLastMapsMenu;
QMenu* fileImportMenu;
QMenu* fileExportMenu;


extern Settings settings;
extern Options options;
extern ImageIO imageIO;

extern QDir vymBaseDir;
extern QDir lastImageDir;
extern QDir lastFileDir;
#if defined(Q_OS_WIN32)
extern QDir vymInstallDir;
#endif
extern QString iconPath;
extern QString flagsPath;

Main::Main(QWidget* parent, const char* name, Qt::WFlags f) :
    QMainWindow(parent,name,f)
{
	mainWindow=this;

	setObjectName ("MainWindow");

	setCaption ("VYM - View Your Mind");

	// Load window settings
#if defined(Q_OS_WIN32)
    if (settings.value("/mainwindow/geometry/maximized", false).toBool())
    {
        setWindowState(Qt::WindowMaximized);
    }
    else
#endif
    {
        resize (settings.value("/mainwindow/geometry/size", QSize (800,600)).toSize());
        move   (settings.value("/mainwindow/geometry/pos",  QPoint(300,100)).toPoint());
    }

	// Sometimes we may need to remember old selections
	prevSelection="";

	// Default color
	currentColor=Qt::black;

	// Create unique temporary directory
	bool ok;
	tmpVymDir=makeTmpDir (ok,"vym");
	if (!ok)
	{
		qWarning ("Mainwindow: Could not create temporary directory, failed to start vym");
		exit (1);
	}
	if (debug) qDebug (QString("vym tmpDir=%1").arg(tmpVymDir) );

	// Create direcctory for clipboard
	clipboardDir=tmpVymDir+"/clipboard";
	clipboardFile="map.xml";
	QDir d(clipboardDir);
	d.mkdir (clipboardDir,true);
	makeSubDirs (clipboardDir);
	clipboardEmpty=true;

	procBrowser=NULL;

	// Satellite windows //////////////////////////////////////////

	// history window
	historyWindow=new HistoryWindow();
	connect (historyWindow, SIGNAL (windowClosed() ), this, SLOT (updateActions()));

	// properties window
	branchPropertyWindow = new BranchPropertyWindow();
	connect (branchPropertyWindow, SIGNAL (windowClosed() ), this, SLOT (updateActions()));

	// Connect TextEditor, so that we can update flags if text changes
	connect (textEditor, SIGNAL (textHasChanged() ), this, SLOT (updateNoteFlag()));
	connect (textEditor, SIGNAL (windowClosed() ), this, SLOT (updateActions()));

	// Connect HistoryWindow, so that we can update flags
	connect (historyWindow, SIGNAL (windowClosed() ), this, SLOT (updateActions()));


	// Initialize script editor
	scriptEditor = new SimpleScriptEditor();
	scriptEditor->move (50,50);

	connect( scriptEditor, SIGNAL( runScript ( QString ) ), 
		this, SLOT( runScript( QString ) ) );
	

	// Initialize Find window
	findWindow=new FindWindow(NULL);
	findWindow->move (x(),y()+70);
	connect (findWindow, SIGNAL( findButton(QString) ), 
		this, SLOT(editFind(QString) ) );	
	connect (findWindow, SIGNAL( somethingChanged() ), 
		this, SLOT(editFindChanged() ) );	

	// Initialize some settings, which are platform dependant
	QString p,s;

		// application to open URLs
		p="/mainwindow/readerURL";
		#if defined(Q_OS_LINUX)
			s=settings.value (p,"xdg-open").toString();
		#else
			#if defined(Q_OS_MACX)
				s=settings.value (p,"/usr/bin/open").toString();

            #else
                #if defined(Q_OS_WIN32)
                    // Assume that system has been set up so that
                    // Explorer automagically opens up the URL
                    // in the user's preferred browser.
                    s=settings.value (p,"explorer").toString();
                #else
					s=settings.value (p,"mozilla").toString();
				#endif
			#endif
		#endif
		settings.setValue( p,s);

		// application to open PDFs
		p="/mainwindow/readerPDF";
		#if defined(Q_OS_LINUX)
			s=settings.value (p,"xdg-open").toString();
		#else
			#if defined(Q_OS_MACX)
				s=settings.value (p,"/usr/bin/open").toString();
            #elif defined(Q_OS_WIN32)
                s=settings.value (p,"acrord32").toString();
			#else
				s=settings.value (p,"acroread").toString();
			#endif
		#endif
		settings.setValue( p,s);

	// width of xLinksMenu
	xLinkMenuWidth=60;
	
	// Create tab widget which holds the maps
	tabWidget= new QTabWidget (this);
	connect( tabWidget, SIGNAL( currentChanged( QWidget * ) ), 
		this, SLOT( editorChanged( QWidget * ) ) );

	lineedit=new QLineEdit (this);
	lineedit->hide();

	setCentralWidget(tabWidget);	

    setupFileActions();
    setupEditActions();
    setupFormatActions();
    setupViewActions();
    setupModeActions();
	setupFlagActions();
    setupNetworkActions();
    setupSettingsActions();
	setupContextMenus();
	setupMacros();
    if (settings.value( "/mainwindow/showTestMenu",false).toBool()) setupTestActions();
    setupHelpActions();
    
    statusBar();

	restoreState (settings.value("/mainwindow/state",0).toByteArray());

	updateGeometry();
}

Main::~Main()
{
	//cout << "Destr MainWindow\n";
	// Save Settings
#if defined(Q_OS_WIN32)
    settings.setValue ("/mainwindow/geometry/maximized", isMaximized());
#endif
	settings.setValue ("/mainwindow/geometry/size", size());
	settings.setValue ("/mainwindow/geometry/pos", pos());
	settings.setValue ("/mainwindow/state",saveState(0));

	settings.setValue ("/mainwindow/view/AntiAlias",actionViewToggleAntiAlias->isOn());
	settings.setValue ("/mainwindow/view/SmoothPixmapTransform",actionViewToggleSmoothPixmapTransform->isOn());
	settings.setValue( "/version/version", vymVersion );
	settings.setValue( "/version/builddate", vymBuildDate );

	settings.setValue( "/mainwindow/autosave/use",actionSettingsAutosaveToggle->isOn() );
	settings.setValue( "/mapeditor/editmode/autoSelectNewBranch",actionSettingsAutoSelectNewBranch->isOn() );
	settings.setValue( "/mainwindow/writeBackupFile",actionSettingsWriteBackupFile->isOn() );
	settings.setValue( "/mapeditor/editmode/autoSelectText",actionSettingsAutoSelectText->isOn() );
	settings.setValue( "/mapeditor/editmode/autoEditNewBranch",actionSettingsAutoEditNewBranch->isOn() );
	settings.setValue( "/mapeditor/editmode/useDelKey",actionSettingsUseDelKey->isOn() );
	settings.setValue( "/mapeditor/editmode/useFlagGroups",actionSettingsUseFlagGroups->isOn() );
	settings.setValue( "/export/useHideExport",actionSettingsUseHideExport->isOn() );

	//TODO save scriptEditor settings

	// call the destructors
	delete textEditor;
	delete historyWindow;
	delete branchPropertyWindow;

	// Remove temporary directory
	removeDir (QDir(tmpVymDir));
}

void Main::loadCmdLine()
{
	/* TODO draw some kind of splashscreen while loading...
	if (qApp->argc()>1)
	{
	}
	*/
	
	QStringList flist=options.getFileList();
	QStringList::Iterator it=flist.begin();

	while (it !=flist.end() )
	{
		fileLoad (*it, NewMap);
		*it++;
	}	
}


void Main::statusMessage(const QString &s)
{
	statusBar()->message( s);
}

void Main::closeEvent (QCloseEvent* )
{
	fileExitVYM();
}

// File Actions
void Main::setupFileActions()
{
	QMenu *fileMenu = menuBar()->addMenu ( tr ("&Map") );
    QToolBar *tb = addToolBar( tr ("&Map") );
	tb->setObjectName ("mapTB");

    QAction *a;
    a = new QAction(QPixmap( iconPath+"filenew.png"), tr( "&New map","File menu" ),this);
	a->setStatusTip ( tr( "New map","Status tip File menu" ) );
	a->setShortcut ( Qt::CTRL + Qt::Key_N );		//New map
    a->addTo( tb );
	fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileNew() ) );
	
    a = new QAction(QPixmap( iconPath+"filenewcopy.png"), tr( "&Copy to new map","File menu" ),this);
	a->setStatusTip ( tr( "Copy selection to mapcenter of a new map","Status tip File menu" ) );
	a->setShortcut ( Qt::CTRL +Qt::SHIFT + Qt::Key_N );		//New map
	fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileNewCopy() ) );
	actionFileNewCopy=a;
	
    a = new QAction( QPixmap( iconPath+"fileopen.png"), tr( "&Open..." ,"File menu"),this);
	a->setStatusTip (tr( "Open","Status tip File menu" ) );
	a->setShortcut ( Qt::CTRL + Qt::Key_O );		//Open map
    a->addTo( tb );
	fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileLoad() ) );
	
	fileLastMapsMenu = fileMenu->addMenu (tr("Open Recent","File menu"));
	fileMenu->addSeparator();
	
    a = new QAction( QPixmap( iconPath+"filesave.png"), tr( "&Save...","File menu" ), this);
	a->setStatusTip ( tr( "Save","Status tip file menu" ));
	a->setShortcut (Qt::CTRL + Qt::Key_S );			//Save map
    a->addTo( tb );
	fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileSave() ) );
	actionFileSave=a;
	
    a = new QAction( QPixmap(iconPath+"filesaveas.png"), tr( "Save &As...","File menu" ), this);
	a->setStatusTip (tr( "Save &As","Status tip file menu" ) );
	fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileSaveAs() ) );

	fileMenu->addSeparator();

	fileImportMenu = fileMenu->addMenu (tr("Import","File menu"));

	a = new QAction(tr("KDE 3 Bookmarks"), this);
	a->setStatusTip ( tr( "Import %1","Status tip file menu" ).arg(tr("KDE 3 bookmarks")));
	a->addTo (fileImportMenu);
	connect( a, SIGNAL( triggered() ), this, SLOT( fileImportKDE3Bookmarks() ) );

	a = new QAction(tr("KDE 4 Bookmarks"), this);
	a->setStatusTip ( tr( "Import %1","Status tip file menu" ).arg(tr("KDE 4 bookmarks")));
	a->addTo (fileImportMenu);
	connect( a, SIGNAL( triggered() ), this, SLOT( fileImportKDE4Bookmarks() ) );

    if (settings.value( "/mainwindow/showTestMenu",false).toBool()) 
	{
		a = new QAction( QPixmap(), tr("Firefox Bookmarks","File menu"),this);
		a->setStatusTip (tr( "Import %1","Status tip file menu").arg(tr("Firefox Bookmarks" ) ));
		a->addTo (fileImportMenu);
		connect( a, SIGNAL( triggered() ), this, SLOT( fileImportFirefoxBookmarks() ) );
	}	

	a = new QAction("Freemind...",this);
	a->setStatusTip ( tr( "Import %1","status tip file menu").arg(" Freemind")  );
	fileImportMenu->addAction (a);
	connect( a, SIGNAL( triggered() ), this, SLOT( fileImportFreemind() ) );

	a = new QAction("Mind Manager...",this);
	a->setStatusTip ( tr( "Import %1","status tip file menu").arg(" Mind Manager")  );
	fileImportMenu->addAction (a);
	connect( a, SIGNAL( triggered() ), this, SLOT( fileImportMM() ) );

    a = new QAction( tr( "Import Dir%1","File menu").arg("..."), this);
	a->setStatusTip (tr( "Import directory structure (experimental)","status tip file menu" ) );
	fileImportMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileImportDir() ) );

	fileExportMenu = fileMenu->addMenu (tr("Export","File menu"));

	a = new QAction( tr("Image%1","File export menu").arg("..."), this);
	a->setStatusTip( tr( "Export map as image","status tip file menu" ));
	connect( a, SIGNAL( triggered() ), this, SLOT( fileExportImage() ) );
	fileExportMenu->addAction (a);

	a = new QAction( "Open Office...", this);
	a->setStatusTip( tr( "Export in Open Document Format used e.g. in Open Office ","status tip file menu" ));
	connect( a, SIGNAL( triggered() ), this, SLOT( fileExportOOPresentation() ) );
	fileExportMenu->addAction (a);

	a = new QAction(  "Webpage (XHTML)...",this );
	a->setShortcut (Qt::ALT + Qt::Key_X);			//Export XHTML
	a->setStatusTip ( tr( "Export as %1","status tip file menu").arg(tr(" webpage (XHTML)","status tip file menu")));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportXHTML() ) );
	fileExportMenu->addAction (a);

    a = new QAction( "Text (ASCII)...", this);
	a->setStatusTip ( tr( "Export as %1").arg("ASCII "+tr("(still experimental)" )));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportASCII() ) );
	fileExportMenu->addAction (a);

    a = new QAction( "Spreadsheet (CSV)...", this);
	a->setStatusTip ( tr( "Export as %1").arg("CSV "+tr("(still experimental)" )));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportCSV() ) );
	fileExportMenu->addAction (a);

	a = new QAction( tr("KDE 3 Bookmarks","File menu"), this);
	a->setStatusTip( tr( "Export as %1").arg(tr("KDE 3 Bookmarks" )));
	connect( a, SIGNAL( triggered() ), this, SLOT( fileExportKDE3Bookmarks() ) );
	fileExportMenu->addAction (a);

	a = new QAction( tr("KDE 4 Bookmarks","File menu"), this);
	a->setStatusTip( tr( "Export as %1").arg(tr("KDE 4 Bookmarks" )));
	connect( a, SIGNAL( triggered() ), this, SLOT( fileExportKDE4Bookmarks() ) );
	fileExportMenu->addAction (a);

    a = new QAction( "Taskjuggler...", this );
    a->setStatusTip( tr( "Export as %1").arg("Taskjuggler "+tr("(still experimental)" )));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportTaskjuggler() ) );
	fileExportMenu->addAction (a);

    a = new QAction( "LaTeX...", this);
    a->setStatusTip( tr( "Export as %1").arg("LaTeX "+tr("(still experimental)" )));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportLaTeX() ) );
	fileExportMenu->addAction (a);

	a = new QAction( "XML..." , this );
	a->setStatusTip (tr( "Export as %1").arg("XML"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportXML() ) );
	fileExportMenu->addAction (a);

	fileMenu->addSeparator();

    a = new QAction(QPixmap( iconPath+"fileprint.png"), tr( "&Print")+QString("..."), this);
	a->setStatusTip ( tr( "Print" ,"File menu") );
	a->setShortcut (Qt::CTRL + Qt::Key_P );			//Print map
    a->addTo( tb );
	fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( filePrint() ) );
	actionFilePrint=a;

    a = new QAction( QPixmap(iconPath+"fileclose.png"), tr( "&Close Map","File menu" ), this);
	a->setStatusTip (tr( "Close Map" ) );
	a->setShortcut (Qt::CTRL + Qt::Key_W );			//Close map
	fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileCloseMap() ) );

    a = new QAction(QPixmap(iconPath+"exit.png"), tr( "E&xit","File menu")+" "+vymName, this);
	a->setStatusTip ( tr( "Exit")+" "+vymName );
	a->setShortcut (Qt::CTRL + Qt::Key_Q );			//Quit vym
	fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExitVYM() ) );
}


//Edit Actions
void Main::setupEditActions()
{
    QToolBar *tb = addToolBar( tr ("&Actions toolbar","Toolbar name") );
    tb->setLabel( "Edit Actions" );
	tb->setObjectName ("actionsTB");
    QMenu *editMenu = menuBar()->addMenu( tr("&Edit","Edit menu") );

    QAction *a;
	QAction *alt;
    a = new QAction( QPixmap( iconPath+"undo.png"), tr( "&Undo","Edit menu" ),this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editUndo() ) );
	a->setStatusTip (tr( "Undo" ) );
	a->setShortcut ( Qt::CTRL + Qt::Key_Z );		//Undo last action
	a->setEnabled (false);
    tb->addAction (a);
	editMenu->addAction (a);
	actionEditUndo=a;
    
	a = new QAction( QPixmap( iconPath+"redo.png"), tr( "&Redo","Edit menu" ), this); 
	a->setStatusTip (tr( "Redo" ));
	a->setShortcut (Qt::CTRL + Qt::Key_Y );			//Redo last action
    tb->addAction (a);
	editMenu->addAction (a);
	connect( a, SIGNAL( triggered() ), this, SLOT( editRedo() ) );
	actionEditRedo=a;
   
	editMenu->addSeparator();
    a = new QAction(QPixmap( iconPath+"editcopy.png"), tr( "&Copy","Edit menu" ), this);
	a->setStatusTip ( tr( "Copy" ) );
	a->setShortcut (Qt::CTRL + Qt::Key_C );			//Copy
	a->setEnabled (false);
    tb->addAction (a);
	editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editCopy() ) );
	actionEditCopy=a;
	
    a = new QAction(QPixmap( iconPath+"editcut.png" ), tr( "Cu&t","Edit menu" ), this);
	a->setStatusTip ( tr( "Cut" ) );
	a->setShortcut (Qt::CTRL + Qt::Key_X );			//Cut
	a->setEnabled (false);
    tb->addAction (a);
	editMenu->addAction (a);
	actionEditCut=a;
    connect( a, SIGNAL( triggered() ), this, SLOT( editCut() ) );
	
    a = new QAction(QPixmap( iconPath+"editpaste.png"), tr( "&Paste","Edit menu" ),this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editPaste() ) );
	a->setStatusTip ( tr( "Paste" ) );
	a->setShortcut ( Qt::CTRL + Qt::Key_V );		//Paste
	a->setEnabled (false);
    tb->addAction (a);
	editMenu->addAction (a);
	actionEditPaste=a;

    // Shortcuts to modify heading:
    a = new QAction(tr( "Edit heading","Edit menu" ),this);
	a->setStatusTip ( tr( "edit Heading" ));
	a->setShortcut ( Qt::Key_Enter);				//Edit heading
//	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editHeading() ) );
	actionListBranches.append(a);
    a = new QAction( tr( "Edit heading","Edit menu" ), this);
	a->setStatusTip (tr( "edit Heading" ));
	a->setShortcut (Qt::Key_Return );				//Edit heading
	//a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editHeading() ) );
	actionListBranches.append(a);
	editMenu->addAction (a);
	actionEditHeading=a;
    a = new QAction( tr( "Edit heading","Edit menu" ), this);
	a->setStatusTip (tr( "edit Heading" ));
	//a->setShortcut ( Qt::Key_F2 );					//Edit heading
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editHeading() ) );
	actionListBranches.append(a);
    
    // Shortcut to delete selection
    a = new QAction( tr( "Delete Selection","Edit menu" ),this);
	a->setStatusTip (tr( "Delete Selection" ));
	a->setShortcut ( Qt::Key_Delete);				//Delete selection
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteSelection() ) );
	actionEditDelete=a;
    
    // Shortcut to add mapcenter
	a= new QAction(tr( "Add mapcenter","Canvas context menu" ), this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editAddMapCenter() ) );
	actionEditAddMapCenter = a;


    // Shortcut to add branch
	alt = new QAction(QPixmap(iconPath+"newbranch.png"), tr( "Add branch as child","Edit menu" ), this);
	alt->setStatusTip ( tr( "Add a branch as child of selection" ));
	alt->setShortcut (Qt::Key_A);					//Add branch
	alt->setShortcutContext (Qt::WindowShortcut);
	addAction (alt);
	connect( alt, SIGNAL( triggered() ), this, SLOT( editNewBranch() ) );
	a = new QAction(QPixmap(iconPath+"newbranch.png"), tr( "Add branch as child","Edit menu" ), this);
	a->setStatusTip ( tr( "Add a branch as child of selection" ));
	a->setShortcut (Qt::Key_Insert);				//Add branch
	connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranch() ) );
	actionListBranches.append(a);
	#if defined (Q_OS_MACX)
		// In OSX show different shortcut in menues, the keys work indepently always			
		actionEditAddBranch=alt;
	#else	
		actionEditAddBranch=a;
	#endif	
	editMenu->addAction (actionEditAddBranch);
	tb->addAction (actionEditAddBranch);


    // Add branch by inserting it at selection
	a = new QAction(tr( "Add branch (insert)","Edit menu" ), this);
	a->setStatusTip ( tr( "Add a branch by inserting and making selection its child" ));
	a->setShortcut (Qt::ALT + Qt::Key_Insert );		//Insert branch
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchBefore() ) );
	a->setEnabled (false);
	actionListBranches.append(a);
	actionEditAddBranchBefore=a;
	a = new QAction(tr( "Add branch (insert)","Edit menu" ),this);
	a->setStatusTip ( tr( "Add a branch by inserting and making selection its child" ));
	a->setShortcut ( Qt::ALT + Qt::Key_A );			//Insert branch
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchBefore() ) );
	actionListBranches.append(a);

	// Add branch above
    a = new QAction(tr( "Add branch above","Edit menu" ), this);
	a->setStatusTip ( tr( "Add a branch above selection" ));
	a->setShortcut (Qt::SHIFT+Qt::Key_Insert );		//Add branch above
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchAbove() ) );
	a->setEnabled (false);
	actionListBranches.append(a);
	actionEditAddBranchAbove=a;
    a = new QAction(tr( "Add branch above","Edit menu" ), this);
	a->setStatusTip ( tr( "Add a branch above selection" ));
	a->setShortcut (Qt::SHIFT+Qt::Key_A );			//Add branch above
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchAbove() ) );
	actionListBranches.append(a);

	// Add branch below 
    a = new QAction(tr( "Add branch below","Edit menu" ), this);
	a->setStatusTip ( tr( "Add a branch below selection" ));
	a->setShortcut (Qt::CTRL +Qt::Key_Insert );		//Add branch below
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchBelow() ) );
	a->setEnabled (false);
	actionListBranches.append(a);
	actionEditAddBranchBelow=a;
    a = new QAction(tr( "Add branch below","Edit menu" ), this);
	a->setStatusTip ( tr( "Add a branch below selection" ));
	a->setShortcut (Qt::CTRL +Qt::Key_A );			// Add branch below
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchBelow() ) );
	actionListBranches.append(a);

    a = new QAction(QPixmap(iconPath+"up.png" ), tr( "Move up","Edit menu" ), this);
	a->setStatusTip ( tr( "Move branch up" ) );
	a->setShortcut (Qt::Key_PageUp );				// Move branch up
	a->setEnabled (false);
    tb->addAction (a);
	editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editMoveUp() ) );
	actionEditMoveUp=a;

    a = new QAction( QPixmap( iconPath+"down.png"), tr( "Move down","Edit menu" ),this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editMoveDown() ) );
	a->setStatusTip (tr( "Move branch down" ) );
	a->setShortcut ( Qt::Key_PageDown );			// Move branch down
	a->setEnabled (false);
    tb->addAction (a);
	editMenu->addAction (a);
	actionEditMoveDown=a;
	
	a = new QAction( QPixmap(iconPath+"editsort.png" ), tr( "Sort children","Edit menu" ), this );
	connect( a, SIGNAL( activated() ), this, SLOT( editSortChildren() ) );
	a->setEnabled (true);
	a->addTo( tb );
	editMenu->addAction (a);
	actionEditSortChildren=a;

	a = new QAction( QPixmap(flagsPath+"flag-scrolled-right.png"), tr( "Scroll branch","Edit menu" ),this);
	a->setShortcut ( Qt::Key_ScrollLock );
	a->setStatusTip (tr( "Scroll branch" ) );
    connect( a, SIGNAL( triggered() ), this, SLOT( editToggleScroll() ) );

	alt = new QAction( QPixmap(flagsPath+"flag-scrolled-right.png"), tr( "Scroll branch","Edit menu" ), this);
	alt->setShortcut ( Qt::Key_S );					// Scroll branch
	alt->setStatusTip (tr( "Scroll branch" )); 
    connect( alt, SIGNAL( triggered() ), this, SLOT( editToggleScroll() ) );
	#if defined(Q_OS_MACX)
		actionEditToggleScroll=alt;
	#else	
		actionEditToggleScroll=a;
	#endif	
	actionEditToggleScroll->setEnabled (false);
	actionEditToggleScroll->setToggleAction(true);
    tb->addAction (actionEditToggleScroll);
    editMenu->addAction ( actionEditToggleScroll);
	editMenu->addAction (actionEditToggleScroll);
	addAction (a);
	addAction (alt);
	actionListBranches.append(actionEditToggleScroll);
	
    a = new QAction( tr( "Unscroll childs","Edit menu" ), this);
	a->setStatusTip (tr( "Unscroll all scrolled branches in selected subtree" ));
	editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editUnscrollChilds() ) );
	
	editMenu->addSeparator();

	a = new QAction( QPixmap(iconPath+"find.png"), tr( "Find...","Edit menu"), this);
	a->setStatusTip (tr( "Find" ) );
	a->setShortcut (Qt::CTRL + Qt::Key_F );				//Find
	editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenFindWindow() ) );
    
	editMenu->addSeparator();

	a = new QAction( QPixmap(flagsPath+"flag-url.png"), tr( "Open URL","Edit menu" ), this);
	a->setShortcut (Qt::CTRL + Qt::Key_U );
	a->setShortcut (tr( "Open URL" ));
    tb->addAction (a);
	addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenURL() ) );
	actionEditOpenURL=a;

	a = new QAction( tr( "Open URL in new tab","Edit menu" ), this);
	a->setStatusTip (tr( "Open URL in new tab" ));
	//a->setShortcut (Qt::CTRL+Qt::Key_U );
	addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenURLTab() ) );
	actionEditOpenURLTab=a;

	a = new QAction( tr( "Open all URLs in subtree","Edit menu" ), this);
	a->setStatusTip (tr( "Open all URLs in subtree" ));
	addAction(a);
	actionListBranches.append(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenMultipleURLTabs() ) );
	actionEditOpenMultipleURLTabs=a;

	a = new QAction(QPixmap(), tr( "Edit URL...","Edit menu"), this);
	a->setStatusTip ( tr( "Edit URL" ) );
	a->setShortcut ( Qt::Key_U );
	a->setShortcutContext (Qt::WindowShortcut);
	actionListBranches.append(a);
	addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editURL() ) );
	actionEditURL=a;
	
	a = new QAction(QPixmap(), tr( "Edit local URL...","Edit menu"), this);
	a->setStatusTip ( tr( "Edit local URL" ) );
	a->setShortcut (Qt::SHIFT +  Qt::Key_U );
	a->setShortcutContext (Qt::WindowShortcut);
	actionListBranches.append(a);
	addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editLocalURL() ) );
	actionEditLocalURL=a;
	
	a = new QAction( tr( "Use heading for URL","Edit menu" ), this);
	a->setStatusTip ( tr( "Use heading of selected branch as URL" ));
	a->setEnabled (false);
	actionListBranches.append(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editHeading2URL() ) );
	actionEditHeading2URL=a;
    
	a = new QAction(tr( "Create URL to Novell Bugzilla","Edit menu" ), this);
	a->setStatusTip ( tr( "Create URL to Novell Bugzilla" ));
	a->setEnabled (false);
	actionListBranches.append(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editBugzilla2URL() ) );
	actionEditBugzilla2URL=a;
    
	a = new QAction(tr( "Create URL to Novell FATE","Edit menu" ), this);
	a->setStatusTip ( tr( "Create URL to Novell FATE" ));
	a->setEnabled (false);
	actionListBranches.append(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editFATE2URL() ) );
	actionEditFATE2URL=a;
	
    a = new QAction(QPixmap(flagsPath+"flag-vymlink.png"), tr( "Open linked map","Edit menu" ), this);
	a->setStatusTip ( tr( "Jump to another vym map, if needed load it first" ));
    tb->addAction (a);
	a->setEnabled (false);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenVymLink() ) );
	actionEditOpenVymLink=a;
	
    a = new QAction(QPixmap(), tr( "Open all vym links in subtree","Edit menu" ), this);
	a->setStatusTip ( tr( "Open all vym links in subtree" ));
	a->setEnabled (false);
	actionListBranches.append(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenMultipleVymLinks() ) );
	actionEditOpenMultipleVymLinks=a;
	

    a = new QAction(tr( "Edit vym link...","Edit menu" ), this);
	a->setEnabled (false);
	a->setStatusTip ( tr( "Edit link to another vym map" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( editVymLink() ) );
	actionListBranches.append(a);
	actionEditVymLink=a;

    a = new QAction(tr( "Delete vym link","Edit menu" ),this);
	a->setStatusTip ( tr( "Delete link to another vym map" ));
	a->setEnabled (false);
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteVymLink() ) );
	actionEditDeleteVymLink=a;

    a = new QAction(QPixmap(flagsPath+"flag-hideexport.png"), tr( "Hide in exports","Edit menu" ), this);
	a->setStatusTip ( tr( "Hide object in exports" ) );
	a->setShortcut (Qt::Key_H );
	a->setToggleAction(true);
    tb->addAction (a);
	a->setEnabled (false);
    connect( a, SIGNAL( triggered() ), this, SLOT( editToggleHideExport() ) );
	actionEditToggleHideExport=a;

    a = new QAction(tr( "Edit Map Info...","Edit menu" ),this);
	a->setStatusTip ( tr( "Edit Map Info" ));
	a->setEnabled (true);
    connect( a, SIGNAL( triggered() ), this, SLOT( editMapInfo() ) );
	actionEditMapInfo=a;

	// Import at selection (adding to selection)
    a = new QAction( tr( "Add map (insert)","Edit menu" ),this);
	a->setStatusTip (tr( "Add map at selection" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( editImportAdd() ) );
	a->setEnabled (false);
	actionListBranches.append(a);
	actionEditImportAdd=a;

	// Import at selection (replacing selection)
    a = new QAction( tr( "Add map (replace)","Edit menu" ), this);
	a->setStatusTip (tr( "Replace selection with map" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( editImportReplace() ) );
	a->setEnabled (false);
	actionListBranches.append(a);
	actionEditImportReplace=a;

	// Save selection 
    a = new QAction( tr( "Save selection","Edit menu" ), this);
	a->setStatusTip (tr( "Save selection" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( editSaveBranch() ) );
	a->setEnabled (false);
	actionListBranches.append(a);
	actionEditSaveBranch=a;

	// Only remove branch, not its childs
    a = new QAction(tr( "Remove only branch ","Edit menu" ), this);
	a->setStatusTip ( tr( "Remove only branch and keep its childs" ));
	a->setShortcut (Qt::ALT + Qt::Key_Delete );
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteKeepChilds() ) );
	a->setEnabled (false);
	addAction (a);
	actionListBranches.append(a);
	actionEditDeleteKeepChilds=a;

	// Only remove childs of a branch
    a = new QAction( tr( "Remove childs","Edit menu" ), this);
	a->setStatusTip (tr( "Remove childs of branch" ));
	a->setShortcut (Qt::SHIFT + Qt::Key_Delete );
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteChilds() ) );
	a->setEnabled (false);
	actionListBranches.append(a);
	actionEditDeleteChilds=a;

    // Shortcuts for navigating with cursor:
    a = new QAction(tr( "Select upper branch","Edit menu" ), this);
	a->setStatusTip ( tr( "Select upper branch" ));
	a->setShortcut (Qt::Key_Up );
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editUpperBranch() ) );
    a = new QAction( tr( "Select lower branch","Edit menu" ),this);
	a->setStatusTip (tr( "Select lower branch" ));
	a->setShortcut ( Qt::Key_Down );
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editLowerBranch() ) );
    a = new QAction(tr( "Select left branch","Edit menu" ), this);
	a->setStatusTip ( tr( "Select left branch" ));
	a->setShortcut (Qt::Key_Left );
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editLeftBranch() ) );
    a = new QAction( tr( "Select child branch","Edit menu" ), this);
	a->setStatusTip (tr( "Select right branch" ));
	a->setShortcut (Qt::Key_Right);
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editRightBranch() ) );
    a = new QAction( tr( "Select first branch","Edit menu" ), this);
	a->setStatusTip (tr( "Select first branch" ));
	a->setShortcut (Qt::Key_Home );
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
	a->setEnabled (false);
	editMenu->addAction (a);
	actionListBranches.append(a);
	actionEditSelectFirst=a;
    connect( a, SIGNAL( triggered() ), this, SLOT( editFirstBranch() ) );
    a = new QAction( tr( "Select last branch","Edit menu" ),this);
	a->setStatusTip (tr( "Select last branch" ));
	a->setShortcut ( Qt::Key_End );
	a->setShortcutContext (Qt::WindowShortcut);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editLastBranch() ) );
	a->setEnabled (false);
	editMenu->addAction (a);
	actionListBranches.append(a);
	actionEditSelectLast=a;

    a = new QAction( tr( "Add Image...","Edit menu" ), this);
	a->setStatusTip (tr( "Add Image" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( editLoadImage() ) );
	actionEditLoadImage=a;

    a = new QAction( tr( "Property window","Dialog to edit properties of selection" )+QString ("..."), this);
	a->setStatusTip (tr( "Set properties for selection" ));
	a->setShortcut ( Qt::CTRL + Qt::Key_I );		//Property window
	a->setShortcutContext (Qt::WindowShortcut);
	a->setToggleAction (true);
	addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( windowToggleProperty() ) );
	actionViewTogglePropertyWindow=a;
}

// Format Actions
void Main::setupFormatActions()
{
    QMenu *formatMenu = menuBar()->addMenu (tr ("F&ormat","Format menu"));

    QToolBar *tb = addToolBar( tr("Format Actions","Format Toolbar name"));
	tb->setObjectName ("formatTB");
    QAction *a;
    QPixmap pix( 16,16);
    pix.fill (Qt::black);
    a= new QAction(pix, tr( "Set &Color" )+QString("..."), this);
	a->setStatusTip ( tr( "Set Color" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectColor() ) );
    a->addTo( tb );
	formatMenu->addAction (a);
	actionFormatColor=a;
    a= new QAction( QPixmap(iconPath+"formatcolorpicker.png"), tr( "Pic&k color","Edit menu" ), this);
	a->setStatusTip (tr( "Pick color\nHint: You can pick a color from another branch and color using CTRL+Left Button" ) );
	a->setShortcut (Qt::CTRL + Qt::Key_K );
    connect( a, SIGNAL( triggered() ), this, SLOT( formatPickColor() ) );
	a->setEnabled (false);
    a->addTo( tb );
	formatMenu->addAction (a);
	actionListBranches.append(a);
	actionFormatPickColor=a;

    a= new QAction(QPixmap(iconPath+"formatcolorbranch.png"), tr( "Color &branch","Edit menu" ), this);
	a->setStatusTip ( tr( "Color branch" ) );
	a->setShortcut (Qt::CTRL + Qt::Key_B);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatColorBranch() ) );
	a->setEnabled (false);
    a->addTo( tb );
	formatMenu->addAction (a);
	actionListBranches.append(a);
	actionFormatColorSubtree=a;

    a= new QAction(QPixmap(iconPath+"formatcolorsubtree.png"), tr( "Color sub&tree","Edit menu" ), this);
	a->setStatusTip ( tr( "Color Subtree" ));
	a->setShortcut (Qt::CTRL + Qt::Key_T);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatColorSubtree() ) );
	a->setEnabled (false);
	formatMenu->addAction (a);
    a->addTo( tb );
	actionListBranches.append(a);
	actionFormatColorSubtree=a;

	formatMenu->addSeparator();
	actionGroupFormatLinkStyles=new QActionGroup ( this);
	actionGroupFormatLinkStyles->setExclusive (true);
    a= new QAction( tr( "Linkstyle Line" ), actionGroupFormatLinkStyles);
	a->setStatusTip (tr( "Line" ));
	a->setToggleAction(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStyleLine() ) );
	formatMenu->addAction (a);
	actionFormatLinkStyleLine=a;
    a= new QAction( tr( "Linkstyle Curve" ), actionGroupFormatLinkStyles);
	a->setStatusTip (tr( "Line" ));
	a->setToggleAction(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStyleParabel() ) );
	formatMenu->addAction (a);
	actionFormatLinkStyleParabel=a;
    a= new QAction( tr( "Linkstyle Thick Line" ), actionGroupFormatLinkStyles );
	a->setStatusTip (tr( "PolyLine" ));
	a->setToggleAction(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStylePolyLine() ) );
	formatMenu->addAction (a);
	actionFormatLinkStylePolyLine=a;
    a= new QAction( tr( "Linkstyle Thick Curve" ), actionGroupFormatLinkStyles);
	a->setStatusTip (tr( "PolyParabel" ) );
	a->setToggleAction(true);
	a->setChecked (true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStylePolyParabel() ) );
	formatMenu->addAction (a);
	actionFormatLinkStylePolyParabel=a;
	
    a = new QAction( tr( "Hide link if object is not selected","Branch attribute" ), this);
	a->setStatusTip (tr( "Hide link" ));
	a->setToggleAction(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatHideLinkUnselected() ) );
	actionFormatHideLinkUnselected=a;

	formatMenu->addSeparator();
    a= new QAction( tr( "&Use color of heading for link","Branch attribute" ),  this);
	a->setStatusTip (tr( "Use same color for links and headings" ));
	a->setToggleAction(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatToggleLinkColorHint() ) );
	formatMenu->addAction (a);
	actionFormatLinkColorHint=a;

    pix.fill (Qt::white);
    a= new QAction( pix, tr( "Set &Link Color"+QString("...") ), this  );
	a->setStatusTip (tr( "Set Link Color" ));
	formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectLinkColor() ) );
    actionFormatLinkColor=a;

    a= new QAction( pix, tr( "Set &Selection Color"+QString("...") ), this  );
	a->setStatusTip (tr( "Set Selection Color" ));
	formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectSelectionColor() ) );
    actionFormatSelectionColor=a;

    a= new QAction( pix, tr( "Set &Background Color" )+QString("..."), this );
	a->setStatusTip (tr( "Set Background Color" ));
	formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectBackColor() ) );
    actionFormatBackColor=a;

    a= new QAction( pix, tr( "Set &Background image" )+QString("..."), this );
	a->setStatusTip (tr( "Set Background image" ));
	formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectBackImage() ) );
    actionFormatBackImage=a;
}

// View Actions
void Main::setupViewActions()
{
    QToolBar *tb = addToolBar( tr("View Actions","View Toolbar name") );
    tb->setLabel( "View Actions" );
	tb->setObjectName ("viewTB");
    QMenu *viewMenu = menuBar()->addMenu ( tr( "&View" ));

    QAction *a;
    a = new QAction(QPixmap(iconPath+"viewmag-reset.png"), tr( "reset Zoom","View action" ), this);
	a->setStatusTip ( tr( "Zoom reset" ) );
	a->setShortcut (Qt::CTRL + Qt::Key_0 );
    a->addTo( tb );
	viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(viewZoomReset() ) );
	
    a = new QAction( QPixmap(iconPath+"viewmag+.png"), tr( "Zoom in","View action" ), this);
	a->setStatusTip (tr( "Zoom in" ));
	a->setShortcut (Qt::CTRL + Qt::Key_Plus);
    a->addTo( tb );
	viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(viewZoomIn() ) );
	
    a = new QAction( QPixmap(iconPath+"viewmag-.png"), tr( "Zoom out","View action" ), this);
	a->setStatusTip (tr( "Zoom out" ));
	a->setShortcut (Qt::CTRL + Qt::Key_Minus );
    a->addTo( tb );
	viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( viewZoomOut() ) );

    a = new QAction( QPixmap(iconPath+"viewshowsel.png"), tr( "Show selection","View action" ), this);
	a->setStatusTip (tr( "Show selection" ));
	a->setShortcut (Qt::Key_Period);
    a->addTo( tb );
	viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( viewCenter() ) );

	viewMenu->addSeparator();	

    a = new QAction(QPixmap(flagsPath+"flag-note.png"), tr( "Show Note Editor","View action" ),this);
	a->setStatusTip ( tr( "Show Note Editor" ));
	a->setShortcut ( Qt::CTRL + Qt::Key_E );
	a->setToggleAction(true);
    a->addTo( tb );
	viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleNoteEditor() ) );
	actionViewToggleNoteEditor=a;

    a = new QAction(QPixmap(iconPath+"history.png"),  tr( "History Window","View action" ),this );
	a->setStatusTip ( tr( "Show History Window" ));
	a->setShortcut ( Qt::CTRL + Qt::Key_H  );
	a->setToggleAction(true);
    a->addTo( tb );
	viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleHistory() ) );
	actionViewToggleHistoryWindow=a;

	viewMenu->addAction (actionViewTogglePropertyWindow);

	viewMenu->addSeparator();	

    a = new QAction(tr( "Antialiasing","View action" ),this );
	a->setStatusTip ( tr( "Antialiasing" ));
	a->setToggleAction(true);
	a->setOn (settings.value("/mainwindow/view/AntiAlias",true).toBool());
	viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleAntiAlias() ) );
	actionViewToggleAntiAlias=a;

    a = new QAction(tr( "Smooth pixmap transformations","View action" ),this );
	a->setStatusTip (a->text());
	a->setToggleAction(true);
	a->setOn (settings.value("/mainwindow/view/SmoothPixmapTransformation",true).toBool());
	viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleSmoothPixmap() ) );
	actionViewToggleSmoothPixmapTransform=a;

    a = new QAction(tr( "Next Map","View action" ), this);
	a->setStatusTip (a->text());
	a->setShortcut (Qt::ALT + Qt::Key_N );
	viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowNextEditor() ) );

    a = new QAction (tr( "Previous Map","View action" ), this );
	a->setStatusTip (a->text());
	a->setShortcut (Qt::ALT + Qt::Key_P );
	viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowPreviousEditor() ) );
}

// Mode Actions
void Main::setupModeActions()
{
    //QPopupMenu *menu = new QPopupMenu( this );
    //menuBar()->insertItem( tr( "&Mode (using modifiers)" ), menu );

    QToolBar *tb = addToolBar( tr ("Modes when using modifiers","Modifier Toolbar name") );
	tb->setObjectName ("modesTB");
    QAction *a;
	actionGroupModModes=new QActionGroup ( this);
	actionGroupModModes->setExclusive (true);
    a= new QAction( QPixmap(iconPath+"modecolor.png"), tr( "Use modifier to color branches","Mode modifier" ), actionGroupModModes);
	a->setShortcut (Qt::Key_J);
    a->setStatusTip ( tr( "Use modifier to color branches" ));
	a->setToggleAction(true);
	a->addTo (tb);
	a->setOn(true);
	actionModModeColor=a;
	
    a= new QAction( QPixmap(iconPath+"modecopy.png"), tr( "Use modifier to copy","Mode modifier" ), actionGroupModModes );
	a->setShortcut( Qt::Key_K); 
    a->setStatusTip( tr( "Use modifier to copy" ));
	a->setToggleAction(true);
	a->addTo (tb);
	actionModModeCopy=a;

    a= new QAction(QPixmap(iconPath+"modelink.png"), tr( "Use modifier to draw xLinks","Mode modifier" ), actionGroupModModes );
	a->setShortcut (Qt::Key_L);
    a->setStatusTip( tr( "Use modifier to draw xLinks" ));
	a->setToggleAction(true);
	a->addTo (tb);
	actionModModeXLink=a;
}

// Flag Actions
void Main::setupFlagActions()
{
	// Create System Flags
	QToolBar *tb=NULL;
	bool avis=true;

	systemFlagsDefault = new FlagRowObj ();
	systemFlagsDefault->setVisibility (false);
	systemFlagsDefault->setName ("systemFlagsDef");

	FlagObj *fo = new FlagObj ();
	fo->load(QPixmap(flagsPath+"flag-note.png"));
	setupFlag (fo,tb,avis,"note",tr("Note","SystemFlag"));

	fo->load(QPixmap(flagsPath+"flag-url.png"));
	setupFlag (fo,tb,avis,"url",tr("URL to Document ","SystemFlag"));
	
	fo->load(QPixmap(flagsPath+"flag-vymlink.png"));
	setupFlag (fo,tb,avis,"vymLink",tr("Link to another vym map","SystemFlag"));

	fo->load(QPixmap(flagsPath+"flag-scrolled-right.png"));
	setupFlag (fo,tb,avis,"scrolledright",tr("subtree is scrolled","SystemFlag"));
	
	fo->load(QPixmap(flagsPath+"flag-tmpUnscrolled-right.png"));
	setupFlag (fo,tb,avis,"tmpUnscrolledright",tr("subtree is temporary scrolled","SystemFlag"));

	fo->load(QPixmap(flagsPath+"flag-hideexport.png"));
	setupFlag (fo,tb,avis,"hideInExport",tr("Hide object in exported maps","SystemFlag"));

	// Create Standard Flags
	tb=addToolBar (tr ("Standard Flags","Standard Flag Toolbar"));
	tb->setObjectName ("standardFlagTB");

	standardFlagsDefault = new FlagRowObj ();
	standardFlagsDefault->setVisibility (false);
	standardFlagsDefault->setName ("standardFlagsDef");
	standardFlagsDefault->setToolBar (tb);

	fo->load(flagsPath+"flag-exclamationmark.png");
	fo->setGroup("standard-mark");
	setupFlag (fo,tb,avis,"exclamationmark",tr("Take care!","Standardflag"));
	
	fo->load(flagsPath+"flag-questionmark.png");
	fo->setGroup("standard-mark");
	setupFlag (fo,tb,avis,"questionmark",tr("Really?","Standardflag"));

	fo->load(flagsPath+"flag-hook-green.png");
	fo->setGroup("standard-hook");
	setupFlag (fo,tb,avis,"hook-green",tr("ok!","Standardflag"));

	fo->load(flagsPath+"flag-cross-red.png");
	fo->setGroup("standard-hook");
	setupFlag (fo,tb,avis,"cross-red",tr("Not ok!","Standardflag"));
	fo->unsetGroup();

	fo->load(flagsPath+"flag-stopsign.png");
	setupFlag (fo,tb,avis,"stopsign",tr("This won't work!","Standardflag"));

	fo->load(flagsPath+"flag-smiley-good.png");
	fo->setGroup("standard-smiley");
	setupFlag (fo,tb,avis,"smiley-good",tr("Good","Standardflag"));

	fo->load(flagsPath+"flag-smiley-sad.png");
	fo->setGroup("standard-smiley");
	setupFlag (fo,tb,avis,"smiley-sad",tr("Bad","Standardflag"));

	fo->load(flagsPath+"flag-smiley-omg.png");
	fo->setGroup("standard-smiley");
	setupFlag (fo,tb,avis,"smiley-omb",tr("Oh no!","Standardflag"));
	// Original omg.png (in KDE emoticons)
	fo->unsetGroup();

	fo->load(flagsPath+"flag-kalarm.png");
	setupFlag (fo,tb,avis,"clock",tr("Time critical","Standardflag"));

	fo->load(flagsPath+"flag-phone.png");
	setupFlag (fo,tb,avis,"phone",tr("Call...","Standardflag"));

	fo->load(flagsPath+"flag-lamp.png");
	setupFlag (fo,tb,avis,"lamp",tr("Idea!","Standardflag"));

	fo->load(flagsPath+"flag-arrow-up.png");
	fo->setGroup("standard-arrow");
	setupFlag (fo,tb,avis,"arrow-up",tr("Important","Standardflag"));

	fo->load(flagsPath+"flag-arrow-down.png");
	fo->setGroup("standard-arrow");
	setupFlag (fo,tb,avis,"arrow-down",tr("Unimportant","Standardflag"));

	fo->load(flagsPath+"flag-arrow-2up.png");
	fo->setGroup("standard-arrow");
	setupFlag (fo,tb,avis,"2arrow-up",tr("Very important!","Standardflag"));

	fo->load(flagsPath+"flag-arrow-2down.png");
	fo->setGroup("standard-arrow");
	setupFlag (fo,tb,avis,"2arrow-down",tr("Very unimportant!","Standardflag"));
	fo->unsetGroup();

	fo->load(flagsPath+"flag-thumb-up.png");
	fo->setGroup("standard-thumb");
	setupFlag (fo,tb,avis,"thumb-up",tr("I like this","Standardflag"));

	fo->load(flagsPath+"flag-thumb-down.png");
	fo->setGroup("standard-thumb");
	setupFlag (fo,tb,avis,"thumb-down",tr("I do not like this","Standardflag"));
	fo->unsetGroup();
	
	fo->load(flagsPath+"flag-rose.png");
	setupFlag (fo,tb,avis,"rose",tr("Rose","Standardflag"));

	fo->load(flagsPath+"flag-heart.png");
	setupFlag (fo,tb,avis,"heart",tr("I just love...","Standardflag"));

	fo->load(flagsPath+"flag-present.png");
	setupFlag (fo,tb,avis,"present",tr("Surprise!","Standardflag"));

	fo->load(flagsPath+"flag-flash.png");
	setupFlag (fo,tb,avis,"flash",tr("Dangerous","Standardflag"));
	
	// Original: xsldbg_output.png
	fo->load(flagsPath+"flag-info.png");
	setupFlag (fo,tb,avis,"info",tr("Info","Standardflag"));

	// Original khelpcenter.png
	fo->load(flagsPath+"flag-lifebelt.png");
	setupFlag (fo,tb,avis,"lifebelt",tr("This will help","Standardflag"));

	// Freemind flags

	avis=false;

	fo->load(flagsPath+"freemind/warning.png");
	setupFlag (fo,tb, avis, "freemind-warning",tr("Important","Freemind-Flag"));

	for (int i=1; i<8; i++)
	{
		fo->load(flagsPath+QString("freemind/priority-%1.png").arg(i));
		setupFlag (fo,tb, avis,QString("freemind-priority-%1").arg(i),tr("Priority","Freemind-Flag"));
	}

	fo->load(flagsPath+"freemind/back.png");
	setupFlag (fo,tb,avis,"freemind-back",tr("Back","Freemind-Flag"));

	fo->load(flagsPath+"freemind/forward.png");
	setupFlag (fo,tb,avis,"freemind-forward",tr("Forward","Freemind-Flag"));

	fo->load(flagsPath+"freemind/attach.png");
	setupFlag (fo,tb,avis,"freemind-attach",tr("Look here","Freemind-Flag"));

	fo->load(flagsPath+"freemind/clanbomber.png");
	setupFlag (fo,tb,avis,"freemind-clanbomber",tr("Dangerous","Freemind-Flag"));

	fo->load(flagsPath+"freemind/desktopnew.png");
	setupFlag (fo,tb,avis,"freemind-desktopnew",tr("Don't forget","Freemind-Flag"));

	fo->load(flagsPath+"freemind/flag.png");
	setupFlag (fo,tb,avis,"freemind-flag",tr("Flag","Freemind-Flag"));


	fo->load(flagsPath+"freemind/gohome.png");
	setupFlag (fo,tb,avis,"freemind-gohome",tr("Home","Freemind-Flag"));


	fo->load(flagsPath+"freemind/kaddressbook.png");
	setupFlag (fo,tb,avis,"freemind-kaddressbook",tr("Telephone","Freemind-Flag"));

	fo->load(flagsPath+"freemind/knotify.png");
	setupFlag (fo,tb,avis,"freemind-knotify",tr("Music","Freemind-Flag"));

	fo->load(flagsPath+"freemind/korn.png");
	setupFlag (fo,tb,avis,"freemind-korn",tr("Mailbox","Freemind-Flag"));

	fo->load(flagsPath+"freemind/mail.png");
	setupFlag (fo,tb,avis,"freemind-mail",tr("Maix","Freemind-Flag"));

	fo->load(flagsPath+"freemind/password.png");
	setupFlag (fo,tb,avis,"freemind-password",tr("Password","Freemind-Flag"));

	fo->load(flagsPath+"freemind/pencil.png");
	setupFlag (fo,tb,avis,"freemind-pencil",tr("To be improved","Freemind-Flag"));

	fo->load(flagsPath+"freemind/stop.png");
	setupFlag (fo,tb,avis,"freemind-stop",tr("Stop","Freemind-Flag"));

	fo->load(flagsPath+"freemind/wizard.png");
	setupFlag (fo,tb,avis,"freemind-wizard",tr("Magic","Freemind-Flag"));

	fo->load(flagsPath+"freemind/xmag.png");
	setupFlag (fo,tb,avis,"freemind-xmag",tr("To be discussed","Freemind-Flag"));

	fo->load(flagsPath+"freemind/bell.png");
	setupFlag (fo,tb,avis,"freemind-bell",tr("Reminder","Freemind-Flag"));

	fo->load(flagsPath+"freemind/bookmark.png");
	setupFlag (fo,tb,avis,"freemind-bookmark",tr("Excellent","Freemind-Flag"));

	fo->load(flagsPath+"freemind/penguin.png");
	setupFlag (fo,tb,avis,"freemind-penguin",tr("Linux","Freemind-Flag"));

	fo->load(flagsPath+"freemind/licq.png");
	setupFlag (fo,tb,avis,"freemind-licq",tr("Sweet","Freemind-Flag"));

	delete (fo);
}

void Main::setupFlag (FlagObj *fo, QToolBar *tb, bool aw, const QString &name, const QString &tooltip)
{
	fo->setName(name);
	fo->setToolTip (tooltip);
	QAction *a=new QAction (fo->getPixmap(),fo->getName(),this);
	if (tb)
	{
		// StandardFlag
		tb->addAction (a);
		fo->setAction (a);
		fo->setAlwaysVisible(aw);
		a->setCheckable(true);
		a->setObjectName(fo->getName());
		a->setToolTip(tooltip);
		connect (a, SIGNAL( triggered() ), this, SLOT( standardFlagChanged() ) );
		standardFlagsDefault->addFlag (fo);	
	} else
	{
		// SystemFlag
		systemFlagsDefault->addFlag (fo);	
	}
}
// Network Actions
void Main::setupNetworkActions()
{
	if (!settings.value( "/mainwindow/showTestMenu",false).toBool() ) 
		return;
    QMenu *netMenu = menuBar()->addMenu(  "Network" );

	QAction *a;

    a = new QAction(  "Start TCPserver for MapEditor",this);
    //a->setStatusTip ( "Set application to open pdf files"));
	a->setShortcut ( Qt::Key_T );		//New TCP server
    connect( a, SIGNAL( triggered() ), this, SLOT( networkStartServer() ) );
	netMenu->addAction (a);

    a = new QAction(  "Connect MapEditor to server",this);
    //a->setStatusTip ( "Set application to open pdf files"));
	a->setShortcut ( Qt::Key_C );		// Connect to server
    connect( a, SIGNAL( triggered() ), this, SLOT( networkConnect() ) );
	netMenu->addAction (a);
}
	
// Settings Actions
void Main::setupSettingsActions()
{
    QMenu *settingsMenu = menuBar()->addMenu( tr( "&Settings" ));

	QAction *a;

    a = new QAction( tr( "Set application to open pdf files","Settings action"), this);
    a->setStatusTip ( tr( "Set application to open pdf files"));
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsPDF() ) );
	settingsMenu->addAction (a);

    a = new QAction( tr( "Set application to open external links","Settings action"), this);
    a->setStatusTip( tr( "Set application to open external links"));
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsURL() ) );
	settingsMenu->addAction (a);

    a = new QAction( tr( "Set path for macros","Settings action")+"...", this);
    a->setStatusTip( tr( "Set path for macros"));
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsMacroDir() ) );
	settingsMenu->addAction (a);

    a = new QAction( tr( "Set number of undo levels","Settings action")+"...", this);
    a->setStatusTip( tr( "Set number of undo levels"));
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsUndoLevels() ) );
	settingsMenu->addAction (a);

	settingsMenu->addSeparator();

    a = new QAction( tr( "Autosave","Settings action"), this);
    a->setStatusTip( tr( "Autosave"));
	a->setToggleAction(true);
	a->setOn ( settings.value ("/mainwindow/autosave/use",false).toBool());
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsAutosaveToggle() ) );
	settingsMenu->addAction (a);
	actionSettingsAutosaveToggle=a;

    a = new QAction( tr( "Autosave time","Settings action")+"...", this);
    a->setStatusTip( tr( "Autosave time"));
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsAutosaveTime() ) );
	settingsMenu->addAction (a);
	actionSettingsAutosaveTime=a;

    a = new QAction( tr( "Write backup file on save","Settings action"), this);
    a->setStatusTip( tr( "Write backup file on save"));
	a->setToggleAction(true);
	a->setOn ( settings.value ("/mainwindow/writeBackupFile",false).toBool());
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsWriteBackupFileToggle() ) );
	settingsMenu->addAction (a);
	actionSettingsWriteBackupFile=a;

	settingsMenu->addSeparator();

    a = new QAction( tr( "Edit branch after adding it","Settings action" ), this );
    a->setStatusTip( tr( "Edit branch after adding it" ));
	a->setToggleAction(true);
	a->setOn ( settings.value ("/mapeditor/editmode/autoEditNewBranch",true).toBool());
	settingsMenu->addAction (a);
	actionSettingsAutoEditNewBranch=a;

    a= new QAction( tr( "Select branch after adding it","Settings action" ), this );
    a->setStatusTip( tr( "Select branch after adding it" ));
	a->setToggleAction(true);
	a->setOn ( settings.value ("/mapeditor/editmode/autoSelectNewBranch",false).toBool() );
	settingsMenu->addAction (a);
	actionSettingsAutoSelectNewBranch=a;
	
    a= new QAction(tr( "Select existing heading","Settings action" ), this);
    a->setStatusTip( tr( "Select heading before editing" ));
	a->setToggleAction(true);
	a->setOn ( settings.value ("/mapeditor/editmode/autoSelectText",true).toBool() );
	settingsMenu->addAction (a);
	actionSettingsAutoSelectText=a;
	
    a= new QAction( tr( "Delete key","Settings action" ), this);
    a->setStatusTip( tr( "Delete key for deleting branches" ));
	a->setToggleAction(true);
	a->setOn ( settings.value ("/mapeditor/editmode/useDelKey",true).toBool() );
	settingsMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsToggleDelKey() ) );
	actionSettingsUseDelKey=a;

    a= new QAction( tr( "Exclusive flags","Settings action" ), this);
    a->setStatusTip( tr( "Use exclusive flags in flag toolbars" ));
	a->setToggleAction(true);
	a->setOn ( settings.value ("/mapeditor/editmode/useFlagGroups",true).toBool() );
	settingsMenu->addAction (a);
	actionSettingsUseFlagGroups=a;
	
    a= new QAction( tr( "Use hide flags","Settings action" ), this);
    a->setStatusTip( tr( "Use hide flag during exports " ));
	a->setToggleAction(true);
	a->setOn ( settings.value ("/export/useHideExport",true).toBool() );
	settingsMenu->addAction (a);
	actionSettingsUseHideExport=a;

    a = new QAction( tr( "Animation","Settings action"), this);
    a->setStatusTip( tr( "Animation"));
	a->setToggleAction(true);
	a->setOn (settings.value("/animation/use",false).toBool() );
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsToggleAnimation() ) );
	if (settings.value( "/mainwindow/showTestMenu",false).toBool() )
	{
		settingsMenu->addAction (a);
	}	
	actionSettingsUseAnimation=a;
}

// Test Actions
void Main::setupTestActions()
{
    QMenu *testMenu = menuBar()->addMenu( tr( "&Test" ));

    QAction *a;
    a = new QAction( "Test function 1" , this);
    a->setStatusTip( "Call test function 1" );
	testMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( testFunction1() ) );

    a = new QAction( "Test function 2" , this);
    a->setStatusTip( "Call test function 2" );
	testMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( testFunction2() ) );

    a = new QAction( "Command" , this);
    a->setStatusTip( "Enter command to call in editor" );
    connect( a, SIGNAL( triggered() ), this, SLOT( testCommand() ) );
	testMenu->addAction (a);
}

// Help Actions
void Main::setupHelpActions()
{
    QMenu *helpMenu = menuBar()->addMenu ( tr( "&Help","Help menubar entry" ));

    QAction *a;
    a = new QAction(  tr( "Open VYM Documentation (pdf) ","Help action" ), this );
    a->setStatusTip( tr( "Open VYM Documentation (pdf)" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( helpDoc() ) );
	helpMenu->addAction (a);

    a = new QAction(  tr( "Open VYM example maps ","Help action" ), this );
    a->setStatusTip( tr( "Open VYM example maps " ));
    connect( a, SIGNAL( triggered() ), this, SLOT( helpDemo() ) );
	helpMenu->addAction (a);

    a = new QAction( tr( "About VYM","Help action" ), this);
    a->setStatusTip( tr( "About VYM")+vymName);
    connect( a, SIGNAL( triggered() ), this, SLOT( helpAbout() ) );
	helpMenu->addAction (a);

    a = new QAction( tr( "About QT","Help action" ), this);
    a->setStatusTip( tr( "Information about QT toolkit" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( helpAboutQT() ) );
	helpMenu->addAction (a);
}

// Context Menus
void Main::setupContextMenus()
{
	QAction*a;

	// Context Menu for branch or mapcenter
	branchContextMenu =new QMenu (this);
	branchContextMenu->addAction (actionViewTogglePropertyWindow);
	branchContextMenu->addSeparator();	

		// Submenu "Add"
		branchAddContextMenu =branchContextMenu->addMenu (tr("Add"));
		branchAddContextMenu->addAction (actionEditPaste );
		branchAddContextMenu->addAction ( actionEditAddBranch );
		branchAddContextMenu->addAction ( actionEditAddBranchBefore );
		branchAddContextMenu->addAction ( actionEditAddBranchAbove);
		branchAddContextMenu->addAction ( actionEditAddBranchBelow );
		branchAddContextMenu->addSeparator();	
		branchAddContextMenu->addAction ( actionEditImportAdd );
		branchAddContextMenu->addAction ( actionEditImportReplace );

		// Submenu "Remove"
		branchRemoveContextMenu =branchContextMenu->addMenu (tr ("Remove","Context menu name"));
		branchRemoveContextMenu->addAction (actionEditCut);
		branchRemoveContextMenu->addAction ( actionEditDelete );
		branchRemoveContextMenu->addAction ( actionEditDeleteKeepChilds );
		branchRemoveContextMenu->addAction ( actionEditDeleteChilds );
		

	actionEditSaveBranch->addTo( branchContextMenu );
	actionFileNewCopy->addTo (branchContextMenu );

	branchContextMenu->addSeparator();	
	branchContextMenu->addAction ( actionEditLoadImage);

	// Submenu for Links (URLs, vymLinks)
	branchLinksContextMenu =new QMenu (this);

		branchContextMenu->addSeparator();	
		branchLinksContextMenu=branchContextMenu->addMenu(tr("References (URLs, vymLinks, ...)","Context menu name"));	
		branchLinksContextMenu->addAction ( actionEditOpenURL );
		branchLinksContextMenu->addAction ( actionEditOpenURLTab );
		branchLinksContextMenu->addAction ( actionEditOpenMultipleURLTabs );
		branchLinksContextMenu->addAction ( actionEditURL );
		branchLinksContextMenu->addAction ( actionEditLocalURL );
		branchLinksContextMenu->addAction ( actionEditHeading2URL );
		branchLinksContextMenu->addAction ( actionEditBugzilla2URL );
		if (settings.value( "/mainwindow/showTestMenu",false).toBool() )
		{
			branchLinksContextMenu->addAction ( actionEditFATE2URL );
		}	
		branchLinksContextMenu->addSeparator();	
		branchLinksContextMenu->addAction ( actionEditOpenVymLink );
		branchLinksContextMenu->addAction ( actionEditOpenMultipleVymLinks );
		branchLinksContextMenu->addAction ( actionEditVymLink );
		branchLinksContextMenu->addAction ( actionEditDeleteVymLink );
		

	// Context Menu for XLinks in a branch menu
	// This will be populated "on demand" in MapEditor::updateActions
	branchContextMenu->addSeparator();	
	branchXLinksContextMenuEdit =branchContextMenu->addMenu (tr ("Edit XLink","Context menu name"));
	branchXLinksContextMenuFollow =branchContextMenu->addMenu (tr ("Follow XLink","Context menu name"));
	connect( branchXLinksContextMenuFollow, SIGNAL( triggered(QAction *) ), this, SLOT( editFollowXLink(QAction * ) ) );
	connect( branchXLinksContextMenuEdit, SIGNAL( triggered(QAction *) ), this, SLOT( editEditXLink(QAction * ) ) );
 	
	
	// Context menu for floatimage
	floatimageContextMenu =new QMenu (this);
	a= new QAction (tr ("Save image","Context action"),this);
	connect (a, SIGNAL (triggered()), this, SLOT (editSaveImage()));
	floatimageContextMenu->addAction (a);

	floatimageContextMenu->addSeparator();	
	actionEditCopy->addTo( floatimageContextMenu );
	actionEditCut->addTo( floatimageContextMenu );

	floatimageContextMenu->addSeparator();	
	floatimageContextMenu->addAction ( actionFormatHideLinkUnselected );

	
	// Context menu for canvas
	canvasContextMenu =new QMenu (this);
	actionEditMapInfo->addTo( canvasContextMenu );
	if (settings.value( "/mainwindow/showTestMenu",false).toBool() ) 
		actionEditAddMapCenter->addTo( canvasContextMenu );
	canvasContextMenu->insertSeparator();	
	actionGroupFormatLinkStyles->addTo( canvasContextMenu );
	canvasContextMenu->insertSeparator();	
	actionFormatLinkColorHint->addTo( canvasContextMenu );
	actionFormatLinkColor->addTo( canvasContextMenu );
	actionFormatSelectionColor->addTo( canvasContextMenu );
	actionFormatBackColor->addTo( canvasContextMenu );
	// actionFormatBackImage->addTo( canvasContextMenu );  //FIXME makes vym too slow: postponed for later version 

	// Menu for last opened files
	// Create actions
	for (int i = 0; i < MaxRecentFiles; ++i) 
	{
        recentFileActions[i] = new QAction(this);
        recentFileActions[i]->setVisible(false);
        fileLastMapsMenu->addAction(recentFileActions[i]);
        connect(recentFileActions[i], SIGNAL(triggered()),
                this, SLOT(fileLoadRecent()));
    }
	setupRecentMapsMenu();
}

void Main::setupRecentMapsMenu()
{
    QStringList files = settings.value("/mainwindow/recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(files[i]);
        recentFileActions[i]->setText(text);
        recentFileActions[i]->setData(files[i]);
        recentFileActions[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActions[j]->setVisible(false);
}

void Main::setupMacros()
{
    for (int i = 0; i <= 11; i++) 
	{
        macroActions[i] = new QAction(this);
        macroActions[i]->setData(i);
        addAction (macroActions[i]);
        connect(macroActions[i], SIGNAL(triggered()),
                this, SLOT(callMacro()));
	}			
	macroActions[0]->setShortcut ( Qt::Key_F1 );
	macroActions[1]->setShortcut ( Qt::Key_F2 );
	macroActions[2]->setShortcut ( Qt::Key_F3 );
	macroActions[3]->setShortcut ( Qt::Key_F4 );
	macroActions[4]->setShortcut ( Qt::Key_F5 );
	macroActions[5]->setShortcut ( Qt::Key_F6 );
	macroActions[6]->setShortcut ( Qt::Key_F7 );
	macroActions[7]->setShortcut ( Qt::Key_F8 );
	macroActions[8]->setShortcut ( Qt::Key_F9 );
	macroActions[9]->setShortcut ( Qt::Key_F10 );
	macroActions[10]->setShortcut ( Qt::Key_F11 );
	macroActions[11]->setShortcut ( Qt::Key_F12 );
}

void Main::hideEvent (QHideEvent * )
{
	if (!textEditor->isMinimized() ) textEditor->hide();
}

void Main::showEvent (QShowEvent * )
{
	if (actionViewToggleNoteEditor->isOn()) textEditor->showNormal();
}


MapEditor* Main::currentMapEditor() const
{
    if ( tabWidget->currentPage() &&
	 tabWidget->currentPage()->inherits( "MapEditor" ) )
		return (MapEditor*)tabWidget->currentPage();
    return NULL;	
}


void Main::editorChanged(QWidget *)
{
	// Unselect all possibly selected objects
	// (Important to update note editor)
	MapEditor *me;
	for (int i=0;i<=tabWidget->count() -1;i++)
	{
		me=(MapEditor*)tabWidget->page(i);
		me->unselect();
	}	
	me=currentMapEditor();
	if (me) me->reselect();

	// Update actions to in menus and toolbars according to editor
	updateActions();
}

void Main::fileNew()
{
	QString fn="unnamed";
	MapEditor* me = new MapEditor ( NULL);
	tabWidget->addTab (me,fn);
	tabWidget->showPage(me);
	me->viewport()->setFocus();
	me->setAntiAlias (actionViewToggleAntiAlias->isOn());
	me->setSmoothPixmap(actionViewToggleSmoothPixmapTransform->isOn());
	
	// For the very first map we do not have flagrows yet...
	me->select("mc:");
}

void Main::fileNewCopy()
{
	QString fn="unnamed";
	MapEditor* oldME =currentMapEditor();
	if (oldME)
	{
		oldME->copy();
		MapEditor* newME = new MapEditor ( NULL);
		if (newME)
		{
			tabWidget->addTab (newME,fn);
			tabWidget->showPage(newME);
			newME->viewport()->setFocus();
			newME->setAntiAlias (actionViewToggleAntiAlias->isOn());
			newME->setSmoothPixmap(actionViewToggleSmoothPixmapTransform->isOn());
			// For the very first map we do not have flagrows yet...
			newME->select("mc:");
			newME->load (clipboardDir+"/"+clipboardFile,ImportReplace, VymMap);
		}

	}
}

ErrorCode Main::fileLoad(QString fn, const LoadMode &lmode, const FileType &ftype)
{
	ErrorCode err=success;
	
	// fn is usually the archive, mapfile the file after uncompressing
	QString mapfile;

	// Make fn absolute (needed for unzip)
	fn=QDir (fn).absPath();

	MapEditor *me;

	if (lmode==NewMap)
	{
		// Check, if map is already loaded
		int i=0;
		while (i<=tabWidget->count() -1)
		{
			me=(MapEditor*)tabWidget->page(i);
			if (me->getFilePath() == fn)
			{
				// Already there, ask for confirmation
				QMessageBox mb( vymName,
					tr("The map %1\nis already opened."
					"Opening the same map in multiple editors may lead \n"
					"to confusion when finishing working with vym."
					"Do you want to").arg(fn),
					QMessageBox::Warning,
					QMessageBox::Yes | QMessageBox::Default,
					QMessageBox::Cancel | QMessageBox::Escape,
					QMessageBox::NoButton);
				mb.setButtonText( QMessageBox::Yes, tr("Open anyway") );
				mb.setButtonText( QMessageBox::Cancel, tr("Cancel"));
				switch( mb.exec() ) 
				{
					case QMessageBox::Yes:
						// load anyway
						i=tabWidget->count();
						break;
					case QMessageBox::Cancel:
						// do nothing
						return aborted;
						break;
				}
			}
			i++;
		}
	}


	// Try to load map
    if ( !fn.isEmpty() )
	{
		me = currentMapEditor();
		int tabIndex=tabWidget->currentPageIndex();
		// Check first, if mapeditor exists
		// If it is not default AND we want a new map, 
		// create a new mapeditor in a new tab
		if ( lmode==NewMap && (!me || !me->isDefault() ) )
		{
			me= new MapEditor ( NULL);
			tabWidget->addTab (me,fn);
			tabIndex=tabWidget->indexOf (me);
			tabWidget->setCurrentPage (tabIndex);
			me->setAntiAlias (actionViewToggleAntiAlias->isOn());
			me->setSmoothPixmap(actionViewToggleSmoothPixmapTransform->isOn());
		}
		
		// Check, if file exists (important for creating new files
		// from command line
		/*
		*/
		if (!QFile(fn).exists() )
		{
			QMessageBox mb( vymName,
				tr("This map does not exist:\n  %1\nDo you want to create a new one?").arg(fn),
				QMessageBox::Question,
				QMessageBox::Yes ,
				QMessageBox::Cancel | QMessageBox::Default,
				QMessageBox::NoButton );

			mb.setButtonText( QMessageBox::Yes, tr("Create"));
			mb.setButtonText( QMessageBox::No, tr("Cancel"));
			switch( mb.exec() ) 
			{
				case QMessageBox::Yes:
					// Create new map
					currentMapEditor()->setFilePath(fn);
					tabWidget->setTabLabel (currentMapEditor(),
						currentMapEditor()->getFileName() );
					statusBar()->message( "Created " + fn , statusbarTime );
					return success;
						
				case QMessageBox::Cancel:
					// don't create new map
					statusBar()->message( "Loading " + fn + " failed!", statusbarTime );
					fileCloseMap();
					return aborted;
			}
		}	

		//tabWidget->currentPage() won't be NULL here, because of above...
		tabWidget->showPage(me);
		me->viewport()->setFocus();

		if (err!=aborted)
		{
			// Save existing filename in case  we import
			QString fn_org=me->getFilePath();

			// Finally load map into mapEditor
			me->setFilePath (fn);
			err=me->load(fn,lmode,ftype);

			// Restore old (maybe empty) filepath, if this is an import
			if (lmode!=NewMap)
				me->setFilePath (fn_org);
		}	

		// Finally check for errors and go home
		if (err==aborted) 
		{
			if (lmode==NewMap) fileCloseMap();
			statusBar()->message( "Could not load " + fn, statusbarTime );
		} else 
		{
			if (lmode==NewMap)
			{
				me->setFilePath (fn);
				tabWidget->changeTab(tabWidget->page(tabIndex), me->getFileName());
				if (!isInTmpDir (fn))
				{
					// Only append to lastMaps if not loaded from a tmpDir
					// e.g. imported bookmarks are in a tmpDir
					addRecentMap(me->getFilePath() );
				}
				actionFilePrint->setEnabled (true);
			}	
			statusBar()->message( "Loaded " + fn, statusbarTime );
		}	
	}
	return err;
}


void Main::fileLoad(const LoadMode &lmode)
{
	QStringList filters;
	filters <<"VYM map (*.vym *.vyp)"<<"XML (*.xml)";
	QFileDialog *fd=new QFileDialog( this);
	fd->setDir (lastFileDir);
	fd->setFileMode (QFileDialog::ExistingFiles);
	fd->setFilters (filters);
	switch (lmode)
	{
		case NewMap:
			fd->setCaption(vymName+ " - " +tr("Load vym map"));
			break;
		case ImportAdd:
			fd->setCaption(vymName+ " - " +tr("Import: Add vym map to selection"));
			break;
		case ImportReplace:
			fd->setCaption(vymName+ " - " +tr("Import: Replace selection with vym map"));
			break;
	}
	fd->show();

	QString fn;
	if ( fd->exec() == QDialog::Accepted )
	{
		lastFileDir=fd->directory().path();
	    QStringList flist = fd->selectedFiles();
		QStringList::Iterator it = flist.begin();
		while( it != flist.end() ) 
		{
			fn = *it;
			fileLoad(*it, lmode);				   
			++it;
		}
	}
	delete (fd);
}

void Main::fileLoad()
{
	fileLoad (NewMap);
}

void Main::fileLoadRecent()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        fileLoad (action->data().toString(), NewMap);
}

void Main::addRecentMap (const QString &fileName)
{

    QStringList files = settings.value("/mainwindow/recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles)
        files.removeLast();

    settings.setValue("/mainwindow/recentFileList", files);

	setupRecentMapsMenu();
}

void Main::fileSave(MapEditor *me, const SaveMode &savemode)
{
	if (!me) return;

	if ( me->getFilePath().isEmpty() ) 
	{
		// We have  no filepath yet,
		// call fileSaveAs() now, this will call fileSave() 
		// again.
		// First switch to editor
		tabWidget->setCurrentWidget (me);
		fileSaveAs(savemode);
	}

	if (me->save (savemode)==success)
	{
		statusBar()->message( 
			tr("Saved  %1").arg(me->getFilePath()), 
			statusbarTime );
		addRecentMap (me->getFilePath() );
	} else		
		statusBar()->message( 
			tr("Couldn't save ").arg(me->getFilePath()), 
			statusbarTime );
}

void Main::fileSave()
{
	fileSave (currentMapEditor(), CompleteMap);
}

void Main::fileSave(MapEditor *me)
{
	fileSave (me,CompleteMap);
}

void Main::fileSaveAs(const SaveMode& savemode)
{
	QString fn;

	if (currentMapEditor())
	{
		if (savemode==CompleteMap)
			fn = Q3FileDialog::getSaveFileName( QString::null, "VYM map (*.vym)", this );
		else		
			fn = Q3FileDialog::getSaveFileName( QString::null, "VYM part of map (*.vyp)", this );
		if ( !fn.isEmpty() ) 
		{
			// Check for existing file
			if (QFile (fn).exists())
			{
				QMessageBox mb( vymName,
					tr("The file %1\nexists already. Do you want to").arg(fn),
					QMessageBox::Warning,
					QMessageBox::Yes | QMessageBox::Default,
					QMessageBox::Cancel | QMessageBox::Escape,
					QMessageBox::NoButton);
				mb.setButtonText( QMessageBox::Yes, tr("Overwrite") );
				mb.setButtonText( QMessageBox::Cancel, tr("Cancel"));
				switch( mb.exec() ) 
				{
					case QMessageBox::Yes:
						// save 
						break;
					case QMessageBox::Cancel:
						// do nothing
						return;
						break;
				}
			} else
			{
				// New file, add extension to filename, if missing
				// This is always .vym or .vyp, depending on savemode
				if (savemode==CompleteMap)
				{
					if (!fn.contains (".vym") && !fn.contains (".xml"))
						fn +=".vym";
				} else		
				{
					if (!fn.contains (".vyp") && !fn.contains (".xml"))
						fn +=".vyp";
				}
			}
	



			// Save now
			currentMapEditor()->setFilePath(fn);
			fileSave(currentMapEditor(), savemode);

			// Set name of tab
			if (savemode==CompleteMap)
				tabWidget->setTabLabel (currentMapEditor(),
					currentMapEditor()->getFileName() );
			return;
		} 
	}
}

void Main::fileSaveAs()
{
	fileSaveAs (CompleteMap);
}

void Main::fileImportKDE3Bookmarks()
{
	ImportKDE3Bookmarks im;
	im.transform();
	if (aborted!=fileLoad (im.getTransformedFile(),NewMap) && currentMapEditor() )
		currentMapEditor()->setFilePath ("");
}

void Main::fileImportKDE4Bookmarks()
{
	ImportKDE4Bookmarks im;
	im.transform();
	if (aborted!=fileLoad (im.getTransformedFile(),NewMap) && currentMapEditor() )
		currentMapEditor()->setFilePath ("");
}

void Main::fileImportFirefoxBookmarks()
{
	Q3FileDialog *fd=new Q3FileDialog( this);
	fd->setDir (vymBaseDir.homeDirPath()+"/.mozilla/firefox");
	fd->setMode (Q3FileDialog::ExistingFiles);
	fd->addFilter ("Firefox "+tr("Bookmarks")+" (*.html)");
	fd->setCaption(tr("Import")+" "+"Firefox "+tr("Bookmarks"));
	fd->show();

	if ( fd->exec() == QDialog::Accepted )
	{
		ImportFirefoxBookmarks im;
	    QStringList flist = fd->selectedFiles();
		QStringList::Iterator it = flist.begin();
		while( it != flist.end() ) 
		{
			im.setFile (*it);
			if (im.transform() && 
				aborted!=fileLoad (im.getTransformedFile(),NewMap,FreemindMap) && 
				currentMapEditor() )
				currentMapEditor()->setFilePath ("");
			++it;
		}
	}
	delete (fd);
}

void Main::fileImportFreemind()
{
	QStringList filters;
	filters <<"Freemind map (*.mm)"<<"All files (*)";
	QFileDialog *fd=new QFileDialog( this);
	fd->setDir (lastFileDir);
	fd->setFileMode (QFileDialog::ExistingFiles);
	fd->setFilters (filters);
	fd->setCaption(vymName+ " - " +tr("Load Freemind map"));
	fd->show();

	QString fn;
	if ( fd->exec() == QDialog::Accepted )
	{
		lastFileDir=fd->directory().path();
	    QStringList flist = fd->selectedFiles();
		QStringList::Iterator it = flist.begin();
		while( it != flist.end() ) 
		{
			fn = *it;
			if ( fileLoad (fn,NewMap, FreemindMap)  )
			{	
				currentMapEditor()->setFilePath ("");
			}	
			++it;
		}
	}
	delete (fd);
}


void Main::fileImportMM()
{
	ImportMM im;

	Q3FileDialog *fd=new Q3FileDialog( this);
	fd->setDir (lastFileDir);
	fd->setMode (Q3FileDialog::ExistingFiles);
	fd->addFilter ("Mind Manager (*.mmap)");
	fd->setCaption(tr("Import")+" "+"Mind Manager");
	fd->show();

	if ( fd->exec() == QDialog::Accepted )
	{
		lastFileDir=fd->dirPath();
	    QStringList flist = fd->selectedFiles();
		QStringList::Iterator it = flist.begin();
		while( it != flist.end() ) 
		{
			im.setFile (*it);
			if (im.transform() && 
				success==fileLoad (im.getTransformedFile(),NewMap) && 
				currentMapEditor() )
				currentMapEditor()->setFilePath ("");

			++it;
		}
	}
	delete (fd);

}

void Main::fileImportDir()
{
	if (currentMapEditor())
		currentMapEditor()->importDir();	
}

void Main::fileExportXML()	
{
	MapEditor *me=currentMapEditor();
	if (me) me->exportXML();
}


void Main::fileExportXHTML()	
{
	MapEditor *me=currentMapEditor();
	if (me) me->exportXHTML();
}

void Main::fileExportImage()	
{
	MapEditor *me=currentMapEditor();
	if (me) me->exportImage();
}

void Main::fileExportASCII()
{
	MapEditor *me=currentMapEditor();
	if (me) me->exportASCII();
}

void Main::fileExportCSV()	//FIXME not scriptable yet
{
	MapEditor *me=currentMapEditor();
	if (me)
	{
		ExportCSV ex;
		ex.setModel (me->getModel());
		ex.addFilter ("CSV (*.csv)");
		ex.setDir(lastImageDir);
		ex.setCaption(vymName+ " -" +tr("Export as CSV")+" "+tr("(still experimental)"));
		if (ex.execDialog() ) 
		{
			me->setExportMode(true);
			ex.doExport();
			me->setExportMode(false);
		}
	}
}

void Main::fileExportLaTeX()	//FIXME not scriptable yet
{
	MapEditor *me=currentMapEditor();
	if (me)
	{
		ExportLaTeX ex;
		ex.setModel (me->getModel());
		ex.addFilter ("Tex (*.tex)");
		ex.setDir(lastImageDir);
		ex.setCaption(vymName+ " -" +tr("Export as LaTeX")+" "+tr("(still experimental)"));
		if (ex.execDialog() ) 
		{
			me->setExportMode(true);
			ex.doExport();
			me->setExportMode(false);
		}
	}
}

void Main::fileExportKDE3Bookmarks()	//FIXME not scriptable yet
{
	ExportKDE3Bookmarks ex;
	MapEditor *me=currentMapEditor();
	if (me)
	{
		ex.setModel (me->getModel());
		ex.doExport();
	}	
}

void Main::fileExportKDE4Bookmarks()	//FIXME not scriptable yet
{
	ExportKDE4Bookmarks ex;
	MapEditor *me=currentMapEditor();
	if (me)
	{
		ex.setModel (me->getModel());
		ex.doExport();
	}	
}

void Main::fileExportTaskjuggler()	//FIXME not scriptable yet
{
	ExportTaskjuggler ex;
	MapEditor *me=currentMapEditor();
	if (me)
	{
		ex.setModel (me->getModel());
		ex.setCaption ( vymName+" - "+tr("Export to")+" Taskjuggler"+tr("(still experimental)"));
		ex.setDir(lastImageDir);
		ex.addFilter ("Taskjuggler (*.tjp)");
		if (ex.execDialog() ) 
		{
			me->setExportMode(true);
			ex.doExport();
			me->setExportMode(false);
		}
	}	
}

void Main::fileExportOOPresentation()	//FIXME not scriptable yet
{
	ExportOOFileDialog *fd=new ExportOOFileDialog( this,vymName+" - "+tr("Export to")+" Open Office");
	// TODO add preview in dialog
	//ImagePreview *p =new ImagePreview (fd);
	//fd->setContentsPreviewEnabled( TRUE );
	//fd->setContentsPreview( p, p );
	//fd->setPreviewMode( QFileDialog::Contents );
	fd->setCaption(vymName+" - " +tr("Export to")+" Open Office");
	fd->setDir (QDir().current());
	if (fd->foundConfig())
	{
		fd->show();

		if ( fd->exec() == QDialog::Accepted )
		{
			QString fn=fd->selectedFile();
			if (!fn.contains (".odp"))
				fn +=".odp";

			//lastImageDir=fn.left(fn.findRev ("/"));
			if (currentMapEditor())
				currentMapEditor()->exportOOPresentation(fn,fd->selectedConfig());	
		}
	} else
	{
		QMessageBox::warning(0, 
		tr("Warning"),
		tr("Couldn't find configuration for export to Open Office\n"));
	}
}

void Main::fileCloseMap()
{
	MapEditor *me = currentMapEditor();
	if (me)
	{
		if (me->hasChanged())
		{
			QMessageBox mb( vymName,
				tr("The map %1 has been modified but not saved yet. Do you want to").arg(me->getFileName()),
				QMessageBox::Warning,
				QMessageBox::Yes | QMessageBox::Default,
				QMessageBox::No,
				QMessageBox::Cancel | QMessageBox::Escape );
			mb.setButtonText( QMessageBox::Yes, tr("Save modified map before closing it") );
			mb.setButtonText( QMessageBox::No, tr("Discard changes"));
			switch( mb.exec() ) 
			{
				case QMessageBox::Yes:
					// save and close
					fileSave(me, CompleteMap);
					break;
				case QMessageBox::No:
				// close  without saving
					break;
				case QMessageBox::Cancel:
					// do nothing
				return;
			}
		} 
		//me->closeMap(); 
		tabWidget->removePage(me);
		if (tabWidget->count()==0)
			actionFilePrint->setEnabled (false);

        //Remove mapEditor;
		// Better would be delete (me), but then we have a Qt error:
		// "QObject: Do not delete object, 'MapEditor', during its event handler!"
		// So we only remove data now and call deconstructor when vym closes later
		me->clear();
	}
}

void Main::filePrint()
{
	if (currentMapEditor())
		currentMapEditor()->print();
}

void Main::fileExitVYM()
{
	// Check if one or more editors have changed
	MapEditor *me;
	int i;
	for (i=0;i<=tabWidget->count() -1;i++)
	{
		
		me=(MapEditor*)tabWidget->page(i);

		// If something changed, ask what to do
		if (me->hasChanged())
		{
			tabWidget->setCurrentPage(i);
			QMessageBox mb( vymName,
				tr("This map is not saved yet. Do you want to"),
				QMessageBox::Warning,
				QMessageBox::Yes | QMessageBox::Default,
				QMessageBox::No,
				QMessageBox::Cancel | QMessageBox::Escape );
			mb.setButtonText( QMessageBox::Yes, tr("Save map") );
			mb.setButtonText( QMessageBox::No, tr("Discard changes") );
			mb.setModal (true);
			mb.show();
			mb.setActiveWindow();
			switch( mb.exec() ) {
				case QMessageBox::Yes:
					// save (the changed editors) and exit
					fileSave(currentMapEditor(), CompleteMap);
					break;
				case QMessageBox::No:
					// exit without saving
					break;
				case QMessageBox::Cancel:
					// don't save and don't exit
				return;
			}
		}
		me->clear();	// remove and unselect now, before mainWindow has gone...
	} // loop over all MEs	
    qApp->quit();
}

void Main::editUndo()
{
	if (currentMapEditor())
		currentMapEditor()->undo();
}

void Main::editRedo()	   
{
	if (currentMapEditor())
		currentMapEditor()->redo();
}

void Main::gotoHistoryStep (int i)	   
{
	if (currentMapEditor())
		currentMapEditor()->gotoHistoryStep (i);
}

void Main::editCopy()
{
	if (currentMapEditor())
		currentMapEditor()->copy();
}

void Main::editPaste()
{
	if (currentMapEditor())
		currentMapEditor()->paste();
}

void Main::editCut()
{
	if (currentMapEditor())
		currentMapEditor()->cut();
}

void Main::editOpenFindWindow()
{
	findWindow->popup();
	findWindow->raise();
	findWindow->setActiveWindow();
}

void Main::editFind(QString s)
{
	bool cs=false;
	BranchObj *bo=currentMapEditor()->findText(s, cs);
	if (bo)
	{	
		statusBar()->message( "Found: " + bo->getHeading(), statusbarTime );
	} else
	{
		QMessageBox::information( findWindow, tr( "VYM -Information:" ),
							   tr("No matches found for \"%1\"").arg(s));
	}	
}

void Main::editFindChanged()
{	// Notify editor, to abort the current find process
	currentMapEditor()->findReset();
}

void Main::openTabs(QStringList urls)
{
	if (!urls.isEmpty())
	{	
		bool success=true;
		QStringList args;
		QString browser=settings.value("/mainwindow/readerURL" ).toString();
		QProcess *p;
		if (!procBrowser ||  procBrowser->state()!=QProcess::Running)
		{
			QString u=urls.takeFirst();
			procBrowser = new QProcess( this );
			args<<u;
			procBrowser->start(browser,args);
			if ( !procBrowser->waitForStarted())
			{
				// try to set path to browser
				QMessageBox::warning(0, 
					tr("Warning"),
					tr("Couldn't find a viewer to open %1.\n").arg(u)+
					tr("Please use Settings->")+tr("Set application to open an URL"));
				return;
			}
#if defined(Q_OS_WIN32)
            // There's no sleep in VCEE, replace it with Qt's QThread::wait().
            this->thread()->wait(3000);
#else
			sleep (3);
#endif
		}
		if (browser.contains("konqueror"))
		{
		cout << "Main::openTabs urls="<<urls.join(" ").toStdString()<<endl;
			for (int i=0; i<urls.size(); i++)
			{
				// Open new browser
				// Try to open new tab in existing konqueror started previously by vym
				p=new QProcess (this);
				args.clear();
#if defined(Q_OS_WIN32)
                // In Win32, pid is not a longlong, but a pointer to a _PROCESS_INFORMATION structure.
                // Redundant change in Win32, as there's no konqueror, but I wanted to follow the original logic.
				args<< QString("konqueror-%1").arg(procBrowser->pid()->dwProcessId)<<
					"konqueror-mainwindow#1"<<
					"newTab" <<
					urls.at(i);
#else
				args<< QString("konqueror-%1").arg(procBrowser->pid())<<
					"konqueror-mainwindow#1"<<
					"newTab" <<
					urls.at(i);
					// qdbus org.kde.konqueror-15367 /konqueror/MainWindow_1 newTab www.suse.de false
#endif
				p->start ("dcop",args);
				if (debug) 
					cout << "MainWindo::openURLs  i="<<i <<"/"<<urls.size()-1 
					<<" args="<<args.join(" ").toStdString()<<endl;

				if ( !p->waitForStarted() ) success=false;
			}
			if (!success)
				QMessageBox::warning(0, 
					tr("Warning"),
					tr("Couldn't start %1 to open a new tab in %2.").arg("dcop").arg("konqueror"));
			return;		
		} else if (browser.contains ("firefox") || browser.contains ("mozilla") )
		{
			for (int i=0; i<urls.size(); i++)
			{
				// Try to open new tab in firefox
				p=new QProcess (this);
				args<< "-remote"<< QString("openurl(%1,new-tab)").arg(urls.at(i));
				p->start (browser,args);
				if ( !p->waitForStarted() ) success=false;
			}			
			if (!success)
				QMessageBox::warning(0, 
					tr("Warning"),
					tr("Couldn't start %1 to open a new tab").arg(browser));
			return;		
		}			
		QMessageBox::warning(0, 
			tr("Warning"),
			tr("Sorry, currently only Konqueror and Mozilla support tabbed browsing."));
	}	
}

void Main::editOpenURL()
{
	// Open new browser
	if (currentMapEditor())
	{	
	    QString url=currentMapEditor()->getURL();
		QStringList args;
		if (url=="") return;
		QString browser=settings.value("/mainwindow/readerURL" ).toString();
		procBrowser = new QProcess( this );
		args<<url;
		procBrowser->start(browser,args);
		if ( !procBrowser->waitForStarted())
		{
			// try to set path to browser
			QMessageBox::warning(0, 
				tr("Warning"),
				tr("Couldn't find a viewer to open %1.\n").arg(url)+
				tr("Please use Settings->")+tr("Set application to open an URL"));
			settingsURL() ; 
		}	
	}	
}
void Main::editOpenURLTab()
{
	if (currentMapEditor())
	{	
	    QStringList urls;
		urls.append(currentMapEditor()->getURL());
		openTabs (urls);
	}	
}
void Main::editOpenMultipleURLTabs()
{
	if (currentMapEditor())
	{	
	    QStringList urls;
		urls=currentMapEditor()->getURLs();

		openTabs (urls);
	}	
}


void Main::editURL()
{
	if (currentMapEditor())
	    currentMapEditor()->editURL();
}

void Main::editLocalURL()
{
	if (currentMapEditor())
	    currentMapEditor()->editLocalURL();
}

void Main::editHeading2URL()
{
	if (currentMapEditor())
	    currentMapEditor()->editHeading2URL();
}

void Main::editBugzilla2URL()
{
	if (currentMapEditor())
	    currentMapEditor()->editBugzilla2URL();
}

void Main::editFATE2URL()
{
	if (currentMapEditor())
	    currentMapEditor()->editFATE2URL();
}

void Main::editHeadingFinished()
{
	// only called from editHeading(), so there is a currentME
	MapEditor *me=currentMapEditor();
	if (me)
	{
		me->setStateEditHeading (false);
		QPoint p;	//Not used here, only to find out pos of branch
		bool ok;
		QString s=me->getHeading(ok,p);

#if defined(Q_OS_MACX) || defined(Q_OS_WIN32)
#else
		if (ok && s!=lineedit->text())
			me->setHeading(lineedit->text());
			
		lineedit->releaseKeyboard();
		lineedit->hide();
		setFocus();
#endif	
		if (!actionSettingsAutoSelectNewBranch->isOn() && 
			!prevSelection.isEmpty()) 
			me->select(prevSelection);
		prevSelection="";
	}
}

void Main::editHeading()
{
	if (currentMapEditor())
	{
		MapEditor *me=currentMapEditor();
		QString oldSel=me->getSelectString();

		if (lineedit->isVisible())
			editHeadingFinished();
		else
		{
			bool ok;
			QPoint p;
			QString s=me->getHeading(ok,p);

			if (ok)
			{
				me->setStateEditHeading (true);
#if defined(Q_OS_MACX) || defined(Q_OS_WIN32)
				p=me->mapToGlobal (p);
				QDialog *d =new QDialog(NULL);
				QLineEdit *le=new QLineEdit (d);
				d->setWindowFlags (Qt::FramelessWindowHint);
				d->setGeometry(p.x(),p.y(),230,25);
				le->resize (d->width()-10,d->height());
				le->setText (s);
				le->selectAll();
				connect (le, SIGNAL (returnPressed()), d, SLOT (accept()));
				d->activateWindow();
				d->exec();
				me->setHeading (le->text());
				delete (le);
				delete (d);
				editHeadingFinished();
#else
				p=me->mapTo (this,p);
				lineedit->setGeometry(p.x(),p.y(),230,25);
				lineedit->setText(s);
				lineedit->setCursorPosition(1);
				lineedit->selectAll();
				lineedit->show();
				lineedit->grabKeyboard();
				lineedit->setFocus();
#endif
			}
		}
	} // currentMapEditor()	
}

void Main::editAttributeFinished()
{
	// only called from editHeading(), so there is a currentME

	/*
	MapEditor *me=currentMapEditor();
	if (me)
	{
		me->setStateEditHeading (false);
		QPoint p;	//Not used here, only to find out pos of branch
		bool ok;
		QString s=me->getHeading(ok,p);

#if defined(Q_OS_MACX)
#else
		if (ok && s!=lineedit->text())
			me->setHeading(lineedit->text());
			
		lineedit->releaseKeyboard();
		lineedit->hide();
		setFocus();
#endif	
		if (!actionSettingsAutoSelectNewBranch->isOn() && 
			!prevSelection.isEmpty()) 
			me->select(prevSelection);
		prevSelection="";
	}
	*/
}

#include "attribute.h"
#include "attributedialog.h"
void Main::editAttribute()
{
	MapEditor *me=currentMapEditor();
	if (me)
	{
		BranchObj *bo=me->getSelectedBranch();
		if (bo)
		{
			AttributeDialog dia(this);
			dia.setTable (me->attributeTable() );
			dia.setBranch (bo);
			dia.setMode (Definition);
			dia.updateTable();
			dia.exec();
		}
	}	
	/*
	if (currentMapEditor())
	{
		MapEditor *me=currentMapEditor();
		QString oldSel=me->getSelectString();

		if (lineedit->isVisible())
			editAttributeFinished();
		else
		{
			bool ok;
			QPoint p;
			QString s=me->getHeading(ok,p);

			if (ok)
			{
				me->setStateEditHeading (true);
#if defined(Q_OS_MACX)
				p=me->mapToGlobal (p);
				QDialog *d =new QDialog(NULL);
				QLineEdit *le=new QLineEdit (d);
				d->setWindowFlags (Qt::FramelessWindowHint);
				d->setGeometry(p.x(),p.y(),230,25);
				le->resize (d->width()-10,d->height());
				le->setText (s);
				le->selectAll();
				connect (le, SIGNAL (returnPressed()), d, SLOT (accept()));
				d->activateWindow();
				d->exec();
				me->setHeading (le->text());
				delete (le);
				delete (d);
				editHeadingFinished();
#else
				p=me->mapTo (this,p);
				lineedit->setGeometry(p.x(),p.y(),230,25);
				lineedit->setText(s);
				lineedit->setCursorPosition(1);
				lineedit->selectAll();
				lineedit->show();
				lineedit->grabKeyboard();
				lineedit->setFocus();
#endif
			}
		} 
	} // currentMapEditor()	

	*/
}

void Main::openVymLinks(const QStringList &vl)
{
	for (int j=0; j<vl.size(); j++)
	{
		// compare path with already loaded maps
		int index=-1;
		int i;
		MapEditor *me;
		for (i=0;i<=tabWidget->count() -1;i++)
		{
			me=(MapEditor*)tabWidget->page(i);
			if (vl.at(j)==me->getFilePath() )
			{
				index=i;
				break;
			}
		}	
		if (index<0)
		// Load map
		{
			if (!QFile(vl.at(j)).exists() )
				QMessageBox::critical( 0, tr( "Critical Error" ),
				   tr("Couldn't open map %1").arg(vl.at(j)));
			else
			{
				fileLoad (vl.at(j), NewMap);
				tabWidget->setCurrentPage (tabWidget->count()-1);	
			}
		} else
			// Go to tab containing the map
			tabWidget->setCurrentPage (index);	
	}
}

void Main::editOpenVymLink()
{
	if (currentMapEditor())
	{
		QStringList vl;
		vl.append(currentMapEditor()->getVymLink());	
		openVymLinks (vl);
	}
}

void Main::editOpenMultipleVymLinks()
{
	QString currentVymLink;
	if (currentMapEditor())
	{
		QStringList vl=currentMapEditor()->getVymLinks();
		openVymLinks (vl);
	}
}

void Main::editVymLink()
{
	if (currentMapEditor())
		currentMapEditor()->editVymLink();	
}

void Main::editDeleteVymLink()
{
	if (currentMapEditor())
		currentMapEditor()->deleteVymLink();	
}

void Main::editToggleHideExport()
{
	if (currentMapEditor())
		currentMapEditor()->toggleHideExport();	
}

void Main::editMapInfo()
{
	if (currentMapEditor())
		currentMapEditor()->editMapInfo();	
}

void Main::editMoveUp()
{
	if (currentMapEditor())
	    currentMapEditor()->moveBranchUp();
}

void Main::editMoveDown()
{
	if (currentMapEditor())
		currentMapEditor()->moveBranchDown();
}

void Main::editSortChildren()
{
	if (currentMapEditor())
		currentMapEditor()->sortChildren();
}

void Main::editToggleScroll()
{
	if (currentMapEditor())
	{
		currentMapEditor()->toggleScroll();	
	}	
}

void Main::editUnscrollChilds()
{
	if (currentMapEditor())
		currentMapEditor()->unscrollChilds();	
}

void Main::editAddMapCenter()
{
	MapEditor *me=currentMapEditor();
	if (!lineedit->isVisible() && me)
	{
		me->addMapCenter ();
	}	
}

void Main::editNewBranch()
{
	MapEditor *me=currentMapEditor();
	if (!lineedit->isVisible() && me)
	{
		BranchObj *bo=(BranchObj*)me->getSelection();
		BranchObj *newbo=me->addNewBranch(0);

		prevSelection=me->getModel()->getSelectString(bo);
		if (newbo) 
			me->select (newbo);
		else
			return;

		if (actionSettingsAutoEditNewBranch->isOn())
		{
			editHeading();
			return;
		}	
		if (!prevSelection.isEmpty()) 
		{
			me->select(prevSelection);
			prevSelection="";
		}

	}	
}

void Main::editNewBranchBefore()
{
	MapEditor *me=currentMapEditor();
	if (!lineedit->isVisible() && me)
	{
		BranchObj *bo=(BranchObj*)me->getSelection();
		BranchObj *newbo=me->addNewBranchBefore();

		if (newbo) 
			me->select (newbo);
		else
			return;

		if (actionSettingsAutoEditNewBranch->isOn())
		{
			if (!actionSettingsAutoSelectNewBranch->isOn())
				prevSelection=me->getModel()->getSelectString(bo); //TODO access directly
			editHeading();
		}
	}	
}

void Main::editNewBranchAbove()
{
	MapEditor *me=currentMapEditor();
	if (!lineedit->isVisible() && me)
	{
		BranchObj *bo=(BranchObj*)me->getSelection();
		BranchObj *newbo=me->addNewBranch (-1);

		if (newbo) 
			me->select (newbo);
		else
			return;

		if (actionSettingsAutoEditNewBranch->isOn())
		{
			if (!actionSettingsAutoSelectNewBranch->isOn())
				prevSelection=me->getModel()->getSelectString (bo);	// TODO access directly
			editHeading();
		}
	}	
}

void Main::editNewBranchBelow()
{
	MapEditor *me=currentMapEditor();
	if (!lineedit->isVisible() && me)
	{
		BranchObj *bo=(BranchObj*)me->getSelection();
		BranchObj *newbo=me->addNewBranch (1);

		if (newbo) 
			me->select (newbo);
		else
			return;

		if (actionSettingsAutoEditNewBranch->isOn())
		{
			if (!actionSettingsAutoSelectNewBranch->isOn())
				prevSelection=me->getModel()->getSelectString(bo);	//TODO access directly
			editHeading();
		}
	}	
}

void Main::editImportAdd()
{
	fileLoad (ImportAdd);
}

void Main::editImportReplace()
{
	fileLoad (ImportReplace);
}

void Main::editSaveBranch()
{
	fileSaveAs (PartOfMap);
}

void Main::editDeleteKeepChilds()
{
	if (currentMapEditor())
		currentMapEditor()->deleteKeepChilds();
}

void Main::editDeleteChilds()
{
	if (currentMapEditor())
		currentMapEditor()->deleteChilds();
}

void Main::editDeleteSelection()
{
	if (currentMapEditor() && actionSettingsUseDelKey->isOn())
		currentMapEditor()->deleteSelection();
}

void Main::editUpperBranch()
{
	if (currentMapEditor())
		currentMapEditor()->selectUpperBranch();
}

void Main::editLowerBranch()
{
	if (currentMapEditor())
		currentMapEditor()->selectLowerBranch();
}

void Main::editLeftBranch()
{
	if (currentMapEditor())
		currentMapEditor()->selectLeftBranch();
}

void Main::editRightBranch()
{
	if (currentMapEditor())
		currentMapEditor()->selectRightBranch();
}

void Main::editFirstBranch()
{
	if (currentMapEditor())
		currentMapEditor()->selectFirstBranch();
}

void Main::editLastBranch()
{
	if (currentMapEditor())
		currentMapEditor()->selectLastBranch();
}

void Main::editLoadImage()
{
	if (currentMapEditor())
		currentMapEditor()->loadFloatImage();
}

void Main::editSaveImage()
{
	if (currentMapEditor())
		currentMapEditor()->saveFloatImage();
}

void Main::editFollowXLink(QAction *a)
{

	if (currentMapEditor())
		currentMapEditor()->followXLink(branchXLinksContextMenuFollow->actions().indexOf(a));
}

void Main::editEditXLink(QAction *a)
{
	if (currentMapEditor())
		currentMapEditor()->editXLink(branchXLinksContextMenuEdit->actions().indexOf(a));
}

void Main::formatSelectColor()
{
	if (currentMapEditor())
	{
		QColor col = QColorDialog::getColor((currentColor ), this );
		if ( !col.isValid() ) return;
		colorChanged( col );
	}	
}

void Main::formatPickColor()
{
	if (currentMapEditor())
		colorChanged( currentMapEditor()->getCurrentHeadingColor() );
}

void Main::colorChanged(QColor c)
{
    QPixmap pix( 16, 16 );
    pix.fill( c );
    actionFormatColor->setIconSet( pix );
	currentColor=c;
}

void Main::formatColorBranch()
{
	if (currentMapEditor())
		currentMapEditor()->colorBranch(currentColor);
}

void Main::formatColorSubtree()
{
	if (currentMapEditor())
		currentMapEditor()->colorSubtree (currentColor);
}

void Main::formatLinkStyleLine()
{
	if (currentMapEditor())
    {
		currentMapEditor()->setMapLinkStyle("StyleLine");
        actionFormatLinkStyleLine->setOn(true);
    }
}

void Main::formatLinkStyleParabel()
{
	if (currentMapEditor())
    {
		currentMapEditor()->setMapLinkStyle("StyleParabel");
        actionFormatLinkStyleParabel->setOn(true);
    }
}

void Main::formatLinkStylePolyLine()
{
	if (currentMapEditor())
    {
		currentMapEditor()->setMapLinkStyle("StylePolyLine");
        actionFormatLinkStylePolyLine->setOn(true);
    }
}

void Main::formatLinkStylePolyParabel()
{
	if (currentMapEditor())
    {
		currentMapEditor()->setMapLinkStyle("StylePolyParabel");
        actionFormatLinkStylePolyParabel->setOn(true);
    }
}

void Main::formatSelectBackColor()
{
	if (currentMapEditor())
		currentMapEditor()->selectMapBackgroundColor();
}

void Main::formatSelectBackImage()
{
	if (currentMapEditor())
		currentMapEditor()->selectMapBackgroundImage();
}

void Main::formatSelectLinkColor()
{
	if (currentMapEditor())
		currentMapEditor()->selectMapLinkColor();
}

void Main::formatSelectSelectionColor()
{
	if (currentMapEditor())
		currentMapEditor()->selectMapSelectionColor();
}

void Main::formatToggleLinkColorHint()
{
	currentMapEditor()->toggleMapLinkColorHint();
}


void Main::formatHideLinkUnselected()	//FIXME get rid of this with imagepropertydialog
{
	if (currentMapEditor())
		currentMapEditor()->setHideLinkUnselected(actionFormatHideLinkUnselected->isOn());
}

void Main::viewZoomReset()
{
	if (currentMapEditor())
	{
		QMatrix m;
		m.reset();
		currentMapEditor()->setMatrix( m );
	}	
}

void Main::viewZoomIn()
{
	if (currentMapEditor())
	{
		QMatrix m = currentMapEditor()->matrix();
		m.scale( 1.25, 1.25 );
		currentMapEditor()->setMatrix( m );
	}	
}

void Main::viewZoomOut()
{
	if (currentMapEditor())
	{
		QMatrix m = currentMapEditor()->matrix();
		m.scale( 0.8, 0.8 );
		currentMapEditor()->setMatrix( m );
	}	
}

void Main::viewCenter()
{
	MapEditor *me=currentMapEditor();
	if (me)
	{
		me->ensureSelectionVisible();
	}	
}

void Main::networkStartServer()
{
	MapEditor *me=currentMapEditor();
	if (me) me->newServer();
}

void Main::networkConnect()
{
	MapEditor *me=currentMapEditor();
	if (me) me->connectToServer();
}

bool Main::settingsPDF()
{
	// Default browser is set in constructor
	bool ok;
	QString text = QInputDialog::getText(
		"VYM", tr("Set application to open PDF files")+":", QLineEdit::Normal,
		settings.value("/mainwindow/readerPDF").toString(), &ok, this );
	if (ok)
		settings.setValue ("/mainwindow/readerPDF",text);
	return ok;
}


bool Main::settingsURL()
{
	// Default browser is set in constructor
	bool ok;
	QString text = QInputDialog::getText(
		"VYM", tr("Set application to open an URL")+":", QLineEdit::Normal,
		settings.value("/mainwindow/readerURL").toString()
		, &ok, this );
	if (ok)
		settings.setValue ("/mainwindow/readerURL",text);
	return ok;
}

void Main::settingsMacroDir()
{
	QDir defdir(vymBaseDir.path() + "/macros");
	if (!defdir.exists())
		defdir=vymBaseDir;
	QDir dir=QFileDialog::getExistingDirectory (
		this,
		tr ("Directory with vym macros:"), 
		settings.value ("/macros/macroDir",defdir.path()).toString()
	);
	if (dir.exists())
		settings.setValue ("/macros/macroDir",dir.absolutePath());
}

void Main::settingsUndoLevels()
{
	bool ok;
	int i = QInputDialog::getInteger(
		this, 
		tr("QInputDialog::getInteger()"),
	    tr("Number of undo/redo levels:"), settings.value("/mapeditor/stepsTotal").toInt(), 0, 1000, 1, &ok);
	if (ok)
	{
		settings.setValue ("/mapeditor/stepsTotal",i);
		QMessageBox::information( this, tr( "VYM -Information:" ),
		   tr("Settings have been changed. The next map opened will have \"%1\" undo/redo levels").arg(i)); 
   }	
}

void Main::settingsAutosaveToggle()
{
	settings.setValue ("/mainwindow/autosave/use",actionSettingsAutosaveToggle->isOn() );
}

void Main::settingsAutosaveTime()
{
	bool ok;
	int i = QInputDialog::getInteger(
		this, 
		tr("QInputDialog::getInteger()"),
	    tr("Number of seconds before autosave:"), settings.value("/mainwindow/autosave/ms").toInt() / 1000, 10, 10000, 1, &ok);
	if (ok)
		settings.setValue ("/mainwindow/autosave/ms",i * 1000);
}

void Main::settingsWriteBackupFileToggle()
{
	settings.setValue ("/mainwindow/writeBackupFile",actionSettingsWriteBackupFile->isOn() );
}

void Main::settingsToggleAnimation()
{
	settings.setValue ("/animation/use",actionSettingsUseAnimation->isOn() );
}

void Main::settingsToggleDelKey()
{
	if (actionSettingsUseDelKey->isOn())
	{
		actionEditDelete->setAccel (QKeySequence (Qt::Key_Delete));
	} else
	{
		actionEditDelete->setAccel (QKeySequence (""));
	}
}

void Main::windowToggleNoteEditor()
{
	if (textEditor->isVisible() )
		windowHideNoteEditor();
	else
		windowShowNoteEditor();
}

void Main::windowToggleHistory()
{
	if (historyWindow->isVisible())
		historyWindow->hide();
	else	
		historyWindow->show();

}

void Main::windowToggleProperty()
{
	if (branchPropertyWindow->isVisible())
		branchPropertyWindow->hide();
	else	
		branchPropertyWindow->show();

	if(currentMapEditor())
	{
		BranchObj *bo=currentMapEditor()->getSelectedBranch();
		if (bo) 
		{
			branchPropertyWindow->setMapEditor(currentMapEditor());
			branchPropertyWindow->setBranch(bo);
			return;
		}
		
	}	
	branchPropertyWindow->setBranch(NULL);
}

void Main::windowToggleAntiAlias()
{
	bool b=actionViewToggleAntiAlias->isOn();
	MapEditor *me;
	for (int i=0;i<tabWidget->count();i++)
	{
		
		me=(MapEditor*)tabWidget->page(i);
		me->setAntiAlias(b);
	}	

}

void Main::windowToggleSmoothPixmap()
{
	bool b=actionViewToggleSmoothPixmapTransform->isOn();
	MapEditor *me;
	for (int i=0;i<tabWidget->count();i++)
	{
		
		me=(MapEditor*)tabWidget->page(i);
		me->setSmoothPixmap(b);
	}	
}

void Main::updateHistory(SimpleSettings &undoSet)
{
	historyWindow->update (undoSet);
}

void Main::updateNoteFlag()
{
	if (currentMapEditor())
		currentMapEditor()->updateNoteFlag();
}

void Main::updateSatellites(MapEditor *me)
{
	branchPropertyWindow->setMapEditor (me);
}

void Main::updateActions()
{
	MapEditor *me=currentMapEditor();
	if (!me) return;

	historyWindow->setCaption (vymName + " - " +tr("History for %1","Window Caption").arg(currentMapEditor()->getFileName()));

	// updateActions is also called when NoteEditor is closed
	actionViewToggleNoteEditor->setOn (textEditor->isVisible());
	actionViewToggleHistoryWindow->setOn (historyWindow->isVisible());
	actionViewTogglePropertyWindow->setOn (branchPropertyWindow->isVisible());

	if (me->getMapLinkColorHint()==LinkableMapObj::HeadingColor) 
		actionFormatLinkColorHint->setOn(true);
	else	
		actionFormatLinkColorHint->setOn(false);

	switch (me->getMapLinkStyle())
	{
		case LinkableMapObj::Line: 
			actionFormatLinkStyleLine->setOn(true);
			break;
		case LinkableMapObj::Parabel:
			actionFormatLinkStyleParabel->setOn(true);
			break;
		case LinkableMapObj::PolyLine:	
			actionFormatLinkStylePolyLine->setOn(true);
			break;
		case LinkableMapObj::PolyParabel:	
			actionFormatLinkStylePolyParabel->setOn(true);
			break;
		default:
			break;
	}	

	// Update colors
	QPixmap pix( 16, 16 );
    pix.fill( me->getMapBackgroundColor() );
    actionFormatBackColor->setIconSet( pix );
    pix.fill( me->getSelectionColor() );
    actionFormatSelectionColor->setIconSet( pix );
    pix.fill( me->getMapDefLinkColor() );
    actionFormatLinkColor->setIconSet( pix );


	actionFileSave->setEnabled( me->hasChanged() );
	if (me->isUndoAvailable())
		actionEditUndo->setEnabled( true);
	else	
		actionEditUndo->setEnabled( false);

	if (me->isRedoAvailable())
		actionEditRedo->setEnabled( true);
	else	
		actionEditRedo->setEnabled( false);

	LinkableMapObj *selection=me->getSelection();
	if (selection)
	{
		if ( (typeid(*selection) == typeid(BranchObj)) || 
			(typeid(*selection) == typeid(MapCenterObj))  )
		{
			BranchObj *bo=(BranchObj*)selection;
			// Take care of links
			if (bo->countXLinks()==0)
			{
				branchXLinksContextMenuEdit->clear();
				branchXLinksContextMenuFollow->clear();
			} else
			{
				BranchObj *bot;
				QString s;
				branchXLinksContextMenuEdit->clear();
				branchXLinksContextMenuFollow->clear();
				for (int i=0; i<=bo->countXLinks();i++)
				{
					bot=bo->XLinkTargetAt(i);
					if (bot)
					{
						s=bot->getHeading();
						if (s.length()>xLinkMenuWidth)
							s=s.left(xLinkMenuWidth)+"...";
						branchXLinksContextMenuFollow->addAction (s);
						branchXLinksContextMenuEdit->addAction (s);
					}	
				}
			}

			standardFlagsDefault->setEnabled (true);

			actionEditToggleScroll->setEnabled (true);
			if ( bo->isScrolled() )
				actionEditToggleScroll->setOn(true);
			else	
				actionEditToggleScroll->setOn(false);

			if ( bo->getURL().isEmpty() )
			{
				actionEditOpenURL->setEnabled (false);
				actionEditOpenURLTab->setEnabled (false);
			}	
			else	
			{
				actionEditOpenURL->setEnabled (true);
				actionEditOpenURLTab->setEnabled (true);
			}
			if ( bo->getVymLink().isEmpty() )
			{
				actionEditOpenVymLink->setEnabled (false);
				actionEditDeleteVymLink->setEnabled (false);
			} else	
			{
				actionEditOpenVymLink->setEnabled (true);
				actionEditDeleteVymLink->setEnabled (true);
			}	

			if (bo->canMoveBranchUp()) 
				actionEditMoveUp->setEnabled (true);
			else	
				actionEditMoveUp->setEnabled (false);
			if (bo->canMoveBranchDown()) 
				actionEditMoveDown->setEnabled (true);
			else	
				actionEditMoveDown->setEnabled (false);


			actionEditToggleHideExport->setEnabled (true);	
			actionEditToggleHideExport->setOn (bo->hideInExport() );	

			actionEditCopy->setEnabled (true);	
			actionEditCut->setEnabled (true);	
			if (!clipboardEmpty)
				actionEditPaste->setEnabled (true);	
			else	
				actionEditPaste->setEnabled (false);	
			for (int i=0; i<actionListBranches.size(); ++i)	
				actionListBranches.at(i)->setEnabled(true);
			actionEditDelete->setEnabled (true);
			actionFormatHideLinkUnselected->setOn
				(selection->getHideLinkUnselected());
		}
		if ( (typeid(*selection) == typeid(FloatImageObj)) )
		{
			FloatObj *fo=(FloatImageObj*)selection;

			actionEditOpenURL->setEnabled (false);
			actionEditOpenVymLink->setEnabled (false);
			actionEditDeleteVymLink->setEnabled (false);	
			actionEditToggleHideExport->setEnabled (true);	
			actionEditToggleHideExport->setOn (fo->hideInExport() );	


			actionEditCopy->setEnabled (true);
			actionEditCut->setEnabled (true);	
			actionEditPaste->setEnabled (false);
			for (int i=0; i<actionListBranches.size(); ++i)	
				actionListBranches.at(i)->setEnabled(false);
			actionEditDelete->setEnabled (true);
			actionFormatHideLinkUnselected->setOn
				( selection->getHideLinkUnselected());
			actionEditMoveUp->setEnabled (false);
			actionEditMoveDown->setEnabled (false);
		}

	} else
	{
		actionEditCopy->setEnabled (false);	
		actionEditCut->setEnabled (false);	
		actionEditPaste->setEnabled (false);	
		for (int i=0; i<actionListBranches.size(); ++i)	
			actionListBranches.at(i)->setEnabled(false);

		actionEditToggleScroll->setEnabled (false);
		actionEditOpenURL->setEnabled (false);
		actionEditOpenVymLink->setEnabled (false);
		actionEditDeleteVymLink->setEnabled (false);	
		actionEditHeading2URL->setEnabled (false);	
		actionEditDelete->setEnabled (false);
		actionEditMoveUp->setEnabled (false);
		actionEditMoveDown->setEnabled (false);
		actionEditToggleHideExport->setEnabled (false);	
	}	
}

Main::ModMode Main::getModMode()
{
	if (actionModModeColor->isOn()) return ModModeColor;
	if (actionModModeCopy->isOn()) return ModModeCopy;
	if (actionModModeXLink->isOn()) return ModModeXLink;
	return ModModeNone;
}

bool Main::autoEditNewBranch()
{
	return actionSettingsAutoEditNewBranch->isOn();
}

bool Main::autoSelectNewBranch()
{
	return actionSettingsAutoSelectNewBranch->isOn();
}

bool Main::useFlagGroups()
{
	return actionSettingsUseFlagGroups->isOn();
}

void Main::windowShowNoteEditor()
{
	textEditor->setShowWithMain(true);
	textEditor->show();
	actionViewToggleNoteEditor->setOn (true);
}

void Main::windowHideNoteEditor()
{
	textEditor->setShowWithMain(false);
	textEditor->hide();
	actionViewToggleNoteEditor->setOn (false);
}

void Main::setScript (const QString &script)
{
	scriptEditor->setScript (script);
}

void Main::runScript (const QString &script)
{
	if (currentMapEditor())
		currentMapEditor()->runScript (script);
}

void Main::runScriptEverywhere (const QString &script)
{
	MapEditor *me;
	for (int i=0;i<=tabWidget->count() -1;i++)
	{
		me=(MapEditor*)tabWidget->page(i);
		if (me) me->runScript (script);
	}	
}

void Main::windowNextEditor()
{
	if (tabWidget->currentPageIndex() < tabWidget->count())
		tabWidget->setCurrentPage (tabWidget->currentPageIndex() +1);
}

void Main::windowPreviousEditor()
{
	if (tabWidget->currentPageIndex() >0)
		tabWidget->setCurrentPage (tabWidget->currentPageIndex() -1);
}

void Main::standardFlagChanged()
{
	if (currentMapEditor())
		currentMapEditor()->toggleStandardFlag(sender()->name());
}

void Main::testFunction1()
{
	if (!currentMapEditor()) return;
	currentMapEditor()->testFunction1();
	//editAttribute();
}

void Main::testFunction2()
{
	if (!currentMapEditor()) return;
	currentMapEditor()->testFunction2();
}

void Main::testCommand()
{
	if (!currentMapEditor()) return;
	scriptEditor->show();
	/*
	bool ok;
	QString com = QInputDialog::getText(
			vymName, "Enter Command:", QLineEdit::Normal,"command", &ok, this );
	if (ok) currentMapEditor()->parseAtom(com);
	*/
}

void Main::helpDoc()
{
	QString locale = QLocale::system().name();
	QString docname;
	if (locale.left(2)=="es")
		docname="vym_es.pdf";
	else	
		docname="vym.pdf";

	QStringList searchList;
	QDir docdir;
	#if defined(Q_OS_MACX)
		searchList << "./vym.app/Contents/Resources/doc";
    #elif defined(Q_OS_WIN32)
        searchList << vymInstallDir.path() + "/share/doc/packages/vym";
	#else
		#if defined(VYM_DOCDIR)
			searchList << VYM_DOCDIR;
		#endif
		// default path in SUSE LINUX
		searchList << "/usr/share/doc/packages/vym";
	#endif

	searchList << "doc";	// relative path for easy testing in tarball
	searchList << "doc/tex";	// Easy testing working on vym.tex
	searchList << "/usr/share/doc/vym-legacy";
	searchList << "/usr/share/doc/vym";	// Debian
	searchList << "/usr/share/doc/packages";// Knoppix

	bool found=false;
	QFile docfile;
	for (int i=0; i<searchList.count(); ++i)
	{
		docfile.setFileName(searchList.at(i)+"/"+docname);
		if (docfile.exists())
		{
			found=true;
			break;
		}	
	}

	if (!found)
	{
		QMessageBox::critical(0, 
			tr("Critcal error"),
			tr("Couldn't find the documentation %1 in:\n%2").arg(searchList.join("\n")));
		return;
	}	

	QStringList args;
	Process *pdfProc = new Process();
    args << QDir::toNativeSeparators(docfile.fileName());

	pdfProc->start( settings.value("/mainwindow/readerPDF").toString(),args);
	if ( !pdfProc->waitForStarted() ) 
	{
		// error handling
		QMessageBox::warning(0, 
			tr("Warning"),
			tr("Couldn't find a viewer to open %1.\n").arg(docfile.fileName())+
			tr("Please use Settings->")+tr("Set application to open PDF files"));
		settingsPDF();	
		return;
	}
}


void Main::helpDemo()
{
	QStringList filters;
	filters <<"VYM example map (*.vym)";
	QFileDialog *fd=new QFileDialog( this);
	#if defined(Q_OS_MACX)
		fd->setDir (QDir("./vym.app/Contents/Resources/demos"));
	#else
		// default path in SUSE LINUX
		fd->setDir (QDir(vymBaseDir.path()+"/demos"));
	#endif

	fd->setFileMode (QFileDialog::ExistingFiles);
	fd->setFilters (filters);
	fd->setCaption(vymName+ " - " +tr("Load vym example map"));
	fd->show();

	QString fn;
	if ( fd->exec() == QDialog::Accepted )
	{
		lastFileDir=fd->directory().path();
	    QStringList flist = fd->selectedFiles();
		QStringList::Iterator it = flist.begin();
		while( it != flist.end() ) 
		{
			fn = *it;
			fileLoad(*it, NewMap);				   
			++it;
		}
	}
	delete (fd);
}


void Main::helpAbout()
{
	AboutDialog ad;
	ad.setName ("aboutwindow");
	ad.setMinimumSize(500,500);
	ad.resize (QSize (500,500));
	ad.exec();
}

void Main::helpAboutQT()
{
	QMessageBox::aboutQt( this, "Qt Application Example" );
}

void Main::callMacro ()
{
    QAction *action = qobject_cast<QAction *>(sender());
	int i=-1;
    if (action)
	{
        i=action->data().toInt();
		QString mDir (settings.value ("macros/macroDir").toString() );

		QString fn=mDir + QString("/macro-%1.vys").arg(i+1);
		QFile f (fn);
		if ( !f.open( QIODevice::ReadOnly ) )
		{
			QMessageBox::warning(0, 
				tr("Warning"),
				tr("Couldn't find a macro at  %1.\n").arg(fn)+
				tr("Please use Settings->")+tr("Set directory for vym macros"));
			return;
		}	

		QTextStream ts( &f );
		QString m= ts.read();

		if (! m.isEmpty())
		{
			//cout <<"Main::callMacro  m="<<qPrintable (m)<<endl;
			currentMapEditor()->runScript (m);
		}	
	}	
}



//////////////////////////////////
/*
@@ -2544,18 +2576,27 @@
 				// Try to open new tab in existing konqueror started previously by vym
 				p=new QProcess (this);
 				args.clear();
-				args<< QString("konqueror-%1").arg(procBrowser->pid())<< 
-					"konqueror-mainwindow#1"<< 
-					"newTab" << 
+#if defined(Q_OS_WIN32)
+                // In Win32, pid is not a longlong, but a pointer to a _PROCESS_INFORMATION structure.
+                // Redundant change in Win32, as there's no konqueror, but I wanted to follow the original logic.
+				args<< QString("konqueror-%1").arg(procBrowser->pid()->dwProcessId)<<
+					"konqueror-mainwindow#1"<<
+					"newTab" <<
 					urls.at(i);
+#else
+				args<< QString("konqueror-%1").arg(procBrowser->pid())<<
+					"konqueror-mainwindow#1"<<
+					"newTab" <<
+					urls.at(i);
+#endif
 				p->start ("dcop",args);
 				if ( !p->waitForStarted() ) success=false;
 			}
*/			
