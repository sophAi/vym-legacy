#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>

#include "findwindow.h"


extern QString vymName;

FindWindow::FindWindow(QWidget* parent)
	: QGroupBox( tr("Find"), parent )

{
	setWindowTitle(vymName + " - " +tr("Find Text"));

    QVBoxLayout* mainLayout = new QVBoxLayout;
    
    QHBoxLayout *row1Layout = new QHBoxLayout;
    // Create a Label
    QLabel* label = new QLabel;
    label->setText( tr("Text to find:"));
    row1Layout->addWidget( label );


	// Create LineEdit (here QComboBox)
    QHBoxLayout *row2Layout = new QHBoxLayout;
    findcombo = new QComboBox;
	findcombo->setMinimumWidth(150);
	findcombo->setEditable(true);
	connect ( findcombo, SIGNAL( highlighted(int) ), 
		this, SLOT( findPressed() ) );
	connect ( findcombo, SIGNAL( textChanged(const QString &) ), 
		this, SLOT( findTextChanged(const QString&) ) );

	row2Layout->addWidget(findcombo);

	// Create Buttons
    QHBoxLayout *row3Layout = new QHBoxLayout;
	clearbutton = new QPushButton;
	clearbutton->setText(tr("Clear"));
	connect ( clearbutton, SIGNAL( clicked() ), this, SLOT( clearLineEdit() ) );
	row3Layout->addWidget (clearbutton);
	
	cancelbutton = new QPushButton;
	cancelbutton->setText(tr("Cancel"));
	cancelbutton->setShortcut (Qt::Key_Escape);
	connect ( cancelbutton, SIGNAL( clicked() ), this, SLOT( cancelPressed() ) );
	row3Layout->addWidget (cancelbutton);
	
	findbutton = new QPushButton;
	findbutton->setText (tr("Find"));
	findbutton->setDefault (true);
	findbutton->setShortcut (Qt::Key_Return);
	connect ( findbutton, SIGNAL( clicked() ), this, SLOT( findPressed() ) );

	row3Layout->addStretch(2);
	row3Layout->addWidget(findbutton);

	mainLayout->addLayout (row1Layout);
	mainLayout->addLayout (row2Layout);
	mainLayout->addLayout (row3Layout);
	setLayout (mainLayout);
}

void FindWindow::popup()
{
	show();
	findcombo->lineEdit()->selectAll();
	findcombo->setFocus();
}

void FindWindow::cancelPressed()
{
	hide();
}

void FindWindow::findPressed()
{
	emit (findButton(findcombo->currentText() ) );
}

void FindWindow::findTextChanged(const QString&)
{
	emit (somethingChanged() );
}

void FindWindow::clearLineEdit()
{
	findcombo->lineEdit()->clear();
}
