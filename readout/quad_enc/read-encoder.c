#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#define SERIAL_PORT "/dev/ttyUSB0"
#define MAX_RESPONSE_LENGTH 26
#define READ_DELAY 30000

#define CPR 536

void printQSBError(const char* errorMessage) {
    printf("%s: %d - %s\n", errorMessage, errno, strerror(errno));
    exit(-1);
}

int open_serial_port(const char *port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1) {
        perror("Error opening serial port");
        exit(-1);
    }
    return fd;
}

void configure_serial_port(int fd) {
    struct termios serial_settings;
    tcgetattr(fd, &serial_settings);
    cfsetispeed(&serial_settings, B230400);
    cfsetospeed(&serial_settings, B230400);
    serial_settings.c_lflag &= ~(ECHO | ECHONL | IEXTEN | ISIG);
    serial_settings.c_lflag |= ICANON;
    tcsetattr(fd, TCSANOW, &serial_settings);
    tcflush(fd, TCIOFLUSH);
}

void sendQSBCommand(int fd, const char *command) {
    char qsbCommand[strlen(command) + 2];
    sprintf(qsbCommand, "%s\n", command);
    if (write(fd, qsbCommand, strlen(qsbCommand)) < 0) {
        printQSBError("Error writing to QSB device");
    }
    usleep(READ_DELAY);
}

int wait_for_data(int fd) {
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    return select(fd + 1, &read_fds, NULL, NULL, &timeout);
}

int readQSBResponse(int fd, char *response, double *angle) {
    if (wait_for_data(fd) > 0) {
        int bytesRead = read(fd, response, MAX_RESPONSE_LENGTH);
        if (bytesRead > 0) {
            response[strcspn(response, "!")] = '\0'; // Null-terminate at '!'
            int encoderCount = (int)strtol(response + 3, NULL, 16); // Extract count
            *angle = encoderCount * (120 * M_PI / CPR); // Convert count to radians
            return encoderCount;
        }
    }
    return -1; // Error case
}

int main() {
    int serial_fd = open_serial_port(SERIAL_PORT);
    configure_serial_port(serial_fd);

    sendQSBCommand(serial_fd, "R0E");

    char response[MAX_RESPONSE_LENGTH];
    double angle;
    
    int encoderCount = readQSBResponse(serial_fd, response, &angle);

    if (encoderCount >= 0) {
        printf("Encoder Count: %d, Angle (radians): %.6f\n", encoderCount, angle);
    } else {
        printf("Failed to read encoder count.\n");
    }
    
    close(serial_fd);
    return 0;
}