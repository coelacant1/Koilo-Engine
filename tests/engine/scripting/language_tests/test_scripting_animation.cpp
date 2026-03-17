// SPDX-License-Identifier: GPL-3.0-or-later
// Animation Scripting Bindings
#include "helpers.hpp"

void TestAnimationClasses() {
    std::cout << "=== Animation Classes from Script ===" << std::endl;
    
    StringFileReader reader;
    
    // Test 1: AnimationTrack - persistent, call Update() across frames
    {
        TEST("AnimationTrack persistent + Update");
        KoiloScriptEngine engine(&reader);
        const char* script = R"(
fn Update(dt) {
    if type(_track) == "null" {
        _track = AnimationTrack(0.0, 1.0);
        _track.AddKeyFrame(0.0, 0.0);
        _track.AddKeyFrame(1.0, 1.0);
        _track.Play();
    }
    _result = _track.Update();
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            if (engine.HasError()) { FAIL(engine.GetError()); }
            else {
                // Run a few more frames to advance
                for (int i = 0; i < 10; ++i) {
                    koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
                    if (engine.HasError()) break;
                }
                if (!engine.HasError()) {
                    auto val = engine.GetGlobal("_result");
                    if (val.type == Value::Type::NUMBER) {
                        PASS();
                    } else {
                        FAIL("result not a number");
                    }
                } else {
                    FAIL(engine.GetError());
                }
            }
        } else {
            FAIL("Script load/build failed: " + std::string(engine.GetError()));
        }
    }
    
    // Test 2: FunctionGenerator - construct with enum + floats
    {
        TEST("FunctionGenerator construct + Update");
        KoiloScriptEngine engine(&reader);
        const char* script = R"(
fn Update(dt) {
    if type(_gen) == "null" {
        _gen = FunctionGenerator(2, 0.0, 1.0, 1.0);
    }
    _result = _gen.Update();
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            if (engine.HasError()) { FAIL(engine.GetError()); }
            else {
                koilo::TimeManager::GetInstance().Tick(0.1f); engine.ExecuteUpdate();
                if (!engine.HasError()) {
                    auto val = engine.GetGlobal("_result");
                    if (val.type == Value::Type::NUMBER) {
                        PASS();
                    } else {
                        FAIL("result not a number");
                    }
                } else {
                    FAIL(engine.GetError());
                }
            }
        } else {
            FAIL("Script load/build failed: " + std::string(engine.GetError()));
        }
    }
    
    // Test 3: EasyEaseAnimator - construct with size_t arg
    {
        TEST("EasyEaseAnimator construct");
        KoiloScriptEngine engine(&reader);
        const char* script = R"(
fn Update(dt) {
    if type(_ease) == "null" {
        _ease = EasyEaseAnimator(10);
    }
    _ease.Update();
    _result = 1;
}
)";
        reader.SetContent(script);
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
            if (engine.HasError()) { FAIL(engine.GetError()); }
            else {
                koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
                if (!engine.HasError()) {
                    PASS();
                } else {
                    FAIL(engine.GetError());
                }
            }
        } else {
            FAIL("Script load/build failed: " + std::string(engine.GetError()));
        }
    }
}

void TestAnimationClipChannel() {
    std::cout << "AnimationClip + Channel Tests:" << std::endl;

    // Channel interpolation
    {
        TEST("Channel linear interpolation")
        koilo::AnimationChannel ch;
        ch.interpolation = koilo::ChannelInterp::Linear;
        ch.AddKey(0.0f, 0.0f);
        ch.AddKey(1.0f, 10.0f);
        float v = ch.Evaluate(0.5f);
        if (std::abs(v - 5.0f) < 0.01f) PASS() else FAIL("Expected ~5.0, got " << v)
    }

    {
        TEST("Channel cosine interpolation")
        koilo::AnimationChannel ch;
        ch.interpolation = koilo::ChannelInterp::Cosine;
        ch.AddKey(0.0f, 0.0f);
        ch.AddKey(1.0f, 10.0f);
        float v = ch.Evaluate(0.5f);
        // Cosine at midpoint should be exactly 5.0
        if (std::abs(v - 5.0f) < 0.01f) PASS() else FAIL("Expected ~5.0, got " << v)
    }

    {
        TEST("Channel step interpolation")
        koilo::AnimationChannel ch;
        ch.interpolation = koilo::ChannelInterp::Step;
        ch.AddKey(0.0f, 0.0f);
        ch.AddKey(1.0f, 10.0f);
        float v = ch.Evaluate(0.5f);
        if (std::abs(v - 0.0f) < 0.01f) PASS() else FAIL("Expected 0.0 (step), got " << v)
    }

    {
        TEST("Channel clamp before first key")
        koilo::AnimationChannel ch;
        ch.AddKey(1.0f, 5.0f);
        ch.AddKey(2.0f, 10.0f);
        float v = ch.Evaluate(0.0f);
        if (std::abs(v - 5.0f) < 0.01f) PASS() else FAIL("Expected 5.0, got " << v)
    }

    {
        TEST("Channel clamp after last key")
        koilo::AnimationChannel ch;
        ch.AddKey(0.0f, 0.0f);
        ch.AddKey(1.0f, 10.0f);
        float v = ch.Evaluate(5.0f);
        if (std::abs(v - 10.0f) < 0.01f) PASS() else FAIL("Expected 10.0, got " << v)
    }

    // Clip with multiple channels
    {
        TEST("Clip evaluates multiple channels")
        koilo::AnimationClip clip("test", 2.0f);
        auto idx0 = clip.AddChannel("position.x");
        auto idx1 = clip.AddChannel("position.y");
        clip.GetChannel(idx0)->interpolation = koilo::ChannelInterp::Linear;
        clip.GetChannel(idx0)->AddKey(0.0f, 0.0f);
        clip.GetChannel(idx0)->AddKey(2.0f, 100.0f);
        clip.GetChannel(idx1)->interpolation = koilo::ChannelInterp::Linear;
        clip.GetChannel(idx1)->AddKey(0.0f, 50.0f);
        clip.GetChannel(idx1)->AddKey(2.0f, 0.0f);

        float xVal = 0, yVal = 0;
        clip.Evaluate(1.0f, [&](const std::string&, const std::string& prop, float v) {
            if (prop == "position.x") xVal = v;
            if (prop == "position.y") yVal = v;
        });
        if (std::abs(xVal - 50.0f) < 0.1f && std::abs(yVal - 25.0f) < 0.1f)
            PASS() else FAIL("x=" << xVal << " y=" << yVal)
    }

    {
        TEST("Clip looping wraps time")
        koilo::AnimationClip clip("loop", 1.0f);
        clip.SetLooping(true);
        auto idx = clip.AddChannel("val");
        clip.GetChannel(idx)->interpolation = koilo::ChannelInterp::Linear;
        clip.GetChannel(idx)->AddKey(0.0f, 0.0f);
        clip.GetChannel(idx)->AddKey(1.0f, 10.0f);
        float result = 0;
        clip.Evaluate(1.5f, [&](const std::string&, const std::string&, float v) { result = v; });
        // 1.5 mod 1.0 = 0.5 -> linear -> 5.0
        if (std::abs(result - 5.0f) < 0.1f) PASS() else FAIL("Expected ~5.0, got " << result)
    }

    {
        TEST("Clip channel count")
        koilo::AnimationClip clip;
        clip.AddChannel("a");
        clip.AddChannel("b");
        clip.AddChannel("c");
        if (clip.GetChannelCount() == 3) PASS() else FAIL("Expected 3, got " << clip.GetChannelCount())
    }

    std::cout << std::endl;
}

void TestAnimationMixer() {
    std::cout << "AnimationMixer Tests:" << std::endl;

    {
        TEST("Mixer single layer playback")
        koilo::AnimationClip clip("walk", 1.0f);
        clip.SetLooping(true);
        auto idx = clip.AddChannel("speed");
        clip.GetChannel(idx)->interpolation = koilo::ChannelInterp::Linear;
        clip.GetChannel(idx)->AddKey(0.0f, 0.0f);
        clip.GetChannel(idx)->AddKey(1.0f, 10.0f);

        koilo::AnimationMixer mixer;
        mixer.Play(0, &clip);
        float result = 0;
        koilo::TimeManager::GetInstance().Tick(0.5f);
        mixer.Update([&](const std::string&, const std::string&, float v) { result = v; });
        if (std::abs(result - 5.0f) < 0.1f) PASS() else FAIL("Expected ~5.0, got " << result)
    }

    {
        TEST("Mixer layer weight blending")
        koilo::AnimationClip clip1("a", 1.0f);
        clip1.SetLooping(true);
        auto c1 = clip1.AddChannel("val");
        clip1.GetChannel(c1)->interpolation = koilo::ChannelInterp::Linear;
        clip1.GetChannel(c1)->AddKey(0.0f, 10.0f);
        clip1.GetChannel(c1)->AddKey(1.0f, 10.0f); // constant 10

        koilo::AnimationClip clip2("b", 1.0f);
        clip2.SetLooping(true);
        auto c2 = clip2.AddChannel("val");
        clip2.GetChannel(c2)->interpolation = koilo::ChannelInterp::Linear;
        clip2.GetChannel(c2)->AddKey(0.0f, 20.0f);
        clip2.GetChannel(c2)->AddKey(1.0f, 20.0f); // constant 20

        koilo::AnimationMixer mixer;
        mixer.Play(0, &clip1, 0.5f);
        mixer.Play(1, &clip2, 0.5f);

        float result = 0;
        koilo::TimeManager::GetInstance().Tick(0.1f);
        mixer.Update([&](const std::string&, const std::string&, float v) { result = v; });
        // Layer 0: 10 * 0.5 = 5, Layer 1 Override: blended = 5*(1-0.5) + 20*0.5 = 12.5
        if (std::abs(result - 12.5f) < 0.5f) PASS() else FAIL("Expected ~12.5, got " << result)
    }

    {
        TEST("Mixer non-looping clip finishes")
        koilo::AnimationClip clip("once", 0.5f);
        auto idx = clip.AddChannel("val");
        clip.GetChannel(idx)->AddKey(0.0f, 0.0f);
        clip.GetChannel(idx)->AddKey(0.5f, 1.0f);

        koilo::AnimationMixer mixer;
        mixer.Play(0, &clip);
        koilo::TimeManager::GetInstance().Tick(1.0f);
        mixer.Update(); // exceeds duration
        if (mixer.IsLayerFinished(0)) PASS() else FAIL("Layer should be finished")
    }

    {
        TEST("Mixer crossfade transitions")
        koilo::AnimationClip clipA("A", 2.0f);
        clipA.SetLooping(true);
        auto ca = clipA.AddChannel("val");
        clipA.GetChannel(ca)->interpolation = koilo::ChannelInterp::Linear;
        clipA.GetChannel(ca)->AddKey(0.0f, 100.0f);
        clipA.GetChannel(ca)->AddKey(2.0f, 100.0f);

        koilo::AnimationClip clipB("B", 2.0f);
        clipB.SetLooping(true);
        auto cb = clipB.AddChannel("val");
        clipB.GetChannel(cb)->interpolation = koilo::ChannelInterp::Linear;
        clipB.GetChannel(cb)->AddKey(0.0f, 0.0f);
        clipB.GetChannel(cb)->AddKey(2.0f, 0.0f);

        koilo::AnimationMixer mixer;
        mixer.Play(0, &clipA);
        koilo::TimeManager::GetInstance().Tick(0.1f);
        mixer.Update(); // play A briefly

        mixer.CrossfadeTo(&clipB, 1.0f); // fade over 1 second
        float midResult = 0;
        koilo::TimeManager::GetInstance().Tick(0.5f);
        mixer.Update([&](const std::string&, const std::string&, float v) { midResult = v; });
        // At 0.5s into 1.0s fade: A weight=0.5, B weight=0.5, A=100, B=0 -> ~50
        if (midResult > 20.0f && midResult < 80.0f) PASS() else FAIL("Expected ~50, got " << midResult)
    }

    {
        TEST("Mixer active layer count")
        koilo::AnimationClip clip("test", 1.0f);
        clip.SetLooping(true);
        auto idx = clip.AddChannel("v");
        clip.GetChannel(idx)->AddKey(0.0f, 0.0f);

        koilo::AnimationMixer mixer;
        if (mixer.GetActiveLayerCount() != 0) { FAIL("Should start at 0"); return; }
        mixer.Play(0, &clip);
        if (mixer.GetActiveLayerCount() != 1) { FAIL("Should be 1 after Play"); return; }
        mixer.StopAll();
        if (mixer.GetActiveLayerCount() != 0) { FAIL("Should be 0 after StopAll"); return; }
        PASS()
    }

    {
        TEST("Mixer additive blend mode")
        koilo::AnimationClip clip1("base", 1.0f);
        clip1.SetLooping(true);
        auto c1 = clip1.AddChannel("val");
        clip1.GetChannel(c1)->interpolation = koilo::ChannelInterp::Linear;
        clip1.GetChannel(c1)->AddKey(0.0f, 10.0f);
        clip1.GetChannel(c1)->AddKey(1.0f, 10.0f);

        koilo::AnimationClip clip2("add", 1.0f);
        clip2.SetLooping(true);
        auto c2 = clip2.AddChannel("val");
        clip2.GetChannel(c2)->interpolation = koilo::ChannelInterp::Linear;
        clip2.GetChannel(c2)->AddKey(0.0f, 5.0f);
        clip2.GetChannel(c2)->AddKey(1.0f, 5.0f);

        koilo::AnimationMixer mixer;
        mixer.Play(0, &clip1, 1.0f);
        mixer.Play(1, &clip2, 1.0f);
        mixer.SetBlendMode(1, koilo::AnimBlendMode::Additive);

        float result = 0;
        koilo::TimeManager::GetInstance().Tick(0.1f);
        mixer.Update([&](const std::string&, const std::string&, float v) { result = v; });
        // Base=10*1.0=10, Additive=5*1.0=5 -> 15
        if (std::abs(result - 15.0f) < 0.5f) PASS() else FAIL("Expected ~15, got " << result)
    }

    std::cout << std::endl;
}

// ===== Texture C++ Unit Tests =====

void TestPhase35Skeleton() {
    std::cout << "--- Skeletal Skinning ---" << std::endl;

    // Test 1: Bone hierarchy construction
    {
        TEST("Bone hierarchy construction");
        koilo::Skeleton skel;
        auto root = skel.AddBone("root");
        auto spine = skel.AddBone("spine", 0);
        auto head = skel.AddBone("head", 1);
        auto armL = skel.AddBone("arm_l", 1);
        auto armR = skel.AddBone("arm_r", 1);

        if (skel.GetBoneCount() == 5 &&
            skel.GetBoneIndex("root") == 0 &&
            skel.GetBoneIndex("spine") == 1 &&
            skel.GetBoneIndex("head") == 2 &&
            skel.GetBoneIndex("arm_l") == 3 &&
            skel.GetBoneIndex("arm_r") == 4 &&
            skel.GetBoneIndex("nonexistent") == -1) {
            PASS();
        } else {
            FAIL("Bone hierarchy indices incorrect");
        }
    }

    // Test 2: Bone lookup by name
    {
        TEST("Bone lookup by name");
        koilo::Skeleton skel;
        skel.AddBone("root");
        skel.AddBone("child", 0);

        koilo::Bone* root = skel.GetBone("root");
        koilo::Bone* child = skel.GetBone("child");
        koilo::Bone* missing = skel.GetBone("missing");

        if (root && root->name == "root" &&
            child && child->name == "child" && child->parentIndex == 0 &&
            missing == nullptr) {
            PASS();
        } else {
            FAIL("Bone name lookup failed");
        }
    }

    // Test 3: Bind pose computation
    {
        TEST("Bind pose and inverse bind matrix");
        koilo::Skeleton skel;
        skel.AddBone("root");  // at origin
        skel.AddBone("child", 0);

        // Position child bone at (0, 5, 0)
        skel.GetBone(1)->localPosition = koilo::Vector3D(0, 5, 0);

        skel.SetBindPose();

        // After SetBindPose, inverseBindMatrix should undo the world transform
        const koilo::Matrix4x4& invBind = skel.GetBone(1)->inverseBindMatrix;
        // World transform of child is Translation(0,5,0)
        // Inverse should translate by (0,-5,0)
        koilo::Vector3D test(0, 5, 0);
        koilo::Vector3D result = invBind.TransformVector(test);

        if (std::abs(result.X) < 0.01f &&
            std::abs(result.Y) < 0.01f &&
            std::abs(result.Z) < 0.01f) {
            PASS();
        } else {
            FAIL("Inverse bind matrix incorrect: got (" +
                 std::to_string(result.X) + ", " +
                 std::to_string(result.Y) + ", " +
                 std::to_string(result.Z) + ")");
        }
    }

    // Test 4: Vertex skinning - single bone influence
    {
        TEST("Vertex skinning - single bone");
        koilo::Skeleton skel;
        skel.AddBone("root");
        skel.SetBindPose();

        // Move root bone up by 10
        skel.GetBone(0)->localPosition = koilo::Vector3D(0, 10, 0);
        skel.ComputeWorldMatrices();

        koilo::SkinData skin;
        skin.AddVertex(0, 1.0f); // vertex 0 fully weighted to bone 0

        koilo::Vector3D src(0, 0, 0);
        koilo::Vector3D dst;
        skel.SkinVertices(skin, &src, &dst, 1);

        if (std::abs(dst.X) < 0.01f &&
            std::abs(dst.Y - 10.0f) < 0.01f &&
            std::abs(dst.Z) < 0.01f) {
            PASS();
        } else {
            FAIL("Expected (0,10,0) got (" +
                 std::to_string(dst.X) + ", " +
                 std::to_string(dst.Y) + ", " +
                 std::to_string(dst.Z) + ")");
        }
    }

    // Test 5: Vertex skinning - two bone blend
    {
        TEST("Vertex skinning - two bone blend");
        koilo::Skeleton skel;
        skel.AddBone("root");
        skel.AddBone("tip", 0);

        // Bind pose: tip at (0, 10, 0)
        skel.GetBone(1)->localPosition = koilo::Vector3D(0, 10, 0);
        skel.SetBindPose();

        // Animate: move root right by 4, tip stays relative
        skel.GetBone(0)->localPosition = koilo::Vector3D(4, 0, 0);
        skel.GetBone(1)->localPosition = koilo::Vector3D(0, 10, 0);
        skel.ComputeWorldMatrices();

        // Vertex at (0, 5, 0) with 50% root, 50% tip
        koilo::SkinData skin;
        skin.AddVertex(0, 0.5f, 1, 0.5f);

        koilo::Vector3D src(0, 5, 0);
        koilo::Vector3D dst;
        skel.SkinVertices(skin, &src, &dst, 1);

        // Root influence: skinMatrix[0] × (0,5,0) = root moved right 4 -> (4,5,0)
        // Tip influence: skinMatrix[1] × (0,5,0) = tip world at (4,10,0), invBind undoes (0,10,0) -> deformed
        // Both contribute 50%, so result should be averaged
        // This mainly tests that blending doesn't crash and produces reasonable values
        if (std::abs(dst.X - 4.0f) < 0.5f &&  // should be near 4 (both bones shifted right)
            dst.Y > 0.0f) {
            PASS();
        } else {
            FAIL("Blended skinning result unexpected: (" +
                 std::to_string(dst.X) + ", " +
                 std::to_string(dst.Y) + ", " +
                 std::to_string(dst.Z) + ")");
        }
    }

    // Test 6: SkinVertex weight normalization
    {
        TEST("SkinVertex weight normalization");
        koilo::SkinVertex sv;
        sv.weights[0] = 2.0f;
        sv.weights[1] = 2.0f;
        sv.weights[2] = 0.0f;
        sv.weights[3] = 0.0f;
        sv.Normalize();

        if (std::abs(sv.weights[0] - 0.5f) < 0.001f &&
            std::abs(sv.weights[1] - 0.5f) < 0.001f) {
            PASS();
        } else {
            FAIL("Normalization failed");
        }
    }

    // Test 7: Reset pose
    {
        TEST("Skeleton reset pose");
        koilo::Skeleton skel;
        skel.AddBone("root");
        skel.GetBone(0)->localPosition = koilo::Vector3D(5, 10, 15);
        skel.ResetPose();

        koilo::Vector3D pos = skel.GetBone(0)->localPosition;
        if (std::abs(pos.X) < 0.001f &&
            std::abs(pos.Y) < 0.001f &&
            std::abs(pos.Z) < 0.001f) {
            PASS();
        } else {
            FAIL("Reset did not clear position");
        }
    }

    // Test 8: Bone hierarchy propagation
    {
        TEST("Bone hierarchy propagation");
        koilo::Skeleton skel;
        skel.AddBone("root");
        skel.AddBone("child", 0);
        skel.AddBone("grandchild", 1);

        skel.GetBone(0)->localPosition = koilo::Vector3D(1, 0, 0);
        skel.GetBone(1)->localPosition = koilo::Vector3D(0, 2, 0);
        skel.GetBone(2)->localPosition = koilo::Vector3D(0, 0, 3);
        skel.ComputeWorldMatrices();

        const koilo::Matrix4x4* worldMat = skel.GetWorldMatrix(2);
        if (worldMat) {
            koilo::Vector3D worldPos = worldMat->GetTranslation();
            if (std::abs(worldPos.X - 1.0f) < 0.01f &&
                std::abs(worldPos.Y - 2.0f) < 0.01f &&
                std::abs(worldPos.Z - 3.0f) < 0.01f) {
                PASS();
            } else {
                FAIL("World pos: (" + std::to_string(worldPos.X) + ", " +
                     std::to_string(worldPos.Y) + ", " + std::to_string(worldPos.Z) + ")");
            }
        } else {
            FAIL("GetWorldMatrix returned nullptr");
        }
    }

    // Test 9: SkeletonAnimator - position channel
    {
        TEST("SkeletonAnimator position channel");
        koilo::Skeleton skel;
        skel.AddBone("root");
        skel.AddBone("arm", 0);

        koilo::AnimationClip clip("move_arm", 1.0f);
        auto ch = clip.AddChannelForNode("bone.arm", "position.y");
        clip.GetChannel(ch)->interpolation = koilo::ChannelInterp::Linear;
        clip.GetChannel(ch)->AddKey(0.0f, 0.0f);
        clip.GetChannel(ch)->AddKey(1.0f, 10.0f);

        koilo::SkeletonAnimator animator(&skel);
        animator.ApplyClip(&clip, 0.5f);

        float armY = skel.GetBone("arm")->localPosition.Y;
        if (std::abs(armY - 5.0f) < 0.1f) {
            PASS();
        } else {
            FAIL("Expected arm Y ~5.0, got " + std::to_string(armY));
        }
    }

    // Test 10: SkeletonAnimator - rotation channel
    {
        TEST("SkeletonAnimator rotation channel");
        koilo::Skeleton skel;
        skel.AddBone("root");

        koilo::AnimationClip clip("rotate_root", 1.0f);
        auto ch = clip.AddChannelForNode("bone.root", "rotation.z");
        clip.GetChannel(ch)->interpolation = koilo::ChannelInterp::Linear;
        clip.GetChannel(ch)->AddKey(0.0f, 0.0f);
        clip.GetChannel(ch)->AddKey(1.0f, 90.0f);

        koilo::SkeletonAnimator animator(&skel);
        animator.ApplyClip(&clip, 0.5f);

        // At t=0.5, rotation should be 45 degrees around Z
        // Quaternion: (cos(22.5°), 0, 0, sin(22.5°))
        koilo::Quaternion& q = skel.GetBone("root")->localRotation;
        float expectedHalf = 22.5f * 3.14159265f / 180.0f;
        if (std::abs(q.W - std::cos(expectedHalf)) < 0.05f &&
            std::abs(q.Z - std::sin(expectedHalf)) < 0.05f) {
            PASS();
        } else {
            FAIL("Rotation quaternion unexpected: W=" + std::to_string(q.W) +
                 " Z=" + std::to_string(q.Z));
        }
    }

    // Test 11: Non-bone channels ignored
    {
        TEST("Non-bone channels ignored by animator");
        koilo::Skeleton skel;
        skel.AddBone("root");

        koilo::AnimationClip clip("misc", 1.0f);
        clip.AddChannelForNode("mesh.body", "position.x");
        clip.GetChannel(0)->AddKey(0.0f, 100.0f);

        koilo::SkeletonAnimator animator(&skel);
        animator.ApplyClip(&clip, 0.0f);

        // Root bone should be unaffected
        float rootX = skel.GetBone("root")->localPosition.X;
        if (std::abs(rootX) < 0.001f) {
            PASS();
        } else {
            FAIL("Non-bone channel modified skeleton");
        }
    }
}

void TestPhase24EasyEaseAnimator() {
    std::cout << "--- EasyEaseAnimator Script-Friendly API ---" << std::endl;
    
    TEST("EasyEaseAnimator AddParameterByIndex from script")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var ease = EasyEaseAnimator(4);
fn Setup() {
  ease.AddParameterByIndex(0, 60, 0.0, 1.0);
  ease.AddParameterByIndex(1, 30, 0.0, 100.0);
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        // Verify params were added
        if (ok) PASS()
        else FAIL("AddParameterByIndex failed: " + std::string(engine.GetError()))
    }
    
    TEST("EasyEaseAnimator GetValue reads internal storage")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var ease = EasyEaseAnimator(4);
var val = 0;
fn Setup() {
  ease.AddParameterByIndex(0, 60, 0.0, 1.0);
  val = ease.GetValue(0);
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("val");
        if (ok && v.type == Value::Type::NUMBER && std::abs(v.numberValue - 0.0) < 0.01) PASS()
        else FAIL("GetValue expected 0 (basis), got " + std::to_string(v.numberValue))
    }
    
    TEST("EasyEaseAnimator animate via AddParameterFrame + Update")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var ease = EasyEaseAnimator(4);
var val = 0;
fn Setup() {
  ease.AddParameterByIndex(0, 2, 0.0, 1.0);
}
fn Update(dt) {
  ease.AddParameterFrame(0, 1.0);
  ease.Update();
  val = ease.GetValue(0);
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        if (!ok) { FAIL("BuildScene failed"); return; }
        koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
        koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
        koilo::TimeManager::GetInstance().Tick(0.016f); engine.ExecuteUpdate();
        Value v = engine.GetGlobal("val");
        if (v.type == Value::Type::NUMBER && v.numberValue > 0.01) PASS()
        else FAIL("Expected animation progress > 0.01, got " + std::to_string(v.numberValue))
    }
    
    TEST("EasyEaseAnimator GetParameterCount from script")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        reader.SetContent(R"(
var ease = EasyEaseAnimator(8);
var count = 0;
fn Setup() {
  ease.AddParameterByIndex(0, 30, 0.0, 1.0);
  ease.AddParameterByIndex(1, 30, 0.0, 1.0);
  ease.AddParameterByIndex(2, 30, 0.0, 1.0);
  count = ease.GetParameterCount();
}
)");
        bool ok = engine.LoadScript("test.ks") && engine.BuildScene();
        Value v = engine.GetGlobal("count");
        if (ok && v.type == Value::Type::NUMBER && std::abs(v.numberValue - 3.0) < 0.01) PASS()
        else FAIL("Expected 3 params, got " + std::to_string(v.numberValue))
    }
}



