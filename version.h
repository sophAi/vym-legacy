#ifndef VERSION_H 
#define VERSION_H

#include <QString>

#define __VYM_NAME "VYM"
#define __VYM_VERSION "1.12.2h"
#define __VYM_CODENAME "Maintenance Update "
//#define __VYM_CODENAME "Codename: development version"
#define __VYM_BUILD_DATE "2009-05-06"


bool checkVersion(const QString &);
bool checkVersion(const QString &, const QString &);

#endif
