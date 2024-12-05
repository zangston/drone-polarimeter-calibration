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
# include <fstream>
# include <iomanip>

# include <memory>
# include <vector>

# include "../zwo-asi-sdk/1.36/linux_sdk/include/ASICamera2.h"

using namespace std;

int main() {
    cout << "ZWO ASICamera test" << endl;
    
    long image_size = 0;
    int bytes_per_pixel = 1;

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

    // Calculate image size in bytes
    int image_dimensions = camera_info.MaxWidth * camera_info.MaxHeight;
    // Correct for monochromatic vs color image
    if (camera_info.IsColorCam == ASI_FALSE)
        bytes_per_pixel = (camera_info.BitDepth > 8) ? 2 : 1;
    else
        bytes_per_pixel = 3;
    image_size = image_dimensions * bytes_per_pixel;
    cout << "Image size: " << image_size << " bytes" << endl;

    // Open and intialize camera
    cout << "Opening camera" << endl;
    if (ASIOpenCamera(camera_info.CameraID) != ASI_SUCCESS) {
        cerr << "Error opening camera" << endl;
        return 1;
    }
    cout << "Initalizing camera" << endl;
    if (ASIInitCamera(camera_info.CameraID) != ASI_SUCCESS) {
        cerr << "Error initializing camera" << endl;
    }

    // Get camera controls
    int asi_num_controls = 0;
    if (ASIGetNumOfControls(camera_info.CameraID, &asi_num_controls) != ASI_SUCCESS) {
        cerr << "Error getting number of controls.\n";
        return 1;
    }
    for (int i = 0; i < asi_num_controls; ++i) {
        ASI_CONTROL_CAPS control_caps;
        if (ASIGetControlCaps(camera_info.CameraID, i, &control_caps) == ASI_SUCCESS) {
            cout << "  Property " << control_caps.Name << ": [" 
                << control_caps.MinValue << ", " << control_caps.MaxValue 
                << "] = " << control_caps.DefaultValue
                << (control_caps.IsWritable ? " (set)" : "") 
                << " - " << control_caps.Description << "\n";
        }
    }

    // Exposure setup
    cout << "Starting exposure" << endl;
    ASI_EXPOSURE_STATUS asi_exp_status;
    ASIGetExpStatus(camera_info.CameraID, &asi_exp_status);
    if (asi_exp_status != ASI_EXP_IDLE) {
        cerr << "Cannot start exposure if camera is not idle. Aborting..." << endl;
        return 1;
    }

    // Set exposure time
    int seconds = 20;
    long exposure_time = seconds * 1000000; // Number of seconds * ms per second
    cout << "Set exposure time: " << exposure_time / 1000000 << " seconds" << endl;
    if (ASISetControlValue(camera_info.CameraID, ASI_EXPOSURE, exposure_time, ASI_FALSE) != ASI_SUCCESS) {
        cerr << "Error setting exposure time" << endl;
        return 1;
    }

    // START THE EXPOSURE!!!!
    if (ASIStartExposure(camera_info.CameraID, ASI_FALSE) != ASI_SUCCESS) {
        cerr << "Error starting exposure" << endl;
        return 1;
    }

    // Wait for camera to take exposure
    while (true) {
        ASIGetExpStatus(camera_info.CameraID, &asi_exp_status);
        if (asi_exp_status == ASI_EXP_SUCCESS) {
            cout << "Successfully took exposure" << endl;
            break;
        }
        else if (asi_exp_status == ASI_EXP_FAILED) {
            cerr << "Exposure capture failed" << endl;
            return 1;
        }
    }

    // Retrieve exposure data
    vector<unsigned char> asi_image(image_size);
    if (ASIGetDataAfterExp(camera_info.CameraID, asi_image.data(), image_size) != ASI_SUCCESS) {
        cerr << "Error reading exposure data" << endl;
        return 1;
    }

    // Print the first few bytes of exposure data
    cout << "Image data (first 20 bytes): " << endl;
    for (int i = 0; i < 20; i++)
    {
        cout << static_cast<int>(asi_image[i]) << " ";
    }
    cout << endl;

    // Save full data to a .txt file
    ofstream myfile ("data-output.txt");
    if (myfile.is_open())
    {
        for(unsigned long i = 0; i < sizeof(asi_image); i++)
        {
            myfile << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(asi_image[i]) << " ";   // Convert bytes to hex
            if ((i + 1) % 16 == 0) myfile << "\n";  // New line every 16 bytes
        }
        myfile.close();
        cout << "Data saved to data-output.txt in hexadecimal format" << endl;
    }
    else 
    {
        cerr << "Unable to open file";
    }

    // Close camera
    cout << "Closing camera" << endl;
    ASICloseCamera(camera_info.CameraID);

    return 0;
}