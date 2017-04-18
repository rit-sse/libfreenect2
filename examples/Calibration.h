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

class Calibrator {
  Viewer viewer;
  bool showViewer;

  size_t center_of_box;
  float center_value;

  size_t left_wall;
  size_t right_wall;
  size_t top_wall;
  size_t bottom_wall;

  float wall_height;
  float *frame_data;

  private:
    void getCenterValue(libfreenect2::Frame *depth);

  public:
    Calibrator(bool showViewer);
    bool calibrated();
    void calibrate(libfreenect2::Frame *depth);
    bool showCalibrationSquare(libfreenect2::Frame *depth);
    void getBoxesDepthMap(BoxFrame* boxframe, libfreenect2::Frame *depth);
};
#endif
