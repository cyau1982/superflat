#ifndef __SuperFlatInterface_h
#define __SuperFlatInterface_h

#include <pcl/CheckBox.h>
#include <pcl/Edit.h>
#include <pcl/Label.h>
#include <pcl/NumericControl.h>
#include <pcl/ProcessInterface.h>
#include <pcl/Sizer.h>
#include <pcl/SpinBox.h>
#include <pcl/ToolButton.h>

#include "SuperFlatInstance.h"

namespace pcl {

class SuperFlatInterface : public ProcessInterface
{
public:
    SuperFlatInterface();
    virtual ~SuperFlatInterface();

    IsoString Id() const override;
    MetaProcess* Process() const override;
    IsoString IconImageSVG() const override;
    InterfaceFeatures Features() const override;
    void ApplyInstance() const override;
    void ResetInstance() override;
    bool Launch(const MetaProcess&, const ProcessImplementation*, bool& dynamic, unsigned& /*flags*/) override;
    ProcessImplementation* NewProcess() const override;
    bool ValidateProcess(const ProcessImplementation&, pcl::String& whyNot) const override;
    bool RequiresInstanceValidation() const override;
    bool ImportProcess(const ProcessImplementation&) override;

private:
    SuperFlatInstance instance;

    struct GUIData
    {
        GUIData(SuperFlatInterface&);

        VerticalSizer   Global_Sizer;
            HorizontalSizer SkyDetectionThreshold_Sizer;
                NumericControl  SkyDetectionThreshold_NumericControl;
            HorizontalSizer StarDetectionSensitivity_Sizer;
                NumericControl  StarDetectionSensitivity_NumericControl;
            HorizontalSizer ObjectDiffusionDistance_Sizer;
                NumericControl  ObjectDiffusionDistance_NumericControl;
            HorizontalSizer NonSkyMaskView_Sizer;
                Label           NonSkyMaskView_Label;
                Edit            NonSkyMaskView_Edit;
                ToolButton      NonSkyMaskView_ToolButton;
            HorizontalSizer Smoothness_Sizer;
                NumericControl  Smoothness_NumericControl;
            HorizontalSizer   Downsample_Sizer;
                Label             Downsample_Label;
                SpinBox           Downsample_SpinBox;
            HorizontalSizer GenerateSkyMask_Sizer;
                CheckBox        GenerateSkyMask_CheckBox;
            HorizontalSizer TestSkyDetection_Sizer;
                CheckBox        TestSkyDetection_CheckBox;
    };

    GUIData* GUI = nullptr;

    void UpdateControls();
    void __GetFocus(Control& sender);
    void __EditCompleted(Edit& sender);
    void __EditValueUpdated(NumericEdit& sender, double value);
    void __SpinBoxValueUpdated(SpinBox& sender, int value);
    void __Click(Button& sender, bool checked);
    void __ViewDrag(Control& sender, const Point& pos, const View& view, unsigned modifiers, bool& wantsView);
    void __ViewDrop(Control& sender, const Point& pos, const View& view, unsigned modifiers);

    friend struct GUIData;
};

PCL_BEGIN_LOCAL
extern SuperFlatInterface* TheSuperFlatInterface;
PCL_END_LOCAL

}	// namespace pcl

#endif  // __SuperFlatInterface_h