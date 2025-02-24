#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <opencv2/opencv.hpp>
#include "/home/declan/RPI/zwo/dependencies/zwo-asi-sdk/1.36/linux_sdk/include/ASICamera2.h"

using namespace std;
using namespace cv;

int main(int argc, char *argv[]) {
    cout << "Starting ZWO ASICamera exposure..." << endl;

    int connected_cameras = ASIGetNumOfConnectedCameras();
    if (connected_cameras < 1) {
        cerr << "No cameras connected!" << endl;
        return 1;
    }

    ASI_CAMERA_INFO camera_info;
    if (ASIGetCameraProperty(&camera_info, 0) != ASI_SUCCESS) {
        cerr << "Error retrieving camera properties!" << endl;
        return 1;
    }

    int width = camera_info.MaxWidth;
    int height = camera_info.MaxHeight;
    int bytes_per_pixel = (camera_info.BitDepth > 8) ? 2 : 1;
    int image_size = width * height * bytes_per_pixel;

    if (ASIOpenCamera(camera_info.CameraID) != ASI_SUCCESS || ASIInitCamera(camera_info.CameraID) != ASI_SUCCESS) {
        cerr << "Error initializing camera" << endl;
        return 1;
    }

    double exposure_seconds = (argc > 1) ? stod(argv[1]) : 20;
    long exposure_time = exposure_seconds * 1000000;
    if (ASISetControlValue(camera_info.CameraID, ASI_EXPOSURE, exposure_time, ASI_FALSE) != ASI_SUCCESS) {
        cerr << "Error setting exposure time" << endl;
        return 1;
    }

    if (ASIStartExposure(camera_info.CameraID, ASI_FALSE) != ASI_SUCCESS) {
        cerr << "Error starting exposure" << endl;
        return 1;
    }

    ASI_EXPOSURE_STATUS exp_status;
    do {
        ASIGetExpStatus(camera_info.CameraID, &exp_status);
    } while (exp_status == ASI_EXP_WORKING);
    
    if (exp_status != ASI_EXP_SUCCESS) {
        cerr << "Exposure failed" << endl;
        return 1;
    }
    
    vector<unsigned char> asi_image(image_size);
    if (ASIGetDataAfterExp(camera_info.CameraID, asi_image.data(), image_size) != ASI_SUCCESS) {
        cerr << "Error retrieving image data" << endl;
        return 1;
    }

    // Convert to OpenCV Mat
    Mat image;
    if (bytes_per_pixel == 2) {
        image = Mat(height, width, CV_16UC1, asi_image.data());
    } else {
        image = Mat(height, width, CV_8UC1, asi_image.data());
    }

    // Save with timestamp
    time_t now = time(0);
    tm *ltm = localtime(&now);
    stringstream filename;
    filename << "exposure-" << 1900 + ltm->tm_year << setfill('0') << setw(2) << 1 + ltm->tm_mon
             << setw(2) << ltm->tm_mday << "-" << setw(2) << ltm->tm_hour << setw(2) << ltm->tm_min
             << setw(2) << ltm->tm_sec << ".png";

    if (!imwrite(filename.str(), image)) {
        cerr << "Error saving image" << endl;
        return 1;
    }

    cout << "Exposure image saved to " << filename.str() << endl;
    return 0;
}