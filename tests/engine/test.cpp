#include <unity.h>
#include "assets/font/testcharacters.hpp"
#include "assets/image/testimage.hpp"
#include "assets/image/testimagesequence.hpp"
#include "assets/model/testindexgroup.hpp"
#include "assets/model/teststatictrianglegroup.hpp"
#include "assets/model/testtrianglegroup.hpp"
#include "core/color/testgradientcolor.hpp"
#include "core/color/testrgbcolor.hpp"
#include "core/control/testbouncephysics.hpp"
#include "core/control/testdampedspring.hpp"
#include "core/control/testpid.hpp"
#include "core/geometry/3d/testcube.hpp"
#include "core/geometry/3d/testplane.hpp"
#include "core/geometry/3d/testsphere.hpp"
#include "core/math/testaxisangle.hpp"
#include "core/math/testdirectionangle.hpp"
#include "core/math/testeulerangles.hpp"
#include "core/math/testeulerorder.hpp"
#include "core/math/testmathematics.hpp"
#include "core/math/testquaternion.hpp"
#include "core/math/testrotation.hpp"
#include "core/math/testrotationmatrix.hpp"
#include "core/math/testtransform.hpp"
#include "core/math/testvector2d.hpp"
#include "core/math/testvector3d.hpp"
#include "core/math/testyawpitchroll.hpp"
#include "core/platform/testustring.hpp"
#include "core/signal/filter/testderivativefilter.hpp"
#include "core/signal/filter/testfftfilter.hpp"
#include "core/signal/filter/testkalmanfilter.hpp"
#include "core/signal/filter/testmaxfilter.hpp"
#include "core/signal/filter/testminfilter.hpp"
#include "core/signal/filter/testpeakdetection.hpp"
#include "core/signal/filter/testquaternionkalmanfilter.hpp"
#include "core/signal/filter/testrampfilter.hpp"
#include "core/signal/filter/testrunningaveragefilter.hpp"
#include "core/signal/filter/testvectorkalmanfilter.hpp"
#include "core/signal/filter/testvectorrunningaveragefilter.hpp"
#include "core/signal/noise/testsimplexnoise.hpp"
#include "core/signal/testfft.hpp"
#include "core/signal/testfftvoicedetection.hpp"
#include "core/signal/testfunctiongenerator.hpp"
#include "core/time/testtimestep.hpp"
#include "core/time/testwait.hpp"
#include "systems/hardware/testvirtualcontroller.hpp"
#include "systems/physics/testboundarymotionsimulator.hpp"
#include "systems/physics/testphysicssimulator.hpp"
#include "systems/physics/testvectorfield2d.hpp"
#include "systems/render/core/testcamera.hpp"
#include "systems/render/core/testcameralayout.hpp"
#include "systems/render/core/testcameramanager.hpp"
#include "systems/render/core/testpixel.hpp"
#include "systems/render/core/testpixelgroup.hpp"
#include "systems/render/material/implementations/testcombinematerial.hpp"
#include "systems/render/material/implementations/testdepthmaterial.hpp"
#include "systems/render/material/implementations/testgradientmaterial.hpp"
#include "systems/render/material/implementations/testimagematerial.hpp"
#include "systems/render/material/implementations/testimagesequencematerial.hpp"
#include "systems/render/material/implementations/testmaterialmask.hpp"
#include "systems/render/material/implementations/testphonglightmaterial.hpp"
#include "systems/render/material/implementations/testtvstaticmaterial.hpp"
#include "systems/render/material/implementations/testuniformcolormaterial.hpp"
#include "systems/render/material/implementations/testuvmapmaterial.hpp"
#include "systems/render/material/implementations/testvectorfield2dmaterial.hpp"
#include "systems/render/material/testmaterialanimator.hpp"
#include "systems/render/post/effects/testfisheye.hpp"
#include "systems/render/post/effects/testglitchx.hpp"
#include "systems/render/post/effects/testhorizontalblur.hpp"
#include "systems/render/post/effects/testmagnet.hpp"
#include "systems/render/post/testcompositor.hpp"
#include "systems/render/raster/helpers/testrastertriangle2d.hpp"
#include "systems/render/raster/helpers/testrastertriangle3d.hpp"
#include "systems/render/raster/testrasterizer.hpp"
#include "systems/render/ray/testraytracer.hpp"
#include "systems/render/shader/implementations/testaudioreactiveparams.hpp"
#include "systems/render/shader/implementations/testaudioreactiveshader.hpp"
#include "systems/render/shader/implementations/testcombineparams.hpp"
#include "systems/render/shader/implementations/testcombineshader.hpp"
#include "systems/render/shader/implementations/testdepthparams.hpp"
#include "systems/render/shader/implementations/testdepthshader.hpp"
#include "systems/render/shader/implementations/testgradientparams.hpp"
#include "systems/render/shader/implementations/testgradientshader.hpp"
#include "systems/render/shader/implementations/testhorizontalrainbowparams.hpp"
#include "systems/render/shader/implementations/testhorizontalrainbowshader.hpp"
#include "systems/render/shader/implementations/testimageparams.hpp"
#include "systems/render/shader/implementations/testimagesequenceparams.hpp"
#include "systems/render/shader/implementations/testimagesequenceshader.hpp"
#include "systems/render/shader/implementations/testimageshader.hpp"
#include "systems/render/shader/implementations/testmaterialmaskparams.hpp"
#include "systems/render/shader/implementations/testmaterialmaskshader.hpp"
#include "systems/render/shader/implementations/testnormalparams.hpp"
#include "systems/render/shader/implementations/testnormalshader.hpp"
#include "systems/render/shader/implementations/testoscilloscopeparams.hpp"
#include "systems/render/shader/implementations/testoscilloscopeshader.hpp"
#include "systems/render/shader/implementations/testphonglightparams.hpp"
#include "systems/render/shader/implementations/testphonglightshader.hpp"
#include "systems/render/shader/implementations/testproceduralnoiseparams.hpp"
#include "systems/render/shader/implementations/testproceduralnoiseshader.hpp"
#include "systems/render/shader/implementations/testspectrumanalyzerparams.hpp"
#include "systems/render/shader/implementations/testspectrumanalyzershader.hpp"
#include "systems/render/shader/implementations/testspiralparams.hpp"
#include "systems/render/shader/implementations/testtvstaticparams.hpp"
#include "systems/render/shader/implementations/testtvstaticshader.hpp"
#include "systems/render/shader/implementations/testuniformcolorparams.hpp"
#include "systems/render/shader/implementations/testuniformcolorshader.hpp"
#include "systems/render/shader/implementations/testuvmapparams.hpp"
#include "systems/render/shader/implementations/testuvmapshader.hpp"
#include "systems/render/shader/implementations/testvectorfield2dparams.hpp"
#include "systems/render/shader/implementations/testvectorfield2dshader.hpp"
#include "systems/scene/animation/testeasyeaseanimator.hpp"
#include "systems/scene/animation/testkeyframe.hpp"
#include "systems/scene/animation/testkeyframetrack.hpp"
#include "systems/scene/deform/testblendshape.hpp"
#include "systems/scene/deform/testblendshapecontroller.hpp"
#include "systems/scene/deform/testmeshalign.hpp"
#include "systems/scene/deform/testmeshdeformer.hpp"
#include "systems/scene/deform/testtrianglegroupdeformer.hpp"
#include "systems/scene/lighting/testlight.hpp"
#include "systems/scene/testmesh.hpp"
#include "systems/scene/testscene.hpp"

void setUp() {}
void tearDown() {}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    TestCharacters::RunAllTests();
    TestImage::RunAllTests();
    TestImageSequence::RunAllTests();
    TestIndexGroup::RunAllTests();
    TestStaticTriangleGroup::RunAllTests();
    TestTriangleGroup::RunAllTests();
    TestGradientColor::RunAllTests();
    TestRGBColor::RunAllTests();
    TestBouncePhysics::RunAllTests();
    TestDampedSpring::RunAllTests();
    TestPID::RunAllTests();
    TestCube::RunAllTests();
    TestPlane::RunAllTests();
    TestSphere::RunAllTests();
    TestAxisAngle::RunAllTests();
    TestDirectionAngle::RunAllTests();
    TestEulerAngles::RunAllTests();
    TestEulerOrder::RunAllTests();
    TestMathematics::RunAllTests();
    TestQuaternion::RunAllTests();
    TestRotation::RunAllTests();
    TestRotationMatrix::RunAllTests();
    TestTransform::RunAllTests();
    TestVector2D::RunAllTests();
    TestVector3D::RunAllTests();
    TestYawPitchRoll::RunAllTests();
    TestUString::RunAllTests();
    TestDerivativeFilter::RunAllTests();
    TestFFTFilter::RunAllTests();
    TestKalmanFilter::RunAllTests();
    TestMaxFilter::RunAllTests();
    TestMinFilter::RunAllTests();
    TestPeakDetection::RunAllTests();
    TestQuaternionKalmanFilter::RunAllTests();
    TestRampFilter::RunAllTests();
    TestRunningAverageFilter::RunAllTests();
    TestVectorKalmanFilter::RunAllTests();
    TestVectorRunningAverageFilter::RunAllTests();
    TestSimplexNoise::RunAllTests();
    TestFFT::RunAllTests();
    TestFFTVoiceDetection::RunAllTests();
    TestFunctionGenerator::RunAllTests();
    TestTimeStep::RunAllTests();
    TestWait::RunAllTests();
    TestVirtualController::RunAllTests();
    TestBoundaryMotionSimulator::RunAllTests();
    TestPhysicsSimulator::RunAllTests();
    TestVectorField2D::RunAllTests();
    TestCamera::RunAllTests();
    TestCameraLayout::RunAllTests();
    TestCameraManager::RunAllTests();
    TestPixel::RunAllTests();
    TestPixelGroup::RunAllTests();
    TestCombineMaterial::RunAllTests();
    TestDepthMaterial::RunAllTests();
    TestGradientMaterial::RunAllTests();
    TestImageMaterial::RunAllTests();
    TestImageSequenceMaterial::RunAllTests();
    TestMaterialMask::RunAllTests();
    TestPhongLightMaterial::RunAllTests();
    TestTVStaticMaterial::RunAllTests();
    TestUniformColorMaterial::RunAllTests();
    TestUVMapMaterial::RunAllTests();
    TestVectorField2DMaterial::RunAllTests();
    TestMaterialAnimator::RunAllTests();
    TestFisheye::RunAllTests();
    TestGlitchX::RunAllTests();
    TestHorizontalBlur::RunAllTests();
    TestMagnet::RunAllTests();
    TestCompositor::RunAllTests();
    TestRasterTriangle2D::RunAllTests();
    TestRasterTriangle3D::RunAllTests();
    TestRasterizer::RunAllTests();
    TestRayTracer::RunAllTests();
    TestAudioReactiveParams::RunAllTests();
    TestAudioReactiveShader::RunAllTests();
    TestCombineParams::RunAllTests();
    TestCombineShader::RunAllTests();
    TestDepthParams::RunAllTests();
    TestDepthShader::RunAllTests();
    TestGradientParams::RunAllTests();
    TestGradientShader::RunAllTests();
    TestHorizontalRainbowParams::RunAllTests();
    TestHorizontalRainbowShader::RunAllTests();
    TestImageParams::RunAllTests();
    TestImageSequenceParams::RunAllTests();
    TestImageSequenceShader::RunAllTests();
    TestImageShader::RunAllTests();
    TestMaterialMaskParams::RunAllTests();
    TestMaterialMaskShader::RunAllTests();
    TestNormalParams::RunAllTests();
    TestNormalShader::RunAllTests();
    TestOscilloscopeParams::RunAllTests();
    TestOscilloscopeShader::RunAllTests();
    TestPhongLightParams::RunAllTests();
    TestPhongLightShader::RunAllTests();
    TestProceduralNoiseParams::RunAllTests();
    TestProceduralNoiseShader::RunAllTests();
    TestSpectrumAnalyzerParams::RunAllTests();
    TestSpectrumAnalyzerShader::RunAllTests();
    TestSpiralParams::RunAllTests();
    TestTVStaticParams::RunAllTests();
    TestTVStaticShader::RunAllTests();
    TestUniformColorParams::RunAllTests();
    TestUniformColorShader::RunAllTests();
    TestUVMapParams::RunAllTests();
    TestUVMapShader::RunAllTests();
    TestVectorField2DParams::RunAllTests();
    TestVectorField2DShader::RunAllTests();
    TestEasyEaseAnimator::RunAllTests();
    TestKeyFrame::RunAllTests();
    TestKeyFrameTrack::RunAllTests();
    TestBlendshape::RunAllTests();
    TestBlendshapeController::RunAllTests();
    TestMeshAlign::RunAllTests();
    TestMeshDeformer::RunAllTests();
    TestTriangleGroupDeformer::RunAllTests();
    TestLight::RunAllTests();
    TestMesh::RunAllTests();
    TestScene::RunAllTests();

    UNITY_END();
}
