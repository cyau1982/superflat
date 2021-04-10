#include "SuperFlatInterface.h"
#include "SuperFlatParameters.h"
#include "SuperFlatProcess.h"

#include <pcl/ErrorHandler.h>
#include <pcl/ViewSelectionDialog.h>

namespace pcl
{

SuperFlatInterface* TheSuperFlatInterface = nullptr;

SuperFlatInterface::SuperFlatInterface()
	: instance(TheSuperFlatProcess)
{
	TheSuperFlatInterface = this;
}

SuperFlatInterface::~SuperFlatInterface()
{
	if (GUI != nullptr)
		delete GUI, GUI = nullptr;
}

IsoString SuperFlatInterface::Id() const
{
	return "SuperFlat";
}

MetaProcess* SuperFlatInterface::Process() const
{
	return TheSuperFlatProcess;
}

IsoString SuperFlatInterface::IconImageSVG() const
{
	return TheSuperFlatProcess->IconImageSVG();
}

InterfaceFeatures SuperFlatInterface::Features() const
{
	return InterfaceFeature::Default;
}

void SuperFlatInterface::ApplyInstance() const
{
	instance.LaunchOnCurrentView();
}

void SuperFlatInterface::ResetInstance()
{
	SuperFlatInstance defaultInstance(TheSuperFlatProcess);
	ImportProcess(defaultInstance);
}

bool SuperFlatInterface::Launch(const MetaProcess& P, const ProcessImplementation*, bool& dynamic, unsigned& /*flags*/)
{
	if (GUI == nullptr)
	{
		GUI = new GUIData(*this);
		SetWindowTitle("SuperFlat");
		UpdateControls();
	}

	dynamic = false;
	return &P == TheSuperFlatProcess;
}

ProcessImplementation* SuperFlatInterface::NewProcess() const
{
	return new SuperFlatInstance(instance);
}

bool SuperFlatInterface::ValidateProcess(const ProcessImplementation& p, String& whyNot) const
{
	if (dynamic_cast<const SuperFlatInstance*>(&p) != nullptr)
		return true;
	whyNot = "Not a SuperFlat instance.";
	return false;
}

bool SuperFlatInterface::RequiresInstanceValidation() const
{
	return true;
}

bool SuperFlatInterface::ImportProcess(const ProcessImplementation& p)
{
	instance.Assign(p);
	UpdateControls();
	return true;
}

#define NO_MASK			String( "<No mask>" )
#define MASK_ID(x)		(x.IsEmpty() ? NO_MASK : x)
#define NONSKY_MASK_ID	MASK_ID(instance.nonSkyMaskViewId)

void SuperFlatInterface::UpdateControls()
{
	GUI->SkyDetectionThreshold_NumericControl.SetValue(instance.skyDetectionThreshold);
	GUI->StarDetectionSensitivity_NumericControl.SetValue(instance.starDetectionSensitivity);
	GUI->ObjectDiffusionDistance_NumericControl.SetValue(instance.objectDiffusionDistance);
	GUI->NonSkyMaskView_Edit.SetText(NONSKY_MASK_ID);
	GUI->Smoothness_NumericControl.SetValue(instance.smoothness);
	GUI->GenerateSkyMask_CheckBox.SetChecked(instance.generateSkyMask);
}

void SuperFlatInterface::__GetFocus(Control& sender)
{
	Edit* e = dynamic_cast<Edit*>(&sender);
	if (e != nullptr)
		if (e->Text() == NO_MASK)
			e->Clear();
}

void SuperFlatInterface::__EditCompleted(Edit& sender)
{
	if (sender == GUI->NonSkyMaskView_Edit)
	{
		try
		{
			String id = sender.Text().Trimmed();
			if (id == NO_MASK)
				id.Clear();
			if (!id.IsEmpty())
				if (!View::IsValidViewId(id))
					throw Error("Invalid view identifier: " + id);
			instance.nonSkyMaskViewId = id;
			sender.SetText(NONSKY_MASK_ID);
		}
		catch (...)
		{
			sender.SetText(NONSKY_MASK_ID);
			try
			{
				throw;
			}
			ERROR_HANDLER
				sender.SelectAll();
			sender.Focus();
		}
	}
}

void SuperFlatInterface::__EditValueUpdated(NumericEdit& sender, double value)
{
	if (sender == GUI->SkyDetectionThreshold_NumericControl)
		instance.skyDetectionThreshold = value;
	else if (sender == GUI->StarDetectionSensitivity_NumericControl)
		instance.starDetectionSensitivity = value;
	else if (sender == GUI->ObjectDiffusionDistance_NumericControl)
		instance.objectDiffusionDistance = value;
	else if (sender == GUI->Smoothness_NumericControl)
		instance.smoothness = value;
}

void SuperFlatInterface::__Click(Button& sender, bool checked)
{
	if (sender == GUI->NonSkyMaskView_ToolButton) {
		ViewSelectionDialog d(instance.nonSkyMaskViewId);
		if (d.Execute() == StdDialogCode::Ok)
		{
			instance.nonSkyMaskViewId = d.Id();
			GUI->NonSkyMaskView_Edit.SetText(NONSKY_MASK_ID);
		}
	} else if (sender == GUI->GenerateSkyMask_CheckBox) {
		instance.generateSkyMask = checked;
	}
}

void SuperFlatInterface::__ViewDrag(Control& sender, const Point& pos, const View& view, unsigned modifiers, bool& wantsView)
{
	if (sender == GUI->NonSkyMaskView_Edit)
		wantsView = true;
}

// ----------------------------------------------------------------------------

void SuperFlatInterface::__ViewDrop(Control& sender, const Point& pos, const View& view, unsigned modifiers)
{
	if (sender == GUI->NonSkyMaskView_Edit) {
		instance.nonSkyMaskViewId = view.FullId();
		GUI->NonSkyMaskView_Edit.SetText(NONSKY_MASK_ID);
	}
}

SuperFlatInterface::GUIData::GUIData(SuperFlatInterface& w)
{
	pcl::Font fnt = w.Font();
	int labelWidth1 = fnt.Width(String("Star detection sensitivity:") + 'T');
	int editWidth1 = fnt.Width(String('0', 12));
	int ui4 = w.LogicalPixelsToPhysical(4);

	SkyDetectionThreshold_NumericControl.label.SetText("Sky detection threshold:");
	SkyDetectionThreshold_NumericControl.label.SetFixedWidth(labelWidth1);
	SkyDetectionThreshold_NumericControl.slider.SetRange(0, 1000);
	SkyDetectionThreshold_NumericControl.slider.SetScaledMinWidth(300);
	SkyDetectionThreshold_NumericControl.SetReal();
	SkyDetectionThreshold_NumericControl.SetRange(TheSFSkyDetectionThresholdParameter->MinimumValue(), TheSFSkyDetectionThresholdParameter->MaximumValue());
	SkyDetectionThreshold_NumericControl.SetPrecision(TheSFSkyDetectionThresholdParameter->Precision());
	SkyDetectionThreshold_NumericControl.edit.SetFixedWidth(editWidth1);
	SkyDetectionThreshold_NumericControl.SetToolTip("<p>This value describes the threshold when comparing the brightness of each pixel to its neighbor area. "
		                                            "If the difference is greater than this threshold, the pixel will not be considered as a part of the sky. "
		                                            "Larger threshold will result in including some faint stars or nebula into sky detection.</p>");
	SkyDetectionThreshold_NumericControl.OnValueUpdated((NumericEdit::value_event_handler) & SuperFlatInterface::__EditValueUpdated, w);
	SkyDetectionThreshold_Sizer.SetSpacing(4);
	SkyDetectionThreshold_Sizer.Add(SkyDetectionThreshold_NumericControl);
	SkyDetectionThreshold_Sizer.AddStretch();

	StarDetectionSensitivity_NumericControl.label.SetText("Star detection sensitivity:");
	StarDetectionSensitivity_NumericControl.label.SetFixedWidth(labelWidth1);
	StarDetectionSensitivity_NumericControl.slider.SetRange(0, 600);
	StarDetectionSensitivity_NumericControl.slider.SetScaledMinWidth(300);
	StarDetectionSensitivity_NumericControl.SetReal();
	StarDetectionSensitivity_NumericControl.SetRange(TheSFStarDetectionSensitivityParameter->MinimumValue(), TheSFStarDetectionSensitivityParameter->MaximumValue());
	StarDetectionSensitivity_NumericControl.SetPrecision(TheSFStarDetectionSensitivityParameter->Precision());
	StarDetectionSensitivity_NumericControl.edit.SetFixedWidth(editWidth1);
	StarDetectionSensitivity_NumericControl.SetToolTip("<p>This value describes the star detection sensitivity. "
		"Increasing sensitivity allows identifying fainter stars, at the risk of identifying noise as stars.</p>");
	StarDetectionSensitivity_NumericControl.OnValueUpdated((NumericEdit::value_event_handler) & SuperFlatInterface::__EditValueUpdated, w);
	StarDetectionSensitivity_Sizer.SetSpacing(4);
	StarDetectionSensitivity_Sizer.Add(StarDetectionSensitivity_NumericControl);
	StarDetectionSensitivity_Sizer.AddStretch();

	ObjectDiffusionDistance_NumericControl.label.SetText("Object diffusion distance:");
	ObjectDiffusionDistance_NumericControl.label.SetFixedWidth(labelWidth1);
	ObjectDiffusionDistance_NumericControl.slider.SetRange(0, 1000);
	ObjectDiffusionDistance_NumericControl.slider.SetScaledMinWidth(300);
	ObjectDiffusionDistance_NumericControl.SetInteger();
	ObjectDiffusionDistance_NumericControl.SetRange(TheSFObjectDiffusionDistanceParameter->MinimumValue(), TheSFObjectDiffusionDistanceParameter->MaximumValue());
	ObjectDiffusionDistance_NumericControl.edit.SetFixedWidth(editWidth1);
	ObjectDiffusionDistance_NumericControl.SetToolTip("<p>After a non-sky mask is generated in sky detection, the mask will be diffused in order to protect the edges "
		                                              "and flares of stars or deep sky objects. This value describes the distance of the diffusion.</p>");
	ObjectDiffusionDistance_NumericControl.OnValueUpdated((NumericEdit::value_event_handler) & SuperFlatInterface::__EditValueUpdated, w);
	ObjectDiffusionDistance_Sizer.SetSpacing(4);
	ObjectDiffusionDistance_Sizer.Add(ObjectDiffusionDistance_NumericControl);
	ObjectDiffusionDistance_Sizer.AddStretch();

	NonSkyMaskView_Label.SetText("Non-sky mask image:");
	NonSkyMaskView_Label.SetFixedWidth(labelWidth1);
	NonSkyMaskView_Label.SetTextAlignment(TextAlign::Right | TextAlign::VertCenter);
	NonSkyMaskView_Edit.SetToolTip("<p>123</p>");
	NonSkyMaskView_Edit.OnGetFocus((Control::event_handler) & SuperFlatInterface::__GetFocus, w);
	NonSkyMaskView_Edit.OnEditCompleted((Edit::edit_event_handler) & SuperFlatInterface::__EditCompleted, w);
	NonSkyMaskView_Edit.OnViewDrag((Control::view_drag_event_handler) & SuperFlatInterface::__ViewDrag, w);
	NonSkyMaskView_Edit.OnViewDrop((Control::view_drop_event_handler) & SuperFlatInterface::__ViewDrop, w);
	NonSkyMaskView_ToolButton.SetIcon(Bitmap(w.ScaledResource(":/icons/select-view.png")));
	NonSkyMaskView_ToolButton.SetScaledFixedSize(20, 20);
	NonSkyMaskView_ToolButton.SetToolTip("<p>User defined non-sky mask which will be added to the generated non-sky mask.</p>");
	NonSkyMaskView_ToolButton.OnClick((Button::click_event_handler) & SuperFlatInterface::__Click, w);
	NonSkyMaskView_Sizer.SetSpacing(4);
	NonSkyMaskView_Sizer.Add(NonSkyMaskView_Label);
	NonSkyMaskView_Sizer.Add(NonSkyMaskView_Edit);
	NonSkyMaskView_Sizer.Add(NonSkyMaskView_ToolButton);

	Smoothness_NumericControl.label.SetText("Smoothness:");
	Smoothness_NumericControl.label.SetFixedWidth(labelWidth1);
	Smoothness_NumericControl.slider.SetRange(0, 1000);
	Smoothness_NumericControl.slider.SetScaledMinWidth(300);
	Smoothness_NumericControl.SetReal();
	Smoothness_NumericControl.SetRange(TheSFSmoothnessParameter->MinimumValue(), TheSFSmoothnessParameter->MaximumValue());
	Smoothness_NumericControl.SetPrecision(TheSFSmoothnessParameter->Precision());
	Smoothness_NumericControl.edit.SetFixedWidth(editWidth1);
	Smoothness_NumericControl.SetToolTip("<p>Smoothness of flat generation.</p>");
	Smoothness_NumericControl.OnValueUpdated((NumericEdit::value_event_handler) & SuperFlatInterface::__EditValueUpdated, w);
	Smoothness_Sizer.SetSpacing(4);
	Smoothness_Sizer.Add(Smoothness_NumericControl);
	Smoothness_Sizer.AddStretch();

	GenerateSkyMask_CheckBox.SetText("Generate sky mask");
	GenerateSkyMask_CheckBox.SetToolTip("<p>If selected, a new image window with a sky mask will be created.</p>");
	GenerateSkyMask_CheckBox.OnClick((Button::click_event_handler) & SuperFlatInterface::__Click, w);
	GenerateSkyMask_Sizer.AddUnscaledSpacing(labelWidth1 + ui4);
	GenerateSkyMask_Sizer.Add(GenerateSkyMask_CheckBox);
	GenerateSkyMask_Sizer.AddStretch();

	Global_Sizer.SetMargin(8);
	Global_Sizer.SetSpacing(4);
	Global_Sizer.Add(SkyDetectionThreshold_Sizer);
	Global_Sizer.Add(StarDetectionSensitivity_Sizer);
	Global_Sizer.Add(ObjectDiffusionDistance_Sizer);
	Global_Sizer.Add(NonSkyMaskView_Sizer);
	Global_Sizer.Add(Smoothness_NumericControl);
	Global_Sizer.Add(GenerateSkyMask_Sizer);

	w.SetSizer(Global_Sizer);

	w.EnsureLayoutUpdated();
	w.AdjustToContents();
	w.SetFixedSize();
}

}	// namespace pcl