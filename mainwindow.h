#ifndef MAINWINDOW_H 
#define MAINWINDOW_H

#include <QMainWindow>

#include "branchpropwindow.h"
#include "file.h"
#include "findwindow.h"
#include "historywindow.h"
#include "mapeditor.h"
#include "simplescripteditor.h"
#include "texteditor.h"


class Main : public QMainWindow 
{
    Q_OBJECT

public:
	/*! Modifier modes are used when CTRL together with a mouse button is pressed */
	enum ModMode {
		ModModeNone,	//!< Unused
		ModModeColor,	//!< Pick color from object
		ModModeCopy,	//!< Copy object
		ModModeXLink	//!< Create a XLink (XLinkObj) from selected object
		};

    Main(QWidget* parent=0, const char* name=0, Qt::WFlags f=0);
    ~Main();
	void loadCmdLine();
	void statusMessage (const QString &);

public slots:
    void fileNew();
    void fileNewCopy();

protected:
	void closeEvent( QCloseEvent* );

private:
    void setupFileActions();
    void setupEditActions();
    void setupFormatActions();
    void setupViewActions();
    void setupModeActions();
    void setupWindowActions();
    void setupFlag(FlagObj *fo, QToolBar *tb, bool aw, const QString &name, const QString &tooltip);
    void setupFlagActions();
    void setupNetworkActions();
    void setupSettingsActions();
    void setupTestActions();
    void setupHelpActions();
    void setupContextMenus();
	void setupRecentMapsMenu();
    void setupMacros();
	void hideEvent (QHideEvent * );
	void showEvent (QShowEvent * );
	MapEditor* currentMapEditor() const;
    
private slots:
	void editorChanged(QWidget*);

    ErrorCode fileLoad(QString ,const LoadMode &, const FileType & ftype=VymMap);
    void fileLoad(const LoadMode &);
    void fileLoad();
	void fileLoadRecent();
	void addRecentMap (const QString &);
    void fileSave(MapEditor*, const SaveMode & );
    void fileSave();
public slots:	
    void fileSave(MapEditor *);	// autosave from MapEditor
private slots:	
    void fileSaveAs(const SaveMode &);
    void fileSaveAs();
    void fileImportKDE3Bookmarks();
    void fileImportKDE4Bookmarks();
    void fileImportFirefoxBookmarks();
    void fileImportFreemind();
    void fileImportMM();
    void fileImportDir();
    void fileExportXML();
    void fileExportXHTML();
    void fileExportImage();
    void fileExportASCII();
    void fileExportCSV();
    void fileExportLaTeX();
    void fileExportKDE3Bookmarks();
    void fileExportKDE4Bookmarks();
    void fileExportTaskjuggler();
    void fileExportOOPresentation();
    void fileCloseMap();
    void filePrint();
    void fileExitVYM();

public slots:
    void editUndo();	
    void editRedo();	
	void gotoHistoryStep (int);
private slots:	
    void editCopy();	
    void editPaste();	
    void editCut();	
    void editOpenFindWindow();
	void editFind(QString);
	void editFindChanged();
private:
	void openTabs(QStringList);
public slots:
	void editOpenURL();
	void editOpenURLTab();
private slots:
	void editOpenMultipleURLTabs();
	void editURL();
	void editLocalURL();
	void editHeading2URL();
	void editBugzilla2URL();
	void editFATE2URL();
	void openVymLinks(const QStringList &);
	void editVymLink();
	void editOpenMultipleVymLinks();
    void editHeadingFinished();
    void editAttributeFinished();
public slots:
    void editHeading();
    void editAttribute();
	void editOpenVymLink();
private slots:
	void editDeleteVymLink();
	void editToggleHideExport();
	void editMapInfo();
    void editMoveUp();	
    void editMoveDown();	
	void editSortChildren();
    void editToggleScroll();
    void editUnscrollChilds();
    void editAddMapCenter();
    void editNewBranch();
    void editNewBranchBefore();
    void editNewBranchAbove();
    void editNewBranchBelow();
    void editImportAdd();
    void editImportReplace();
    void editSaveBranch();
    void editDeleteKeepChilds();
    void editDeleteChilds();
    void editDeleteSelection();
    void editUpperBranch();
    void editLowerBranch();
    void editLeftBranch();
    void editRightBranch();
    void editFirstBranch();
    void editLastBranch();
    void editLoadImage();
    void editSaveImage();
    void editFollowXLink (QAction *);
    void editEditXLink (QAction *);

    void formatSelectColor();
    void formatPickColor();
    void colorChanged(QColor);
    void formatColorBranch();
    void formatColorSubtree();
	void formatLinkStyleLine();
	void formatLinkStyleParabel();
	void formatLinkStylePolyLine();
	void formatLinkStylePolyParabel();
    void formatSelectBackColor();
    void formatSelectBackImage();
    void formatSelectLinkColor();
    void formatSelectSelectionColor();
    void formatToggleLinkColorHint();
	void formatHideLinkUnselected();

    void viewZoomReset();
    void viewZoomIn();
    void viewZoomOut();
    void viewCenter();

public slots:
	void networkStartServer();
	void networkConnect();
	bool settingsPDF();
	bool settingsURL();
	void settingsMacroDir();
	void settingsToggleDelKey();
	void settingsUndoLevels();
	void settingsAutosaveToggle();
	void settingsAutosaveTime();
	void settingsWriteBackupFileToggle();
	void settingsToggleAnimation();

	void windowToggleNoteEditor();
	void windowToggleHistory();
	void windowToggleProperty();
	void updateHistory(SimpleSettings &);
	void windowToggleAntiAlias();
	void windowToggleSmoothPixmap();
	void updateNoteFlag();
	void updateSatellites(MapEditor *);
	void updateActions();
	ModMode getModMode();
	bool autoEditNewBranch();
	bool autoSelectNewBranch();
	bool useFlagGroups();
	void setScript(const QString &);
	void runScript(const QString &);
	void runScriptEverywhere (const QString &);

private slots:
	void windowNextEditor();
	void windowPreviousEditor();
	void windowShowNoteEditor();
	void windowHideNoteEditor();

	void standardFlagChanged();

    void testFunction1();
    void testFunction2();
    void testCommand();

    void helpDoc();
    void helpDemo();
    void helpAbout();
    void helpAboutQT();

	void callMacro ();

private:
	QTabWidget *tabWidget;
	FindWindow *findWindow;
	QProcess *procBrowser;

	QStringList imageTypes;

	QLineEdit *lineedit;	// to enter headings of branches
	QString prevSelection;

	HistoryWindow *historyWindow;

	BranchPropertyWindow *branchPropertyWindow;
	SimpleScriptEditor *scriptEditor;

	QList <QAction*> actionListBranches;

	QColor currentColor;

	int xLinkMenuWidth;

	QMenu *recentFilesMenu;
	enum { MaxRecentFiles = 9 };
    QAction *recentFileActions[MaxRecentFiles];

    QAction *macroActions[12];
	QStringList macro;

	QAction* actionFileNewCopy;
	QAction* actionFileSave;
	QAction* actionFilePrint;
	QAction* actionEditUndo;
	QAction* actionEditRedo;
	QAction *actionEditCopy;
	QAction *actionEditCut;
	QAction *actionEditPaste;
	QAction *actionEditMoveUp;
	QAction *actionEditMoveDown;
	QAction *actionEditSortChildren;
	QAction *actionEditToggleScroll;
	QAction* actionEditOpenURL;
	QAction* actionEditOpenURLTab;
	QAction* actionEditOpenMultipleURLTabs;
	QAction* actionEditURL;
	QAction* actionEditLocalURL;
	QAction* actionEditHeading2URL;
	QAction* actionEditBugzilla2URL;
	QAction* actionEditFATE2URL;
	QAction *actionEditOpenVymLink;
	QAction *actionEditOpenMultipleVymLinks;
	QAction *actionEditVymLink;
	QAction *actionEditDeleteVymLink;
	QAction *actionEditToggleHideExport;
	QAction *actionEditMapInfo;
	QAction *actionEditHeading;
	QAction *actionEditDelete;
	QAction *actionEditAddMapCenter;
	QAction *actionEditAddBranch;
	QAction *actionEditAddBranchBefore;
	QAction *actionEditAddBranchAbove;
	QAction *actionEditAddBranchBelow;
	QAction *actionEditDeleteKeepChilds;
	QAction *actionEditDeleteChilds;
	QAction *actionEditImportAdd;
	QAction *actionEditImportReplace;
	QAction *actionEditSaveBranch;
	QAction *actionEditSelectFirst;
	QAction *actionEditSelectLast;
	QAction *actionEditLoadImage;

	QAction* actionFormatColor;
	QAction* actionFormatPickColor;
	QAction* actionFormatColorBranch;
	QAction* actionFormatColorSubtree;
	QAction* actionFormatLinkColorHint;
	QAction* actionFormatBackColor;
	QAction* actionFormatBackImage;
	QAction* actionFormatLinkColor;
	QAction* actionFormatSelectionColor;

	QActionGroup* actionGroupModModes;
	QAction* actionModModeColor;
	QAction* actionModModeXLink;
	QAction* actionModModeCopy;

	QActionGroup *actionGroupFormatFrameTypes;


	QActionGroup *actionGroupFormatLinkStyles;
	QAction *actionFormatLinkStyleLine;
	QAction *actionFormatLinkStyleParabel;
	QAction *actionFormatLinkStylePolyLine;
	QAction *actionFormatLinkStylePolyParabel;
	QAction *actionFormatHideLinkUnselected;

	QAction *actionViewToggleNoteEditor;
	QAction *actionViewToggleHistoryWindow;
	QAction *actionViewTogglePropertyWindow;
	QAction *actionViewToggleAntiAlias;
	QAction *actionViewToggleSmoothPixmapTransform;
	QAction* actionViewCenter;

	QAction* actionSettingsAutoEditNewBranch;
	QAction* actionSettingsAutoSelectNewBranch;
	QAction* actionSettingsAutoSelectText;
	QAction* actionSettingsUseDelKey;
	QAction* actionSettingsUseFlagGroups;
	QAction* actionSettingsUseHideExport;
	QAction* actionSettingsAutosaveToggle;
	QAction* actionSettingsAutosaveTime;
	QAction* actionSettingsWriteBackupFile;
	QAction* actionSettingsUseAnimation;
};


#endif

