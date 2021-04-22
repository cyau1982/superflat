#include "SuperFlatParameters.h"

namespace pcl
{

SFSkyDetectionThreshold* TheSFSkyDetectionThresholdParameter = nullptr;
SFStarDetectionSensitivity* TheSFStarDetectionSensitivityParameter = nullptr;
SFObjectDiffusionDistance* TheSFObjectDiffusionDistanceParameter = nullptr;
SFSmoothness* TheSFSmoothnessParameter = nullptr;
SFGenerateSkyMask* TheSFGenerateSkyMaskParameter = nullptr;
SFTestSkyDetection* TheSFTestSkyDetectionParameter = nullptr;
SFDownsample* TheSFDownsampleParameter = nullptr;

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

SFStarDetectionSensitivity::SFStarDetectionSensitivity(MetaProcess* P) : MetaFloat(P)
{
    TheSFStarDetectionSensitivityParameter = this;
}

IsoString SFStarDetectionSensitivity::Id() const
{
    return "starDetectionSensitivity";
}

int SFStarDetectionSensitivity::Precision() const
{
    return 2;
}

double SFStarDetectionSensitivity::MinimumValue() const
{
    return 0.0;
}

double SFStarDetectionSensitivity::MaximumValue() const
{
    return 6.0;
}

double SFStarDetectionSensitivity::DefaultValue() const
{
    return 4.0;
}

SFObjectDiffusionDistance::SFObjectDiffusionDistance(MetaProcess* P) : MetaInt8(P)
{
    TheSFObjectDiffusionDistanceParameter = this;
}

IsoString SFObjectDiffusionDistance::Id() const
{
    return "objectDiffusionDistance";
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
    return 5.0;
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
    TheSFGenerateSkyMaskParameter = this;
}

IsoString SFGenerateSkyMask::Id() const
{
    return "generateSkyMask";
}

bool SFGenerateSkyMask::DefaultValue() const
{
    return false;
}

SFTestSkyDetection::SFTestSkyDetection(MetaProcess* P) : MetaBoolean(P)
{
    TheSFTestSkyDetectionParameter = this;
}

IsoString SFTestSkyDetection::Id() const
{
    return "testSkyDetection";
}

bool SFTestSkyDetection::DefaultValue() const
{
    return false;
}

SFDownsample::SFDownsample(MetaProcess* P) : MetaUInt32(P)
{
    TheSFDownsampleParameter = this;
}

IsoString SFDownsample::Id() const
{
    return "downsample";
}

double SFDownsample::DefaultValue() const
{
    return 2;
}

double SFDownsample::MinimumValue() const
{
    return 1;
}

double SFDownsample::MaximumValue() const
{
    return 16;
}

}	// namespace pcl