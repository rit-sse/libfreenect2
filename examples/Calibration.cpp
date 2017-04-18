#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include "Calibration.h"
#include "viewer.h"

static float give = 40; // The amount of give before we detect a wall
static int variance = 4; // How big of a square to grab 4 is an 8x8

Calibrator::Calibrator(bool showViewer) {
  if (showViewer) {
    viewer.initialize();
  }
}

bool Calibrator::calibrated() {
  return top_wall && bottom_wall && left_wall && right_wall;
}

void Calibrator::getCenterValue(libfreenect2::Frame *depth) {
  float pre_avg = 0;
  float average = 0;
  int counter = 0;
  int missed = 0;

  center_of_box = (((depth->width * depth->height) / 2) + (depth->width / 2));

  for(size_t x = (center_of_box - variance); x < (center_of_box + variance); x++) {
    for(size_t y = (x - (depth->width * variance)); y < (x + (depth->width * variance)); y+= depth->width) {
      counter++;
      if(frame_data[y] != 0) {
        pre_avg += frame_data[y];
      } else {
        missed ++;
      }
    }
  }

  average = (pre_avg / (counter - missed)); // Average Center Value

  center_value = average - give;
}

void Calibrator::calibrate(libfreenect2::Frame *depth) {
  frame_data = (float*)depth->data;

  getCenterValue(depth); // Sets center_value and center_of_box

  // Find left wall
  for (size_t i = center_of_box; i > (center_of_box - (depth->width / 2)); i--) {
    if(frame_data[i] != 0 && frame_data[i] < center_value) {
      left_wall = (depth->width / 2) + 1 - (center_of_box - i); // Always add one because its square wall_height = frame_data[i];
      wall_height = frame_data[i];
      break;
    }
  }

  // Find right wall
  for (size_t i = center_of_box; i < ((center_of_box + (depth->width / 2)) - 1); i++) { // we are at middle right so subtract 1
    if(frame_data[i] != 0 && frame_data[i] < center_value) {
      right_wall = (depth->width / 2) + 1 + (i - center_of_box); // Always add one because its square
      if (frame_data[i] > wall_height) wall_height = frame_data[i];
      break;
    }
  }

  // Find top wall
  // Top is the bottom but that makes me confused so this is the top wall
  for (size_t i = center_of_box; i > (center_of_box - (depth->width * (depth->height / 2))); i-= depth->width) {
    if(frame_data[i] != 0 && frame_data[i] < center_value) {
      top_wall = (depth->height / 2) - ((center_of_box - i) / depth->width);
      if (frame_data[i] > wall_height) wall_height = frame_data[i];
      break;
    }
  }

  // Find bottom wall
  for (size_t i = center_of_box; i < (center_of_box + (depth->width * ((depth->height / 2) - 1))); i+= depth->width) {
    if(frame_data[i] != 0 && frame_data[i] < center_value) {
      bottom_wall = (depth->height / 2) + 1 + ((i - center_of_box) / depth->width); // Magical + 1
      if (frame_data[i] > wall_height) wall_height = frame_data[i];
      break;
    }
  }

  printf("Center Value %f\n", center_value);
  printf("L: %zu R: %zu T: %zu B: %zu\n", left_wall, right_wall, top_wall, bottom_wall);
}

bool Calibrator::showCalibrationSquare(libfreenect2::Frame *depth) {
  calibrate(depth);
  for (size_t i = ((top_wall + 1) * depth->width); i < (bottom_wall * depth->width); i+= depth->width) {
    for (size_t j = left_wall + 1; j < right_wall; j++) {
      frame_data[i + j] = 100;
    }
  }

  viewer.addFrame("depth", depth);
  // viewer.addFrame("RGB", &boxFrame);
  // viewer.addFrame("ir", ir);
  // viewer.addFrame("registered", &registered);

  return false || viewer.render();
}

void Calibrator::getBoxesDepthMap(BoxFrame* boxframe, libfreenect2::Frame *depth) {
  frame_data = (float *)depth->data;

  float *data = (float *)malloc(4 * (right_wall - left_wall) * (bottom_wall - top_wall));

  for (size_t i = ((top_wall + 1) * depth->width); i < (bottom_wall * depth->width); i+= depth->width) {
    for (size_t j = left_wall + 1; j < right_wall; j++) {
      *data = frame_data[i + j];
      data++;
    }
  }

  boxframe->initialize((right_wall - left_wall), (bottom_wall - top_wall), data);
}

BoxFrame::BoxFrame(){
	width = 0;
	height = 0;
}

BoxFrame::BoxFrame(size_t w, size_t h, float *d) {
  width = w;
  height = h;
  data = d;
}

void BoxFrame::initialize(size_t w, size_t h, float* d) {
	this->width = w;
	this->height = h;
	this->data = d;
}
