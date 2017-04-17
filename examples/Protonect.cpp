/*
 * This file is part of the OpenKinect Project. http://www.openkinect.org
 *
 * Copyright (c) 2011 individual OpenKinect contributors. See the CONTRIB file
 * for details.  This code is licensed to you under the terms of the Apache License, version
 * 2.0, or, at your option, the terms of the GNU General Public License,
 * version 2.0. See the APACHE20 and GPL2 files for the text of the licenses,
 * or the following URLs:
 * http://www.apache.org/licenses/LICENSE-2.0 http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * If you redistribute this file in source form, modified or unmodified, you
 * may:
 *   1) Leave this header intact and distribute it under the same terms,
 *      accompanying it with the APACHE20 and GPL20 files, or
 *   2) Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
 *   3) Delete the GPL v2 clause and accompany it with the APACHE20 file
 * In all cases you must keep the copyright notice intact and include a copy
 * of the CONTRIB file.
 *
 * Binary distributions must follow the binary distribution requirements of
 * either License.
 */

#include <iostream>
#include <cstdlib>
#include <signal.h>
#include <string.h>

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>
#include "viewer.h"

// Whether the running application should shut down.
bool protonect_shutdown = false;

void sigint_handler(int s) {
  protonect_shutdown = true;
}

bool protonect_paused = false;
libfreenect2::Freenect2Device *devtopause;

//Doing non-trivial things in signal handler is bad. If you want to pause,
//do it in another thread.
//Though libusb operations are generally thread safe, I cannot guarantee
//everything above is thread safe when calling start()/stop() while
//waitForNewFrame().
void sigusr1_handler(int s) {
  if (devtopause == 0)
    return;
  if (protonect_paused)
    devtopause->start();
  else
    devtopause->stop();
  protonect_paused = !protonect_paused;
}

// Custom logger
#include <fstream>
#include <cstdlib>
class MyFileLogger: public libfreenect2::Logger {
  private:
    std::ofstream logfile_;
  public:
    MyFileLogger(const char *filename) {
      if (filename)
        logfile_.open(filename);
      level_ = Debug;
    }
    bool good() {
      return logfile_.is_open() && logfile_.good();
    }
    virtual void log(Level level, const std::string &message) {
      logfile_ << "[" << libfreenect2::Logger::level2str(level) << "] " << message << std::endl;
    }
};

int main(int argc, char *argv[]) {
  std::string program_path(argv[0]);
  std::cerr << "Version: " << LIBFREENECT2_VERSION << std::endl;
  std::cerr << "Environment variables: LOGFILE=<protonect.log>" << std::endl;
  std::cerr << "Usage: " << program_path << " [-gpu=<id>] [gl | cl | clkde | cuda | cudakde | cpu] [<device serial>]" << std::endl;
  std::cerr << "        [-noviewer] [-norgb | -nodepth] [-help] [-version]" << std::endl;
  std::cerr << "        [-frames <number of frames to process>]" << std::endl;
  std::cerr << "To pause and unpause: pkill -USR1 Protonect" << std::endl;
  size_t executable_name_idx = program_path.rfind("Protonect");

  std::string binpath = "/";

  if(executable_name_idx != std::string::npos) {
    binpath = program_path.substr(0, executable_name_idx);
  }

  // This code sets up a logger
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
  // avoid flooing the very slow Windows console with debug messages
  libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Info));
#else
  // create a console logger with debug level (default is console logger with info level)
  libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Debug));
#endif
  MyFileLogger *filelogger = new MyFileLogger(getenv("LOGFILE"));
  if (filelogger->good())
    libfreenect2::setGlobalLogger(filelogger);
  else
    delete filelogger;

  libfreenect2::Freenect2 freenect2;
  libfreenect2::Freenect2Device *dev = 0;
  libfreenect2::PacketPipeline *pipeline = new libfreenect2::OpenGLPacketPipeline();  // Use OpenGL

  std::string serial = "";

  // Find the device
  // Some time dosn't work the frist time
  if(freenect2.enumerateDevices() == 0) {
    std::cout << "no device connected!" << std::endl;
    return -1;
  }
  if (serial == "") {
    serial = freenect2.getDefaultDeviceSerialNumber();
  }

  // Open the device
  dev = freenect2.openDevice(serial, pipeline);

  // Check to make sure it opened correctly
  if(dev == 0) {
    std::cout << "failure opening device!" << std::endl;
    return -1;
  }

  // Code was written for more than one device so this is here
  devtopause = dev;

  signal(SIGINT,sigint_handler);
#ifdef SIGUSR1
  signal(SIGUSR1, sigusr1_handler);
#endif
  protonect_shutdown = false;


  // Set up frame listeners
  int types = libfreenect2::Frame::Color | libfreenect2::Frame::Ir | libfreenect2::Frame::Depth;
  libfreenect2::SyncMultiFrameListener listener(types);
  libfreenect2::FrameMap frames;

  // Attach them
  dev->setColorFrameListener(&listener);
  dev->setIrAndDepthFrameListener(&listener);

  // Start getting frames
  if (!dev->start()) return -1;

  std::cout << "device serial: " << dev->getSerialNumber() << std::endl;
  std::cout << "device firmware: " << dev->getFirmwareVersion() << std::endl;

  Viewer viewer;
  viewer.initialize();

  // Loop while we should run and display frames
  while(!protonect_shutdown) {
    if (!listener.waitForNewFrame(frames, 10*1000)) {
      std::cout << "timeout!" << std::endl;
      return -1;
    }

    // Frames
    libfreenect2::Frame *rgb = frames[libfreenect2::Frame::Color];
    libfreenect2::Frame *ir = frames[libfreenect2::Frame::Ir];
    libfreenect2::Frame *depth = frames[libfreenect2::Frame::Depth];

    // The amount of liway
    float give = 40;
    size_t center_of_box = (depth->width * depth->height) / 2;

    size_t left_wall_cord = 0;
    size_t right_wall_cord = 0;
    size_t top_wall_cord = 0;
    size_t bottom_wall_cord = 0;
    
    float* frame_data = (float*)depth->data;

    float pre_avg = 0;
    int missed = 0;
    int variance = 4;
    int counter = 0;
    for(int x = (center_of_box - variance); x < (center_of_box + variance); x++){
      for(int y = (x - (depth->width * variance)); y < (x + (depth->width * variance)); y+= depth->width)
      {
        counter++;
        if(frame_data[y] != 0){
          //printf("%f \n", frame_data[y]);
          pre_avg += frame_data[y];
        }
        else
        {
          missed ++;
        }
      }
    }
    //printf("counter = %d \n", counter);
    //printf("missed: %d \n", missed);
    float center_value_no_give = (pre_avg / (counter - missed));
    float center_value = center_value_no_give + give;
    float left_wall_value = 0;
    float right_wall_value = 0;

    // Find left wall
    for (int i = center_of_box; i > (center_of_box - (depth->width / 2) + 1); i--) {
      if(frame_data[i] == 0)
      {
        //printf("skip frame...");
        continue;
      }
      if(frame_data[i] > center_value) {
        left_wall_cord = center_of_box - (center_of_box - i);
        left_wall_value = frame_data[i];
        //printf("left wall found, value edited \n");
        break;
      } 
    }
    // Find right wall
    for (int i = center_of_box; i < (center_of_box + (depth->width / 2) - 1); i++) {
      if(frame_data[i] == 0)
      {
        //printf("skip frame...");
        continue;
      }
      if( frame_data[i] > center_value) {
        right_wall_cord = center_of_box + (i - center_of_box); 
        right_wall_value = frame_data[i];
        //printf("right wall found, value edited \n");
        break;
      }
    }


    /*
    // Find top wall
    for (size_t i = center_of_box; i < (center_of_box - (depth->height / 2)); i-=depth->width) {
      if( depth->data[i] > center_high) {
        top_wall = (center_of_box - i) / depth->width;
        break;
      }
    }
    // Find bottom wall
    for (size_t i = center_of_box; i < (center_of_box + (depth->height / 2)); i+=depth->width) {
      if( depth->data[i] > center_high) {
        bottom_wall = (i + center_of_box) / depth->width;
        break;
      }
    }
    */

    if (left_wall_value != 0){
      //printf("Width: %zu Height: %zu \n", depth->width, depth->height);
      printf("Center_value: %f Left_value: %f Right_value: %f\n", center_value_no_give, left_wall_value, right_wall_value);
      printf("Center_cord: %zu, Left_cord: %zu, right_cord: %zu \n\n\n", center_of_box, left_wall_cord, right_wall_cord);
    }

    viewer.addFrame("depth", depth);
    // viewer.addFrame("RGB", &boxFrame);
    // viewer.addFrame("ir", ir);
    // viewer.addFrame("registered", &registered);

    protonect_shutdown = protonect_shutdown || viewer.render();

    // Call this when we are done with the frame
    listener.release(frames);
  }

  dev->stop();
  dev->close();

  return 0;
}
