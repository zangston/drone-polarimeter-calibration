#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>      // Contains file controls like O_RDWR
#include <errno.h>      // Error integer and strerror() function
#include <termios.h>    // Contains POSIX terminal control definitions
#include <unistd.h>     // write(), read(), close(), sleep()
#include <sys/select.h> // fd_set
#include <stdlib.h>     // malloc
#include <sys/time.h>

#define FALSE 0
#define TRUE  1

// Default port. Assumes one QSB plugged in
// and no other USB devices posing as serial
// devices.
#define SERIAL_PORT "/dev/ttyUSB0" 

// The response from a QSB has the general format:
// Element: Response Register Data TimeStamp '!' EOR
// Bytes : 1 2 8 8 1 1..2
//
//
// Response, Register, Data, and '!' are always present.
//
// The rest of the elements are determined by the EOR (15) register.
// Each feature is enabled with a bit=1 or disabled with a bit=0.
// The default value is: CR/LF = 0x03.
//
// BITS: B3 B2 B1 B0
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
// Response[space]Register[space]Data[space]TimeStamp[space]!EOR
//
//
// The longest possible response is 26 character when all terminators 
// are on and spaces separate the elements. The configuration used in 
// this demo only add a line feed to the command response. 
const int MAX_RESPONSE_LENGTH = 13;

// The USB link between the QSB and the computer introduces
// considerable lag. So, we need to wait a bit when sending
// an command before we get the response. These values are
// empirical and may need adjustment on other systems.
const int READ_DELAY = 20000; // 2ms
const int WRITE_DELAY = 60000; // 6ms

// Rather than getting into complications reading the
// keyboard for input, we will just trap Ctrl+C and 
// notify the app when the user is done.
int CloseRequested = FALSE;

void printQSBError(char* errorMessage)
{
    printf("%s: %d - %s", errorMessage, errno, strerror(errno));
    exit(-1);
}

int open_serial_port(const char *port)
{
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1) {
        printQSBError("Error opening serial port");
    }
}

void configure_serial_port(int fd) {
    struct termios serial_settings;

    tcgetattr(fd, &serial_settings);
    cfsetispeed(&serial_settings, B230400);
    cfsetospeed(&serial_settings, B230400);

    serial_settings.c_lflag &= ~(ECHO |ECHONL | IEXTEN |ISIG); 
    serial_settings.c_lflag |= ICANON;

    tcsetattr(fd, TCSANOW, &serial_settings);
    tcflush(fd, TCIOFLUSH);
}

// The QSB presents itself to the system as a vanilla
// UART and we can talk to it using the standard
// POSIX IO functions.
//
// In the utilitarian functions below, 'fd' is
// a standard UNIX file handle pointing to the serial
// port where the devices is hosted.
void sendQSBCommand(int fd, const char *command) {
    // Append a new line character to the command.
    char* qsbCommand = (char *)malloc(strlen(command) + 1);

sprintf(qsbCommand, "%s\n", command);
    if (write(fd, qsbCommand, strlen(qsbCommand)) >= 0)
    {
        if (command[0] == 'W') {
            // Sleep to allow the device to 
            // process the Write command.
            usleep(WRITE_DELAY);
        }
        else {
            // Sleep to allow the device to 
            // process the Read or Stream command.
            usleep(READ_DELAY); 
        }
    } else {
        printQSBError("Error writing to QSB device");
    }    
}

/// @brief Used to wait up to 2 seconds for data to arrive in serial buffer.
/// @param fd 
/// @return The number of FDS ready or -1 for err.
int wait_for_data(int fd) {
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    return select(fd + 1, &read_fds, NULL, NULL, &timeout);
}

int readQSBResponse(int fd, char *response) {
    int bytesRead = -1;
    if (wait_for_data(fd) > 0) {
        bytesRead = read(fd, response, MAX_RESPONSE_LENGTH);
        if (bytesRead > 0) {
            int end = strcspn(response, "!");
            response[end] = '\0';
        }
    }
    return bytesRead;
}

void processQSBResponse(const char *response) {
    if (response[0] == 'r' || 
       response[0] == 'w' ||
       response[0] == 's') {
        char command_type = response[0];
        char register_str[3];
        strncpy(register_str, response +1, 2);
        register_str[2] = '\0';
        const char *value = response + 3;

        if (command_type == 'r') {
            printf("Read Register %s: Value = %s\n", register_str, value);
        } else if (command_type == 'w') {
            printf("Wrote Register %s: Value = %s\n", register_str, value);
        } else if (command_type == 's') {
            printf("Stream Register %s, Value = %s\n", register_str, value);
        }
    } else if (response[0] == 'e') {
        const char *value = response + 1;
        printf("QSB Error Value = %s\n", value);
    } else {
        printf("Invalid Response = %s\n", response);
    }
}

int getCountFromQsbResponse(int fd, char *response)
{
    int bytesRead = -1;

    if (wait_for_data(fd) > 0) {
        bytesRead = read(fd, response, MAX_RESPONSE_LENGTH);
        if (bytesRead > 0) {
            int end = strcspn(response, "!");
            response[end] = '\0';
        }
    } else {
         return -1;
    }

    const char *hexValue = response + 3;    
    return (int)strtol(hexValue, NULL, 16);
}

void process_command(int fd, char *command) {
    sendQSBCommand(fd, command);

    char response[MAX_RESPONSE_LENGTH];
    readQSBResponse(fd, response);

    processQSBResponse(response);
}

void ctrlCHandler(int signal)
{
    CloseRequested = TRUE;
}

/// @brief Gets the number milliseconds between the now and the start time.
/// @param startTime 
/// @return elapsed time in milliseconds
double getMillisecondsFrom(struct timeval* start)
{
    struct timeval stop;
    gettimeofday(&stop, NULL);
    return  ((stop.tv_sec - start->tv_sec) * 1000 + (stop.tv_usec - start->tv_usec)/1000.0);
}

/// @brief Gets the number milliseconds from startTime to now and sets startTime to now.
/// @param startTime 
/// @return elapsed time in milliseconds
double getMillisecondsFromAndReset(struct timeval* start)
{
    struct timeval stop;
    gettimeofday(&stop, NULL);
    double ms =  ((stop.tv_sec - start->tv_sec) * 1000 + (stop.tv_usec - start->tv_usec)/1000.0);
    *start = stop;
    return ms;
}

/// @brief Get the timestamp in milliseconds.
/// @return 
double getTimeInMilliseconds()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return ((now.tv_sec * 1000) + (now.tv_usec)/1000.0);
}

// *************************************************
//
// General flow of operation:
// 1. Open the port and configure it.
// The QSB UART wants 230.4K Baud, 8-n-1
// 2. Set up the QSB to read an encoder.
// 3. Loop polling and printing the current
// encoder count.
// 4. Upon close request, close the port and
// exit.
//
// *************************************************
int main() {

    // Register the Ctrl-C handler.
    signal(SIGINT, ctrlCHandler);

    // Open the serial port.
    int serial_fd = open_serial_port(SERIAL_PORT);
    
    // Configure the serial port. Baud 230400,8,1,N
    configure_serial_port(serial_fd);

    // Set the command response to include a line feed character.
    process_command(serial_fd, "W151");

    // Read and print the product inversion
    // Register: VERSION (0x14)
    process_command(serial_fd, "R14");

    // Set encoder in quadrature mode.
    // Register: MODE (0x00)
    process_command(serial_fd, "W0000");

    // Count up, 1X, Modulo-N.
    // Register: MDR0 (0x03)
    process_command(serial_fd, "W030C");

    // Set maximum count (Preset) to decimal 499
    // Register: DTR (0x08)
    process_command(serial_fd, "W081F3");

    // Setup some timing variables.
    struct timeval startLoopTime;    
    struct timeval startTime;
    gettimeofday(&startLoopTime, NULL);
    gettimeofday(&startTime, NULL);
    double elapseMilliseconds;
    double timestampMilliseconds;
    // This loop sends a read encoder position command
    // to the QSB device and then reads the response.
    // It will iterate a 100 times or until Ctrl-C is pressed.
    char response[MAX_RESPONSE_LENGTH];
    int count;
    int iterations = 0;
    while (CloseRequested == FALSE && iterations < 100)
    {
        iterations++;
        // Do stuff.
        // Read current count.
        // Register: READ ENCODER (0x0E)
        sendQSBCommand(serial_fd, "R0E");
        count = getCountFromQsbResponse(serial_fd, response);
        if (count >= 0) {
            elapseMilliseconds = getMillisecondsFromAndReset(&startTime);
            printf("Current count: %d, %.2f ms         \r", count, elapseMilliseconds);
        } else {
            elapseMilliseconds = getMillisecondsFrom(&startTime);
            printf("No updates in %.2f ms           \r", elapseMilliseconds);
        }
        fflush(stdout);        
    }
    elapseMilliseconds = getMillisecondsFrom(&startLoopTime);
    printf("Loop %d times: %.2f ms\n", iterations, elapseMilliseconds);

    // Make sure there is nothing in the serial buffers.
    tcflush(serial_fd, TCIOFLUSH);

    // Prepare to stream the encoder position.
    // Set the threshold 1 (stream count if changes by 1)
    process_command(serial_fd, "W0B1");
    
    // Set the interval to 0 (stream count at fastest rate)
    process_command(serial_fd, "W0C0");

    // Begin streaming the Encoder count register.
    process_command(serial_fd, "S0E");

    // This loop checks for data waiting on the serial port
    // and if there is data waiting, then read the response.
    // It will iterate until Ctrl-C is pressed.;
    iterations = 0;
    gettimeofday(&startLoopTime, NULL);
    while (CloseRequested == FALSE && iterations < 1000 )
    {
        iterations++;
        // Do stuff.

        // Read the streamed current count.        
        count = getCountFromQsbResponse(serial_fd, response);
        if (count >= 0) {
            //elapseMilliseconds = getMillisecondsFromAndReset(&startTime);
            //printf("Current count: %d, %.2f ms         \r", count, elapseMilliseconds);
            timestampMilliseconds = getTimeInMilliseconds();
            printf("Current count: %d, %.2f ms         \r", count, timestampMilliseconds);
        } else {
            //elapseMilliseconds = getMillisecondsFrom(&startTime);
            //printf("No updates in %.2f ms           \r", elapseMilliseconds);
            timestampMilliseconds = getTimeInMilliseconds();
            printf("Current count: %d, %.2f ms         \r", count, timestampMilliseconds);
        }
        fflush(stdout);
    }
    elapseMilliseconds = getMillisecondsFromAndReset(&startLoopTime);
    printf("\nLoop %d times: %.2f ms\n", iterations, elapseMilliseconds);

    // This reads the encoder count and stops the streaming.
    process_command(serial_fd, "R0E");

    close(serial_fd);
    puts("\n");
    return(0);
}