#ifndef __SuperFlatParameters_h
#define __SuperFlatParameters_h

#include <pcl/MetaParameter.h>

namespace pcl
{

PCL_BEGIN_LOCAL

class SFSkyDetectionThreshold : public MetaFloat
{
public:
    SFSkyDetectionThreshold(MetaProcess*);

    IsoString Id() const override;
    int Precision() const override;
    double MinimumValue() const override;
    double MaximumValue() const override;
    double DefaultValue() const override;
};

extern SFSkyDetectionThreshold* TheSFSkyDetectionThresholdParameter;

class SFStarDetectionSensitivity : public MetaFloat
{
public:
    SFStarDetectionSensitivity(MetaProcess*);

    IsoString Id() const override;
    int Precision() const override;
    double MinimumValue() const override;
    double MaximumValue() const override;
    double DefaultValue() const override;
};

extern SFStarDetectionSensitivity* TheSFStarDetectionSensitivityParameter;

class SFObjectDiffusionDistance : public MetaInt8
{
public:
    SFObjectDiffusionDistance(MetaProcess*);

    IsoString Id() const override;
    double MinimumValue() const override;
    double MaximumValue() const override;
    double DefaultValue() const override;
};

extern SFObjectDiffusionDistance* TheSFObjectDiffusionDistanceParameter;

class SFSmoothness : public MetaFloat
{
public:
    SFSmoothness(MetaProcess*);

    IsoString Id() const override;
    int Precision() const override;
    double MinimumValue() const override;
    double MaximumValue() const override;
    double DefaultValue() const override;
};

extern SFSmoothness* TheSFSmoothnessParameter;

class SFGenerateSkyMask : public MetaBoolean
{
public:
    SFGenerateSkyMask(MetaProcess*);

    IsoString Id() const override;
    bool DefaultValue() const override;
};

extern SFGenerateSkyMask* TheSFGenerateSkyMask;

PCL_END_LOCAL

}	// namespace pcl

#endif	// __SuperFlatParameters_h