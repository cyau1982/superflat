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
    bool IsHistoryUpdater(const View& v) const override;
    UndoFlags UndoMode(const View&) const override;
    bool CanExecuteOn(const View&, pcl::String& whyNot) const override;
    bool ExecuteOn(View&) override;
    void* LockParameter(const MetaParameter*, size_type tableRow) override;

private:
    float skyDetectionThreshold;
    float starDetectionSensitivity;
    int objectDiffusionDistance;
    String nonSkyMaskViewId;
    float smoothness;
    bool generateSkyMask;

    ImageVariant inpaintRemaining;

    template <class P>
    static void genSkyMask(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>& ref, GenericImage<P>& maskImage, int y, int channel);
    template <class P>
    static void diffuse(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>& pyramid, GenericImage<P>& maskImage, int y, int channel);
    template <class P>
    static void gconv(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>& input, GenericImage<P>& conv, int y, int channel);
    template <class P>
    static void calcWeight(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>& input, GenericImage<P>& weight, int y, int channel);
    template <class P>
    static void inpaint(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>& inputs, GenericImage<P>& output, int y, int channel);

    friend class SuperFlatProcess;
    friend class SuperFlatInterface;
};

}	// namespace pcl

#endif	// __SuperFlatInstance_h