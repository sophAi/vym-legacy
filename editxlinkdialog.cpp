#include "editxlinkdialog.h"

#include <typeinfo>
#include <QColorDialog>

EditXLinkDialog::EditXLinkDialog (QWidget *parent):QDialog (parent)
{
	ui.setupUi (this);

	delink=false;
	xlo=NULL;
	selection=NULL;

	connect ( ui.widthBox, SIGNAL (valueChanged( int)), this, SLOT (widthChanged (int)));
	connect ( ui.colorButton, SIGNAL (clicked( )), this, SLOT (colorButtonPressed()));
	connect ( ui.setColorHeadingButton, SIGNAL (clicked( )), this, SLOT (setColorHeadingButtonPressed()));
	connect ( ui.deleteButton, SIGNAL (clicked( )), this, SLOT (deleteButtonPressed()));
}

void EditXLinkDialog::deleteButtonPressed()
{
	delink=true;
	accept();
}

bool EditXLinkDialog::deleteXLink()
{
	return delink;
}	


void EditXLinkDialog::widthChanged( int  w)
{
	xlo->setWidth(w);
}

void EditXLinkDialog::setXLink( XLinkObj * xo)
{
	xlo=xo;
	ui.colorButton->setPaletteBackgroundColor (xlo->getColor() );
	ui.widthBox->setValue(xlo->getWidth());
}

void EditXLinkDialog::setSelection(LinkableMapObj *s)
{
	selection=s;
}

void EditXLinkDialog::colorButtonPressed()
{
	if (xlo)
	{	
		QColor col = QColorDialog::getColor(xlo->getColor(), this );
		if ( !col.isValid() ) return;
		xlo->setColor( col );
		ui.colorButton->setPaletteBackgroundColor (col);
	}
}

void EditXLinkDialog::setColorHeadingButtonPressed()
{
	if (xlo)
	{	
		if (selection && 
			((typeid(*selection) == typeid(BranchObj)) || 
			 (typeid(*selection) == typeid(MapCenterObj)))  )
		{
			QColor col=((BranchObj*)(selection))->getColor();
			xlo->setColor(col);
			ui.colorButton->setPaletteBackgroundColor (col);
		}
	}
}

bool EditXLinkDialog::useSettingsGlobal ()
{
	return ui.useSettings->isChecked();
}
