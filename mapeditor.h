#ifndef MAPEDITOR_H
#define MAPEDITOR_H

#include <QGraphicsView>
#include <QtNetwork>

#include "attribute.h"
#include "file.h"
#include "misc.h"
#include "parser.h"
#include "ornamentedobj.h"
#include "selection.h"
#include "settings.h"
#include "vymmodel.h"

class VymModel;
class Selection;

/*! \brief Main widget in vym to display and edit a map */

class MapEditor : public QGraphicsView, public XMLObj {
    Q_OBJECT

public:
    /*! \brief State of the mapeditor 
		
		While a heading is edited, the state has to change, so
		that e.g. no other object might get selected. This is done
		in Main by calling setStateEditHeading
	*/
	enum State {
		Idle,			//!< Idle, waiting for user event
		EditHeading		//!< Editing heading (dont't select another branch now)
	};
    /*! \brief Networking states
		
		In Network modus we want to switch of saveState, autosave, ...
	*/
	enum NetState {
		Offline,			//!< Offline
		Client,				//!< I am the client and connected to server
		Server				//!< I am the server
	};
    MapEditor(QWidget* parent=0);
	~MapEditor();
	VymModel* getModel();
	QGraphicsScene * getScene();
	State getState();					//!< Return State of MapEditor
	void setStateEditHeading (bool);	//!< If set to true, State will change to EditHeading
	bool isRepositionBlocked(); //!< While load or undo there is no need to update graphicsview
	bool isSaveStateBlocked();			//!< block while undo/redo or while running scripts
	void setSaveStateBlocked(bool);		//!< block saving the undo/redo state
	bool isSelectBlocked();		//!< true, if no change of selection is possible, e.g. while editing the heading of abranch
	
protected:
	/*! \brief Get name of object
	  
	  Returns heading of a branch or name of an object for use in comment
	  of undo/redo history
	*/ 
	QString getName(const LinkableMapObj*);	// Get e.g. heading or filename
	void makeTmpDirs();		// create temporary directories

	/*! This function saves all information of the map to disc.
	    saveToDir also calls the functions for all BranchObj and other objects in the map.
		The structure of the map itself is returned as QString and passed back to Main, 
		where saveToDir is called initially
	*/	
    QString saveToDir (const QString &tmpdir, const QString &prefix, bool writeflags, const QPointF &offset, LinkableMapObj *saveSel);

	/*! \brief Get directory, where current step in history is save

		saveState creates a directory for each step in history. This function returns the
		path of the current directory
	*/
	QString getHistoryDir();

	/*! \brief Save the current changes in map 

		Two commands and selections are saved:

			- undocommand and undoselection to undo the change
			- redocommand and redoselection to redo the action after an undo

		Additionally a comment is logged. 

	*/	
    void saveState(
		const SaveMode& savemode, 
		const QString &undoSelection, 
		const QString &undoCommand, 
		const QString &redoSelection, 
		const QString &redoCommand, 
		const QString &comment, 
		LinkableMapObj *saveSelection);
	/*! Overloaded for convenience */
    void saveStateChangingPart(
		LinkableMapObj *undoSelection, 
		LinkableMapObj* redoSelection, 
		const QString &redoCommand, 
		const QString &comment);
	/*! Overloaded for convenience */
    void saveStateRemovingPart(
		LinkableMapObj *redoSelection, 
		const QString &comment);
	/*! Overloaded for convenience */
    void saveState(
		LinkableMapObj *undoSelection, 
		const QString &undoCommand, 
		LinkableMapObj *redoSelection, 
		const QString &redoCommand, 
		const QString &comment); 
	/*! Overloaded for convenience */
    void saveState(
		const QString &undoSelection, 
		const QString &undoCommand, 
		const QString &redoSelection, 
		const QString &redoCommand, 
		const QString &comment) ;
    void saveState(
		const QString &undoCommand, 
		const QString &redoCommand, 
		const QString &comment) ;

public:	

	/* \brief Process one command and its parameters */
    void parseAtom (const QString &atom);	

	/* \brief Runs the script */
	void runScript (QString script);
private:
    void addFloatImageInt(const QPixmap &img);

public:
	bool isDefault();		//!< true, if map is still the empty default map
    bool hasChanged();		//!< true, if something has changed and is not saved yet
	void setChanged();		//!< called from TextEditor via LinkableMapObj
	void closeMap();		//!< Closes the map

	/*! \brief Sets filepath, filename and mapname

	     If the filepath is "/home/tux/map.xml", then the filename will be set
		 to map.xml. The destname is needed for vymLinks, pointing to another map. 
		 The destname holds the real name of the file, after it has been compressed, e.g. "map.vym"
	*/	 

	/*! \brief Set File path

	     The destname is needed to construct the references between maps
	*/	 
	void setFilePath (QString filepath,QString destname);	
	void setFilePath (QString);	//!< Overloaded for convenience
	QString getFilePath ();	//!< Full path e.g. "/home/tux/map.xml"
	QString getFileName ();	//!< e.g. "map.xml"
	QString getMapName ();	//!< e.g. "map"
	QString getDestPath (); //!< e.g. "/home/tux/map.vym"

	/*! \brief Load map

		The data is read from file. Depending on LoadMode the current
		selection gets replaced by data or the data is appended.
	*/	
    ErrorCode load (QString, const LoadMode &, const FileType& );	// newmap, import/replace selection
public:
	/*! \brief Save the map to file */
    ErrorCode save(const SaveMode &);	
	/* FIXME no needed any longer
	void setZipped(bool);		//!< Set or unset compression of map with zip save map zipped
	bool saveZipped();			//!< True, if file will be saved zipped
	*/
    void print();				//!< Print the map
	void setAntiAlias (bool);	//!< Set or unset antialiasing
	void setSmoothPixmap(bool); //!< Set or unset smoothing of pixmaps
private:
    QPixmap getPixmap();
	void setHideTmpMode (HideTmpMode);	// temporary hide stuff
	HideTmpMode getHideTmpMode();		// temporary hide stuff
public:
	/*! Set or unset temporary hiding of objects during export  */
	void setExportMode (bool);

	/*! Export as ASCII text to file */
	void exportASCII (QString fname="",bool askForName=true);  

	/*! Save as image */
    void exportImage (QString fname="",bool askForName=true,QString format="PNG");

    /*! Export as OpenOfficeOrg presentation */
    void exportOOPresentation(const QString &,const QString &);	

	/*! Export as XHTML to directory */
    void exportXHTML(const QString& dir="", bool askForName=true);	

	/*! Export as XTML to directory */
    void exportXML(QString dir="", bool askForName=true);

    void clear();						//!< Clear map
    void copy();						//!< Copy to clipboard
    void redo();						//!< Redo last action
	bool isRedoAvailable();				//!< True, if redo is available
    void undo();						//!< Undo last action
	bool isUndoAvailable();				//!< True, if undo is available
	void gotoHistoryStep (int);			//!< Goto a specifig step in history
private:	
    void addMapReplaceInt(const QString & undoSel, const QString & path);
    void addMapInsertInt (const QString & path, int pos);
    void pasteNoSave(const int &n);		//!< paste clipboard to branch
public:	
    void paste();		//!< Paste clipboard to branch and backup
    void cut();			//!< Cut to clipboard (and copy)
	/*! \brief Move absolutly

		Move absolutly to (x,y).
	*/	
    void move    (const double &x, const double &y);
	/*! \brief Move relativly

		Move relativly to (x,y).
	*/	
    void moveRel (const double &x, const double &y);
    void moveBranchUp();	//!< Move branch up
    void moveBranchDown();	//!< Move branch down
	void sortChildren();	//!< Sort children lexically
private:	
	void linkTo(const QString &);
public:	
	QString getHeading (bool &ok,QPoint &p); //!< Get heading, ok if selection is branch
	void setHeading(const QString &);		//!< Set heading of branch	
private:
	void setHeadingInt(const QString &);
	void setVymLinkInt(const QString &);	// Set vymLink for selection
	/*! \brief Add new mapcenter

	    Disclaimer: Still experimental, not fully supported.
	*/	
public:	
    BranchObj* addMapCenter();		

	/*! \brief Add new branch

		Depending on num the new branch is created

		-3 above selection as child of selections parent
		-2 as child of selection
		-1 below selection as child of selections parent
		0..n	insert at a specific position in selections parent
		(needed for free relinking)
	*/	
private:	
    BranchObj* addNewBranchInt(int);		// pos allows to add above/below selection
public:	
	/*! \Add new branch
		
		Depending on num the new branch is created
		-1 above selection
		 0 as child of selection
		 1 below selection
	*/
    BranchObj* addNewBranch(int pos);		
    BranchObj* addNewBranchBefore();		//!< Insert branch between selection and its parent
    void deleteSelection();					//!< Delete selection
	LinkableMapObj* getSelection();			//!< Returns selection
	BranchObj* getSelectedBranch();			// returns selected branch or NULL
	FloatImageObj* getSelectedFloatImage();	// returns selected branch or NULL
	void unselect();						// before changing current noteedit
	void reselect();						// after  changing current noteedit
	bool select(const QString &);			// Select by string
	bool select(LinkableMapObj *lmo);		// Select by pointer
	QString getSelectString();
private:	
	void selectInt(LinkableMapObj*);	
	void selectNextBranchInt();		// Increment number of branch
	void selectPrevBranchInt();		// Decrement number of branch
public:	
    void selectUpperBranch();
    void selectLowerBranch();
    void selectLeftBranch();
    void selectRightBranch();
    void selectFirstBranch();
    void selectLastBranch();
    void selectMapBackgroundImage();
    void setMapBackgroundImage(const QString &);
    void selectMapBackgroundColor();
    void setMapBackgroundColor(QColor);
    QColor getMapBackgroundColor();
    QColor getCurrentHeadingColor();
    void colorBranch(QColor);
    void colorSubtree(QColor);
	void toggleStandardFlag(QString);
    BranchObj* findText(QString,bool);		// Find object
    void findReset();						// Reset Find 
	void setURL(const QString &);			// set  URL
	void editURL();							// edit URL
	void editLocalURL();					// edit URL to local file
	QString getURL();						// returns URL of selection or ""
	QStringList getURLs();					// returns URLs of subtree
	void editHeading2URL();					// copy heading to URL
	void editBugzilla2URL();				// create URL to Bugzilla
	void editFATE2URL();					// create URL to FATE
	void editVymLink();						// edit link to another map
	void deleteVymLink();					// delete link to another map
	QString getVymLink();					// return path to map
	QStringList getVymLinks();				// return paths in subtree
	void setHideExport(bool);				// toggle the export flag
	void toggleHideExport();				// toggle the export flag
	void deleteKeepChilds();				// remove but keep childs
	void deleteChilds();					// remove childs
	void editMapInfo();						// dialog to enter author, ...
	void ensureSelectionVisible();		
	void updateSelection();					// update geometry of selection
	void updateActions();					// update e.g. format buttons
	void updateNoteFlag();					// when TextEditor changes
	void setMapAuthor (const QString &);
	void setMapComment(const QString &);
	void setMapLinkStyle (const QString &);	// Set style of link
	LinkableMapObj::Style getMapLinkStyle ();	// requested in LMO
	void setMapDefLinkColor(QColor);		// default color of links
	void setMapLinkColorHintInt();			// color of links
	void setMapLinkColorHint(LinkableMapObj::ColorHint);// color of links
	LinkableMapObj::ColorHint getMapLinkColorHint();
	QColor getMapDefLinkColor();
	void setMapDefXLinkColor(QColor);
	QColor getMapDefXLinkColor();
	void setMapDefXLinkWidth (int);
	int getMapDefXLinkWidth();
	void toggleMapLinkColorHint();			// after changing linkStyles
    void selectMapLinkColor();
    void selectMapSelectionColor();
private:	
    void setSelectionColorInt(QColor);
public:	
    void setSelectionColor(QColor);
    QColor getSelectionColor();
	bool scrollBranch(BranchObj*);
	bool unscrollBranch(BranchObj*);
    void toggleScroll();
    void unscrollChilds();
private:	
	FloatImageObj* loadFloatImageInt (QString);
public:	
	void loadFloatImage ();
private:	
	void saveFloatImageInt (FloatImageObj*, const QString &, const QString &);
public:	
	void saveFloatImage ();
	void setFrameType(const FrameObj::FrameType &);
	void setFrameType(const QString &);
	void setFramePenColor (const QColor &);
	void setFrameBrushColor (const QColor &);
	void setFramePadding (const int &);
	void setFrameBorderWidth (const int &);
	void setIncludeImagesVer(bool);
	void setIncludeImagesHor(bool);
	void setHideLinkUnselected (bool);
	bool getHideLinkUnselected ();
private:	
    void importDirInt(BranchObj *,QDir);
    void importDirInt(const QString&);
public:	
    void importDir();
	void followXLink (int);
	void editXLink (int);
	AttributeTable* attributeTable();
    void testFunction1();					// just testing new stuff
    void testFunction2();					// just testing new stuff
											// set /mainwindo/showTestMenu=true...

protected:
	virtual void contextMenuEvent ( QContextMenuEvent *e );
    virtual void keyPressEvent(QKeyEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mouseDoubleClickEvent(QMouseEvent*);
    virtual void resizeEvent( QResizeEvent * );

	void dragEnterEvent (QDragEnterEvent *);
	void dragMoveEvent (QDragMoveEvent *);
	void dragLeaveEvent (QDragLeaveEvent *);
	void dropEvent (QDropEvent *);


protected:
	void sendSelection();

public:
	void newServer();
	void connectToServer();

private slots:	
	void newClient();
	void sendData(const QString &s);
	void readData();
	void displayNetworkError (QAbstractSocket::SocketError);

	void autosave ();
	void fileChanged();

private:
	State state;				// State of MapEditor
	QGraphicsScene *mapScene;
	VymModel *model;					// Vym Map, includding several mapCenters
	QTimer *autosaveTimer;
	QTimer *fileChangedTimer;
	QDateTime fileChangedTime;

	bool adjustCanvasRequested;	// collect requests until end of user event
	BranchObj *editingBO;		// entering Text into BO

	QColor defLinkColor;		// default color for links
	QColor defXLinkColor;		// default color for xlinks
	int defXLinkWidth;			// default width for xlinks
	LinkableMapObj::ColorHint linkcolorhint;// use heading color or own color
	LinkableMapObj::Style linkstyle;		// default style for links

    QCursor HandOpenCursor;		// cursor while moving canvas view
	QCursor PickColorCursor;	// cursor while picking color 
	QCursor CopyCursor;			// cursor while picking color 
	QCursor XLinkCursor;		// cursor while picking color 
	bool pickingColor;
	bool drawingLink;			// true while creating a link
	bool copyingObj;			// true while creating a link
	XLinkObj* tmpXLink;

	Selection xelection;	

	QString latestSelection;		// select string of latest added object

    MapObj* movingObj;				// moving a MapObj
	MapObj* linkingObj_src;			// part of a link
    QPointF movingObj_orgPos;		// org. pos of mouse before move
    QPointF movingObj_orgRelPos;	// org. relative pos of mouse before move
    QPointF movingObj_start;		// rel. pos of mouse to absPos 
    QPointF movingCont_start;		// inital pos of moving Content or
    QPointF movingVec;				// how far has Content moved

	QPointF contextMenuPos;					// position where context event was triggered

    QPrinter* printer;				// Printing

    bool mapDefault;				// Flag if map is untouched
    bool mapChanged;				// Flag if undo is possible
	bool mapUnsaved;				// Flag if map should be saved

	Parser parser;				// Parser stuff for scripting

	AttributeTable *attrTable;

	bool printFrame;			// Print frame around map
	bool printFooter;			// Print footer below map

	bool zipped;				// should map be zipped
	static	int mapNum;			// unique number for Editor
	FileType fileType;			// type of file, e.g. vym, freemind...
	QString fileName;			// short name of file (for tab)
	QString filePath;			// path to file which will be saved
	QString fileDir;			// dir where file is saved
	QString destPath;			// path to .vym file (needed for vymlinks)
	QString mapName;			// fileName without ".vym"

	QString tmpMapDir;			// tmp directory with undo history
	QString histPath;			// Path to history file
	SimpleSettings undoSet;		// undo/redo commands, saved in histPath
	int stepsTotal;				// total number of steps (undos+redos) 
	int curStep;				// Current step in history (ring buffer)
	int curClipboard;			// number of history step, which is the current clipboard
	int redosAvail;				// Available number of redo steps
	int undosAvail;				// Available number of undo steps
	bool blockReposition;		// block while load or undo
	bool blockSaveState;		// block while load or undo

	BranchObj* itFind;			// next object in find process
	bool EOFind;				// true, if search failed

	QPoint exportOffset;		// set before export, used in save
	HideTmpMode hidemode;	// true while exporting to hide some stuff

	// Network connections **Experimental**
	NetState netstate;			// offline, client, server
	QTcpServer *tcpServer;		// Act as server in conference mode (experimental)
	QList <QTcpSocket*> clientList;		// List of connected clients
	quint16 sendCounter;		// Increased with every sent command

	QTcpSocket	*clientSocket;	// socket of this client
	QString server;				// server address of this client
	int port;					// server port of this client
	void displayClientError(QAbstractSocket::SocketError socketError);

};
#endif

