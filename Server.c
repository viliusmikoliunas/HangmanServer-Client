#include "Libraries.h"
#define MAXCLIENTS 10
#define maxWordLength 30
static const int lives = 5;
static const char* wordbankPath = "WordBank.txt";
static const char* statisticsPath = "Statistics.txt";
static const char* tempFilePath = "TempBuff.txt";

char* GenerateUserString(char* word)
{
	char* userString = malloc(maxWordLength);
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
    fd = fopen(wordbankPath,"r");
	
    char currentWord[maxWordLength];
    char words[300][maxWordLength];
    int wordCount=0;
    while(fgets(currentWord,maxWordLength,fd)!=NULL)
    {
        strcpy(words[wordCount],currentWord);
        wordCount++;
    }
    fclose(fd);
    //choosing random word
    char *answer = malloc(maxWordLength);
	strcpy(answer,words[GetRandomNumber(wordCount)]);
	char *p = strchr(answer,'\n');
	*p = '\0';
    return answer;
}

int FindEmptyUser(int c_sockets[]){
    for (int i = 0; i <  MAXCLIENTS; i++)
	{
        if (c_sockets[i] == -1)
		{
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

void SetupUser(int user_id)
{
	hangman.lives[user_id] = lives;
	
	char* tempWord = GetRandomWord();
	strcpy(hangman.word[user_id],tempWord);
	
	char* tempUserString = GenerateUserString(hangman.word[user_id]);
	strcpy(hangman.userString[user_id],tempUserString);
	
	memset(hangman.usedLetters[user_id],0,sizeof(hangman.usedLetters[user_id]));
	hangman.usedLetterCounter[user_id]=0;
	
	free(tempUserString);
	free(tempWord);
}

void SaveUsername(char* buffer, int user_id)
{	//buffer = U:username
	buffer+=strlen(usernameHandle);
	strcpy(hangman.username[user_id],buffer);
}

void SplitFileLine(char* line)
{
	char* currentWord = line;
	char* nextWord = strchr(line,'|');
	*nextWord = '\0';
	char* username = malloc(maxUsernameLength);
	strcpy(username,currentWord);
	statistics.username = username;

	currentWord = nextWord+1;
	nextWord = strchr(currentWord,'|');
	*nextWord = '\0';
	char* wins = malloc(5);
    strcpy(wins,currentWord);
	statistics.wins = wins;

	currentWord = nextWord+1;
	nextWord = strchr(currentWord,'\n');
	*nextWord = '\0';
	char* losses = malloc(5);
	strcpy(losses,currentWord);
	statistics.losses = losses;
}

char* AssembleFileString(char* username, char* wins, char* losses)
{
    char* line = malloc(maxUsernameLength+15);
    strcpy(line,username);
    strcat(line,"|");
    strcat(line,wins);
    strcat(line,"|");
    strcat(line,losses);
    strcat(line,"\0\n");

    return line;
}
char* UpdateStatistics(char* line, bool gameStatus)//username|wins|losses\n
{
	SplitFileLine(line);
	
	//updating number
    if(gameStatus)//game won - wins++
    {
        long number = strtol(statistics.wins,NULL,10);
        number++;
        sprintf(statistics.wins,"%d",number);
    }
    else//game lost - losses++
    {
        long number = strtol(statistics.losses,NULL,10);
        number++;
        sprintf(statistics.losses,"%d",number);
    }
    return AssembleFileString(statistics.username,statistics.wins,statistics.losses);
}
void CopyTextFile(FILE *dest, FILE* source)
{
    char c;
    rewind(source);
    rewind(dest);

    while((c=fgetc(source))!=EOF)
    {
        fputc(c,dest);
    }
}
void SaveStatistics(char* username, bool gameStatus)
{	//gamestatus: true - won, false - lost
    FILE *fd,*fb;
    int maxLineLength = maxWordLength + 15;
    char* currentFileLine = malloc(maxLineLength);
    fd = fopen(statisticsPath,"r+");
    fb = fopen(tempFilePath,"w+");
    bool userStatsUpdated = false;

    while(fgets(currentFileLine,maxLineLength,fd))
    {
		char* usernameFromCurrentLine = currentFileLine;
		char* temp = strchr(usernameFromCurrentLine,'|');
		*temp = '\0';
		if(strcmp(usernameFromCurrentLine,username)==0)
        {
			*temp = '|';
            char* updatedLine = UpdateStatistics(currentFileLine,gameStatus);
            fprintf(fb,"%s\r\n",updatedLine);
            userStatsUpdated = true;
			
			free(updatedLine);
			free(statistics.username);
			free(statistics.wins);
			free(statistics.losses);
        }
        else
        {
			*temp = '|';
            fprintf(fb,"%s",currentFileLine);
        }
    }
	free(currentFileLine);
    if(!userStatsUpdated)
    {
        char* newLine;
        if(gameStatus)
        {
            newLine = AssembleFileString(username,"1","0");
        }
        else
        {
            newLine = AssembleFileString(username,"0","1");
        }
        fprintf(fb,"%s\r\n",newLine);
		free(newLine);
    }
	CopyTextFile(fd,fb);
	fclose(fb);
	fclose(fd);
}

int ProcessGameMove(char* buffer, int socket, int user_id)//needs division
{
	char* gameMove = malloc(10);
	if(strstr(buffer,"\n")!=NULL)
	{
		char *p = strchr(buffer,'\n');
		*p = '\0';
	}
	strcpy(gameMove,buffer+strlen(gameMoveHandle));//removes gameHandle
	char ch = *gameMove;
	
	if(strlen(gameMove)>1) return 1;//too long
	if(!isalpha(ch)) return 2;//not letter	
	if(strchr(hangman.usedLetters[user_id],ch)!=NULL) return 3;//letter already used

	char* tempUserString = malloc(maxWordLength);//strlen(hangman.userString[user_id])
	strcpy(tempUserString,hangman.userString[user_id]);
	
	for(int i=0;i<strlen(tempUserString);i++)//reveal letters
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
		SetupUser(user_id);
		return 0;
	}
	
	else if(strcmp(hangman.userString[user_id],tempUserString)==0)//if letter is not in word
	{
		hangman.lives[user_id]--;
		if(hangman.lives[user_id]<=0)//game lost
		{
			send(socket,gameLostHandle,strlen(gameLostHandle),0);
			SaveStatistics(hangman.username[user_id],0);
			SetupUser(user_id);
			return 0;
		}
		//send lives left
		char* livesMsg = malloc(10);
		char* livesLeft = malloc(5);
		strcpy(livesMsg,livesHandle);
		sprintf(livesLeft,"%d",hangman.lives[user_id]);
		strcat(livesMsg,livesLeft);
		send(socket,livesMsg,strlen(livesMsg),0);

		free(livesLeft);
		free(livesMsg);
	}
	else //send user string _________
	{
		char* messageToSend = malloc(50);
		strcpy(messageToSend,userStringHandle);
		strcat(messageToSend,hangman.userString[user_id]);
		send(socket,messageToSend,strlen(messageToSend),0);
		free(messageToSend);
	}
	
	*(hangman.usedLetters[user_id]+hangman.usedLetterCounter[user_id]) = tolower(ch);
	hangman.usedLetterCounter[user_id]++;
	free(tempUserString);
	free(gameMove);
	return 0;
}

void SendStatistics(int socket, char* username)//<---------------------TBC
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
			SplitFileLine(currentLine);
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
		strcat(messageToSend,"-\0");
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
		strcat(allStatistics,"-\0");
	}
	else
	{
		strcat(allStatistics,"\0");
	}
	send(socket,allStatistics,strlen(allStatistics),0);
	free(allStatistics);
	return 0;
}

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
	
    if ((port < 1024) || (port > 65535))
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
            int client_id = FindEmptyUser(c_sockets);
            if (client_id != -1)
			{
                clientaddrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, clientaddrlen);
                c_sockets[client_id] = accept(l_socket, 
                    (struct sockaddr*)&clientaddr, &clientaddrlen);
				SetupUser(client_id);
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
					printf("User \"%s\" sends:%s\n",hangman.username[i],buffer);
					
					if(strstr(buffer,usernameHandle)!=NULL)//if just connected, save username to array
					{
						SaveUsername(buffer,i);
					}
					else if (strstr(buffer,specificUserStatsHandle)!=NULL)//send spec stats
					{
						SendStatistics(c_sockets[i],buffer+2);
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
