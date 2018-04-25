#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "ccitt16.h"

#define DEBUG 1

#define PACKET_LENGTH 6
#define ACK_LENGTH    2
#define MAX_ADDRESS 100
#define MAX_NAME    100


