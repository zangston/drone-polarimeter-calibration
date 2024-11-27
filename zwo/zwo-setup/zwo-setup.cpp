/*******************************************************************
 * Code adapted into C++ from zwo-asi-c-example,
 * originally written by Github user vrruiz
 * 
 * Date accessed: 27 November 2024
 * Commit hash: 5eb5cd5
 * Committed 30 December 2019
 * 
 * Availability: https://github.com/vrruiz/zwo-asi-c-example
 ******************************************************************/

# include <iostream>

# include <memory>
# include <vector>

# include "../zwo-asi-sdk/1.36/linux_sdk/include/ASICamera2.h"

using namespace std;

int main() {
    cout << "ZWO ASICamera test" << endl;
    
    long image_size = 0;
    int image_bytes = 1;

    // Read camera info, exit if no cameras are connected
    int connected_cameras = ASIGetNumOfConnectedCameras();
    cout << connected_cameras << endl;
    if (connected_cameras < 1) {
        cerr << "No cameras connected!" << endl;
        return 0;
    }

    // Get connected camera's properties
    ASI_CAMERA_INFO camera_info;
    if (ASIGetCameraProperty(&camera_info, 0) != ASI_SUCCESS) {
        cerr << "Error retrieving camera properties!" << endl;
        return 1;
    }

    // Display camera properties
    cout << "Camera properties: " << endl;
    cout << "Name: " << camera_info.Name << endl;
    cout << "Camera ID: " << camera_info.CameraID << endl;
    cout << "Width & Height: " << camera_info.MaxWidth << " x " << camera_info.MaxHeight << endl;
    cout << "Color: " << (camera_info.IsColorCam == ASI_TRUE ? "Yes" : "No") << endl;
    cout << "  Bayer pattern: " << camera_info.BayerPattern << endl;
    cout << "  Pixel size: " << camera_info.PixelSize << " microns" << endl;
    cout << "  e-/ADU: " << camera_info.ElecPerADU << endl;
    cout << "  Bit depth: " << camera_info.BitDepth << endl;
    cout << "  Trigger cam: " << (camera_info.IsTriggerCam ? "Yes" : "No") << endl;

    // Calculate image size


    // Close camera
    cout << "Closing camera" << endl;
    ASICloseCamera(camera_info.CameraID);

    return 0;
}