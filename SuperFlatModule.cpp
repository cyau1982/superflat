#define MODULE_VERSION_MAJOR     1
#define MODULE_VERSION_MINOR     0
#define MODULE_VERSION_REVISION  0
#define MODULE_VERSION_BUILD     0
#define MODULE_VERSION_LANGUAGE  eng

#define MODULE_RELEASE_YEAR      2021
#define MODULE_RELEASE_MONTH     4
#define MODULE_RELEASE_DAY       3

#include "SuperFlatModule.h"
#include "SuperFlatProcess.h"
#include "SuperFlatInterface.h"

namespace pcl
{

SuperFlatModule::SuperFlatModule()
{
}

const char* SuperFlatModule::Version() const
{
    return PCL_MODULE_VERSION(MODULE_VERSION_MAJOR,
                              MODULE_VERSION_MINOR,
                              MODULE_VERSION_REVISION,
                              MODULE_VERSION_BUILD,
                              MODULE_VERSION_LANGUAGE);
}

IsoString SuperFlatModule::Name() const
{
    return "SuperFlat";
}

String SuperFlatModule::Description() const
{
    return "PixInsight SuperFlat Module";
}

String SuperFlatModule::Company() const
{
    return "N/A";
}

String SuperFlatModule::Author() const
{
    return "Johnny Qiu";
}

String SuperFlatModule::Copyright() const
{
    return "Copyright (c) 2021 Johnny Qiu";
}

String SuperFlatModule::TradeMarks() const
{
    return "JQ";
}

String SuperFlatModule::OriginalFileName() const
{
#ifdef __PCL_LINUX
    return "superflat-pxm.so";
#endif
#ifdef __PCL_FREEBSD
    return "superflat-pxm.so";
#endif
#ifdef __PCL_MACOSX
    return "superflat-pxm.dylib";
#endif
#ifdef __PCL_WINDOWS
    return "superflat-pxm.dll";
#endif
}

void SuperFlatModule::GetReleaseDate(int& year, int& month, int& day) const
{
    year = MODULE_RELEASE_YEAR;
    month = MODULE_RELEASE_MONTH;
    day = MODULE_RELEASE_DAY;
}

}   // namespace pcl

PCL_MODULE_EXPORT int InstallPixInsightModule(int mode)
{
    new pcl::SuperFlatModule;

    if (mode == pcl::InstallMode::FullInstall) {
        new pcl::SuperFlatProcess;
        new pcl::SuperFlatInterface;
    }

    return 0;
}