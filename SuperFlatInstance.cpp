#include <pcl/AutoViewLock.h>
#include <pcl/Console.h>
#include <pcl/StandardStatus.h>
#include <pcl/View.h>

#include "SuperFlatInstance.h"
#include "SuperFlatParameters.h"

namespace pcl
{

SuperFlatInstance::SuperFlatInstance(const MetaProcess* m)
    : ProcessImplementation(m)
    , skyDetectionThreshold(TheSFSkyDetectionThresholdParameter->DefaultValue())
    , objectDiffusionDistance(TheSFObjectDiffusionDistanceParameter->DefaultValue())
    , smoothness(TheSFSmoothnessParameter->DefaultValue())
    , generateSkyMask(TheSFGenerateSkyMask->DefaultValue())
{
}

SuperFlatInstance::SuperFlatInstance(const SuperFlatInstance& x)
    : ProcessImplementation(x)
{
    Assign(x);
}

void SuperFlatInstance::Assign(const ProcessImplementation& p)
{
    const SuperFlatInstance* x = dynamic_cast<const SuperFlatInstance*>(&p);
    if (x != nullptr) {
        skyDetectionThreshold = x->skyDetectionThreshold;
        objectDiffusionDistance = x->objectDiffusionDistance;
        generateSkyMask = x->generateSkyMask;
    }
}

UndoFlags SuperFlatInstance::UndoMode(const View&) const
{
    return UndoFlag::PixelData;
}

bool SuperFlatInstance::CanExecuteOn(const View& view, pcl::String& whyNot) const
{
    if (view.Image().IsComplexSample())
    {
        whyNot = "SuperFlat cannot be executed on complex images.";
        return false;
    }

    return true;
}

bool SuperFlatInstance::ExecuteOn(View& view)
{
    AutoViewLock lock(view);

    StandardStatus status;
    Console console;

    console.EnableAbort();

    ImageVariant image = view.Image();

    if (image.IsComplexSample())
        return false;

    image.SetStatusCallback(&status);

    return true;
}

void* SuperFlatInstance::LockParameter(const MetaParameter* p, size_type /*tableRow*/)
{
    return 0;
}

}	// namespace pcl