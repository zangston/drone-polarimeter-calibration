#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <CCfits/CCfits>
#include "/home/declan/RPI/zwo/dependencies/zwo-asi-sdk/1.36/linux_sdk/include/ASICamera2.h"

using namespace std;
using namespace CCfits;

int main(int argc, char *argv[]) {
    cout << "ZWO ASICamera FITS save test" << endl;

    long image_size = 0;
    int bytes_per_pixel = 2; // 14-bit data stored in 16-bit container

    // Get camera info
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
    image_size = width * height * bytes_per_pixel;

    // Open and initialize the camera
    if (ASIOpenCamera(camera_info.CameraID) != ASI_SUCCESS || ASIInitCamera(camera_info.CameraID) != ASI_SUCCESS) {
        cerr << "Error initializing camera" << endl;
        return 1;
    }

    // Set exposure time
    double exposure_seconds = (argc > 1) ? stod(argv[1]) : 20;
    long exposure_time = exposure_seconds * 1000000;
    ASISetControlValue(camera_info.CameraID, ASI_EXPOSURE, exposure_time, ASI_FALSE);

    // Start exposure
    if (ASIStartExposure(camera_info.CameraID, ASI_FALSE) != ASI_SUCCESS) {
        cerr << "Error starting exposure" << endl;
        return 1;
    }

    // Wait for exposure to complete
    ASI_EXPOSURE_STATUS asi_exp_status;
    do {
        ASIGetExpStatus(camera_info.CameraID, &asi_exp_status);
    } while (asi_exp_status == ASI_EXP_WORKING);

    if (asi_exp_status != ASI_EXP_SUCCESS) {
        cerr << "Exposure failed" << endl;
        return 1;
    }

    // Retrieve image data
    vector<unsigned char> asi_image(image_size);
    if (ASIGetDataAfterExp(camera_info.CameraID, asi_image.data(), image_size) != ASI_SUCCESS) {
        cerr << "Error reading exposure data" << endl;
        return 1;
    }

    // Convert 8-bit packed data into 16-bit array
    vector<unsigned short> fits_image(width * height);
    for (size_t i = 0; i < fits_image.size(); ++i) {
        fits_image[i] = (static_cast<unsigned short>(asi_image[2 * i]) << 8) |
                        (static_cast<unsigned short>(asi_image[2 * i + 1]));
    }

    // Generate filename with timestamp
    time_t now = time(0);
    tm *ltm = localtime(&now);
    stringstream filename;
    filename << "image_data-"
             << 1900 + ltm->tm_year << "_"
             << setw(2) << setfill('0') << 1 + ltm->tm_mon << "_"
             << setw(2) << setfill('0') << ltm->tm_mday << "_"
             << setw(2) << setfill('0') << ltm->tm_hour
             << setw(2) << setfill('0') << ltm->tm_min
             << setw(2) << setfill('0') << ltm->tm_sec
             << "-" << exposure_seconds << "s"
             << ".fits";

    // Save data to FITS file
    try {
        auto pFits = make_unique<FITS>(filename.str(), Write);
        long naxes[2] = { width, height };
        pFits->addImage("IMAGE", SHORT_IMG, 2, naxes);
        pFits->pHDU().write(1, fits_image.size(), fits_image);
        cout << "Saved FITS file: " << filename.str() << endl;
    } catch (FITS::CantCreate&) {
        cerr << "Error: Cannot create FITS file" << endl;
        return 1;
    }

    // Close camera
    ASICloseCamera(camera_info.CameraID);
    return 0;
}