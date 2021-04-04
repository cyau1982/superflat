#ifndef __SuperFlatProcess_h
#define __SuperFlatProcess_h

#include <pcl/MetaProcess.h>

namespace pcl
{

class SuperFlatProcess : public MetaProcess
{
public:
    SuperFlatProcess();

    IsoString Id() const override;
    IsoString Category() const override;
    uint32 Version() const override;
    String Description() const override;
    IsoString IconImageSVG() const override;
    ProcessInterface* DefaultInterface() const override;
    ProcessImplementation* Create() const override;
    ProcessImplementation* Clone(const ProcessImplementation&) const override;
    bool NeedsValidation() const override;
    bool CanProcessCommandLines() const override;
};

PCL_BEGIN_LOCAL
extern SuperFlatProcess* TheSuperFlatProcess;
PCL_END_LOCAL

}	// namespace pcl

#endif	// __SuperFlatProcess_h