#ifndef PROCESS_H
#define PROCESS_H

#include <QProcess>
#include <QString>


using namespace std;

class Process:public QProcess
{
	Q_OBJECT
public:
    Process ();
	~Process ();
	void clear();
	QString getErrout();
	QString getStdout();
	

public slots:
	virtual void readProcErrout();
	virtual void readProcStdout();

private:
	QString errOut;
	QString stdOut;
};

#endif
