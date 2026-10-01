#ifndef MAPCENTEROBJ_H
#define MAPCENTEROBJ_H


#include <QDate>

#include "branchobj.h"

/*! \brief The center of the map is a special branch. */

/////////////////////////////////////////////////////////////////////////////
class MapCenterObj:public BranchObj {
public:
    MapCenterObj ();
    MapCenterObj (QGraphicsScene *);
    ~MapCenterObj ();
    void clear();
    void init();
    virtual void move      (double,double);
    virtual void move      (QPointF absPos);
    virtual void moveBy    (double,double);
    virtual void moveAll   (double,double);
    virtual void moveAllBy (double,double);
    virtual void updateLink();
    virtual void updateRelPositions();
    LinkableMapObj* findMapObj(QPointF,LinkableMapObj*);	// find MapObj 
	virtual QString saveToDir (const QString&,const QString&,int, const QPointF&);// Save data recursivly to tempdir
	void setVersion(const  QString &);
	void setAuthor  (const QString &);
	QString getAuthor ();
	void setComment (const QString &);
	QString getComment ();
	QString getDate();
private:
	QString version;	//!< version string saved in vym file
	QString author;
	QString comment;
	QDate date;
};
#endif
