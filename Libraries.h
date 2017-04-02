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
#include <errno.h>

#define BUFFLEN 1024
#define maxUsernameLength 20

static char* usernameHandle = "U:";
static char* gameMoveHandle = "M:";
static char* livesHandle = "L:";
static char* statisticsHandle = "S:";
static char* specificUserStatsHandle = "E:";
static char* allStatisticsHandle = "A:";
static char* userStringHandle = "R:";

static char* gameWonHandle = "/W";
static char* gameLostHandle = "/L";

static char* quitHandle = "/Q";
static char* statHandle = "/S";
static char* disconnectHandle = "/D";
static char* allUserStatHandle = "/A";
