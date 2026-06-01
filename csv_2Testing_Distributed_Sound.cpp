#include "al/app/al_App.hpp"
#include "al/sound/al_SoundFile.hpp"

using namespace al;

struct MyApp : App {
  SoundFilePlayerTS player;
  std::vector<float> buffer;

  void onInit() override {
    if (!player.open("data/count.wav")) {
      std::cout << "Could not open file\n";
      quit();
      return;
    }

    player.setPlay();
  }

  void onSound(AudioIOData& io) override {
    int frames = io.framesPerBuffer();
    int channels = player.soundFile.channels;

    buffer.resize(frames * channels);

    player.getFrames(frames, buffer.data(), buffer.size());

    while (io()) {
      int i = io.frame() * channels;

      io.out(0) = buffer[i];

      if (channels > 1)
        io.out(1) = buffer[i + 1];
      else
        io.out(1) = buffer[i];
    }
  }
};

int main() {
  MyApp app;
  app.configureAudio(44100, 512, 2, 0);
  app.start();
}