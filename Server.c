#include "Libraries.h"
#define MAXCLIENTS 10
#define maxWordLength 20
static const int lives = 5;
static const char* wordbankPath = "WordBank.txt";
static const char* statisticsPath = "Statistics.txt";
static const char* tempFilePath = "TempBuff.txt";

char* GenerateUserString(char* word)
{
	char* userString = malloc(30);
	int i=0;
	while(*(word+i)!='\0'&&*(word+i)!='\r')
	{
		*(userString+i) = '_';
		i++;
	}
	*(userString+i) = '\0';
	return userString;
}

int GetRandomNumber(int max)
{
    time_t t;
    srand((unsigned) time(&t));
    return rand()%max;
}
char* GetRandomWord()
{
    FILE *fd;
    int wordLength = 50;

    char currentWord[wordLength];
    char words[100][wordLength];
    int wordCount=0;

    fd = fopen(wordbankPath,"r");
    while(fgets(currentWord,wordLength,fd)!=NULL)
    {
        strncpy(words[wordCount],currentWord,strlen(currentWord));
        wordCount++;
    }
    fclose(fd);
    //choosing random word
    char *answer = malloc(20);
	strcpy(answer,words[GetRandomNumber(wordCount)]);
	*(answer+strlen(answer)-1) = '\0';
    return answer;
}

int findemptyuser(int c_sockets[]){
    int i;
    for (i = 0; i <  MAXCLIENTS; i++){
        if (c_sockets[i] == -1){
            return i;
        }
    }
    return -1;
}

struct hangmanGame{
	int lives[MAXCLIENTS];
	char word[MAXCLIENTS][maxWordLength];
	char userString[MAXCLIENTS][maxWordLength];
	char username[MAXCLIENTS][maxUsernameLength];
	char usedLetters[MAXCLIENTS][30];
	int usedLetterCounter[30];
}hangman;

struct PlayerStats
{
	char* username;
	char* wins;
	char* losses;
}statistics;

void SetupNewUser(int user_id)
{
	hangman.lives[user_id] = lives;
	
	char* tempWord = GetRandomWord();
	strcpy(hangman.word[user_id],tempWord);
	//puts(hangman.word[user_id]);
	char* tempUserString = GenerateUserString(hangman.word[user_id]);
	strcpy(hangman.userString[user_id],tempUserString);
	strcpy(hangman.userString[user_id],tempUserString);
	
	memset(hangman.usedLetters[user_id],0,sizeof(hangman.usedLetters[user_id]));
	hangman.usedLetterCounter[user_id]=0;
	
	//puts(hangman.userString[user_id]);
}

void SaveUsername(char* buffer, int user_id)
{//buffer = U:|strlen(username)|username
	
	char* name = buffer+strlen(usernameHandle)+1;//name = strlen(username)|username
	char* usernameLength = malloc(3);
	int counter=0;
	while(*(name+counter)!='|')
	{
		*(usernameLength+counter) = *(name+counter);
		counter++;
	}
	long length = strtol(usernameLength,NULL,10);
	char* username = malloc(maxUsernameLength);
	for(int i=0;i<length;i++)
	{
		*(username+i) = *(buffer+strlen(usernameHandle)+strlen(usernameLength)+2+i);
	}
	*(username+length) = '\0';
	strcpy(hangman.username[user_id],username);
	free(username);
	free(usernameLength);
}

void DissasembleString(char* line)
{
    int letterCounter=0;
    int i=0;
    char *username = malloc(50);
    while(*(line+letterCounter)!='|')
    {
        *(username+i) = *(line+letterCounter);
        i++;
        letterCounter++;//spin through username
    }

    letterCounter++;//wins|losses\n

    char* wins = malloc(5);
    i = 0;
    while(*(line+letterCounter)!='|')//read wins
    {
        *(wins+i) = *(line+letterCounter);
        i++;
        letterCounter++;
    }
    *(wins+i) = '\0';
    letterCounter++;

    char* losses = malloc(5);
    i = 0;
    while(*(line+letterCounter)!='\n')//read losses
    {
        *(losses+i) = *(line+letterCounter);
        i++;
        letterCounter++;
    }
    *(losses+i) = '\0';

    statistics.username = username;
    statistics.wins = wins;
    statistics.losses = losses;
}

char* AssembleStringBack(char* username, char* wins, char* losses)
{
    char* line = malloc(50);
	//strcpy(line,"\n");
    strcpy(line,username);
    strcat(line,"|");
    strcat(line,wins);
    strcat(line,"|");
    strcat(line,losses);
    strcat(line,"\0\n");

    return line;
}
char* UpdateStatistics(char* line, char* username, bool gameStatus)//username|wins|losses\n
{
	DissasembleString(line);
	
//updating number
    if(gameStatus)
    {
        long number = strtol(statistics.wins,NULL,10);
        number++;
        sprintf(statistics.wins,"%d",number);
    }
    else
    {
        long number = strtol(statistics.losses,NULL,10);
        number++;
        sprintf(statistics.losses,"%d",number);
    }
    //

    return AssembleStringBack(username,statistics.wins,statistics.losses);

}

void SaveStatistics(char* username, bool gameStatus)
{
    FILE *fd,*fb;
    int maxLineLength = 60;
    char* currentFileLine = malloc(maxLineLength);
    fd = fopen(statisticsPath,"r+");
    fb = fopen(tempFilePath,"w+");
    bool userStatsUpdated = false;

    while(fgets(currentFileLine,maxLineLength,fd)!=NULL)
    {
        if(strstr(currentFileLine,username)!=NULL)
        {
            char* updatedLine = UpdateStatistics(currentFileLine,username,gameStatus);
            fprintf(fb,"%s\r\n",updatedLine);
            userStatsUpdated = true;
        }
        else
        {
            fprintf(fb,"%s",currentFileLine);
        }
    }
    if(!userStatsUpdated)
    {
        char* newLine;
        if(gameStatus)
        {
            newLine = AssembleStringBack(username,"1","0");
        }
        else
        {
            newLine = AssembleStringBack(username,"0","1");
        }
        fprintf(fb,"%s\r\n",newLine);
    }
    //copy file

    char c;
    rewind(fb);
    rewind(fd);
	//copying from temp file to main
    while((c=fgetc(fb))!=EOF)
    {
        fputc(c,fd);
    }
    free(currentFileLine);
    fclose(fb);
    fclose(fd);
}

int ProcessGameMove(char* buffer, int socket, int user_id)
{
	char* gameMove = malloc(10);
	if(strstr(buffer,"\n")!=NULL) *(buffer+strlen(buffer)-1) = '\0';
	strcpy(gameMove,buffer+strlen(gameMoveHandle));//removes gameHandle
	if(strlen(gameMove)>1)
	{
		return 1;//too long
	}
	char ch = *gameMove;
	if(!isalpha(ch))
	{
		return 2;//not letter
	}		
	if(strchr(hangman.usedLetters[user_id],ch)!=NULL) 
	{
		return 3;//letter already used
	}
	char* tempGuessString = malloc(strlen(hangman.userString[user_id]));
	strcpy(tempGuessString,hangman.userString[user_id]);
	for(int i=0;i<strlen(tempGuessString);i++)
	{
		if(*(hangman.word[user_id]+i)==tolower(ch))
		{
			*(hangman.userString[user_id]+i) = tolower(ch);
		}
	}
	
	if(strstr(hangman.word[user_id],hangman.userString[user_id])!=NULL)//won
	{
		send(socket,gameWonHandle,strlen(gameWonHandle),0);
		SaveStatistics(hangman.username[user_id],1);
		SetupNewUser(user_id);
		return 0;
	}
	
	else if(strcmp(hangman.userString[user_id],tempGuessString)==0)//if letter is not in word
	{
		hangman.lives[user_id]--;
		if(hangman.lives[user_id]<=0)
		{
			send(socket,gameLostHandle,strlen(gameLostHandle),0);
			SaveStatistics(hangman.username[user_id],0);
			SetupNewUser(user_id);
			return 0;
		}
		
		char* livesMsg = malloc(5);
		char* livesLeft = malloc(5);
		strcpy(livesMsg,livesHandle);
		sprintf(livesLeft,"%d",hangman.lives[user_id]);
		strcat(livesMsg,livesLeft);
		send(socket,livesMsg,strlen(livesMsg),0);
		//puts(livesMsg);
		
		free(livesLeft);
		free(livesMsg);
	}
	else 
	{
		char* messageToSend = malloc(50);
		strcpy(messageToSend,userStringHandle);
		strcat(messageToSend,hangman.userString[user_id]);
		send(socket,messageToSend,strlen(messageToSend),0);
		free(messageToSend);
	}
	*(hangman.usedLetters[user_id]+hangman.usedLetterCounter[user_id]) = tolower(ch);
	hangman.usedLetterCounter[user_id]++;
	free(tempGuessString);
	free(gameMove);
	return 0;
}

void SendStatistics(int socket, char* username)
{
	char* messageToSend = malloc(50);
	strcpy(messageToSend,statisticsHandle);
	
	FILE *fd;
	fd=fopen(statisticsPath,"r");
	char* currentLine = malloc(50);
	bool wereStatisticsSent = false;
	while(fgets(currentLine,50,fd) && !wereStatisticsSent)
	{
		char* usernameFromCurrentLine = currentLine;
		char* temp = strchr(usernameFromCurrentLine,'|');
		*temp = '\0';
		if(strcmp(usernameFromCurrentLine,username)==0)
		{
			*temp = '|';
			DissasembleString(currentLine);
			strcat(messageToSend,statistics.wins);
			strcat(messageToSend,"|");
			strcat(messageToSend,statistics.losses);
			strcat(messageToSend,"\0");
			send(socket,messageToSend,strlen(messageToSend),0);
			wereStatisticsSent = true;
		}
	}
	
	if(!wereStatisticsSent)
	{
		strcat(messageToSend,"0\0");
		send(socket,messageToSend,strlen(messageToSend),0);
	}
	free(currentLine);
	free(messageToSend);
	fclose(fd);
}
int SendAllStatistics(int socket)
{
	FILE *fd;
	fd = fopen(statisticsPath,"r");
	char* allStatistics = malloc(BUFFLEN);
	strcpy(allStatistics,allStatisticsHandle);
	char* oneLine = malloc(50);
	
	while(fgets(oneLine,50,fd))
	{
		strcat(allStatistics,oneLine);
	}
	fclose(fd);
	free(oneLine);
	
	if(strlen(allStatistics)==strlen(allStatisticsHandle))
	{
		strcat(allStatistics,"0\0");
	}
	else
	{
		//strcat(allStatistics,endOfAllStatisticsChar);
		strcat(allStatistics,"\0");
	}
	send(socket,allStatistics,strlen(allStatistics),0);
	free(allStatistics);
	return 0;
}
/*void SendSpecificUserStats(char* buffer, int socket)//E:username  prob useless
{
	char* msgToSend = malloc(50);
	strcpy(msgToSend,specificUserStatsHandle);
	char* username = malloc(40);
	strcpy(username,buffer);
	memmove(username,username+2,strlen(username)-2);
	
	FILE *fd;
    int maxLineLength = 40;
    char* currentFileLine = malloc(maxLineLength);
    fd = fopen(statisticsPath,"r+");
	bool wasSent = false;
    while(fgets(currentFileLine,maxLineLength,fd)!=NULL)
    {
        if(strstr(currentFileLine,username)!=NULL)
        {
            strcat(msgToSend,currentFileLine);
            strcat(msgToSend,"\0");
			send(socket,msgToSend,strlen(msgToSend),0);
			wasSent = true;
        }
    }
	if(!wasSent)
	{
		strcat(msgToSend,"0\0");
		send(socket,msgToSend,strlen(msgToSend),0);
	}
	printf("Siuntinys:%s\n",msgToSend);
	free(currentFileLine);
	free(username);
	free(msgToSend);
}*/
int main(int argc, char *argv[]){
    unsigned int port;
    unsigned int clientaddrlen;
    int l_socket;
    int c_sockets[MAXCLIENTS];
    fd_set read_set;

    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;

    int maxfd = 0;
    int i;

    char buffer[BUFFLEN];
/*
    if (argc != 2){
        fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
        return -1;
    }*/


    //port = atoi(argv[1]);
	port = atoi("7896");
	
    if ((port < 1) || (port > 65535))
	{
        fprintf(stderr, "ERROR #1: invalid port specified.\n");
        return -1;
    }

    if ((l_socket = socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
        fprintf(stderr, "ERROR #2: cannot create listening socket.\n");
        return -1;
    }

    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(port);

    if (bind (l_socket, (struct sockaddr *)&servaddr,sizeof(servaddr))<0)
	{
        fprintf(stderr,"ERROR #3: bind listening socket.\n");
        return -1;
    }

    if (listen(l_socket, 5) <0)
	{
        fprintf(stderr,"ERROR #4: error in listen().\n");
        return -1;
    }                           

    for (i = 0; i < MAXCLIENTS; i++)
	{
        c_sockets[i] = -1;
    }


    while(1)
	{
        FD_ZERO(&read_set);
        for (i = 0; i < MAXCLIENTS; i++)
		{
            if (c_sockets[i] != -1)
			{
                FD_SET(c_sockets[i], &read_set);
				maxfd = (c_sockets[i] > maxfd) ? c_sockets[i] : maxfd;
            }
        }

        FD_SET(l_socket, &read_set);
		maxfd = (l_socket > maxfd) ? l_socket : maxfd;
        
        select(maxfd+1, &read_set, NULL , NULL, NULL);

        if (FD_ISSET(l_socket, &read_set))
		{
            int client_id = findemptyuser(c_sockets);
            if (client_id != -1)
			{
                clientaddrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, clientaddrlen);
                c_sockets[client_id] = accept(l_socket, 
                    (struct sockaddr*)&clientaddr, &clientaddrlen);
				SetupNewUser(client_id);
                printf("Connected: %s\n",inet_ntoa(clientaddr.sin_addr));
            }
        }
        for (i = 0; i < MAXCLIENTS; i++)
		{
            if (c_sockets[i] != -1)
			{
                if (FD_ISSET(c_sockets[i], &read_set))
				{
                    memset(&buffer,0,BUFFLEN);
                    int r_len = recv(c_sockets[i],&buffer,BUFFLEN,0);
					//int w_len;
					printf("User \"%s\" sends:|%s|\n",hangman.username[i],buffer);
					if(r_len<0)
					{
						send(c_sockets[i],disconnectHandle,strlen(disconnectHandle),0);
						close(c_sockets[i]);
						c_sockets[i] = -1;
						printf("%d\n",errno);
					}
					else if(strstr(buffer,usernameHandle)!=NULL)//if username save username to array
					{
						SaveUsername(buffer,i);
						//printf("%s\n",hangman.username[i]);
					}
					else if (strstr(buffer,specificUserStatsHandle)!=NULL)//send spec stats
					{
						SendStatistics(c_sockets[i],buffer+2);
						//SendSpecificUserStats(buffer,c_sockets[i]);
					}
					
					else if (strstr(buffer,gameMoveHandle)!=NULL)
					{
						ProcessGameMove(buffer,c_sockets[i],i);
					}
					
					else if (strstr(buffer,statHandle)!=NULL)//send user stats
					{
						SendStatistics(c_sockets[i],hangman.username[i]);
					}
					
					else if(strstr(buffer,allUserStatHandle)!=NULL)//all stats
					{
						int err = SendAllStatistics(c_sockets[i]);
					}
					
					else if (buffer[0]=='/')//menu sequences
					{
						if(strstr(buffer,quitHandle)!=NULL)
						{
							send(c_sockets[i],disconnectHandle,strlen(disconnectHandle),0);
							close(c_sockets[i]);
							c_sockets[i] = -1;
						}
					}
                }
            }
        }
    }

    return 0;
}
