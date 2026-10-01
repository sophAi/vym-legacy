#ifndef SELECTION_H
#define SELECTION_H

#include <QString>
#include <QList>
#include <QGraphicsScene>


class BranchObj;
class FloatImageObj;
class MapCenterObj;
class LinkableMapObj;
class VymModel;


class Selection 
{
public:
	enum Type {Undefined,Branch,MapCenter,FloatImage};
	Selection ();
	~Selection();
	void copy(const Selection&);
	void clear();
	void setModel (VymModel *);
	void update();
	void setColor (QColor c);
	QColor getColor ();
	bool select (LinkableMapObj*);
	bool select (const QString &);
	bool reselect ();
	void unselect ();
	bool isEmpty();
	uint count();
	Type type();
	LinkableMapObj * first();		// first in selection list
	LinkableMapObj * single();		// NULL, if multiple selected
	BranchObj* getBranch();
	FloatImageObj* getFloatImage();

	QString getSelectString();

private:
	void init();
	QList <LinkableMapObj*> selectList;
	QList <LinkableMapObj*> lastSelectList;
	QList <QGraphicsRectItem*> selboxList;
	VymModel *model;
	QGraphicsScene *scene;

	QColor color;
};
#endif

