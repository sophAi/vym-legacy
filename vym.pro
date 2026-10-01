TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release debug
CONFIG += x86 ppc

QMAKE_CXXFLAGS += -fPIC
QMAKE_CFLAGS += -fPIC

TRANSLATIONS += lang/vym_de.ts
TRANSLATIONS += lang/vym_en.ts
TRANSLATIONS += lang/vym_es.ts
TRANSLATIONS += lang/vym_fr.ts
TRANSLATIONS += lang/vym_it.ts
TRANSLATIONS += lang/vym_pt_BR.ts
TRANSLATIONS += lang/vym_ru.ts
TRANSLATIONS += lang/vym_zh_CN.ts
TRANSLATIONS += lang/vym_sv.ts
TRANSLATIONS += lang/vym_zh_TW.ts

# Manifest embedding was suggested by Qt docs somewhere...
win32: CONFIG += embed_manifest_exe

# Without this, M_PI, and M_PI_2 won`t be defined.
win32:DEFINES *= _USE_MATH_DEFINES

ICON =icons/vym.icns

QT += qt3support
QT += network

HEADERS	+= \
	aboutdialog.h \
	animpoint.h \
	attribute.h \
	attributedelegate.h\
	attributedialog.h \
	attributewidget.h \
	branchobj.h \
	branchpropwindow.h\
	editxlinkdialog.h \
	exportoofiledialog.h \
	exportxhtmldialog.h\
	exports.h \
	extrainfodialog.h \
	file.h \
	findwindow.h \
	flagobj.h \
	flagrowobj.h \
	floatimageobj.h \
	floatobj.h \
	frameobj.h \
	geometry.h \
	headingobj.h \
	highlighter.h \
	historywindow.h \
	imageobj.h \
	imports.h \
	linkablemapobj.h \
	mainwindow.h \
	mapcenterobj.h \
	mapeditor.h \
	mapobj.h \
	misc.h \
	noteobj.h \
	options.h \
	ornamentedobj.h \
	parser.h \
	process.h \
	selection.h \
	showtextdialog.h\
	simplescripteditor.h\
	texteditor.h \
	version.h \
	vymmodel.h \
	xlinkobj.h \
	xml-base.h \
	xml-vym.h \
	xml-freemind.h \
	xmlobj.h\
	xsltproc.h \
	settings.h \
	warningdialog.h

SOURCES	+= \
	aboutdialog.cpp \
	animpoint.cpp \
	attribute.cpp \
	attributedelegate.cpp \
	attributedialog.cpp \
	attributewidget.cpp \
	branchobj.cpp \
	branchpropwindow.cpp \
	editxlinkdialog.cpp \
	exportoofiledialog.cpp \
	exports.cpp \
	exportxhtmldialog.cpp \
	extrainfodialog.cpp \
	file.cpp \
	findwindow.cpp \
	flagobj.cpp \
	flagrowobj.cpp \
	floatimageobj.cpp \
	floatobj.cpp \
	frameobj.cpp \
	geometry.cpp \
	headingobj.cpp \
	highlighter.cpp \
	historywindow.cpp \
	imageobj.cpp \
	imports.cpp \
	linkablemapobj.cpp \
	main.cpp \
	mainwindow.cpp \
	mapcenterobj.cpp \
	mapeditor.cpp \
	mapobj.cpp \
	misc.cpp \
	noteobj.cpp \
	options.cpp \
	ornamentedobj.cpp \
	parser.cpp \
	process.cpp \
	selection.cpp \
	showtextdialog.cpp \
	simplescripteditor.cpp \
	texteditor.cpp \
	version.cpp \
	vymmodel.cpp \
	xlinkobj.cpp \
	xml-base.cpp \
	xml-vym.cpp \
	xml-freemind.cpp \
	xmlobj.cpp \
	xsltproc.cpp \
	settings.cpp \
	warningdialog.cpp

FORMS = \
	attributewidget.ui \
	branchpropwindow.ui \
	exportxhtmldialog.ui \
	extrainfodialog.ui \
	editxlinkdialog.ui \
	historywindow.ui \
	simplescripteditor.ui \
	showtextdialog.ui \
	warningdialog.ui

win32 {
	HEADERS += mkdtemp.h
	SOURCES += mkdtemp.cpp
	RC_FILE = vym.rc
}

#The following lines were inserted by qt3to4
QT += xml

TARGET  = vym-legacy


isEmpty( PREFIX ) {
	PREFIX = /usr
	count( INSTALLDIR, 1 ) {
		PREFIX = $${INSTALLDIR}
		message( "Please use PREFIX instead of INSTALLDIR" )
	}
}
isEmpty( BINDIR ) {
	BINDIR = $${PREFIX}/bin
}
isEmpty( DATADIR ) {
	DATADIR = $${PREFIX}/share
}
isEmpty( DOCDIR ) {
	DOCDIR = $${DATADIR}/doc/vym-legacy
}

message( "Installation directory" )
message( $$PREFIX )


target.path = $${BINDIR}
INSTALLS += target

support.files = styles/ scripts/ icons/ flags/ lang/ macros/ exports/ demos/
support.path = $${DATADIR}/vym-legacy
INSTALLS += support 

doc.files = doc/vym.pdf
doc.path = $${DOCDIR}
INSTALLS += doc
DEFINES += VYM_DOCDIR=\\\"$${DOCDIR}\\\"
DEFINES += VYM_BASEDIR=\\\"$${support.path}\\\"

