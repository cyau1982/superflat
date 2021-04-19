#include <random>
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
    mask.CopyImage(image);
    mask.EnsureUniqueImage();
    mask.SetStatusCallback(nullptr);
    MorphologicalTransformation sf;
    sf.SetStructure(CircularStructure(25));
    sf.SetOperator(SelectionFilter(0.9f));
    for (int i = 0; i < objectDiffusionDistance; i++) {
        sf >> mask;
        image.Status() += 1;
    }
    if (image.BitsPerSample() == 32) {
        ReferenceArray<GenericImage<FloatPixelTraits>> input;
        input << &static_cast<Image&>(*ref);
        for (int c = 0; c < image.NumberOfChannels(); c++)
            SuperFlatThread<FloatPixelTraits>::dispatch(genSkyMask<FloatPixelTraits>, this, input, static_cast<Image&>(*mask), c);
    } else if (image.BitsPerSample() == 64) {
        ReferenceArray<GenericImage<DoublePixelTraits>> input;
        input << &static_cast<DImage&>(*ref);
        for (int c = 0; c < image.NumberOfChannels(); c++)
            SuperFlatThread<DoublePixelTraits>::dispatch(genSkyMask<DoublePixelTraits>, this, input, static_cast<DImage&>(*mask), c);
    }
    image.Status() += 1;

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
    image.Status().Initialize("Inpainting", image.NumberOfChannels() + 1);
    ImageVariant flat0;
    flat0.CopyImage(flat);
    flat0.EnsureUniqueImage();
    flat0.SetStatusCallback(nullptr);
    image.Status() += 1;
    if (image.BitsPerSample() == 32) {
        ReferenceArray<GenericImage<FloatPixelTraits>> input;
        input << &static_cast<Image&>(*flat0);
        for (int c = 0; c < image.NumberOfChannels(); c++) {
            SuperFlatThread<FloatPixelTraits>::dispatch(inpaint<FloatPixelTraits>, this, input, static_cast<Image&>(*flat), c);
            image.Status() += 1;
        }
    } else if (image.BitsPerSample() == 64) {
        ReferenceArray<GenericImage<DoublePixelTraits>> input;
        input << &static_cast<DImage&>(*flat0);
        for (int c = 0; c < image.NumberOfChannels(); c++) {
            SuperFlatThread<DoublePixelTraits>::dispatch(inpaint<DoublePixelTraits>, this, input, static_cast<DImage&>(*flat), c);
            image.Status() += 1;
        }
    }
    image.Status().Complete();

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
void SuperFlatInstance::inpaint(SuperFlatInstance* superFlat, ReferenceArray<GenericImage<P>>& inputs, GenericImage<P>& output, int y, int channel)
{
    typename P::sample* pOut = output.ScanLine(y, channel);
    GenericImage<P>& input = inputs[0];
    const int n = 32;
    const int distance = pcl::Max(output.Width(), output.Height());
    std::minstd_rand rg(output.Height() * channel + y);
    std::uniform_real_distribution<float> ud(-0.5f, 0.5f);

    for (int x = 0; x < output.Width(); x++) {
        typename P::sample in = input(x, y, channel);
        if (in > 0.0) {
            pOut[x] = in;
            continue;
        }
        typename P::sample p = 0.0;
        float w0 = 0.0f;
        for (int i = 0; i < n; i++) {
            float rad = pcl::Pi() * 2.0f * i / n;
            float step_x = pcl::Cos(rad);
            float step_y = pcl::Sin(rad);
            for (int j = 1; j < distance; j = (j < 16) ? j + 1 : j * 1.1f) {
                if ((j < 64) && (i % 2 != 0))
                    continue;
                float w = 1.0f / float(j);
                if (w < w0 * 0.01f)
                    continue;
                float rx = ud(rg) * j * 6.0f / n;
                float ry = ud(rg) * j * 6.0f / n;
                rx = 0.0;
                ry = 0.0;
                int ix = int(x + step_x * j + rx + 0.5f);
                int iy = int(y + step_y * j + ry + 0.5f);
                if (ix < 0)
                    ix = 0;
                else if (ix >= input.Width())
                    ix = input.Width() - 1;
                if (iy < 0)
                    iy = 0;
                else if (iy >= input.Height())
                    iy = input.Height() - 1;
                typename P::sample in = input(ix, iy, channel);
                if (in == 0.0)
                    continue;
                p += in * w;
                w0 += w;
                break;
            }
        }
        if (w0 > 0.0f)
            pOut[x] = p / w0;
        else
            pOut[x] = 0.0;
    }
}

}	// namespace pcl