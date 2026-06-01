#include "al/app/al_App.hpp"
#include "al/sound/al_SoundFile.hpp"
#include "al/io/al_File.hpp"

using namespace al;

struct MyApp : App {
  SoundFilePlayerTS player;
  std::vector<float> buffer;

  SearchPaths searchPaths;

  void onInit() override {
    searchPaths.addAppPaths();
    searchPaths.addRelativePath("../data");

    std::string soundPath =
        searchPaths.find("count.wav").filepath();

    if (!player.open(soundPath.c_str())) {
      std::cout << "Could not open file: "
                << soundPath << std::endl;
      quit();
      return;
    }

    player.setPlay();
  }

  void onSound(AudioIOData &io) override {
    int frames = io.framesPerBuffer();
    int channels = player.soundFile.channels;

    buffer.resize(frames * channels);

    player.getFrames(
        frames,
        buffer.data(),
        buffer.size());

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