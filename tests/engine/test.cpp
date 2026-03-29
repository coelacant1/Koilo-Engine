// SPDX-License-Identifier: GPL-3.0-or-later
#include <unity.h>
#include "assets/font/testcharacters.hpp"
#include "assets/image/testimage.hpp"
#include "assets/image/testimagesequence.hpp"
#include "assets/image/testtexture.hpp"
#include "assets/model/testindexgroup.hpp"
#include "assets/model/teststatictrianglegroup.hpp"
#include "assets/model/testtrianglegroup.hpp"
#include "assets/testmorphtarget.hpp"
#include "assets/testkoilomeshloader.hpp"
#include "core/color/testcolor565.hpp"
#include "core/color/testcolor888.hpp"
#include "core/color/testcolorconverter.hpp"
#include "core/color/testcolorgray.hpp"
#include "core/color/testcolormono.hpp"
#include "core/color/testcolorpalette.hpp"
#include "core/color/testgradientcolor.hpp"
#include "core/control/testbouncephysics.hpp"
#include "core/control/testdampedspring.hpp"
#include "core/control/testpid.hpp"
#include "core/geometry/2d/testaabb2d.hpp"
#include "core/geometry/2d/testbounds.hpp"
#include "core/geometry/2d/testcircle.hpp"
#include "core/geometry/2d/testcircle2d.hpp"
#include "core/geometry/2d/testcorners.hpp"
#include "core/geometry/2d/testellipse.hpp"
#include "core/geometry/2d/testellipse2d.hpp"
#include "core/geometry/2d/testrectangle.hpp"
#include "core/geometry/2d/testrectangle2d.hpp"
#include "core/geometry/2d/testshape.hpp"
#include "core/geometry/2d/testtriangle2d.hpp"
#include "core/geometry/3d/testaabb.hpp"
#include "core/geometry/3d/testcube.hpp"
#include "core/geometry/3d/testplane.hpp"
#include "core/geometry/3d/testsphere.hpp"
#include "core/geometry/3d/testtriangle3d.hpp"
#include "core/geometry/testray.hpp"
#include "core/math/testaxisangle.hpp"
#include "core/math/testdirectionangle.hpp"
#include "core/math/testeulerangles.hpp"
#include "core/math/testeulerconstants.hpp"
#include "core/math/testeulerconstantswrapper.hpp"
#include "core/math/testeulerorder.hpp"
#include "core/math/testmathematics.hpp"
#include "core/math/testmatrix4x4.hpp"
#include "core/math/testquaternion.hpp"
#include "core/math/testrotation.hpp"
#include "core/math/testrotationmatrix.hpp"
#include "core/math/testtransform.hpp"
#include "core/math/testvector2d.hpp"
#include "core/math/testvector3d.hpp"
#include "core/math/testyawpitchroll.hpp"
#include "core/platform/testconsole.hpp"
#include "core/platform/testrandom.hpp"
#include "core/platform/testreflection.hpp"
#include "core/platform/testustring.hpp"
#include "core/platform/testfilewatcher.hpp"
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
#include "core/signal/testviseme.hpp"
#include "core/time/testtimemanager.hpp"
#include "core/time/testtimestep.hpp"
#include "core/time/testwait.hpp"
#include "debug/testcolor.hpp"
#include "debug/testdebugbox.hpp"
#include "debug/testdebugdraw.hpp"
#include "debug/testdebugline.hpp"
#include "debug/testdebugoverlaystats.hpp"
#include "debug/testdebugrenderer.hpp"
#include "debug/testdebugsphere.hpp"
#include "debug/testdebugtext.hpp"
#include "debug/testprofiler.hpp"
#include "debug/testprofileresult.hpp"
#include "debug/testprofilescope.hpp"
#include "debug/testprofilestats.hpp"
#include "ecs/components/testtagcomponent.hpp"
#include "ecs/components/testtransformcomponent.hpp"
#include "ecs/components/testvelocitycomponent.hpp"
#include "ecs/testcomponent.hpp"
#include "ecs/testentity.hpp"
#include "ecs/testentitymanager.hpp"
#include "ecs/testscriptentitymanager.hpp"

#include "modules/testmoduleloader.hpp"
#include "ksl/testelfloader.hpp"
#include "ksl/testkslmaterial.hpp"
#include "scripting/testargmarshaller.hpp"
#include "scripting/testatomtable.hpp"
#include "scripting/testbytecodecompiler.hpp"
#include "scripting/testcompiledscript.hpp"
#include "scripting/testconstant.hpp"
#include "scripting/testcoroutinemanager.hpp"
#include "scripting/testcoroutinestate.hpp"
#include "scripting/testdisplayconfig.hpp"
#include "scripting/testfielddef.hpp"
#include "scripting/testheaparray.hpp"
#include "scripting/testkoiloscriptlexer.hpp"
#include "scripting/testkoiloscriptparser.hpp"
#include "scripting/testreflectedobject.hpp"
#include "scripting/testreflectionbridge.hpp"
#include "scripting/testsavedframe.hpp"
#include "scripting/testsavediterator.hpp"
#include "scripting/testscriptclass.hpp"
#include "scripting/testscriptcontext.hpp"
#include "scripting/testscriptinstance.hpp"
#include "scripting/testsignalregistry.hpp"
#include "scripting/testtoken.hpp"
#include "scripting/testvalue.hpp"
#ifdef KOILO_ENABLE_AI
#include "systems/ai/behaviortree/testactionnode.hpp"
#include "systems/ai/behaviortree/testbehaviortreeaction.hpp"
#include "systems/ai/behaviortree/testbehaviortreenode.hpp"
#include "systems/ai/behaviortree/testconditionnode.hpp"
#include "systems/ai/behaviortree/testinverternode.hpp"
#include "systems/ai/behaviortree/testparallelnode.hpp"
#include "systems/ai/behaviortree/testrepeaternode.hpp"
#include "systems/ai/behaviortree/testselectornode.hpp"
#include "systems/ai/behaviortree/testsequencenode.hpp"
#include "systems/ai/behaviortree/testsucceedernode.hpp"
#include "systems/ai/behaviortree/testwaitnode.hpp"
#include "systems/ai/pathfinding/testgridnode.hpp"
#include "systems/ai/pathfinding/testpathfinder.hpp"
#include "systems/ai/pathfinding/testpathfindergrid.hpp"
#include "systems/ai/statemachine/teststate.hpp"
#include "systems/ai/statemachine/teststatemachine.hpp"
#include "systems/ai/statemachine/teststatetransition.hpp"
#include "systems/ai/testbehaviortree.hpp"
#include "systems/ai/testpathfinding.hpp"
#include "systems/ai/testscriptaimanager.hpp"
#endif
#ifdef KOILO_ENABLE_AUDIO
#include "systems/audio/testaudiobackend.hpp"
#include "systems/audio/testaudioclip.hpp"
#include "systems/audio/testaudiolistener.hpp"
#include "systems/audio/testaudiomanager.hpp"
#include "systems/audio/testaudiosource.hpp"
#include "systems/audio/testscriptaudiomanager.hpp"
#endif
#include "systems/display/backends/embedded/testhub75backend.hpp"
#ifdef KL_HAVE_OPENGL_BACKEND
#include "systems/display/backends/gpu/testopenglbackend.hpp"
#endif
#include "systems/display/backends/gpu/testsdl3backend.hpp"
#include "systems/display/backends/testnullbackend.hpp"
#include "systems/display/testdisplayinfo.hpp"
#include "systems/display/testdisplaymanager.hpp"
#include "systems/display/testframebuffer.hpp"
#include "systems/display/testidisplaybackend.hpp"
#include "systems/input/testgamepad.hpp"
#include "systems/input/testinputcodes.hpp"
#include "systems/input/testinputmanager.hpp"
#include "systems/input/testkeyboard.hpp"
#include "systems/input/testmouse.hpp"
#ifdef KOILO_ENABLE_PARTICLES
#include "systems/particles/testparticle.hpp"
#include "systems/particles/testparticleemitter.hpp"
#include "systems/particles/testparticleemitterconfig.hpp"
#include "systems/particles/testparticlesystem.hpp"
#endif
#include "systems/physics/testboxcollider.hpp"
#include "systems/physics/testcapsulecollider.hpp"
#include "systems/physics/testcollider.hpp"
#include "systems/physics/testcollisionevent.hpp"
#include "systems/physics/testcollisioninfo.hpp"
#include "systems/physics/testcollisionmanager.hpp"
#include "systems/physics/testphysicsmaterial.hpp"
#include "systems/physics/testphysicsraycast.hpp"
#include "systems/physics/testphysicsworld.hpp"
#include "systems/physics/testraycasthit.hpp"
#include "systems/physics/testrigidbody.hpp"
#include "systems/physics/testspherecollider.hpp"
#include "systems/physics/testvectorfield2d.hpp"
#include "systems/profiling/testmemoryallocation.hpp"
#include "systems/profiling/testmemoryprofiler.hpp"
#include "systems/profiling/testmemoryscope.hpp"
#include "systems/profiling/testmemorystats.hpp"
#include "systems/profiling/testperformanceprofiler.hpp"
#include "systems/profiling/testperfprofilescope.hpp"
#include "systems/profiling/testprofileframe.hpp"
#include "systems/profiling/testprofilesample.hpp"
#include "systems/render/core/testcamera.hpp"
#include "systems/render/core/testcamerabase.hpp"
#include "systems/render/core/testcameralayout.hpp"
#include "systems/render/core/testcameramanager.hpp"
#include "systems/render/core/testpixel.hpp"
#include "systems/render/core/testpixelgroup.hpp"
#include "systems/render/material/testimaterial.hpp"
#include "systems/render/raster/helpers/testrastertriangle2d.hpp"
#include "systems/render/raster/helpers/testrastertriangle3d.hpp"
#include "systems/render/raster/testrasterizer.hpp"
#include "systems/render/ray/testrayhitinfo.hpp"
#include "systems/render/ray/testrayintersection.hpp"
#include "systems/render/ray/testraytracer.hpp"
#include "systems/render/ray/testraytracesettings.hpp"

#include "systems/render/shader/testsurfaceproperties.hpp"
#include "systems/render/sky/testsky.hpp"
#include "systems/render/testcanvas2d.hpp"
#include "systems/render/testsoftwarerenderbackend.hpp"
#include "systems/render/rhi/testrhitypes.hpp"
#include "systems/render/rhi/testrhipipeline.hpp"
#include "systems/scene/animation/testanimationchannel.hpp"
#include "systems/scene/animation/testanimationclip.hpp"
#include "systems/scene/animation/testanimationlayer.hpp"
#include "systems/scene/animation/testanimationmixer.hpp"
#include "systems/scene/animation/testbone.hpp"
#include "systems/scene/animation/testeasyeaseanimator.hpp"
#include "systems/scene/animation/testkeyframe.hpp"
#include "systems/scene/animation/testskeleton.hpp"
#include "systems/scene/animation/testskeletonanimator.hpp"
#include "systems/scene/animation/testskinvertex.hpp"
#include "systems/scene/animation/testtimeline.hpp"
#include "systems/scene/deform/testblendshape.hpp"
#include "systems/scene/deform/testblendshapecontroller.hpp"
#include "systems/scene/deform/testblendshapetransformcontroller.hpp"
#include "systems/scene/deform/testmeshalign.hpp"
#include "systems/scene/deform/testmeshdeformer.hpp"
#include "systems/scene/deform/testtrianglegroupdeformer.hpp"
#include "systems/scene/lighting/testlight.hpp"
#include "systems/scene/testmesh.hpp"
#include "systems/scene/testmeshraycast.hpp"
#include "systems/scene/testmorphablemesh.hpp"
#include "systems/scene/testprimitivemesh.hpp"
#include "systems/scene/testscene.hpp"

#include "systems/scene/testscenenode.hpp"
#include "systems/scene/testskindata.hpp"
#include "systems/scene/testsprite.hpp"
#include "systems/font/testfont.hpp"
#include "systems/ui/testui.hpp"
#include "systems/ui/testwidget.hpp"
#include "systems/ui/testautoinspector.hpp"
#include "systems/ui/testruntimecontext.hpp"
#include "systems/ui/testundostack.hpp"
#include "systems/ui/testselection.hpp"
#include "systems/ui/testlogbuffer.hpp"
#include "systems/ui/testeditorapp.hpp"
#include "systems/ui/testanimation.hpp"
#include "systems/ui/testlocalization.hpp"
#include "systems/world/testcomponentserializerentry.hpp"
#include "systems/world/testlevel.hpp"
#include "systems/world/testlevelserializer.hpp"
#include "systems/world/testreflectionserializer.hpp"
#include "systems/world/testscriptworldmanager.hpp"
#include "systems/world/testserializedentity.hpp"
#include "systems/world/testserializedlevel.hpp"
#include "systems/world/testworldmanager.hpp"
#include "kernel/testkernel.hpp"
#include "kernel/testconsole.hpp"

void setUp() {}
void tearDown() {}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();

    // Run UI/font tests first (before display tests that segfault)
    RunFontTests();
    TestUI::RunAllTests();
    TestWidget::RunAllTests();
    TestAutoInspector::RunAllTests();
    TestRuntimeContext::RunAllTests();
    TestUndoStack::RunAllTests();
    TestSelection::RunAllTests();
    TestLogBuffer::RunAllTests();
    TestEditorApp::RunAllTests();
    TestAnimation::RunAllTests();
    TestLocalization::RunAllTests();

    TestCharacters::RunAllTests();
    TestImage::RunAllTests();
    TestImageSequence::RunAllTests();
    TestTexture::RunAllTests();
    TestIndexGroup::RunAllTests();
    TestStaticTriangleGroup::RunAllTests();
    TestTriangleGroup::RunAllTests();
    TestMorphTarget::RunAllTests();
    TestKoiloMeshLoader::RunAllTests();
    TestColor565::RunAllTests();
    TestColor888::RunAllTests();
    TestColorConverter::RunAllTests();
    TestColorGray::RunAllTests();
    TestColorMono::RunAllTests();
    TestColorPalette::RunAllTests();
    TestGradientColor::RunAllTests();
    TestBouncePhysics::RunAllTests();
    TestDampedSpring::RunAllTests();
    TestPID::RunAllTests();
    TestAABB2D::RunAllTests();
    TestBounds::RunAllTests();
    TestCircle::RunAllTests();
    TestCircle2D::RunAllTests();
    TestCorners::RunAllTests();
    TestEllipse::RunAllTests();
    TestEllipse2D::RunAllTests();
    TestRectangle::RunAllTests();
    TestRectangle2D::RunAllTests();
    TestShape::RunAllTests();
    TestTriangle2D::RunAllTests();
    TestAABB::RunAllTests();
    TestCube::RunAllTests();
    TestPlane::RunAllTests();
    TestSphere::RunAllTests();
    TestTriangle3D::RunAllTests();
    TestRay::RunAllTests();
    TestAxisAngle::RunAllTests();
    TestDirectionAngle::RunAllTests();
    TestEulerAngles::RunAllTests();
    TestEulerConstants::RunAllTests();
    TestEulerConstantsWrapper::RunAllTests();
    TestEulerOrder::RunAllTests();
    TestMathematics::RunAllTests();
    TestMatrix4x4::RunAllTests();
    TestQuaternion::RunAllTests();
    TestRotation::RunAllTests();
    TestRotationMatrix::RunAllTests();
    TestTransform::RunAllTests();
    TestVector2D::RunAllTests();
    TestVector3D::RunAllTests();
    TestYawPitchRoll::RunAllTests();
    TestConsole::RunAllTests();
    TestRandom::RunAllTests();
    TestReflection::RunAllTests();
    TestUString::RunAllTests();
    TestFileWatcher::RunAllTests();
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
    TestViseme::RunAllTests();
    TestTimeManager::RunAllTests();
    TestTimeStep::RunAllTests();
    TestWait::RunAllTests();
    TestColor::RunAllTests();
    TestDebugBox::RunAllTests();
    TestDebugDraw::RunAllTests();
    TestDebugLine::RunAllTests();
    TestDebugOverlayStats::RunAllTests();
    TestDebugRenderer::RunAllTests();
    TestDebugSphere::RunAllTests();
    TestDebugText::RunAllTests();
    TestProfiler::RunAllTests();
    TestProfileResult::RunAllTests();
    TestProfileScope::RunAllTests();
    TestProfileStats::RunAllTests();
    TestTagComponent::RunAllTests();
    TestTransformComponent::RunAllTests();
    TestVelocityComponent::RunAllTests();
    TestComponent::RunAllTests();
    TestEntity::RunAllTests();
    TestEntityManager::RunAllTests();
    TestScriptEntityManager::RunAllTests();

    TestModuleLoader::RunAllTests();
    TestElfLoader::RunAllTests();
    TestKSLMaterial::RunAllTests();
    TestArgMarshaller::RunAllTests();
    TestAtomTable::RunAllTests();
    TestBytecodeCompiler::RunAllTests();
    TestCompiledScript::RunAllTests();
    TestConstant::RunAllTests();
    TestCoroutineManager::RunAllTests();
    TestCoroutineState::RunAllTests();
    TestDisplayConfig::RunAllTests();
    TestFieldDef::RunAllTests();
    TestHeapArray::RunAllTests();
    TestKoiloScriptLexer::RunAllTests();
    TestKoiloScriptParser::RunAllTests();
    TestReflectedObject::RunAllTests();
    TestReflectionBridge::RunAllTests();
    TestSavedFrame::RunAllTests();
    TestSavedIterator::RunAllTests();
    TestScriptClass::RunAllTests();
    TestScriptContext::RunAllTests();
    TestScriptInstance::RunAllTests();
    TestSignalRegistry::RunAllTests();
    TestToken::RunAllTests();
    TestValue::RunAllTests();
#ifdef KOILO_ENABLE_AI
    TestActionNode::RunAllTests();
    TestBehaviorTreeAction::RunAllTests();
    TestBehaviorTreeNode::RunAllTests();
    TestConditionNode::RunAllTests();
    TestInverterNode::RunAllTests();
    TestParallelNode::RunAllTests();
    TestRepeaterNode::RunAllTests();
    TestSelectorNode::RunAllTests();
    TestSequenceNode::RunAllTests();
    TestSucceederNode::RunAllTests();
    TestWaitNode::RunAllTests();
    TestGridNode::RunAllTests();
    TestPathfinder::RunAllTests();
    TestPathfinderGrid::RunAllTests();
    TestState::RunAllTests();
    TestStateMachine::RunAllTests();
    TestStateTransition::RunAllTests();
    TestBehaviorTree::RunAllTests();
    TestPathfinding::RunAllTests();
    TestScriptAIManager::RunAllTests();
#endif
#ifdef KOILO_ENABLE_AUDIO
    TestAudioBackend::RunAllTests();
    TestAudioClip::RunAllTests();
    TestAudioListener::RunAllTests();
    TestAudioManager::RunAllTests();
    TestAudioSource::RunAllTests();
    TestScriptAudioManager::RunAllTests();
#endif
    TestHUB75Backend::RunAllTests();
#ifdef KL_HAVE_OPENGL_BACKEND
    TestOpenGLBackend::RunAllTests();
#endif
    TestSDL3Backend::RunAllTests();
    TestNullBackend::RunAllTests();
    TestDisplayInfo::RunAllTests();
    TestDisplayManager::RunAllTests();
    TestFramebuffer::RunAllTests();
    TestIDisplayBackend::RunAllTests();
    TestGamepad::RunAllTests();
    TestInputCodes::RunAllTests();
    TestInputManager::RunAllTests();
    TestKeyboard::RunAllTests();
    TestMouse::RunAllTests();
#ifdef KOILO_ENABLE_PARTICLES
    TestParticle::RunAllTests();
    TestParticleEmitter::RunAllTests();
    TestParticleEmitterConfig::RunAllTests();
    TestParticleSystem::RunAllTests();
#endif
    TestBoxCollider::RunAllTests();
    TestCapsuleCollider::RunAllTests();
    TestCollider::RunAllTests();
    TestCollisionEvent::RunAllTests();
    TestCollisionInfo::RunAllTests();
    TestCollisionManager::RunAllTests();
    TestPhysicsMaterial::RunAllTests();
    TestPhysicsRaycast::RunAllTests();
    TestPhysicsWorld::RunAllTests();
    TestRaycastHit::RunAllTests();
    TestRigidBody::RunAllTests();
    TestSphereCollider::RunAllTests();
    TestVectorField2D::RunAllTests();
    TestMemoryAllocation::RunAllTests();
    TestMemoryProfiler::RunAllTests();
    TestMemoryScope::RunAllTests();
    TestMemoryStats::RunAllTests();
    TestPerformanceProfiler::RunAllTests();
    TestPerfProfileScope::RunAllTests();
    TestProfileFrame::RunAllTests();
    TestProfileSample::RunAllTests();
    TestCamera::RunAllTests();
    TestCameraBase::RunAllTests();
    TestCameraLayout::RunAllTests();
    TestCameraManager::RunAllTests();
    TestPixel::RunAllTests();
    TestPixelGroup::RunAllTests();
    TestIMaterial::RunAllTests();
    TestRasterTriangle2D::RunAllTests();
    TestRasterTriangle3D::RunAllTests();
    TestRasterizer::RunAllTests();
    TestRayHitInfo::RunAllTests();
    TestRayIntersection::RunAllTests();
    TestRayTracer::RunAllTests();
    TestRayTraceSettings::RunAllTests();

    TestSurfaceProperties::RunAllTests();
    TestSky::RunAllTests();
    TestCanvas2D::RunAllTests();
    TestSoftwareRenderBackend::RunAllTests();
    TestRHITypes::RunAllTests();
    TestRHIPipeline::RunAllTests();
    TestAnimationChannel::RunAllTests();
    TestAnimationClip::RunAllTests();
    TestAnimationLayer::RunAllTests();
    TestAnimationMixer::RunAllTests();
    TestBone::RunAllTests();
    TestEasyEaseAnimator::RunAllTests();
    TestKeyFrame::RunAllTests();

    TestSkeleton::RunAllTests();
    TestSkeletonAnimator::RunAllTests();
    TestSkinVertex::RunAllTests();
    TestTimeline::RunAllTests();
    TestBlendshape::RunAllTests();
    TestBlendshapeController::RunAllTests();
    TestBlendshapeTransformController::RunAllTests();
    TestMeshAlign::RunAllTests();
    TestMeshDeformer::RunAllTests();
    TestTriangleGroupDeformer::RunAllTests();
    TestLight::RunAllTests();
    TestMesh::RunAllTests();
    TestMeshRaycast::RunAllTests();
    TestMorphableMesh::RunAllTests();
    TestPrimitiveMesh::RunAllTests();
    TestScene::RunAllTests();

    TestSceneNode::RunAllTests();
    TestSkinData::RunAllTests();
    TestSprite::RunAllTests();
    TestComponentSerializerEntry::RunAllTests();
    TestLevel::RunAllTests();
    TestLevelSerializer::RunAllTests();
    TestReflectionSerializer::RunAllTests();
    TestScriptWorldManager::RunAllTests();
    TestSerializedEntity::RunAllTests();
    TestSerializedLevel::RunAllTests();
    TestWorldManager::RunAllTests();
    TestKernel::RunAllTests();
    TestKernelConsole::RunAllTests();

    UNITY_END();
}
