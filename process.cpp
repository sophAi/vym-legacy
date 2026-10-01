#include "process.h"

/////////////////////////////////////////////////////////////////
// Process
/////////////////////////////////////////////////////////////////
Process::Process()
{
	connect( this, SIGNAL(readyReadStandardError()),
			 this, SLOT(readProcErrout()) );
	connect( this, SIGNAL(readyReadStandardOutput()),
			 this, SLOT(readProcStdout()) );
	clear();		 
}

Process::~Process()
{
}

void Process::clear()
{
	errOut="";
	stdOut="";
}

void Process::readProcErrout()
{
	errOut+=readAllStandardError();
}

void Process::readProcStdout()
{
	stdOut+=readAllStandardOutput();
}

QString Process::getErrout()
{
	return errOut;
}

QString Process::getStdout()
{
	return stdOut;
}
