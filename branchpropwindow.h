#ifndef BRANCHPROPERTYWINDOW_H
#define BRANCHPROPERTYWINDOW_H

#include "ui_branchpropwindow.h"

#include <QDialog>
#include <QCloseEvent>
#include <QtGui>


#include "attributedelegate.h"
#include "branchobj.h"
#include "mapeditor.h"


class QAbstractItemModel;

class BranchPropertyWindow:public QDialog
{
	Q_OBJECT
public:
	BranchPropertyWindow (QWidget *parent=0);
	~BranchPropertyWindow ();
	void setBranch (BranchObj *);
	void setMapEditor (MapEditor *);

private slots:
	void frameTypeChanged (int);
	void framePenColorClicked ();
	void frameBrushColorClicked ();
	void framePaddingChanged(int);
	void frameBorderWidthChanged(int);
	void linkHideUnselectedChanged (int);
	void incImgVerChanged (int);
	void incImgHorChanged (int);
	void addAttributeClicked();
	void deleteAttributeClicked();

signals:
	void windowClosed();
	
protected:
	void closeEvent( QCloseEvent* );

private:
	void connectSignals();
	void disconnectSignals();
	
	Ui::BranchPropertyWindow ui;

	BranchObj *branch;
	MapEditor *mapEditor;

	QColor penColor;
	QColor brushColor;

	QAbstractItemModel *attributeModel;

	AttributeDelegate delegate;
};

#endif // 
