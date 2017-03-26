#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

#define BUFFLEN 1024
#define maxUsernameLength 50

static char* usernameHandle = "U:";
static char* gameMoveHandle = "M:";
static char* livesHandle = "L:";

static char* quitHandle = "/Q";
static char* playHandle = "/P";
static char* statHandle = "/S";
static char* disconnectHandle = "/D";