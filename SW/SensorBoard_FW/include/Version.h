#pragma once

#include "GitHash.h"

//! \brief The major version of the firmware
#define VERSION_MAJOR "2"
//! \brief The minor version of the firmware
#define VERSION_MINOR "0"
//! \brief The patch version of the firmware
#define VERSION_PATCH "1"
//! \brief All versions as one string with the git commit hash at the end
#define VERSION_STRING VERSION_MAJOR "." VERSION_MINOR "." VERSION_PATCH "_" GIT_HASH
//! \brief Is it a beta version?
#define VERSION_BETA true

#if VERSION_BETA
	//! \brief Full version string, containing all information
	#define VERSION_FULL "b" VERSION_STRING
#else
	//! \brief Full version string, containing all information
	#define VERSION_FULL VERSION_STRING
#endif
