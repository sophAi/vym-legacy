#include <QApplication>

#include "flagrowobj.h"
#include "mainwindow.h"
#include "options.h"
#include "settings.h"
#include "version.h"

#if defined(Q_OS_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Global variables
TextEditor *textEditor;			// used in Constr. of LinkableMapObj
								// initialized in mainwindow
QString vymName;
QString vymVersion;
QString vymBuildDate;
QString vymCodeName;

Main *mainWindow;				// used in BranchObj::select()								
QString tmpVymDir;				// All temp files go there, created in mainwindow
QString clipboardDir;			// Clipboard used in all mapEditors
QString clipboardFile;			// Clipboard used in all mapEditors
QDir vymBaseDir;				// Containing all styles, scripts, images, ...
QDir lastImageDir;
QDir lastFileDir;
#if defined(Q_OS_WIN32)
QDir vymInstallDir;
#endif
QString iconPath;				// Pointing to icons used for toolbars
QString flagsPath;				// Pointing to flags
bool clipboardEmpty;			
bool debug;						// global debugging flag
FlagRowObj *systemFlagsDefault;	// used to copy from in LinkableMapObj
FlagRowObj *standardFlagsDefault;


Settings settings ("InSilmaril","vym"); // Organization, Application name

Options options;
ImageIO imageIO;

int statusbarTime=3500;

int main(int argc, char* argv[])
{
	//Q_INIT_RESOURCE (application);

    QApplication app(argc,argv);

	vymName=__VYM_NAME;
	vymVersion=__VYM_VERSION;
	vymBuildDate=__VYM_BUILD_DATE;
	vymCodeName=__VYM_CODENAME;


	// Reading and initializing options commandline options
	options.add ("debug", Option::Switch, "d", "debug");
	options.add ("version", Option::Switch, "v","version");
	options.add ("local", Option::Switch, "l", "local");
	options.add ("help", Option::Switch, "h", "help");
	options.add ("quit", Option::Switch, "q", "quit");
	options.add ("run", Option::String, "r", "run");
	options.add ("test", Option::String, "t", "test");
	options.add ("fontsize", Option::String, "fs", "fontsize");
	options.setHelpText (
		"VYM - View Your Mind\n"
		"--------------------\n\n"
		"Information about vym can be found in vym.pdf,\n"
		"which should be part of the vym package.\n"
		"It is also available at the project homepage:\n\n"
		"http://www.InSilmaril.de/vym\n\n"
		"Options:\n"
		"  -fs, --fontsize <pt> Set UI font size (default: 11pt)\n"
		"  -d,  --debug         Turn on debugging\n"
		"  -v,  --version       Show version\n"
		"  -h,  --help          Show this help\n");
	if (options.parse())
	{
		cout << endl << qPrintable( options.getHelpText())<<endl;
		return 1;
	}

	int fontSize = 0;
	if (options.isOn ("fontsize"))
		fontSize = options.getArg ("fontsize").toInt();
	else if (getenv("VYM_FONT_SIZE"))
		fontSize = QString(getenv("VYM_FONT_SIZE")).toInt();

	if (fontSize > 0)
	{
		QFont f = app.font();
		f.setPointSize(fontSize);
		app.setFont(f);
	}
	else if (app.font().pointSize() <= 9)
	{
		QFont f = app.font();
		f.setPointSize(11);
		app.setFont(f);
	}

	debug=options.isOn ("debug");

	if (options.isOn ("version"))
	{
		cout << "VYM - View Your Mind (c) 2004-2007 Uwe Drechsel "  << endl
			<<"   Version: "<<__VYM_VERSION <<endl
			<<"Build date: "<<__VYM_BUILD_DATE << endl
			<<"  "<<__VYM_CODENAME<<endl;
			
		return 0;	
	}		
	
	// Use /usr/share/vym or /usr/local/share/vym or . ?
	// First try options
	if (options.isOn ("local"))
	{
		vymBaseDir.setPath (vymBaseDir.currentDirPath());
	} else
	// then look for environment variable
	if (getenv("VYMHOME")!=0)
	{
		vymBaseDir.setPath (getenv("VYMHOME"));
	} else
	// ok, let's find my way on my own
	{
		#if defined (Q_OS_MACX)
			vymBaseDir.setPath(vymBaseDir.currentDirPath() +"/vym.app/Contents/Resources");

        #elif defined (Q_OS_WIN32)
            QString basePath;

            wchar_t wbuf[512];
            if (GetModuleFileName(NULL, wbuf, 512))
            {
                QString mfn(QString::fromWCharArray(wbuf));
                mfn.replace('\\', '/');
                if (mfn.endsWith("/bin/vym.exe", Qt::CaseInsensitive))
                {
                    mfn.chop(12);
                    basePath = mfn;
                }
            }

            if (basePath.isEmpty())
                basePath = vymBaseDir.currentDirPath();

            vymInstallDir.setPath(basePath);
            vymBaseDir.setPath(basePath + "/share/vym");

		#else
			#if defined(VYM_BASEDIR)
				vymBaseDir.setPath (VYM_BASEDIR);
			#else
				vymBaseDir.setPath ("/usr/share/vym-legacy");
			#endif
			if (!vymBaseDir.exists())
			{
				vymBaseDir.setPath ("/usr/share/vym-legacy");
				if (!vymBaseDir.exists())
				{
					vymBaseDir.setPath ("/usr/share/vym");
					if (!vymBaseDir.exists())
					{
						vymBaseDir.setPath ("/usr/local/share/vym-legacy");
						if (!vymBaseDir.exists())
						{
							vymBaseDir.setPath ("/usr/local/share/vym");
							if (!vymBaseDir.exists())
								vymBaseDir.setPath(vymBaseDir.currentDirPath() );
						}
					}
				}
			}		
		#endif
	}

	iconPath=vymBaseDir.path()+"/icons/";
	flagsPath=vymBaseDir.path()+"/flags/";

	// Some directories
	lastImageDir=QDir().current();
	lastFileDir=QDir().current();

	if (options.isOn ("help"))
	{
		cout << qPrintable (options.getHelpText())<<endl;
		return 0;	
	}	

	// Initialize translations
	QTranslator translator (0);
	translator.load( QString("vym_")+QTextCodec::locale(), vymBaseDir.path() + "/lang");
    app.installTranslator( &translator );

	// Initializing the row of system flags
	// is done in first call to MapEditor(),
	// because we need at least one canvas first
	systemFlagsDefault=NULL;
	standardFlagsDefault=NULL;

	// Initialize window of TextEditor
	textEditor = new TextEditor();
	textEditor->setIcon (QPixmap (iconPath+"vym-editor.png"));

	// Initialize mainwindow 
#if defined(Q_OS_WIN32)
    Main m(0, 0, (Qt::Window | Qt::MSWindowsOwnDC));
#else
    Main m;
#endif

	//m.resize(m.sizeHint());
	m.setIcon (QPixmap (iconPath+"vym-48x48.png"));
	m.show();
	m.fileNew();
	m.loadCmdLine();

	// Run script
	if (options.isOn ("run"))
	{
		QString script;
		QString fn=options.getArg ("run");
		if ( !fn.isEmpty() )
		{
			QFile f( fn );
			if ( !f.open( QIODevice::ReadOnly ) )
			{
				QMessageBox::warning(0, 
					QObject::tr("Error"),
					QObject::tr("Couldn't open %1.\n").arg(fn));
				return 0;
			}	

			QTextStream ts( &f );
			script= ts.read();
			f.close();
			m.setScript (script);
			m.runScriptEverywhere (script);
		}
	}		
	
	// For benchmarking we may want to quit instead of entering event loop
	if (options.isOn ("quit"))
	{
		return 0;
	}	

    QObject::connect( &app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()) );

    return app.exec();
}
