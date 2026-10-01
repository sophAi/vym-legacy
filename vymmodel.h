#ifndef VYMMODEL_H
#define VYMMODEL_H

#include <QGraphicsScene>

#include "mapcenterobj.h"
#include "mapeditor.h"


/*! \brief This will later be divided into Model/View
*/

class VymModel : public QObject{
	Q_OBJECT

public:
	VymModel();
	~VymModel ();
    void clear();
    void init();
	void setMapEditor(MapEditor *me);	// FIXME should not be necessary in Model/View
	MapEditor* getMapEditor();
	void setVersion(const  QString &);
	void setAuthor  (const QString &);
	QString getAuthor ();
	void setComment (const QString &);
	QString getComment ();
	QString getDate();
	void setScene(QGraphicsScene *s);
	QGraphicsScene *getScene();
	MapCenterObj* addMapCenter();
	MapCenterObj* addMapCenter(QPointF absPos);
	MapCenterObj* removeMapCenter(MapCenterObj *mco);
	MapCenterObj* getMapCenterNum (int i);
	int countMapCenters ();
	int countBranches();

	BranchObj* first();					// FIXME replaced by ModelIndex later
	BranchObj* next(BranchObj *bo);		// FIXME replaced by ModelIndex later

    LinkableMapObj* findMapObj(QPointF,LinkableMapObj*);	// find MapObj 
    LinkableMapObj* findObjBySelect (const QString &s);		// find MapObj by select string
    LinkableMapObj* findID (const QString &s);				// find MapObj by previously set ID
	QString saveToDir (const QString&,const QString&,int, const QPointF&);// Save data recursivly to tempdir


////////////////////////////////////////// View related
    // void updateLink();  FIXME needed?
    void updateRelPositions();

	QRectF getTotalBBox();
	void reposition();					//!< Call reposition for all MCOs
	void setHideTmp (HideTmpMode mode);	
	QPolygonF shape(BranchObj *bo);		//!< Returns arbitrary shape of subtree
	void moveAway (LinkableMapObj *lmo);//!< Autolayout: Move all out of the way

	// Animation  **experimental**
private slots:
	void animate();						//!< Called by timer to animate stuff
public:
	void startAnimation (const QPointF &start, const QPointF &dest);
	void stopAnimation  (MapObj *mo);
private:	
	QTimer *animationTimer;
	bool animationUse;
	uint animationTicks;
	uint animationInterval;
	int timerId;				// animation timer
	QList <MapObj*> animObjList;// list with animated objects

////////////////////////////////////////// Selection related 
public:
	LinkableMapObj* getSelection();
	BranchObj* getSelectedBranch();
	bool select (const QString &s);
	QString getSelectString (LinkableMapObj *lmo);

private:
	QGraphicsScene *mapScene;
	MapEditor *mapEditor;
	QList <MapCenterObj*> mapCenters;
	QString version;	//!< version string saved in vym file
	QString author;
	QString comment;
	QDate date;
};


#endif
