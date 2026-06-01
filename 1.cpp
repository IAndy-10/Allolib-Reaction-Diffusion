/*
  reaction-diffusion/6.cpp
  Same as 5.cpp but with automatic simulation reset on a fixed interval.
*/

#include "Gamma/Filter.h"
#include "al/app/al_DistributedApp.hpp"
#include "al/app/al_GUIDomain.hpp"
#include "al/graphics/al_Texture.hpp"
#include "al/math/al_Random.hpp"
#include "al/sound/al_SoundFile.hpp"
#include "al/sound/al_Lbap.hpp"
#include "al/sound/al_Speaker.hpp"
#include "al/sound/al_StereoPanner.hpp"
#include "al/sphere/al_AlloSphereSpeakerLayout.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

using namespace al;

// ── Helpers ──────────────────────────────────────────────────────────────────

std::string slurp3(std::string fileName) {
  std::fstream file(fileName);
  std::string out;
  while (file.good()) {
    std::string line;
    std::getline(file, line);
    out += line + "\n";
  }
  return out;
}

std::string sourceDir3() {
  std::string p = __FILE__;
  return p.substr(0, p.rfind("/"));
}

// ── Constants ─────────────────────────────────────────────────────────────────

static const int SIM_W      = 512;
static const int SIM_H      = 512;
static const int SPHERE_LAT = 300;  // must match rd_display.vert.glsl
static const int SPHERE_LON = 600;  // must match rd_display.vert.glsl
static const int SIM_STEPS  = 8;

// ── Auto-reset timing ─────────────────────────────────────────────────────────
// Change this value to adjust how often the simulation resets (in seconds).
static const double AUTO_RESET_INTERVAL = 8.0;

// ── Shared State ──────────────────────────────────────────────────────────────

struct WorldState {
  Pose     camera;
  int32_t  paletteIndex = 0;
  uint32_t resetCount   = 0;
  float    dA           = 1.0f;
  float    dB           = 0.5f;
  float    feed         = 0.055f;
  float    k            = 0.062f;
  float    simDt        = 0.009f;
  float    dispScale    = 5.0f;
};

// ── App ───────────────────────────────────────────────────────────────────────

struct MyApp : public DistributedAppWithState<WorldState> {
  // ── Simulation textures (ping-pong) ──────────────────────────────────────
  Texture texA, texB;
  GLuint  fboA = 0, fboB = 0;
  bool    pingA = true;

  // ── GPU meshes & shaders ─────────────────────────────────────────────────
  GLuint        emptyVAO = 0;
  Mesh          gridMesh;
  ShaderProgram simShader;
  ShaderProgram dispShader;

  uint32_t lastResetCount = 0;
  double   resetTimer     = 0.0;  // accumulates elapsed time on primary

  float countingTime = 0.f;

  // ── Audio ─────────────────────────────────────────────────────────────────
  SoundFilePlayerTS   player;
  std::vector<float>  audioBuffer;
  uint64_t            samplePos   = 0;   // tracks playback position
  double              playbackSec = 0.0;

  // ── DSP: filter + spatializer ────────────────────────────────────────────
  gam::Biquad<> filterL;              // low-pass filter, left channel
  gam::Biquad<> filterR;              // low-pass filter, right channel

  Speakers     speakerLayout;
  Spatializer* spatializer{nullptr};
  double       mSoundElapsedTime{0.0};  // drives the source orbit
  Vec3f        srcPos{0.f, 0.f, 0.f};  // current 3D source position

  // ── Drum onsets ───────────────────────────────────────────────────────────
  std::vector<double> drumOnsets;
  size_t              nextOnsetIdx = 0;


  // ── GUI Parameters (primary only) ────────────────────────────────────────
  Parameter    p_dA          {"/dA",          "", 1.0f,   0.1f,   2.0f};
  Parameter    p_dB          {"/dB",          "", 0.5f,   0.1f,   1.0f};
  Parameter    p_feed        {"/feed",        "", 0.055f, 0.01f,  0.1f};
  Parameter    p_k           {"/k",           "", 0.062f, 0.01f,  0.1f};
  Parameter    p_simDt       {"/simDt",       "", 0.009f, 0.001f, 0.02f};
  Parameter    p_dispScale   {"/dispScale",   "", 5.0f,   0.0f,  20.0f};
  ParameterInt p_palette     {"/palette",     "", 0,      0,     15};
  Parameter    p_filterCutoff{"/filterCutoff","", 8000.f, 20.f,  20000.f};
  Parameter    p_panSpeed    {"/panSpeed",    "", 0.05f,  0.01f,  0.5f};
  Parameter    p_gainDB{"/gainDB", "", 0.0f, -40.0f, 6.0f};

  // ── Colour palettes ──────────────────────────────────────────────────────
  // Designed for low-brightness projection: dark backgrounds, fully saturated
  // electric colours. High contrast between A, B and background.
  struct Palette { Vec3f a, b, bg; };
  std::vector<Palette> palettes = {
    // ── cyan / neon-magenta ───────────────────────────────────────────────
    { {0.0f,  1.0f, 1.0f},  {1.0f, 0.0f, 0.8f},  {0.0f,  0.05f, 0.12f} }, // N°0  original  · bg: deep-teal
    // ── electric-violet / hot-pink ────────────────────────────────────────
    { {0.35f, 0.0f, 1.0f},  {1.0f, 0.0f, 0.35f}, {0.12f, 0.0f,  0.25f} }, // N°1  original  · bg: deep-purple
    // ── hard-yellow / electric-blue ───────────────────────────────────────
    { {1.0f,  1.0f, 0.0f},  {0.0f, 0.5f, 1.0f},  {0.08f, 0.04f, 0.2f } }, // N°2  original  · bg: deep-indigo
    // ── neon-pink / neon-aqua  ────────────────────────────────────────────
    { {1.0f,  0.0f, 0.5f},  {0.0f, 1.0f, 0.9f},  {0.05f, 0.0f,  0.02f} }, // N°3  original  · bg: near-black-red
    // ── ice-blue / electric-amber ─────────────────────────────────────────
    { {0.0f,  0.8f, 1.0f},  {1.0f, 0.5f, 0.0f},  {0.0f,  0.0f,  0.12f} }, // N°4  original  · bg: deep-blue
    // ── neon-pink / neon-aqua  (second pair) ─────────────────────────────
    { {1.0f,  0.0f, 0.5f},  {0.0f, 1.0f, 0.9f},  {0.05f, 0.0f,  0.02f} }, // N°5 original  · bg: near-black-red
    // ── black / white / grayscale ─────────────────────────────────────────
    { {1.0f,  1.0f, 1.0f},  {0.15f,0.15f,0.15f}, {0.0f,  0.0f,  0.0f } }, // N°6 pure B&W          · white / dark-gray / black
  };

  // ── Load drum onsets from CSV ─────────────────────────────────────────────
  void loadDrumOnsets(const std::string& path) {
    std::ifstream f(path);
    if (!f.good()) {
      std::cerr << "WARNING: Could not open drum onsets: " << path << std::endl;
      return;
    }
    std::string line;
    while (std::getline(f, line)) {
      if (line.empty()) continue;
      drumOnsets.push_back(std::stod(line));
    }
    std::cout << "Loaded " << drumOnsets.size() << " drum onsets from " << path << std::endl;
  }

  // ── Seed initial state ────────────────────────────────────────────────────
  void initSim(uint32_t seed) {
    srand(seed + 42);
    std::vector<float> data(SIM_W * SIM_H * 4, 0.f);
    for (int i = 0; i < SIM_W * SIM_H; i++) {
      data[i * 4 + 0] = 1.0f;
      data[i * 4 + 1] = 0.0f;
    }

    auto seedCircle = [&](int cx, int cy, int r) {
      for (int y = cy - r; y <= cy + r; y++)
        for (int x = cx - r; x <= cx + r; x++) {
          if (x < 0 || x >= SIM_W || y < 0 || y >= SIM_H) continue;
          if ((x-cx)*(x-cx) + (y-cy)*(y-cy) <= r*r)
            data[(y * SIM_W + x) * 4 + 1] = 1.0f;
        }
    };

    for (int i = 0; i < 15; i++)
      seedCircle(rand() % SIM_W, rand() % SIM_H, 10 + rand() % 30);
    for (int i = 0; i < 50; i++)
      seedCircle(rand() % SIM_W, rand() % SIM_H, 1 + rand() % 3);

    texA.submit(data.data(), Texture::RGBA, Texture::FLOAT);
    texB.submit(data.data(), Texture::RGBA, Texture::FLOAT);
    pingA = true;
  }

  // ─────────────────────────────────────────────────────────────────────────

  void onInit() override {
    auto cuttleboneDomain =
        CuttleboneStateSimulationDomain<WorldState>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }

    if (isPrimary()) {
      // ── Spatializer setup ──────────────────────────────────────────────
      audioIO().channelsBus(1);

      // ── Allosphere (uncomment when deploying, remove stereo lines) ─────
      // speakerLayout = AlloSphereSpeakerLayoutCompensated();
      // spatializer   = new Lbap(speakerLayout);

      // ── Stereo development ─────────────────────────────────────────────
      speakerLayout = StereoSpeakerLayout();
      spatializer   = new StereoPanner(speakerLayout);

      spatializer->compile();

      auto GUIdomain = GUIDomain::enableGUI(defaultWindowDomain());
      auto& gui = GUIdomain->newGUI();
      gui.add(p_palette);
      gui.add(p_dA);
      gui.add(p_dB);
      gui.add(p_feed);
      gui.add(p_k);
      gui.add(p_simDt);
      gui.add(p_dispScale);
      gui.add(p_filterCutoff);
      gui.add(p_panSpeed);
      gui.add(p_gainDB);

      // ── Open audio file ────────────────────────────────────────────────
      std::string audioPath = sourceDir3() + "/Nala Sinephro - Continuum 1 compressed .wav";
      if (!player.open(audioPath.c_str())) {
        std::cerr << "WARNING: Could not open audio file: " << audioPath << std::endl;
      } else {
        player.setLoop();
        player.setPlay();
        
      }

      loadDrumOnsets(sourceDir3() + "/drums_onsets.csv");
    }
  }

  void onSound(AudioIOData& io) override {
    if (!isPrimary()) return;

    int frames   = io.framesPerBuffer();
    int channels = player.soundFile.channels;
    audioBuffer.resize(frames * channels);
    player.getFrames(frames, audioBuffer.data(), audioBuffer.size());

    // Update filter cutoff from parameter
    filterL.freq(p_filterCutoff);
    filterR.freq(p_filterCutoff);

    // Fill bus(0) with filtered mono signal, scaled by dB gain
    float gain = std::pow(10.f, p_gainDB / 20.f);
    while (io()) {
      int   i    = io.frame() * channels;
      float rawL = audioBuffer[i];
      float rawR = (channels > 1) ? audioBuffer[i + 1] : rawL;
      float mono = (filterL(rawL) + filterR(rawR)) * 0.5f * gain;
      io.bus(0)  = mono;
    }

    // Spatialize the bus buffer to output channels
    spatializer->prepare(io);
    spatializer->renderBuffer(io, srcPos, io.busBuffer(0), frames);
    spatializer->finalize(io);

    samplePos  += frames;
    playbackSec = double(samplePos) / 44100.0;
  }

  void onCreate() override {
    nav().pos(-140.f, 0.f, 0.f);
    nav().faceToward(Vec3f(0.f, 0.f, 0.f));    

    std::string dir = sourceDir3();
    simShader.compile(slurp3(dir + "/rd_sim.vert.glsl"),
                      slurp3(dir + "/rd_sim.frag.glsl"));
    dispShader.compile(slurp3(dir + "/rd_display.vert.glsl"),
                       slurp3(dir + "/rd_display.frag.glsl"));

    for (auto* t : {&texA, &texB}) {
      t->filter(Texture::LINEAR);
      t->wrap(Texture::REPEAT);
      t->mipmap(false);
      t->create2D(SIM_W, SIM_H, Texture::RGBA32F, Texture::RGBA, Texture::FLOAT);
    }

    glGenFramebuffers(1, &fboA);
    glBindFramebuffer(GL_FRAMEBUFFER, fboA);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, texA.id(), 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &fboB);
    glBindFramebuffer(GL_FRAMEBUFFER, fboB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, texB.id(), 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenVertexArrays(1, &emptyVAO);

    gridMesh.primitive(Mesh::TRIANGLES);
    for (int i = 0; i < SPHERE_LAT * SPHERE_LON * 6; i++) gridMesh.vertex(0, 0, 0);

    initSim(0);
  }

  void onAnimate(double dt_) override {
    if (isPrimary()) {
      // ── Auto-reset ─────────────────────────────────────────────────────
      // Modify AUTO_RESET_INTERVAL at the top of the file to change timing.
      resetTimer += dt_;
      if (resetTimer >= AUTO_RESET_INTERVAL) {
        resetTimer = 0.0;
        state().resetCount++;
        initSim(state().resetCount);
        p_palette = (p_palette + 1) % int(palettes.size());
      }

      // ── Orbit sound source (Lissajous) ─────────────────────────────────
      mSoundElapsedTime += dt_;
      float tta = float(mSoundElapsedTime * p_panSpeed * 2.0 * M_PI);
      srcPos = Vec3f(6.f * std::cos(tta),
                     5.f * std::sin(2.8f * tta),
                     6.f * std::sin(tta));

      // ── Drum onset → palette trigger ───────────────────────────────────
      while (nextOnsetIdx < drumOnsets.size() &&
             playbackSec >= drumOnsets[nextOnsetIdx]) {
        p_palette = (p_palette + 1) % int(palettes.size());
        nextOnsetIdx++;
      }

      state().camera       = nav();
      state().paletteIndex = p_palette;
      state().dA           = p_dA;
      state().dB           = p_dB;
      state().feed         = p_feed;
      state().k            = p_k;
      state().simDt        = p_simDt;
      state().dispScale    = p_dispScale;
    } else {
      nav().set(state().camera);

      if (state().resetCount != lastResetCount) {
        lastResetCount = state().resetCount;
        initSim(lastResetCount);
      }
    }

    // ── Smooth camera travel in → out → repeat ───────────────────────────
    // CAMERA_SPEED: units/second.  CAMERA_START: starting distance on X axis.
    // Phase 0–30 s: travel inward to center.
    // Phase 30–58 s: travel outward back to start.
    // Resets and loops every ~58 seconds.
    static const float CAMERA_SPEED = 5.0f;
    static const float CAMERA_START = 140.0f;
    static const float PHASE_IN     = 30.0f;          // seconds inward
    static const float PHASE_CYCLE  = PHASE_IN + (CAMERA_START / CAMERA_SPEED); // ~58 s

    countingTime += float(dt_);
    if (countingTime >= PHASE_CYCLE) {
      countingTime = 0.f;
      nav().pos(-CAMERA_START, 0.f, 0.f);
      nav().faceToward(Vec3f(0.f, 0.f, 0.f));
    }

    Vec3f pos  = nav().pos();
    float dist = pos.mag();

    if (countingTime < PHASE_IN) {
      // Move inward toward center at constant speed.
      if (dist > 0.01f) {
        float move = std::min(CAMERA_SPEED * float(dt_), dist);
        nav().pos(pos * (1.f - move / dist));
      } else {
        nav().pos(0.f, 0.f, 0.f);
      }
    } else {
      // Move outward back to starting position, capped at CAMERA_START.
      if (dist < CAMERA_START - 0.01f) {
        float move = std::min(CAMERA_SPEED * float(dt_), CAMERA_START - dist);
        nav().pos(dist > 0.01f ? pos * ((dist + move) / dist)
                               : Vec3f(-move, 0.f, 0.f));
      }
    }
  }

  void onDraw(Graphics& g) override {
    float dA        = state().dA;
    float dB        = state().dB;
    float feed      = state().feed;
    float k         = state().k;
    float dt        = state().simDt;
    float dispScale = state().dispScale;

    // ── Save GL state ────────────────────────────────────────────────────
    GLint prevFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    // ── Ping-pong simulation ─────────────────────────────────────────────
    glViewport(0, 0, SIM_W, SIM_H);
    for (int i = 0; i < SIM_STEPS; i++) {
      Texture& src    = pingA ? texA : texB;
      GLuint   dstFBO = pingA ? fboB : fboA;

      glBindFramebuffer(GL_FRAMEBUFFER, dstFBO);
      simShader.begin();
      src.bind(0);
      simShader.uniform("u_texture",    0);
      simShader.uniform("u_resolution", float(SIM_W), float(SIM_H));
      simShader.uniform("u_dA",   dA);
      simShader.uniform("u_dB",   dB);
      simShader.uniform("u_feed", feed);
      simShader.uniform("u_k",    k);
      simShader.uniform("u_dt",   dt);
      glBindVertexArray(emptyVAO);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      glBindVertexArray(0);
      src.unbind(0);
      simShader.end();

      pingA = !pingA;
    }

    // ── Restore window framebuffer ───────────────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
    glViewport(vp[0], vp[1], vp[2], vp[3]);

    // ── Display pass ─────────────────────────────────────────────────────
    g.clear(0.f, 0.f, 0.f);
    g.depthTesting(true);
    g.blending(false);
    glDisable(GL_CULL_FACE);

    Texture&       rdTex = pingA ? texA : texB;
    const Palette& pal   = palettes[state().paletteIndex];

    rdTex.bind(0);
    g.shader(dispShader);
    g.shader().uniform("u_texture",   0);
    g.shader().uniform("u_colorA",    pal.a);
    g.shader().uniform("u_colorB",    pal.b);
    g.shader().uniform("u_colorBg",   pal.bg);
    g.shader().uniform("u_dispScale", dispScale);
    g.draw(gridMesh);
    rdTex.unbind(0);
  }

  bool onKeyDown(const Keyboard& kb) override {
    if (!isPrimary()) return false;

    if (kb.key() == ' ') {
      resetTimer = 0.0;  // restart the auto-reset countdown
      state().resetCount++;
      initSim(state().resetCount);
      return true;
    }
    if (kb.key() == 'c' || kb.key() == 'C') {
      p_palette = (p_palette + 1) % int(palettes.size());
      return true;
    }
    return false;
  }
};

int main() {
  MyApp app;
  app.configureAudio(44100, 512, 2, 0);
  gam::sampleRate(44100);
  app.start();
}
