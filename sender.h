#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "AddCongestion.h"
#include "ccitt16.h"

#define DEBUG 1
#define BUFFER_SIZE 1024
#define MAX_ADDRESS 100
#define MAX_LENGTH 100
#define MAX_PACKETS 10000
#define PACKET_SIZE 6
#define ACK_SIZE 2
#define TIMEOUT 3.0


int main(int argc, char **argv);
void c_socket(int *s_socket);
int c_connect(int socket);
void c_write(int r_socket, int num_seq, unsigned char *packet, char *buffer, double BER, int *end);
void c_read(char *filename, char *buffer);
void c_close(int s_socket, int r_socket);
