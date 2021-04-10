#include <pcl/AutoViewLock.h>
#include <pcl/Console.h>
#include <pcl/FFTConvolution.h>
#include <pcl/MorphologicalTransformation.h>
#include <pcl/MultiscaleLinearTransform.h>
#include <pcl/PixelInterpolation.h>
#include <pcl/Resample.h>
#include <pcl/StandardStatus.h>
#include <pcl/VariableShapeFilter.h>
#include <pcl/View.h>

#include "SuperFlatInstance.h"
#include "SuperFlatParameters.h"

namespace pcl
{

template <class P>
class SuperFlatThread : public Thread
{
public:
    typedef void(*LineProcessFunc)(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>&, GenericImage<P>&, int, int);

    SuperFlatThread(int id, LineProcessFunc lineProcessFunc, const AbstractImage::ThreadData& data, SuperFlatInstance* superFlat,
                    ReferenceArray<GenericImage<P>>& srcImages, GenericImage<P>& dstImage,
                    int channel, int firstRow, int endRow)
        : m_id(id)
        , m_lineProcessFunc(lineProcessFunc)
        , m_data(data)
        , m_superFlat(superFlat)
        , m_srcImages(srcImages)
        , m_dstImage(dstImage)
        , m_channel(channel)
        , m_firstRow(firstRow)
        , m_endRow(endRow)
        , m_threadErrorMsg("")
    {
    }

    void Run() override
    {
        INIT_THREAD_MONITOR();
        try {
            for (int y = m_firstRow; y < m_endRow; y++) {
                m_lineProcessFunc(m_superFlat, m_srcImages, m_dstImage, y, m_channel);
                UPDATE_THREAD_MONITOR(65536);
            }
        } catch (...) {
            volatile AutoLock lock(m_data.mutex);
            try {
                throw;
            } catch (Exception& x) {
                m_threadErrorMsg = x.Message();
            } catch (std::bad_alloc&) {
                m_threadErrorMsg = "Out of memory";
            } catch (...) {
                m_threadErrorMsg = "Unknown error";
            }
        }
    }

    static void dispatch(LineProcessFunc lineProcessFunc, SuperFlatInstance* superFlat,
                         ReferenceArray<GenericImage<P>>& srcImages, GenericImage<P>& dstImage, int channel)
    {
        Array<size_type> L = Thread::OptimalThreadLoads(dstImage.Height(), 1, PCL_MAX_PROCESSORS);
        ReferenceArray<SuperFlatThread> threads;
        AbstractImage::ThreadData data(dstImage, dstImage.NumberOfPixels());
        for (int i = 0, n = 0; i < int(L.Length()); n += int(L[i++]))
            threads << new SuperFlatThread(i, lineProcessFunc, data, superFlat, srcImages, dstImage, channel, n, n + int(L[i]));
        AbstractImage::RunThreads(threads, data);
        for (SuperFlatThread& t : threads)
            if (t.m_threadErrorMsg != "")
                throw Error(t.m_threadErrorMsg);
        threads.Destroy();
        dstImage.Status() = data.status;
    }

private:
    int m_id;
    LineProcessFunc m_lineProcessFunc;
    const AbstractImage::ThreadData& m_data;
    SuperFlatInstance* m_superFlat;
    ReferenceArray<GenericImage<P>>& m_srcImages;
    GenericImage<P>& m_dstImage;
    int m_channel;
    int m_firstRow;
    int m_endRow;
    String m_threadErrorMsg;
};

SuperFlatInstance::SuperFlatInstance(const MetaProcess* m)
    : ProcessImplementation(m)
    , skyDetectionThreshold(TheSFSkyDetectionThresholdParameter->DefaultValue())
    , starDetectionSensitivity(TheSFStarDetectionSensitivityParameter->DefaultValue())
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
        starDetectionSensitivity = x->starDetectionSensitivity;
        objectDiffusionDistance = x->objectDiffusionDistance;
        smoothness = x->smoothness;
        generateSkyMask = x->generateSkyMask;
    }
}

bool SuperFlatInstance::IsHistoryUpdater(const View& view) const
{
    return false;
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
    } else if (!view.Image().IsFloatSample()) {
        whyNot = "SuperFlat can only be executed on float images.";
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

    if (image.IsComplexSample() || !view.Image().IsFloatSample())
        return false;

    image.SetStatusCallback(&status);

    // Step 1: Star detection
    ImageVariant starMask;
    starMask.CopyImage(image);
    starMask.EnsureUniqueImage();
    starMask.SetStatusCallback(nullptr);
    image.Status().Initialize("Performing star detection", 3);
    MultiscaleLinearTransform mlt(4);
    mlt << starMask;
    mlt.DisableLayer(0);
    mlt.DisableLayer(4);
    mlt >> starMask;
    starMask.Truncate(0.0f, 1.0f);
    starMask.Normalize();
    image.Status() += 1;

    MorphologicalTransformation mf;
    mf.SetStructure(BoxStructure(3));
    mf.SetOperator(MedianFilter());
    mf >> starMask;
    starMask.Binarize(pcl::Pow10(-starDetectionSensitivity));
    image.Status() += 1;

    MorphologicalTransformation df;
    df.SetStructure(CircularStructure(2 * objectDiffusionDistance + 3));
    df.SetOperator(DilationFilter());
    df >> starMask;
    image.Status() += 1;
    starMask.Invert();

    // Step 2: Convolution
    ImageVariant ref;
    ref.CopyImage(image);
    ref.EnsureUniqueImage();
    ref.SetStatusCallback(nullptr);
    image.Status().Initialize("Creating sky mask", objectDiffusionDistance + 3);
    float sigma = 255.0f;
    VariableShapeFilter H(sigma, 5.0f, 0.01f, 1.0f, 0.0f);
    FFTConvolution(H) >> ref;

    // Step 3: Create sky mask
    ImageVariant mask;
    mask.CreateImageAs(image);
    mask.AllocateImage(image.Width(), image.Height(), image.NumberOfChannels(), image.ColorSpace());
    mask.SetStatusCallback(nullptr);
    mask.One();
    for (int i = -1; i < objectDiffusionDistance; i++) {
        ImageVariant conv;
        conv.CopyImage(image);
        conv.EnsureUniqueImage();
        conv.SetStatusCallback(nullptr);
        if (i >= 0) {
            float sigma = Pow2<float>(float(i));
            VariableShapeFilter H(sigma, 5.0f, 0.01f, 1.0f, 0.0f);
            FFTConvolution(H) >> conv;
        }
        if (image.BitsPerSample() == 32) {
            ReferenceArray<GenericImage<FloatPixelTraits>> input;
            input << &static_cast<Image&>(*ref);
            for (int c = 0; c < image.NumberOfChannels(); c++)
                SuperFlatThread<FloatPixelTraits>::dispatch(genSkyMask<FloatPixelTraits>, this, input, static_cast<Image&>(*conv), c);
        } else if (image.BitsPerSample() == 64) {
            ReferenceArray<GenericImage<DoublePixelTraits>> input;
            input << &static_cast<DImage&>(*ref);
            for (int c = 0; c < image.NumberOfChannels(); c++)
                SuperFlatThread<DoublePixelTraits>::dispatch(genSkyMask<DoublePixelTraits>, this, input, static_cast<DImage&>(*conv), c);
        }
        mask.Multiply(conv);
        image.Status() += 1;
    }

    // Step 4: Remove noise using 3x3 median filter, add star mask
    mf >> mask;
    mask.Multiply(starMask);

    image.Status() += 1;

    // Step 5: Add user-defined non-sky mask
    if (!nonSkyMaskViewId.IsEmpty()) {
        View nonSkyMaskView = View::ViewById(nonSkyMaskViewId);
        if (nonSkyMaskView.IsNull())
            throw Error("No such view (non-sky mask): " + nonSkyMaskViewId);

        ImageVariant nonSkyMask;
        {
            AutoViewLock viewLock(nonSkyMaskView);
            nonSkyMask.CopyImage(nonSkyMaskView.Image());
        }

        if ((nonSkyMaskView.Width() != view.Width()) || (nonSkyMaskView.Height() != view.Height()) || (nonSkyMask.NumberOfChannels() != image.NumberOfChannels()))
            throw Error("Non-sky mask must match the same format (width, height, channel) of the image beging processed.");

        nonSkyMask.SetStatusCallback(nullptr);
        nonSkyMask.Binarize(0.5);
        mask.Multiply(nonSkyMask.Invert());
    }

    // Step 6: Extract sky as flat
    ImageVariant flat;
    flat.CopyImage(image);
    flat.EnsureUniqueImage();
    flat.SetStatusCallback(nullptr);
    flat.Multiply(mask);
    image.Status() += 1;
    image.Status().Complete();

    // Step 7: Inpaint
    // Step 7.1: Gaussian pyramid
    ReferenceArray<ImageVariant> pyramid;
    ReferenceArray<ImageVariant> pyramidWeight;
    ImageVariant* lastImg = &flat;
    int w = flat.Width();
    int h = flat.Height();
    int it = 0;
    image.Status().Initialize("Inpainting", pcl::Log2(pcl::Min(w, h)) + 1);
    while ((w > 0) && (h > 0)) {
        ImageVariant* conv = new ImageVariant;
        ImageVariant* weight = new ImageVariant;
        if (it++ == 0) {
            conv->CopyImage(flat);
            conv->EnsureUniqueImage();
            weight->CopyImage(mask);
            weight->EnsureUniqueImage();
        } else {
            conv->CreateImageAs(flat);
            conv->AllocateImage(w, h, flat.NumberOfChannels(), flat.ColorSpace());
            weight->CreateImageAs(flat);
            weight->AllocateImage(w, h, flat.NumberOfChannels(), flat.ColorSpace());
            if (image.BitsPerSample() == 32) {
                ReferenceArray<GenericImage<FloatPixelTraits>> input;
                input << &static_cast<Image&>(**lastImg);
                for (int c = 0; c < image.NumberOfChannels(); c++) {
                    SuperFlatThread<FloatPixelTraits>::dispatch(gconv<FloatPixelTraits>, this, input, static_cast<Image&>(**conv), c);
                    SuperFlatThread<FloatPixelTraits>::dispatch(calcWeight<FloatPixelTraits>, this, input, static_cast<Image&>(**weight), c);
                }
            } else if (image.BitsPerSample() == 64) {
                ReferenceArray<GenericImage<DoublePixelTraits>> input;
                input << &static_cast<DImage&>(**lastImg);
                for (int c = 0; c < image.NumberOfChannels(); c++) {
                    SuperFlatThread<DoublePixelTraits>::dispatch(gconv<DoublePixelTraits>, this, input, static_cast<DImage&>(**conv), c);
                    SuperFlatThread<DoublePixelTraits>::dispatch(calcWeight<DoublePixelTraits>, this, input, static_cast<DImage&>(**conv), c);
                }
            }
        }
        pyramid << conv;
        pyramidWeight << weight;
        lastImg = conv;
        w /= 2;
        h /= 2;
        image.Status() += 1;
    }
    image.Status().Complete();

    // Step 7.2: Reconstruct
    if (image.BitsPerSample() == 32) {
        ReferenceArray<GenericImage<FloatPixelTraits>> inputs;
        for (int i = 0; i < pyramid.Length(); i++) {
            BicubicFilterPixelInterpolation bs(2, 2, CubicBSplineFilter());
            Resample rs(bs, flat.Width() / pyramid[i].Width(), flat.Height() / pyramid[i].Height());
            rs >> pyramid[i];
            rs >> pyramidWeight[i];
            inputs << &static_cast<Image&>(*pyramid[i]);
            inputs << &static_cast<Image&>(*pyramidWeight[i]);
        }
        for (int c = 0; c < image.NumberOfChannels(); c++)
            SuperFlatThread<FloatPixelTraits>::dispatch(inpaint<FloatPixelTraits>, this, inputs, static_cast<Image&>(*flat), c);
    } else if (image.BitsPerSample() == 64) {
        ReferenceArray<GenericImage<DoublePixelTraits>> inputs;
        for (int i = 0; i < pyramid.Length(); i++) {
            BicubicFilterPixelInterpolation bs(2, 2, CubicBSplineFilter());
            Resample rs(bs, flat.Width() / pyramid[i].Width(), flat.Height() / pyramid[i].Height());
            rs >> pyramid[i];
            rs >> pyramidWeight[i];
            inputs << &static_cast<DImage&>(*pyramid[i]);
            inputs << &static_cast<DImage&>(*pyramidWeight[i]);
        }
        for (int c = 0; c < image.NumberOfChannels(); c++)
            SuperFlatThread<DoublePixelTraits>::dispatch(inpaint<DoublePixelTraits>, this, inputs, static_cast<DImage&>(*flat), c);
    }

    pyramid.Destroy();

    // Step 8: Blur
    VariableShapeFilter H2(pcl::Pow(1.7f, smoothness), 5.0f, 0.01f, 1.0f, 0.0f);
    FFTConvolution(H2) >> flat;

    IsoString id = view.FullId() + "_flat";
    ImageWindow OutputWindow = ImageWindow(flat.Width(), flat.Height(), flat.NumberOfChannels(), flat.BitsPerSample(), true, flat.IsColor(), true, id);
    if (OutputWindow.IsNull())
        throw Error("Unable to create image window: " + id);
    OutputWindow.MainView().Lock();
    OutputWindow.MainView().Image().CopyImage(flat);
    OutputWindow.MainView().Unlock();
    OutputWindow.Show();

    if (generateSkyMask) {
        IsoString id = view.FullId() + "_skymask";
        ImageWindow OutputWindow = ImageWindow(mask.Width(), mask.Height(), mask.NumberOfChannels(), mask.BitsPerSample(), true, mask.IsColor(), true, id);
        if (OutputWindow.IsNull())
            throw Error("Unable to create image window: " + id);
        OutputWindow.MainView().Lock();
        OutputWindow.MainView().Image().CopyImage(mask);
        OutputWindow.MainView().Unlock();
        OutputWindow.Show();
    }

    return true;
}

void* SuperFlatInstance::LockParameter(const MetaParameter* p, size_type /*tableRow*/)
{
    return 0;
}

template <class P>
void SuperFlatInstance::genSkyMask(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>& ref, GenericImage<P>& maskImage, int y, int channel)
{
    const typename P::sample* pRef = ref[0].ScanLine(y, channel);
    typename P::sample* pMask = maskImage.ScanLine(y, channel);
    for (int x = 0; x < maskImage.Width(); x++)
        if (pMask[x] > pRef[x] + superFlat->skyDetectionThreshold)
            pMask[x] = 0.0;
        else
            pMask[x] = 1.0;
}

template <class P>
void SuperFlatInstance::diffuse(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>& pyramid, GenericImage<P>& maskImage, int y, int channel)
{
    typename P::sample* pMask = maskImage.ScanLine(y, channel);
    for (int x = 0; x < maskImage.Width(); x++) {
        bool b = false;
        for (int i = 0; i < pyramid.Length(); i++) {
            if (pyramid[i](x, y, channel) < 0.99) {
                b = true;
                break;
            }
        }
        if (b)
            pMask[x] = 0.0;
    }
}

template <class P>
void SuperFlatInstance::gconv(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>& input, GenericImage<P>& conv, int y, int channel)
{
    typename P::sample* pOut = conv.ScanLine(y, channel);
    const GenericImage<P>& in = input[0];
    int w = in.Width(), h = in.Height();
    float scaleX = w / conv.Width(), scaleY = h / conv.Height();

    for (int x = 0; x < conv.Width(); x++) {
        int ix = (x + 0.5f) * scaleX - 0.5f;
        int iy = (y + 0.5f) * scaleY - 0.5f;
        const int radius = 9;
        const float r2 = radius * radius;
        typename P::sample p = 0.0;
        float w0 = 0.0f;
        for (int dy = iy - radius; dy <= iy + radius; dy++) {
            if ((dy < 0) || (dy >= h))
                continue;
            for (int dx = ix - radius; dx <= ix + radius; dx++) {
                if ((dx < 0) || (dx >= w))
                    continue;
                float d2 = (dx - ix) * (dx - ix) + (dy - iy) * (dy - iy);
                if (d2 > r2)
                    continue;
                float w = 1.0f - d2 / r2;
                typename P::sample p0 = in(dx, dy, channel);
                if ((p0 == 0.0) || pcl::IsNaN(p0))
                    continue;
                p += p0 * w;
                w0 += w;
            }
        }
        if (w0 > 0.0)
            pOut[x] = p / w0;
        else
            pOut[x] = NAN;
    }
}

template <class P>
void SuperFlatInstance::calcWeight(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>& input, GenericImage<P>& weight, int y, int channel)
{
    typename P::sample* pOut = weight.ScanLine(y, channel);
    const GenericImage<P>& in = input[0];
    int w = in.Width(), h = in.Height();
    float scaleX = w / weight.Width(), scaleY = h / weight.Height();

    for (int x = 0; x < weight.Width(); x++) {
        int ix = (x + 0.5f) * scaleX - 0.5f;
        int iy = (y + 0.5f) * scaleY - 0.5f;
        const int radius = 9;
        const float r2 = radius * radius;
        typename P::sample p = 0.0;
        float w0 = 0.0f;
        float wall = 0.0f;
        for (int dy = iy - radius; dy <= iy + radius; dy++) {
            if ((dy < 0) || (dy >= h))
                continue;
            for (int dx = ix - radius; dx <= ix + radius; dx++) {
                if ((dx < 0) || (dx >= w))
                    continue;
                float d2 = (dx - ix) * (dx - ix) + (dy - iy) * (dy - iy);
                if (d2 > r2)
                    continue;
                float w = 1.0f - d2 / r2;
                wall += w;
                typename P::sample p0 = in(dx, dy, channel);
                if ((p0 == 0.0) || pcl::IsNaN(p0))
                    continue;
                w0 += w;
            }
        }
        pOut[x] = w0 / wall;
    }
}

template <class P>
void SuperFlatInstance::inpaint(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>& inputs, GenericImage<P>& output, int y, int channel)
{
    typename P::sample* pOut = output.ScanLine(y, channel);

    for (int x = 0; x < output.Width(); x++) {
        int s = 1;
        float w0 = 0.0f;
        typename P::sample p0 = 0.0;
        float f = 1.0f;
        for (int i = 0; i < inputs.Length(); i+= 2) {
            typename P::sample p = inputs[i](x, y, channel);
            typename P::sample w = inputs[i + 1](x, y, channel);
            if (pcl::IsNaN(p)) {
                p = 0.0;
                w = 0.0;
            }
            p0 += p * w * f;
            w0 += w * f;
            f *= 0.25f;
            if (w > 0.5)
                break;
        }
        pOut[x] = p0 / w0;
    }
}

}	// namespace pcl