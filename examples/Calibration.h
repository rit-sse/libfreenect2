#ifndef calibration
#define calibration

#include <libfreenect2/libfreenect2.hpp>
#include "viewer.h"
#include <cstdlib>

class BoxFrame {
  public:
    BoxFrame();
    BoxFrame(size_t width, size_t height, float *data);
    size_t width;
    size_t height;
    float *data;
    void initialize(size_t width, size_t height, float* data);
};

class RedSquare {
  unsigned int *frame_data;

  size_t leftWall;
  size_t rightWall;
  size_t topWall;
  size_t bottomWall;

  static const size_t width = 1920;
  static const size_t height = 1080;

  public:
  size_t leftPos;
  size_t rightPos;
  size_t topPos;
  size_t bottomPos;
  void draw();
  void drawSquare();
  bool moveLeft(libfreenect2::Frame *color);
  bool moveRight(libfreenect2::Frame *color);
  bool moveUp(libfreenect2::Frame *color);
  bool moveDown(libfreenect2::Frame *color);
  RedSquare(size_t l, size_t r, size_t t, size_t b, unsigned int *fd);
};

class Calibrator {
  Viewer viewer;
  bool showViewer;

  size_t center_of_box;
  float center_value;

  float wall_height;
  float *frame_data;

  private:
  void getCenterValue(libfreenect2::Frame *depth);

  public:
  Calibrator(bool showViewer);
  size_t left_wall;
  size_t right_wall;
  size_t top_wall;
  size_t bottom_wall;
  bool calibrated();
  void calibrate(libfreenect2::Frame *depth);
  bool showCalibrationSquare(libfreenect2::Frame *depth);
  void getBoxesDepthMap(BoxFrame* boxframe, libfreenect2::Frame *depth);
};
#endif
