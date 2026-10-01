#include "xmlobj.h"

#include <QRegExp>


// returns masked "<" ">" "&"
QString quotemeta(const QString &s)
{
	QString r=s;
    QRegExp  rx("&(?!amp;)");
    r.replace ( rx,"&amp;");
    rx.setPattern( ">");
    r.replace ( rx,"&gt;");
    rx.setPattern( "<");
    r.replace ( rx,"&lt;");
    rx.setPattern( "\"");
    r.replace ( rx,"&quot;");
    return r;
}



int XMLObj::actindent=0;		// make instance of actindent

XMLObj::XMLObj()
{
    indentwidth=4;
}


// returns <s at />
QString XMLObj::singleElement(QString s, QString at)
{
    return indent() + "<" + s +" " + at +" " + "/>\n";
}

// returns <s>
QString XMLObj::beginElement(QString s)
{
    return indent() + "<" + s + ">\n";
}

// returns <s at>
QString XMLObj::beginElement(QString s, QString at)
{
    return indent() + "<" + s + " " + at + ">\n";
}

// returns </s>
QString XMLObj::endElement(QString s)
{
    return indent() + "</" + s + ">\n";
}

// returns  at="val"
QString XMLObj::attribut(QString at, QString val)
{
    return " " + at + "=\"" + quotemeta (val) + "\""; 
}

// returns <s> val </s>
QString XMLObj::valueElement(QString el, QString val)
{
    return indent() + "<" + el + ">" + quotemeta(val) + "</" +el + ">\n";
}

// returns <s at> val </s>
QString XMLObj::valueElement(QString el, QString val, QString at)
{
    return indent() + "<" + el + " " + at + ">" + quotemeta(val) + "</" +el + ">\n";
}

void XMLObj::incIndent()
{
    actindent++;
}	

void XMLObj::decIndent()
{
    actindent--;
    if (actindent<0) actindent=0;
}	

QString XMLObj::indent()
{
    QString s;
    int i;
    for (i=0; i<actindent*indentwidth; i++)
    {
		s= s + " ";
    }
    return s;
}	

