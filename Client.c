#include "Libraries.h"
#include <fcntl.h>


//recommended - digit
static char userQuitChar = '0';
static char userStartChar = '1';
static char userStatsChar = '2';

void SendUsername(int socket, char* username)
{//U:|strlen(username)|username
	*(username+strlen(username)-1) = '\0';//cut \n symbol from the back
	char* msgToSend = (char*)malloc(strlen(username)+5);
	strcpy(msgToSend,usernameHandle);
	strcat(msgToSend,"|");
	
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
int SendGameMove(int socket,char* move)
{
	*(move+strlen(move)-1) = '\0'; //getting rid of \n
	printf("Inputas:%s|\n",move);
	if(strlen(move)!=1) return 1;//input is longer than one char
	int ch = (int)*move;
	if(!isalpha(ch)) return 2;//input is not a letter
	char* gameMove = malloc(3);
	
	strcpy(gameMove,gameMoveHandle);
	strcat(gameMove,move);
	
	write(socket,gameMove,strlen(gameMove));
	return 0;
}
int SendGameMove2(int socket, char* buffer)
{
	//printf("User:%s|\n",buffer);
	if(strlen(buffer)>2) return 1;
	char ch = *buffer;
	if(!isalpha(ch)) return 2;
	char* msgToSend = malloc(3);
	
	strcpy(msgToSend,gameMoveHandle);
	strncat(msgToSend,buffer,1);
	//printf("Siuntinys:%s|\n",msgToSend);
	
	write(socket,msgToSend,strlen(msgToSend));
	free(msgToSend);
	return 0;
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
	
	char* username = malloc(maxUsernameLength);
	printf("Enter username:");
	fgets(username,maxUsernameLength,stdin);
	
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
			//i = read(0,&sendbuffer,BUFFLEN);
			fgets(sendbuffer,BUFFLEN,stdin);
			if(sendbuffer[0]==userQuitChar) //users wants to quit
			{
				write(s_socket, quitHandle,strlen(quitHandle));
			}
			
			if(gameUnderway)
			{
				int err = SendGameMove2(s_socket,sendbuffer);
				//if (err==0) puts("Great success");
				//else if(err==1) puts("Too long");
				//else if (err==2) puts("Not letter");
			}

			if(!gameUnderway)
			{
				if(sendbuffer[0]==userStartChar)//user wants to play
				{
					gameUnderway = true;
					//write(s_socket, playHandle,strlen(playHandle));
				}
				else if (sendbuffer[0] == userStatsChar)//user wants statistics
				{
					write(s_socket, statHandle,strlen(statHandle));
				}
			}		
            //else write(s_socket, sendbuffer,i);//!!! i
        }
    }
	puts("You have been disconnected");
    close(s_socket);
    return 0;
}