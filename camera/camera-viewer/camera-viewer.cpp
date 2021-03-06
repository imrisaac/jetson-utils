/*
 * Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "gstCamera.h"
#include "glDisplay.h"
#include "commandLine.h"

#include <opencv2/core/utility.hpp>
#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/video/video.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <signal.h>


bool signal_recieved = false;

void sig_handler(int signo)
{
	if( signo == SIGINT )
	{
		printf("received SIGINT\n");
		signal_recieved = true;
	}
}


int main( int argc, char** argv ){
	commandLine cmdLine(argc, argv);

  cv::Mat camera_frame;
  cv::Mat camera_frame_BGR;
  cv::Mat Temp;
	
	/*
	 * attach signal handler
	 */	
	if( signal(SIGINT, sig_handler) == SIG_ERR )
		printf("\ncan't catch SIGINT\n");

	/*
	 * create the camera device
	 */
	gstCamera* camera = gstCamera::Create(1920,
								   1080,
								   cmdLine.GetString("camera"));

    cv::VideoWriter writer;

  std::string gstSink = "appsrc ! " 
                        "videoconvert ! "
                        "omxh264enc "
                          "preset-level=1 "
                          "control_rate=2 "
                          "insert-vui=true "
                          "bitrate=2500000 ! "
                        "mpegtsmux "
                          "alignment=7 ! "
                        "udpsink "
                          "host=192.168.0.255 "
                          "port=49510 "
                          "sync=false "
                          "async=false "
                          "close-socket=false "; // 300ms

	if( !camera ){
		printf("\ncamera-viewer:  failed to initialize camera device\n");
		return 0;
	}
	
	printf("\ncamera-viewer:  successfully initialized camera device (%ux%u)\n", camera->GetWidth(), camera->GetHeight());
	
	
	/*
	 * create openGL window
	 */
	glDisplay* display = NULL;
	
	if( !display )
		printf("camera-viewer:  failed to create openGL display\n");
	
	/*
	 * start streaming
	 */
	if( !camera->Open() )
	{
		printf("camera-viewer:  failed to open camera for streaming\n");
		return 0;
	}
	
	printf("camera-viewer:  camera open for streaming\n");
	
  int imgWidth = camera->GetWidth();
  int imgHeight = camera->GetHeight();

  writer.open(gstSink, 0, (double)120, cv::Size(imgWidth, imgHeight), true);
  // capture latest image
  uchar3* imgRGBA = NULL;

	/*
	 * processing loop
	 */
	while( !signal_recieved )
	{

    // Capture RGBA gives us 4 channel 8 bits per pixel
		if( !camera->CaptureRGBA(&imgRGBA, 2000, true) ){
			printf("camera-viewer:  failed to capture RGBA image\n");
    }else{

      cudaDeviceSynchronize();
      camera_frame = cv::Mat((int)camera->GetHeight(), (int)camera->GetWidth(), CV_8UC3, imgRGBA);

      // // This is the most time consuming step converting from RGBA 32 bit float to BGR 8 bit int
      // camera_frame.convertTo(Temp, CV_8UC4);

      // cv::cvtColor(Temp, camera_frame_BGR, cv::COLOR_RGBA2BGR);

      // printf("frame size %d %d\n", camera_frame.cols, camera_frame.rows);

      // if(camera_frame.cols != 0){
      //   cv::imshow("Converted",camera_frame);
      //   cv::waitKey(33);
      // }

      if(camera_frame.cols != 0 && camera_frame.rows != 0){
          writer << camera_frame;
        }

      // update display
      if( display != NULL )
      {
        //display->RenderOnce((float*)imgRGBA, camera->GetWidth(), camera->GetHeight());
        
        // update status bar
        char str[256];
        sprintf(str, "Camera Viewer (%ux%u) | %.0f FPS", camera->GetWidth(), camera->GetHeight(), display->GetFPS());
        display->SetTitle(str);	

        // check if the user quit
        if( display->IsClosed() )
          signal_recieved = true;
      }
    }
	}
	

	/*
	 * destroy resources
	 */
	printf("\ncamera-viewer:  shutting down...\n");
	
	SAFE_DELETE(camera);
	SAFE_DELETE(display);

	printf("camera-viewer:  shutdown complete.\n");
	return 0;
}
