#include <Python.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>


#define FALSE 0
#define TRUE  1


// Default port. Assumes one QSB plugged in
// and no other USB devices posing as serial
// devices. 
#define SERIAL_PORT "/dev/ttyUSB0"          //"/dev/QSB-S_US_Digital"


// The response from a QSB has the general format:
// 
//     Element: Response  Register  Data  TimeStamp '!'  EOR
//     Bytes  :     1         2       8       8      1   1..2
//
//
// Response, Register, Data, and '!' are always present.
//
// The rest of the elements are determined by the EOR (15) register.
// Each feature is enabled with a bit=1 or disabled with a bit=0.  
// The default value is: CR/LF = 0x03.
// 
// BITS:   B3 B2 B1 B0
// b
// B0 = Line Feed
// B1 = Carriage Return
// B2 = 4-byte Time Stamp appended to the response
// B3 = Spaces between returned fields
// 
// The timestamp is a 4-byte value but is appended to the 
// packet as an 8-byte hex string.
// 
// If spaces are used, the resulting format is:
// 
//   Response[space]Register[space]Data[space]TimeStamp[space]!EOR
// 
// 
// The number below is size of the longest possible response 
// received from the QSB when all terminators are on and spaces
// separate the elements. It also allows for the string termination.
const int BUFFER_SIZE = 27;


// The USB link between the QSB and the computer introduces
// considerable lag. So, we need to wait a bit when sending
// an instruction before we get the response. The value of
// 5 ms is empirical and may need adjustment on other systems.
const int FIVE_MILLISECONDS = 5000;


// How many times we will try a non-blocking read of the 
// device before we call it a failed operation.
const int MAX_READ_TRIES = 10;



// Rather than getting into complications reading the
// keyboard for input, we will just trap Ctrl+C and 
// notify the app when the user is done.
int CloseRequested = FALSE;



void printQsbError(char* errorMessage)
{
        printf("%s: %d - %s", errorMessage, errno, strerror(errno));
        exit(-1);
}




// The QSB presents itself to the system as a vanilla
// UART and we can talk to it using the standard 
// POSIX IO functions.
//
// In the two utilitarian functions below, 'qsb' is
// a standard UNIX file handle pointing to the serial
// port where the devices is hosted.


void sendQsbInstruction(int qsb, char* command)
{
    // Create a padded instruction string that includes
    // CR+LF. The QSB will be happy with just CR or LF too.
    char* qsbCommand = (char *)malloc(strlen(command) + 3);
    sprintf(qsbCommand, "%s\r\n", command);

    int ioResult = write(qsb, qsbCommand, strlen(qsbCommand));
    free(qsbCommand);

    if (ioResult < 0)
    {
        printQsbError("Error writing to QSB device");
    }
}


void readQsbResponse(int qsb, char* response, int responseSize)
{
    int i = 0;
    int ioResult;
    char ch;            //mod
    do
    {
        printf("loopy\n");

        // ioResult = read(qsb, response, responseSize);
        ioResult = read(qsb, &ch, 1);       //mod
        printf("%c\n", ch);
        printf("%d\n", ioResult);
        if (ioResult > 0)
        {
            response[i] = ch;
            i++;
        }
        // mod ^^^^^^^^^^^^^
        // This delay is to give some time to the device to
        // pipe the information to the serial port.
        usleep(FIVE_MILLISECONDS);
        //i++;
    } while (ch != '!' && i < responseSize);      //mod
    //while (ioResult < 0 && errno == EAGAIN && i < MAX_READ_TRIES);

    if (i == MAX_READ_TRIES)
    {
        printQsbError("Error reading from QSB device");
    }

    // Remove the trailing CR+LF if any, and trim to proper size.
    int end = strcspn(response, "\r\n");
    response[end] = '\0';

    if (ioResult < responseSize)
    {
        response[ioResult] = '\0';
    }
}


// Every instruction sent to the QSB is acknowledged 
// with a corresponding response string. We then send
// the instruction and retrieve the response as a 
// single command transaction.
void qsbCommand(int qsb, char* command, char* response, int responseSize)
{
    sendQsbInstruction(qsb, command);
    readQsbResponse(qsb, response, responseSize);
}



void ctrlCHandler(int signal)
{
    CloseRequested = TRUE;
}


// ************************************************* 
// 
//  General flow of operation:
//   1. Open the port and configure it.
//      The QSB UART wants 230.4K Baud, 8-n-1
//   2. Set up the QSB to read an encoder.
//   3. Loop polling and printing the current
//      encoder count.
//   4. Upon close request, close the port and
//      exit. 
// 
// *************************************************

int main (int argc, char *argv[])
{
    printf("loop1\n");

    //initialize Python
    Py_Initialize();
    PyObject *pyDict = PyDict_New();


    // Register the Ctrl-C handler.
    signal(SIGINT, ctrlCHandler);



    // Open the port.
    int qsb = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (qsb < 0)
    {
        printQsbError("Error opening QSB device");
    }



    // Configure it.
    struct termios qsbConfiguration;
    tcgetattr(qsb, &qsbConfiguration);

    qsbConfiguration.c_cflag = B230400 | CS8;
    qsbConfiguration.c_lflag &=~(ECHO | ECHONL | IEXTEN | ISIG);
    qsbConfiguration.c_lflag |= ICANON;
    cfsetospeed(&qsbConfiguration, B230400);
    tcflush(qsb, TCIFLUSH);
    tcsetattr(qsb, TCSANOW, &qsbConfiguration);

    // Below is the basic communications protocol with 
    // the QSB. The commands were composed using the
    // QSB command list available for download at the
    // US Digital site. 


    char* command;
    char response[BUFFER_SIZE];
    printf("loop2\n");

    // first command sent, set the EOR termination and feeder line to include a line feed character
    qsbCommand(qsb, "W151", response, BUFFER_SIZE);

    // Read and print product information.
    // Register: VERSION (0x14)
    qsbCommand(qsb, "R14", response, BUFFER_SIZE);
    printf("Product info: %s\n", response);
    
    // Set encoder in quadrature mode.
    // Register: MODE (0x00)
    qsbCommand(qsb, "W0000", response, BUFFER_SIZE);
    printf("Quadrature response: %s\n", response);

    // Count up, 1X, Modulo-N.
    // Register: MDR0 (0x03)
    qsbCommand(qsb, "W030C", response, BUFFER_SIZE);
    printf("Count configuration: %s\n", response);

    // Set maximum count (Preset) to decimal 499
    // Register: DTR (0x08)
    qsbCommand(qsb, "W081F3", response, BUFFER_SIZE);
    printf("Maximum count: %s\n", response);

    // Set baud rate to 115200 bits/sec (matches encoder baud rate)
    // Register: ??? Decbot approved 
    qsbCommand(qsb, "W1640A", response, BUFFER_SIZE);
    printf("Baud rate: %s\n", response);    //doesn't print a response so do we know if it works
    printf("loop3\n");

    //motor shenanigans
    /*  //I don't think the I/O pins are setup in qsb-d stick
    qsbCommand(qsb, "W123E8", response, BUFFER_SIZE);
    printf("Jog rate: %s\n", response);
    //activate
    qsbCommand(qsb, "W162", response, BUFFER_SIZE);
    //execute
    qsbCommand(qsb, "W169", response, BUFFER_SIZE);
`   */

    puts("\nUse Ctrl+C to exit.\n\n");


    while (CloseRequested == FALSE)
    {

        // Do stuff.
        printf("loop\n");

        // Read current count.
        // Register: READ ENCODER (0x0E)
        qsbCommand(qsb, "R0E", response, BUFFER_SIZE);
        printf("%s\n", response);
        if (strlen(response) > 0){
            //parsing
            char copystr[BUFFER_SIZE];
            strncpy(copystr, response, BUFFER_SIZE - 1);
            copystr[BUFFER_SIZE - 1] = '\0';

            char *token;
            char *data = NULL;
            int tokenIndex = 0;
            const int dataTokenIndex = 2;   //where data resides if deciding to add more to raw response line such as timestamp this must change
            static double lastGoodRadians = 0;

            token = strtok(copystr, " ");
            while (token != NULL) {
                if (tokenIndex == dataTokenIndex) {
                    data = token;
                    break;
                }
                tokenIndex++;
                token = strtok(NULL, " ");
            }
                
            //change from ASCII to int
            if (data != NULL) {
                long int int_val = strtol(data, NULL, 16);
                double radians = int_val * (2*M_PI / 499.0);  //change 500 depending on the count cycle
                // printf("int count: %ld | radians %lf\n", int_val, radians);

                //Python do stuff

                //get timestamp better way to do this by taking it from QSB-D but using this for now cuz Chat GPT 4 said so
                struct timeval tv;
                gettimeofday(&tv, NULL);
                double timestamp = tv.tv_sec + tv.tv_usec / 1000000.0;

                //add to Python dict
                PyObject *pyRadians = PyFloat_FromDouble(radians);
                PyObject *pyTimestamp = PyFloat_FromDouble(timestamp);
                PyDict_SetItem(pyDict, pyTimestamp, pyRadians);
                lastGoodRadians = radians;
            }
            else{
                printf("no data\n");
                printf("response %s\n", response);
                double radians = lastGoodRadians;
                //get timestamp better way to do this by taking it from QSB-D but using this for now cuz Chat GPT 4 said so
                struct timeval tv;
                gettimeofday(&tv, NULL);
                double timestamp = tv.tv_sec + tv.tv_usec / 1000000.0;

                //add to Python dict
                PyObject *pyRadians = PyFloat_FromDouble(radians);
                PyObject *pyTimestamp = PyFloat_FromDouble(timestamp);
                PyDict_SetItem(pyDict, pyTimestamp, pyRadians);
                //break;
            }
        }
        else{
            // printf("empty data\n");
            continue;
        }
    }

    //get rid of all the extra zeros and the repeating values then we will be golden!!!
    //pickle the heck out of it
    PyObject *pPickleModule = PyImport_ImportModule("pickle");
    PyObject *pPickleDict = PyObject_CallMethod(pPickleModule, "dumps", "O", pyDict);

    //Write to file
    FILE *file = fopen("nerd_file.pkl", "wb");
    Py_ssize_t size;
    char *buffer; 
    PyBytes_AsStringAndSize(pPickleDict, &buffer, &size);
    fwrite(buffer, size, 1, file);
    fclose(file);

    //cleanup
    Py_DECREF(pyDict);
    Py_DECREF(pPickleModule);
    Py_DECREF(pPickleDict);

    close(qsb);
    puts("\n");
    Py_Finalize();
    return(0);
}
