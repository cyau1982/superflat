#ifndef __SuperFlatInstance_h
#define __SuperFlatInstance_h

#include <pcl/ProcessImplementation.h>
#include <pcl/MetaParameter.h> // pcl_enum

namespace pcl
{

class SuperFlatInstance : public ProcessImplementation
{
public:
    SuperFlatInstance(const MetaProcess*);
    SuperFlatInstance(const SuperFlatInstance&);

    void Assign(const ProcessImplementation&) override;
    UndoFlags UndoMode(const View&) const override;
    bool CanExecuteOn(const View&, pcl::String& whyNot) const override;
    bool ExecuteOn(View&) override;
    void* LockParameter(const MetaParameter*, size_type tableRow) override;

private:
    float skyDetectionThreshold;
    float objectDiffusionDistance;
    String nonSkyMaskViewId;
    float smoothness;
    bool generateSkyMask;

    friend class SuperFlatProcess;
    friend class SuperFlatInterface;
};

}	// namespace pcl

#endif	// __SuperFlatInstance_h