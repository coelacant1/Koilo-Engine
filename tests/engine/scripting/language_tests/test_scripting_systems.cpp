// SPDX-License-Identifier: GPL-3.0-or-later
// Particles & Audio Scripting Bindings
#include "helpers.hpp"
#include <koilo/systems/particles/particleemitter.hpp>
#include <koilo/systems/particles/particlesystem.hpp>

void TestParticleSystem() {
    using namespace koilo;
    std::cout << "--- ParticleSystem ---" << std::endl;

    // Test 1: Emitter creation and pool
    {
        ParticleEmitter emitter;
        TEST("Default emitter")
        if (!emitter.IsPlaying() && emitter.GetActiveParticleCount() == 0) PASS() else FAIL("bad defaults")
    }

    // Test 2: Manual emit
    {
        ParticleEmitter emitter;
        emitter.Play();
        emitter.Emit();
        TEST("Manual emit")
        if (emitter.GetActiveParticleCount() == 1) PASS() else FAIL("count: " + std::to_string(emitter.GetActiveParticleCount()))
    }

    // Test 3: Emit burst
    {
        ParticleEmitter emitter;
        emitter.EmitBurst(5);
        TEST("Emit burst")
        if (emitter.GetActiveParticleCount() == 5) PASS() else FAIL("count: " + std::to_string(emitter.GetActiveParticleCount()))
    }

    // Test 4: Update simulation
    {
        ParticleEmitter emitter;
        emitter.SetPosition(10, 10, 0);
        emitter.SetVelocityRange(5, 0, 0, 5, 0, 0);
        emitter.SetLifetime(1.0f, 1.0f);
        emitter.SetGravity(0, 0, 0);
        emitter.Emit();
        koilo::TimeManager::GetInstance().Tick(0.5f); emitter.Update();
        TEST("Update moves particles")
        if (emitter.GetActiveParticleCount() == 1) PASS() else FAIL("particle died early")
    }

    // Test 5: Particles die after lifetime
    {
        ParticleEmitter emitter;
        emitter.SetLifetime(0.1f, 0.1f);
        emitter.Emit();
        koilo::TimeManager::GetInstance().Tick(0.2f); emitter.Update();
        TEST("Particle death")
        if (emitter.GetActiveParticleCount() == 0) PASS() else FAIL("particle didn't die")
    }

    // Test 6: Render to buffer
    {
        ParticleEmitter emitter;
        emitter.SetPosition(2, 2, 0);
        emitter.SetVelocityRange(0, 0, 0, 0, 0, 0);
        emitter.SetStartColor(1.0f, 0.0f, 0.0f);
        emitter.SetEndColor(1.0f, 0.0f, 0.0f);
        emitter.Emit();

        Color888 buf[16]; // 4x4
        for (int i = 0; i < 16; i++) buf[i] = Color888(0, 0, 0);
        emitter.Render(buf, 4, 4);
        TEST("Render single pixel")
        // Pixel at (2,2) = index 10 should be red
        if (buf[10].R > 200 && buf[10].G < 10) PASS() else FAIL("R=" + std::to_string(buf[10].R))
    }

    // Test 7: Render size > 1
    {
        ParticleEmitter emitter;
        emitter.SetPosition(2, 2, 0);
        emitter.SetVelocityRange(0, 0, 0, 0, 0, 0);
        emitter.SetStartColor(0.0f, 1.0f, 0.0f);
        emitter.SetEndColor(0.0f, 1.0f, 0.0f);
        emitter.SetParticleSize(3);
        emitter.Emit();

        Color888 buf[25]; // 5x5
        for (int i = 0; i < 25; i++) buf[i] = Color888(0, 0, 0);
        emitter.Render(buf, 5, 5);
        TEST("Render size 3")
        // Center (2,2)=12 and neighbor (1,2)=11 should both be green
        if (buf[12].G > 200 && buf[11].G > 200) PASS() else FAIL("size 3 didn't spread")
    }

    // Test 8: ParticleSystem multi-emitter
    {
        ParticleSystem sys;
        ParticleEmitter* e1 = sys.AddEmitter(10);
        ParticleEmitter* e2 = sys.AddEmitter(10);
        e1->Emit();
        e2->EmitBurst(3);
        TEST("System multi-emitter")
        if (sys.GetTotalActiveParticles() == 4 && sys.GetEmitterCount() == 2) PASS()
        else FAIL("total: " + std::to_string(sys.GetTotalActiveParticles()))
    }

    // Test 9: ParticleSystem render
    {
        ParticleSystem sys;
        ParticleEmitter* e = sys.AddEmitter(10);
        e->SetPosition(1, 1, 0);
        e->SetVelocityRange(0, 0, 0, 0, 0, 0);
        e->SetStartColor(0.0f, 0.0f, 1.0f);
        e->SetEndColor(0.0f, 0.0f, 1.0f);
        e->Emit();

        Color888 buf[9]; // 3x3
        for (int i = 0; i < 9; i++) buf[i] = Color888(0, 0, 0);
        sys.Render(buf, 3, 3);
        TEST("System render")
        // (1,1) = index 4 should be blue
        if (buf[4].B > 200 && buf[4].R < 10) PASS() else FAIL("B=" + std::to_string(buf[4].B))
    }

    // Test 10: Clear
    {
        ParticleEmitter emitter;
        emitter.EmitBurst(10);
        emitter.Clear();
        TEST("Clear kills particles")
        if (emitter.GetActiveParticleCount() == 0) PASS() else FAIL("not cleared")
    }

    // Test 11: Reflection
    {
        ParticleEmitter::Describe();
        ParticleSystem::Describe();
        const ClassDesc* pe = ReflectionBridge::FindClass("ParticleEmitter");
        const ClassDesc* ps = ReflectionBridge::FindClass("ParticleSystem");
        TEST("ParticleEmitter reflected")
        if (pe != nullptr) PASS() else FAIL("not found")
        TEST("ParticleSystem reflected")
        if (ps != nullptr) PASS() else FAIL("not found")
    }

    // Test 12: Script access to `particles` global
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script = R"(
display.SetWidth(8);
display.SetHeight(8);
display.SetPixelWidth(8);
display.SetPixelHeight(8);

fn Setup() {
    var e = particles.AddEmitter(50);
    e.SetPosition(4, 4, 0);
    e.SetEmissionRate(20);
    e.SetStartColor(1.0, 0.5, 0.0);
}
)";
        reader.SetContent(script);
        TEST("Particles global from script")
        if (engine.LoadScript("test.ks") && engine.BuildScene()) {
            auto* ps = engine.GetParticleSystem();
            if (ps->GetEmitterCount() == 1)
                PASS() else FAIL("emitter count: " + std::to_string(ps->GetEmitterCount()))
        } else { FAIL(engine.GetError()); }
    }

    std::cout << std::endl;
}

// --- Engine Hardening Tests ---
#include <koilo/systems/render/shader/ishader.hpp>

void TestPhase19AudioSystem() {
    std::cout << "--- Audio System ---" << std::endl;

    // -- AudioClip tests --

    TEST("AudioClip: default construction")
    {
        koilo::AudioClip clip;
        if (!clip.IsLoaded() && clip.GetDuration() == 0.0f && clip.GetSampleRate() == 44100) PASS()
        else FAIL("Default AudioClip state wrong")
    }

    TEST("AudioClip: named construction")
    {
        koilo::AudioClip clip("test_sound");
        if (clip.GetName() == "test_sound" && !clip.IsLoaded()) PASS()
        else FAIL("Named AudioClip state wrong")
    }

    TEST("AudioClip: LoadFromMemory with 16-bit mono")
    {
        koilo::AudioClip clip("mem_clip");
        // 100 samples of 16-bit mono at 44100Hz
        std::vector<uint8_t> data(200, 0);
        bool ok = clip.LoadFromMemory(data.data(), data.size(), koilo::AudioFormat::Mono16, 44100);
        float expectedDuration = 100.0f / 44100.0f;
        if (ok && clip.IsLoaded() && std::abs(clip.GetDuration() - expectedDuration) < 0.001f) PASS()
        else FAIL("LoadFromMemory failed, dur=" + std::to_string(clip.GetDuration()))
    }

    TEST("AudioClip: LoadFromMemory with stereo 16-bit")
    {
        koilo::AudioClip clip;
        // 50 frames of stereo 16-bit (50 * 2ch * 2bytes = 200 bytes)
        std::vector<uint8_t> data(200, 0);
        bool ok = clip.LoadFromMemory(data.data(), data.size(), koilo::AudioFormat::Stereo16, 22050);
        float expectedDuration = 50.0f / 22050.0f;
        if (ok && clip.IsLoaded() && std::abs(clip.GetDuration() - expectedDuration) < 0.001f) PASS()
        else FAIL("Stereo16 LoadFromMemory failed")
    }

    TEST("AudioClip: Unload clears data")
    {
        koilo::AudioClip clip;
        std::vector<uint8_t> data(200, 0);
        clip.LoadFromMemory(data.data(), data.size(), koilo::AudioFormat::Mono16, 44100);
        clip.Unload();
        if (!clip.IsLoaded() && clip.GetDataSize() == 0 && clip.GetDuration() == 0.0f) PASS()
        else FAIL("Unload didn't clear state")
    }

    TEST("AudioClip: LoadFromFile with invalid path returns false")
    {
        koilo::AudioClip clip;
        bool ok = clip.LoadFromFile("/nonexistent/path.wav");
        if (!ok && !clip.IsLoaded()) PASS()
        else FAIL("LoadFromFile should fail for invalid path")
    }

    // -- AudioSource tests --

    TEST("AudioSource: default state is stopped")
    {
        koilo::AudioSource src;
        if (src.IsStopped() && !src.IsPlaying() && !src.IsPaused()) PASS()
        else FAIL("Default AudioSource should be stopped")
    }

    TEST("AudioSource: Play/Pause/Stop state transitions")
    {
        auto clip = std::make_shared<koilo::AudioClip>();
        std::vector<uint8_t> data(1000, 0);
        clip->LoadFromMemory(data.data(), data.size(), koilo::AudioFormat::Mono16, 44100);

        koilo::AudioSource src(clip);
        src.Play();
        bool playing = src.IsPlaying();
        src.Pause();
        bool paused = src.IsPaused();
        src.Play();
        src.Stop();
        bool stopped = src.IsStopped();

        if (playing && paused && stopped) PASS()
        else FAIL("State transitions failed: play=" + std::to_string(playing) + " pause=" + std::to_string(paused) + " stop=" + std::to_string(stopped))
    }

    TEST("AudioSource: volume/pitch/pan clamping")
    {
        koilo::AudioSource src;
        src.SetVolume(1.5f);
        src.SetPitch(-0.5f);
        src.SetPan(2.0f);
        if (src.GetVolume() <= 1.0f && src.GetPitch() >= 0.1f && src.GetPan() <= 1.0f) PASS()
        else FAIL("Clamping failed")
    }

    TEST("AudioSource: Update advances playback position")
    {
        auto clip = std::make_shared<koilo::AudioClip>();
        std::vector<uint8_t> data(88200, 0);  // 1 second of mono 16-bit at 44100
        clip->LoadFromMemory(data.data(), data.size(), koilo::AudioFormat::Mono16, 44100);

        koilo::AudioSource src(clip);
        src.Play();
        koilo::TimeManager::GetInstance().Tick(0.5f); src.Update();
        if (src.GetPlaybackPosition() > 0.4f && src.GetPlaybackPosition() < 0.6f) PASS()
        else FAIL("Playback position wrong: " + std::to_string(src.GetPlaybackPosition()))
    }

    TEST("AudioSource: looping wraps position")
    {
        auto clip = std::make_shared<koilo::AudioClip>();
        std::vector<uint8_t> data(88200, 0);  // 1 second
        clip->LoadFromMemory(data.data(), data.size(), koilo::AudioFormat::Mono16, 44100);

        koilo::AudioSource src(clip);
        src.SetLoop(true);
        src.Play();
        koilo::TimeManager::GetInstance().Tick(1.5f); src.Update();
        // Should still be playing (looped)
        if (src.IsPlaying()) PASS()
        else FAIL("Looping source should still be playing after exceeding duration")
    }

    TEST("AudioSource: 3D properties")
    {
        koilo::AudioSource src;
        src.SetSpatial(true);
        src.SetPosition(koilo::Vector3D(1, 2, 3));
        src.SetMinDistance(0.5f);
        src.SetMaxDistance(50.0f);
        src.SetRolloffFactor(2.0f);
        if (src.IsSpatial() && src.GetMinDistance() == 0.5f &&
            src.GetMaxDistance() == 50.0f && src.GetRolloffFactor() == 2.0f) PASS()
        else FAIL("3D properties not set correctly")
    }

    // -- AudioManager tests --

    TEST("AudioManager: clip management")
    {
        koilo::AudioManager mgr;
        // Can't test LoadClip from file without a real file, but test GetClip/UnloadClip
        auto clip = mgr.GetClip("nonexistent");
        if (clip == nullptr) PASS()
        else FAIL("GetClip should return nullptr for unknown clip")
    }

    TEST("AudioManager: volume controls")
    {
        koilo::AudioManager mgr;
        mgr.SetMasterVolume(0.8f);
        mgr.SetMusicVolume(0.6f);
        mgr.SetSFXVolume(0.4f);
        mgr.SetVoiceVolume(0.2f);
        if (mgr.GetMasterVolume() == 0.8f && mgr.GetMusicVolume() == 0.6f &&
            mgr.GetSFXVolume() == 0.4f && mgr.GetVoiceVolume() == 0.2f) PASS()
        else FAIL("Volume controls failed")
    }

    TEST("AudioManager: active source count")
    {
        koilo::AudioManager mgr;
        if (mgr.GetActiveSourceCount() == 0) PASS()
        else FAIL("Should start with 0 active sources")
    }

    // -- ScriptAudioManager tests --

    TEST("ScriptAudioManager: master volume round-trip")
    {
        koilo::ScriptAudioManager audio;
        audio.SetMasterVolume(0.75f);
        if (audio.GetMasterVolume() == 0.75f) PASS()
        else FAIL("Volume round-trip failed")
    }

    TEST("ScriptAudioManager: active count starts at zero")
    {
        koilo::ScriptAudioManager audio;
        if (audio.GetActiveCount() == 0) PASS()
        else FAIL("Should start at 0")
    }

    TEST("ScriptAudioManager: GetManager direct access")
    {
        koilo::ScriptAudioManager audio;
        koilo::AudioManager* mgr = audio.GetManager();
        if (mgr != nullptr) PASS()
        else FAIL("Direct manager access failed")
    }

    // -- Script integration --

    TEST("Script: audio global is accessible")
    {
        StringFileReader reader;
        KoiloScriptEngine engine(&reader);
        std::string script =
            "fn Setup() {\n"
            "  audio.SetMasterVolume(0.5);\n"
            "}\n"
            "fn Update(dt) {\n"
            "}\n";
        reader.SetContent(script);
        engine.LoadScript("test.ks");
        engine.BuildScene();
        auto* mgr = engine.GetAudio();
        if (mgr->GetMasterVolume() == 0.5f) PASS()
        else FAIL("Script audio global didn't set volume, got " + std::to_string(mgr->GetMasterVolume()))
    }
}



