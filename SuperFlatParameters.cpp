#include "SuperFlatParameters.h"

namespace pcl
{

SFSkyDetectionThreshold* TheSFSkyDetectionThresholdParameter = nullptr;
SFObjectDiffusionDistance* TheSFObjectDiffusionDistanceParameter = nullptr;
SFSmoothness* TheSFSmoothnessParameter = nullptr;
SFGenerateSkyMask* TheSFGenerateSkyMask = nullptr;

SFSkyDetectionThreshold::SFSkyDetectionThreshold(MetaProcess* P) : MetaFloat(P)
{
    TheSFSkyDetectionThresholdParameter = this;
}

IsoString SFSkyDetectionThreshold::Id() const
{
    return "skyDetectionThreshold";
}

int SFSkyDetectionThreshold::Precision() const
{
    return 5;
}

double SFSkyDetectionThreshold::MinimumValue() const
{
    return 0.00001;
}

double SFSkyDetectionThreshold::MaximumValue() const
{
    return 0.01;
}

double SFSkyDetectionThreshold::DefaultValue() const
{
    return 0.0001;
}

SFObjectDiffusionDistance::SFObjectDiffusionDistance(MetaProcess* P) : MetaFloat(P)
{
    TheSFObjectDiffusionDistanceParameter = this;
}

IsoString SFObjectDiffusionDistance::Id() const
{
    return "objectDiffusionDistance";
}

int SFObjectDiffusionDistance::Precision() const
{
    return 2;
}

double SFObjectDiffusionDistance::MinimumValue() const
{
    return 0.0;
}

double SFObjectDiffusionDistance::MaximumValue() const
{
    return 10.0;
}

double SFObjectDiffusionDistance::DefaultValue() const
{
    return 1.0;
}

SFSmoothness::SFSmoothness(MetaProcess* P) : MetaFloat(P)
{
    TheSFSmoothnessParameter = this;
}

IsoString SFSmoothness::Id() const
{
    return "smoothness";
}

int SFSmoothness::Precision() const
{
    return 2;
}

double SFSmoothness::MinimumValue() const
{
    return 0.0;
}

double SFSmoothness::MaximumValue() const
{
    return 10.0;
}

double SFSmoothness::DefaultValue() const
{
    return 5.0;
}

SFGenerateSkyMask::SFGenerateSkyMask(MetaProcess* P) : MetaBoolean(P)
{
    TheSFGenerateSkyMask = this;
}

IsoString SFGenerateSkyMask::Id() const
{
    return "generateSkyMask";
}

bool SFGenerateSkyMask::DefaultValue() const
{
    return false;
}

}	// namespace pcl