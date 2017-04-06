#include "Libraries.h"
#include <fcntl.h>

//menu keys; recommended - digit
static char userQuitChar = '0';
static char userStartChar = '1';
static char userStatsChar = '2';
static char specificUserStatsChar = '3';
static char allUserStatsChar = '4';

void SendUsername(int socket, char* username)
{
	char *p = strchr(username,'\n');
	*p = '\0';//remove \n symbol from the back
	
	char* msgToSend = malloc(maxUsernameLength+15);
	strcpy(msgToSend,usernameHandle);
	strcat(msgToSend,username);
	strcat(msgToSend,"\0");
	write(socket,msgToSend,strlen(msgToSend));
	free(msgToSend);
}
void PrintMenu()
{	
	puts("");
	printf("%c.Quit\n",userQuitChar);
	printf("%c.Start\n",userStartChar);
	printf("%c.Personal Stats\n",userStatsChar);
	printf("%c.Specific user Stats\n",specificUserStatsChar);
	printf("%c.All Stats\n",allUserStatsChar);
}
int SendGameMove(int socket, char* buffer)
{
	if(strlen(buffer)>2) return 1;//input is not 1 letter
	char letter = *buffer;
	if(!isalpha(letter)) return 2;//input is not a letter
	
	char* msgToSend = malloc(5);
	strcpy(msgToSend,gameMoveHandle);
	*(msgToSend+strlen(gameMoveHandle)) = letter;
	*(msgToSend+strlen(gameMoveHandle)+1) = '\0';
	
	write(socket,msgToSend,strlen(msgToSend));
	free(msgToSend);
	return 0;
}
void DisplayWinsLosses(char* winsLosses)
{
	char* currentElement = winsLosses;
	char* nextElement = strchr(currentElement,'|');
	*nextElement = '\0';
	printf("Wins:%s ",currentElement);
	currentElement = nextElement+1;
	
	nextElement = strchr(currentElement,'\0');
	*nextElement = '\0';
	printf("Losses:%s\n",currentElement);
}
void PrintOneUserStatistics(char* line)
{
	char* currentElement = line;
	char* nextElement = strchr(currentElement,'|');
	*nextElement = '\0';
	printf("Username:%s\n",currentElement);
	currentElement = nextElement+1;
	
	DisplayWinsLosses(currentElement);
	puts("");
}
void PrintAllStatistics(char* lines)
{
	lines+=strlen(allStatisticsHandle);
	char* currentLine = lines;
	while(currentLine)
	{
		char *nextLine = strchr(currentLine,'\n');
		if(nextLine) *nextLine = '\0';
		if(strlen(currentLine)<=1) break;//reached the end
		PrintOneUserStatistics(currentLine);
		if(nextLine) *nextLine = '\n';
		currentLine = nextLine ? (nextLine+1) : NULL;
	}
}
void RequestSpecificUserStatistics(int socket)
{
	char* msgToSend = malloc(maxUsernameLength+10);
	char* username = malloc(maxUsernameLength);
	strcpy(msgToSend,specificUserStatsHandle);
	fgets(username,maxUsernameLength,stdin);
	strcat(msgToSend,username);
	char* p = strchr(msgToSend,'\n');
	*p = '\0';
	write(socket, msgToSend,strlen(msgToSend));
	free(username);
	free(msgToSend);
}
int main(int argc, char *argv[]){
    unsigned int port;
    int s_socket;
    struct sockaddr_in servaddr; 
    fd_set read_set;

    char recvbuffer[BUFFLEN];
    char sendbuffer[BUFFLEN];

    int i;

    if (argc != 3){
        fprintf(stderr,"USAGE: %s <ip> <port>\n",argv[0]);
        exit(1);
    }

    port = atoi(argv[2]);
	
    if ((port < 1024) || (port > 65535)){
        printf("ERROR #1: invalid port specified.\n");
        exit(1);
    }

    if ((s_socket = socket(AF_INET, SOCK_STREAM,0))< 0){
        fprintf(stderr,"ERROR #2: cannot create socket.\n");
        exit(1);
    }
                                
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port); 
    
	 
    if ( inet_aton(argv[1], &servaddr.sin_addr) <= 0 ) {
        fprintf(stderr,"ERROR #3: Invalid remote IP address.\n");
        exit(1);
    }
	
	char* username = malloc(maxUsernameLength);
	printf("Enter username:");
	fgets(username,maxUsernameLength,stdin);
	
    if (connect(s_socket,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"ERROR #4: error in connect().\n");
        exit(1);
    }
	
	puts("Welcome to Hangman");
	PrintMenu();
	
    memset(&sendbuffer,0,BUFFLEN);
    fcntl(0,F_SETFL,fcntl(0,F_GETFL,0)|O_NONBLOCK);
	
	static bool usernameSent = false;
	static bool gameUnderway = false;
	static bool requestedForSpecificStats = false;
	
    while (1)
	{
        FD_ZERO(&read_set);
        FD_SET(s_socket,&read_set);
        FD_SET(0,&read_set);

        select(s_socket+1,&read_set,NULL,NULL,NULL);

        if (FD_ISSET(s_socket, &read_set))//recieve
		{
            memset(&recvbuffer,0,BUFFLEN);
            i = read(s_socket, &recvbuffer, BUFFLEN);
			if(strstr(recvbuffer,disconnectHandle)!=NULL)
			{
				break;
            }
			else if(strstr(recvbuffer,statisticsHandle)!=NULL)
			{
				if(recvbuffer[2]=='-') puts("No records of user");
				else DisplayWinsLosses(recvbuffer+strlen(statisticsHandle));
			}
			else if(strstr(recvbuffer,gameWonHandle)!=NULL)
			{
				puts("Congratulations you won!");
				PrintMenu();
				gameUnderway = false;
			}
			else if (strstr(recvbuffer,gameLostHandle)!=NULL)
			{
				puts("You have lost :(");
				PrintMenu();
				gameUnderway = false;
			}
			else if (strstr(recvbuffer,livesHandle)!=NULL)
			{
				printf("Lives left:%s\n",recvbuffer+strlen(livesHandle));
			}
			else if (strstr(recvbuffer,userStringHandle)!=NULL)
			{
				printf("%s\n",recvbuffer+strlen(userStringHandle));
			}
			else if (strstr(recvbuffer,allStatisticsHandle)!=NULL)
			{
				if(*(recvbuffer+strlen(allStatisticsHandle))=='-')
				{
					puts("No statistics avaivable");
				}
				else
				{
					PrintAllStatistics(recvbuffer);
				}
			}
        }
		
		else if(FD_ISSET(0,&read_set)&&!usernameSent)
		{
			SendUsername(s_socket,username);
			usernameSent = true;
		}
			
        else if (FD_ISSET(0,&read_set))//send
		{
			if(requestedForSpecificStats)
			{
				RequestSpecificUserStatistics(s_socket);
				requestedForSpecificStats = false;
			}
			else
			{
				fgets(sendbuffer,BUFFLEN,stdin);
			
				if(sendbuffer[0]==userQuitChar) //users wants to quit
				{
					write(s_socket, quitHandle,strlen(quitHandle));
				}
				
				if(gameUnderway)
				{
					SendGameMove(s_socket,sendbuffer);
				}

				if(!gameUnderway)
				{
					if(sendbuffer[0]==userStartChar)
					{
						gameUnderway = true;
					}
					else if (sendbuffer[0] == userStatsChar)
					{
						write(s_socket, statHandle,strlen(statHandle));
					}
					else if (sendbuffer[0] == specificUserStatsChar)
					{
						requestedForSpecificStats = true;
						puts("Enter username:");
					}
					else if (sendbuffer[0] == allUserStatsChar)
					{
						write(s_socket,allUserStatHandle,strlen(allUserStatHandle));
					}
				}
			}
		
		}
    }
	puts("You have been disconnected");
    close(s_socket);
    return 0;
}