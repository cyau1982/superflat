#include <pcl/Console.h>
#include <pcl/Arguments.h>
#include <pcl/View.h>
#include <pcl/Exception.h>

#include "SuperFlatProcess.h"
#include "SuperFlatParameters.h"
#include "SuperFlatInstance.h"
#include "SuperFlatInterface.h"

namespace pcl
{

SuperFlatProcess* TheSuperFlatProcess = nullptr;

SuperFlatProcess::SuperFlatProcess()
{
    TheSuperFlatProcess = this;

    // Instantiate process parameters
    new SFSkyDetectionThreshold(this);
    new SFStarDetectionSensitivity(this);
    new SFObjectDiffusionDistance(this);
    new SFSmoothness(this);
    new SFDownsample(this);
    new SFGenerateSkyMask(this);
    new SFTestSkyDetection(this);
}

IsoString SuperFlatProcess::Id() const
{
    return "SuperFlat";
}

IsoString SuperFlatProcess::Category() const
{
    return "BackgroundModelization";
}

// ----------------------------------------------------------------------------

uint32 SuperFlatProcess::Version() const
{
    return 0x100;
}

// ----------------------------------------------------------------------------

String SuperFlatProcess::Description() const
{
    return "";
}

// ----------------------------------------------------------------------------

IsoString SuperFlatProcess::IconImageSVG() const
{
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
           "<svg width=\"512\" height=\"512\" version=\"1.1\" viewBox=\"0 0 135.47 135.47\" xmlns=\"http://www.w3.org/2000/svg\">"
           "<defs>"
           "<radialGradient id=\"a\" r=\"0.75\">"
           "<stop stop-color=\"#ffffff\" offset=\"0\"/>"
           "<stop stop-color=\"#3f3f3f\" offset=\"1\"/>"
           "</radialGradient>"
           "</defs>"
           "<g transform=\"translate(0 -161.53)\">"
           "<g transform=\"translate(-18.464 -2.6177)\">"
           "<rect x=\"27.989\" y=\"173.68\" width=\"116.42\" height=\"116.42\" fill=\"url(#a)\" stroke=\"#000\" stroke-width=\"6.35\"/>"
           "</g>"
           "</g>"
           "</svg>";
}
// ----------------------------------------------------------------------------

ProcessInterface* SuperFlatProcess::DefaultInterface() const
{
    return TheSuperFlatInterface;
}
// ----------------------------------------------------------------------------

ProcessImplementation* SuperFlatProcess::Create() const
{
    return new SuperFlatInstance(this);
}

// ----------------------------------------------------------------------------

ProcessImplementation* SuperFlatProcess::Clone(const ProcessImplementation& p) const
{
    const SuperFlatInstance* instPtr = dynamic_cast<const SuperFlatInstance*>(&p);
    return (instPtr != 0) ? new SuperFlatInstance(*instPtr) : 0;
}

// ----------------------------------------------------------------------------

bool SuperFlatProcess::NeedsValidation() const
{
    return false;
}

// ----------------------------------------------------------------------------

bool SuperFlatProcess::CanProcessCommandLines() const
{
    return false;
}

}	// namespace pcl