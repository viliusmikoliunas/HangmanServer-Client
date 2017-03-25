#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define BUFFLEN 1024

static char* quitHandle = "/Q";
static char* playHandle = "/P";
static char* statHandle = "/S";
static char* disconnectHandle = "/D";

//recommended - single digit
static char userQuitChar = '0';
static char userStartChar = '1';
static char userStatsChar = '2';

void SendUsername(int socket, char* username)
{//U:|strlen(username)|username
	char* msgToSend;
	msgToSend = (char*)malloc(5+strlen(username));
	strcpy(msgToSend,"U:|");
	
	char* usernameLength = malloc(2);
	sprintf(usernameLength,"%d",strlen(username));
	strcat(msgToSend,usernameLength);
	strcat(msgToSend,"|");
	
	strcat(msgToSend,username);
	strcat(msgToSend,"\0");
	write(socket,msgToSend,strlen(msgToSend));
}
void PrintMenu()
{	
	printf("%c.Quit\n",userQuitChar);
	printf("%c.Start\n",userStartChar);
	printf("%c.Stats\n",userStatsChar);
}
int main(int argc, char *argv[]){
    unsigned int port;
    int s_socket;
    struct sockaddr_in servaddr; // Serverio adreso struktura
    fd_set read_set;

    char recvbuffer[BUFFLEN];
    char sendbuffer[BUFFLEN];

    int i;
/*
    if (argc != 3){
        fprintf(stderr,"USAGE: %s <ip> <port>\n",argv[0]);
        exit(1);
    }
*/
    //port = atoi(argv[2]);
	port = atoi("7896");
	/*
    if ((port < 1) || (port > 65535)){
        printf("ERROR #1: invalid port specified.\n");
        exit(1);
    }*/

    if ((s_socket = socket(AF_INET, SOCK_STREAM,0))< 0){
        fprintf(stderr,"ERROR #2: cannot create socket.\n");
        exit(1);
    }
                                
   /*
    * I�valoma ir u�pildoma serverio struktura
    */
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET; // nurodomas protokolas (IP)
    servaddr.sin_port = htons(port); // nurodomas portas
    
    /*
     * I�verciamas simboliu eiluteje u�ra�ytas ip i skaitine forma ir
     * nustatomas serverio adreso strukturoje.
     */
	 
    //if ( inet_aton(argv[1], &servaddr.sin_addr) <= 0 ) {
    if ( inet_aton("127.0.0.1", &servaddr.sin_addr) <= 0 ) {
        fprintf(stderr,"ERROR #3: Invalid remote IP address.\n");
        exit(1);
    }

	//char* username = "Klientas";
	
	char* username;
	printf("Enter username:");
	scanf("%s",username);
	
	
    if (connect(s_socket,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"ERROR #4: error in connect().\n");
        exit(1);
    }
	else 
	{
		puts("Welcome to Hangman");
		PrintMenu();
	}
    memset(&sendbuffer,0,BUFFLEN);
    fcntl(0,F_SETFL,fcntl(0,F_GETFL,0)|O_NONBLOCK);
	
	static bool usernameSent = false;
	static bool gameUnderway = false;
    while (1)
	{
        FD_ZERO(&read_set);
        FD_SET(s_socket,&read_set);
        FD_SET(0,&read_set);

        select(s_socket+1,&read_set,NULL,NULL,NULL);

		if(!usernameSent)
		{
			SendUsername(s_socket,username);
			usernameSent = true;
		}
		
        if (FD_ISSET(s_socket, &read_set))//gavimas
		{
            memset(&recvbuffer,0,BUFFLEN);
            i = read(s_socket, &recvbuffer, BUFFLEN);
			if(strstr(recvbuffer,disconnectHandle)!=NULL)
			{
				break;
            }
			printf("%s\n",recvbuffer);
        }
		
        else if (FD_ISSET(0,&read_set))//siuntimas
		{
            i = read(0,&sendbuffer,BUFFLEN);
			
			if(sendbuffer[0]==userQuitChar) 
			{
				write(s_socket, quitHandle,strlen(quitHandle));
			}
			else if(sendbuffer[0]==userStartChar && !gameUnderway)
			{
				gameUnderway = true;
				write(s_socket, playHandle,strlen(playHandle));
			}
			else if (sendbuffer[0] == userStatsChar && !gameUnderway)
			{
				write(s_socket, statHandle,strlen(statHandle));
			}
			
            else write(s_socket, sendbuffer,i);
        }
    }
	puts("You have been disconnected");
    close(s_socket);
    return 0;
}