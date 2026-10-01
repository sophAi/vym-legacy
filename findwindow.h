#ifndef FINDWINDOW_H 
#define FINDWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLayout>
#include <QLabel>


class FindWindow : public QGroupBox
{
	Q_OBJECT

public:
	FindWindow (QWidget* parent=0);

public slots:	
	void popup();
	void cancelPressed();
	void findPressed();
	void findTextChanged(const QString&);
	void clearLineEdit();

signals:
	void findButton(QString);		// emit to mainwindow
	void somethingChanged();	// emit to mainwindow

private:
	QGroupBox *findbox;
	QComboBox *findcombo;
	QPushButton *clearbutton;
	QPushButton *findbutton;
	QPushButton *cancelbutton;
};


#endif
