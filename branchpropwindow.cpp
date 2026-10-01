#include "branchpropwindow.h"

#include <QColorDialog>

#include "frameobj.h"
#include "settings.h"

extern Settings settings;
extern QString vymName;


BranchPropertyWindow::BranchPropertyWindow (QWidget *parent): QDialog (parent)
{
	ui.setupUi (this);

	setCaption(vymName +" - " +tr ("Property Editor","Window caption"));

	branch=NULL;
	mapEditor=NULL;

	ui.tabWidget->setEnabled(false);

	penColor=QColor (Qt::black);
	brushColor=QColor (Qt::black);
    QPixmap pix( 16,16);
    pix.fill (penColor);
	ui.framePenColorButton->setPixmap (pix);
	ui.frameBrushColorButton->setPixmap (pix);

	// Create Model and View to hold attributes
	attributeModel = new QStandardItemModel (1,3,this);
	attributeModel->setHeaderData(0, Qt::Horizontal, tr("Name","Branchprop window: Attribute name"));
	attributeModel->setHeaderData(1, Qt::Horizontal, tr("Value","Branchprop window: Attribute value"));
	attributeModel->setHeaderData(2, Qt::Horizontal, tr("Type","Branchprop window: Attribute type"));
	ui.attributeTableView->setModel (attributeModel);
	

	// Load Settings
	resize (settings.value ( "/satellite/propertywindow/geometry/size", QSize(450,600)).toSize());
	move   (settings.value ( "/satellite/propertywindow/geometry/pos", QPoint (250,50)).toPoint());
	
	if (settings.value ( "/satellite/propertywindow/showWithMain",true).toBool())
		show();
	else	
		hide();

	// FIXME for now remove attribute tab
	ui.tabWidget->removeTab (3);

}

BranchPropertyWindow::~BranchPropertyWindow ()
{
	settings.setValue( "/satellite/propertywindow/geometry/size", size() );
	settings.setValue( "/satellite/propertywindow/geometry/pos", pos() );
	settings.setValue( "/satellite/propertywindow/showWithMain",isVisible() );
}

void BranchPropertyWindow::setBranch (BranchObj *bo)
{
	disconnectSignals();
	branch=bo;
	if (bo) 
	{
		ui.tabWidget->setEnabled (true);

		// Frame
		FrameObj::FrameType t=branch->getFrameType();
		if (t==FrameObj::NoFrame)
		{
			ui.frameTypeCombo->setCurrentIndex (0);
			penColor=Qt::white;
			brushColor=Qt::white;
			ui.colorGroupBox->setEnabled (false);
			ui.framePaddingSpinBox->setEnabled (false);
			ui.frameWidthSpinBox->setEnabled (false);
			ui.framePaddingLabel->setEnabled (false);
			ui.frameBorderLabel->setEnabled (false);
		} else	
		{
			penColor=bo->getFramePenColor();
			brushColor=bo->getFrameBrushColor();
			QPixmap pix( 16,16);
			pix.fill (penColor);
			ui.framePenColorButton->setPixmap (pix);
			pix.fill (brushColor);
			ui.frameBrushColorButton->setPixmap (pix);
			ui.colorGroupBox->setEnabled (true);
			ui.framePaddingSpinBox->setEnabled (true);
			ui.framePaddingSpinBox->setValue (bo->getFramePadding());
			ui.frameWidthSpinBox->setEnabled (true);
			ui.frameWidthSpinBox->setValue (bo->getFrameBorderWidth());
			ui.framePaddingLabel->setEnabled (true);
			ui.frameBorderLabel->setEnabled (true);

			switch (t)
			{
				case FrameObj::Rectangle: 
					ui.frameTypeCombo->setCurrentIndex (1);
					break;
				case FrameObj::Ellipse: 
					ui.frameTypeCombo->setCurrentIndex (2);
					break;
				default: 
					break;
			}
		}	
		
		// Link
		if (branch->getHideLinkUnselected())
			ui.hideLinkIfUnselected->setCheckState (Qt::Checked);
		else	
			ui.hideLinkIfUnselected->setCheckState (Qt::Unchecked);

		// Layout
		if (branch->getIncludeImagesVer())
			ui.incImgVer->setCheckState (Qt::Checked);
		else	
			ui.incImgVer->setCheckState (Qt::Unchecked);
		if (branch->getIncludeImagesHor())
			ui.incImgHor->setCheckState (Qt::Checked);
		else	
			ui.incImgHor->setCheckState (Qt::Unchecked);

		// Attributes
		attributeModel->removeRows(0, attributeModel->rowCount(), QModelIndex());

		// FIXME some samples for testing
		QStringList attrTypes=mapEditor->attributeTable()->getTypes();
		for (int i=0; i<attrTypes.count()-1;i++)
		{
			attributeModel->insertRow (i,QModelIndex ());
			attributeModel->setData(attributeModel->index(i, 0, QModelIndex()), QString ("Name %1").arg(i));
			attributeModel->setData(attributeModel->index(i, 1, QModelIndex()), i);
			attributeModel->setData(attributeModel->index(i, 2, QModelIndex()), attrTypes.at(i));
		}


		ui.attributeTableView->resizeColumnsToContents();

		// Initialize Delegate
		delegate.setAttributeTable (mapEditor->attributeTable());
		ui.attributeTableView->setItemDelegate (&delegate);


		// Finally activate signals
		connectSignals();
	} else
	{
		ui.tabWidget->setEnabled (false);
	}
}

void BranchPropertyWindow::setMapEditor (MapEditor *me)
{
	mapEditor=me;
	if (mapEditor) 
		setBranch (mapEditor->getSelectedBranch() );
	else
		ui.tabWidget->setEnabled (false);
		
}

void BranchPropertyWindow::frameTypeChanged (int i)
{
	if (mapEditor)
	{
		switch (i)
		{
			case 0: mapEditor->setFrameType (FrameObj::NoFrame); break;
			case 1: 
				mapEditor->setFrameType (FrameObj::Rectangle); 
				break;
			case 2: 
				mapEditor->setFrameType (FrameObj::Ellipse); 
				mapEditor->setFramePadding (5); 
				break;
		}
		setBranch (branch);
	}	
}

void BranchPropertyWindow::framePenColorClicked()
{
	if (mapEditor) 
	{	
		QColor col = QColorDialog::getColor( penColor, this );
		if ( col.isValid() ) 
		{
			penColor=col;
			mapEditor->setFramePenColor (penColor);
		}	
	}
}

void BranchPropertyWindow::frameBrushColorClicked()
{
	if (mapEditor) 
	{
		QColor col = QColorDialog::getColor( brushColor, this );
		if ( col.isValid() ) 
		{
			brushColor=col;
			mapEditor->setFrameBrushColor (brushColor);
		}	
	}	
}

void BranchPropertyWindow::framePaddingChanged(int i)
{
	if (mapEditor) mapEditor->setFramePadding (i);
}

void BranchPropertyWindow::frameBorderWidthChanged(int i)
{
	if (mapEditor) mapEditor->setFrameBorderWidth(i);
}

void BranchPropertyWindow::linkHideUnselectedChanged (int i)
{
	if (!branch) return;
	mapEditor->setHideLinkUnselected(i);
}

void BranchPropertyWindow::incImgVerChanged (int  i)
{
	if (mapEditor) mapEditor->setIncludeImagesVer (i);
}

void BranchPropertyWindow::incImgHorChanged (int  i)
{
	if (mapEditor) mapEditor->setIncludeImagesHor (i);
}

void BranchPropertyWindow::closeEvent( QCloseEvent* ce )
{
    ce->accept();	// can be reopened with show()
	hide();
	emit (windowClosed() );
    return;
}

void BranchPropertyWindow::addAttributeClicked()
{
	// Add empty line for adding attributes
	attributeModel->insertRow (attributeModel->rowCount (),QModelIndex ());
	attributeModel->setData(attributeModel->index(attributeModel->rowCount()-1, 0, QModelIndex()),  "Add new");
	attributeModel->setData(attributeModel->index(attributeModel->rowCount()-1, 2, QModelIndex()),  "Undefined");

	// Select attribute from list
	ui.attributeTableView->edit (attributeModel->index(attributeModel->rowCount()-1,0, QModelIndex() ));
	ui.attributeTableView->resizeColumnsToContents();

//	QString attname=attributeModel->in
//	attributeModel->setData(attributeModel->index(attributeModel->rowCount()-1, 2, QModelIndex()),  );



	ui.attributeTableView->edit (attributeModel->index(attributeModel->rowCount()-1,1, QModelIndex() ));

}

void BranchPropertyWindow::deleteAttributeClicked()
{
	cout << "BPW::delete\n";
}

void BranchPropertyWindow::connectSignals()
{
	// Frame
	connect ( 
		ui.framePenColorButton, SIGNAL (clicked()), 
		this, SLOT (framePenColorClicked()));
	connect ( 
		ui.framePaddingSpinBox, SIGNAL (valueChanged( int)), 
		this, SLOT (framePaddingChanged (int)));
	connect ( 
		ui.frameWidthSpinBox, SIGNAL (valueChanged( int)), 
		this, SLOT (frameBorderWidthChanged (int)));
	connect ( 
		ui.frameBrushColorButton, SIGNAL (clicked()), 
		this, SLOT (frameBrushColorClicked()));
	connect ( 
		ui.frameTypeCombo, SIGNAL (currentIndexChanged( int)), 
		this, SLOT (frameTypeChanged (int)));


	// Link	
	connect ( 
		ui.hideLinkIfUnselected, SIGNAL (stateChanged( int)), 
		this, SLOT (linkHideUnselectedChanged (int)));

	// Layout	
	connect ( 
		ui.incImgVer, SIGNAL (stateChanged( int)), 
		this, SLOT (incImgVerChanged (int)));
	connect ( 
		ui.incImgHor, SIGNAL (stateChanged( int)), 
		this, SLOT (incImgHorChanged (int)));

	// Attributes	
	connect ( 
		ui.addAttributeButton, SIGNAL (clicked()), 
		this, SLOT (addAttributeClicked()));
	connect ( 
		ui.deleteAttributeButton, SIGNAL (clicked()), 
		this, SLOT (deleteAttributeClicked()));
}


void BranchPropertyWindow::disconnectSignals()
{
	// Frame 
	disconnect ( ui.frameTypeCombo, 0,0,0);
	disconnect ( ui.framePenColorButton, 0,0,0);
	disconnect ( ui.framePaddingSpinBox, 0,0,0);
	disconnect ( ui.frameWidthSpinBox, 0,0,0);
	disconnect ( ui.frameBrushColorButton, 0,0,0);

	// Link	
	disconnect ( ui.hideLinkIfUnselected, 0,0,0);

	// Layout	
	disconnect ( ui.incImgVer, 0,0,0);
	disconnect ( ui.incImgHor, 0,0,0);

	// Attributes
	disconnect ( ui.addAttributeButton, 0,0,0);
	disconnect ( ui.deleteAttributeButton, 0,0,0);
}


