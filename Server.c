#include "Libraries.h"

#define MAXCLIENTS 10
#define maxWordLength 20
static const int lives = 5;

char* GenerateUserString(char* word)
{
	char* userString = malloc(strlen(word));
	for(int i=0;i<strlen(word)-1;i++)
	{
		*(userString+i) = '_';
	}
	*(userString+strlen(word)-1) = '\0';
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

    fd = fopen("WordBank.txt","r");
    while(fgets(currentWord,wordLength,fd)!=NULL)
    {
        strncpy(words[wordCount],currentWord,strlen(currentWord));
        wordCount++;
    }
    fclose(fd);
    //choosing random word
    static char *answer;
    answer = words[GetRandomNumber(wordCount)];
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

void SetupNewUser(int user_id)
{
	hangman.lives[user_id] = lives;
	
	char* tempWord = GetRandomWord();
	strcpy(hangman.word[user_id],tempWord);
	*(hangman.word[user_id]+strlen(hangman.word[user_id])-1) = '\0';// \n ->> \0
	
	char* tempUserString = GenerateUserString(hangman.word[user_id]);
	strcpy(hangman.userString[user_id],tempUserString);
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
	strncpy(hangman.username[user_id],username,(int)length);
	free(username);
	free(usernameLength);
}
int ProcessGameMove(char* buffer, int socket, int user_id)
{
	char* gameMove = malloc(10);
	strcpy(gameMove,buffer+strlen(gameMoveHandle));//removes gameHandle
	if(strlen(gameMove)>1) return 1;//too long
	char ch = *gameMove;
	if(!isalpha(ch)) return 2;//not letter
	if(strchr(hangman.usedLetters[user_id],ch)!=NULL) return 3;//letter already used
	
	char* tempGuessString = malloc(strlen(hangman.userString[user_id]));
	strcpy(tempGuessString,hangman.userString[user_id]);
	for(int i=0;i<strlen(tempGuessString);i++)
	{
		if(*(hangman.word[user_id]+i)==tolower(ch))
		{
			*(hangman.userString[user_id]+i) = tolower(ch);
		}
	}
	if(strcmp(hangman.userString[user_id],tempGuessString)==0)
	{
		hangman.lives[user_id]--;
		char* livesMsg = malloc(5);
		char* livesLeft = malloc(5);
		strcpy(livesMsg,livesHandle);
		sprintf(livesLeft,"%d",hangman.lives[user_id]);
		strcat(livesMsg,livesLeft);
		send(socket,livesMsg,strlen(livesMsg),0);
			puts(livesMsg);
		
		free(livesLeft);
		free(livesMsg);
	}
	send(socket,hangman.userString[user_id],strlen(hangman.userString[user_id]),0);
		puts(hangman.userString[user_id]);
	*(hangman.usedLetters[user_id]+hangman.usedLetterCounter[user_id]) = tolower(ch);
	hangman.usedLetterCounter[user_id]++;
	free(tempGuessString);
	free(gameMove);
	return 0;
}
void DisconnectUser(int socket)
{
	close(socket);
	socket = -1;
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
    }
*/

    //port = atoi(argv[1]);
	port = atoi("7896");
	/*
    if ((port < 1) || (port > 65535)){
        fprintf(stderr, "ERROR #1: invalid port specified.\n");
        return -1;
    }*/

    if ((l_socket = socket(AF_INET, SOCK_STREAM,0)) < 0){
        fprintf(stderr, "ERROR #2: cannot create listening socket.\n");
        return -1;
    }

    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(port);

    if (bind (l_socket, (struct sockaddr *)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"ERROR #3: bind listening socket.\n");
        return -1;
    }

    if (listen(l_socket, 5) <0){
        fprintf(stderr,"ERROR #4: error in listen().\n");
        return -1;
    }                           

    for (i = 0; i < MAXCLIENTS; i++){
        c_sockets[i] = -1;
    }


    while(1){
        FD_ZERO(&read_set);
        for (i = 0; i < MAXCLIENTS; i++){
            if (c_sockets[i] != -1){
                FD_SET(c_sockets[i], &read_set);
				maxfd = (c_sockets[i] > maxfd) ? c_sockets[i] : maxfd;
            }
        }

        FD_SET(l_socket, &read_set);
		maxfd = (l_socket > maxfd) ? l_socket : maxfd;
        
        select(maxfd+1, &read_set, NULL , NULL, NULL);

        if (FD_ISSET(l_socket, &read_set)){
            int client_id = findemptyuser(c_sockets);
            if (client_id != -1){
                clientaddrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, clientaddrlen);
                c_sockets[client_id] = accept(l_socket, 
                    (struct sockaddr*)&clientaddr, &clientaddrlen);
				SetupNewUser(client_id);
                printf("Connected: %s\n",inet_ntoa(clientaddr.sin_addr));
            }
        }
        for (i = 0; i < MAXCLIENTS; i++){
            if (c_sockets[i] != -1){
                if (FD_ISSET(c_sockets[i], &read_set)){
                    memset(&buffer,0,BUFFLEN);
                    int r_len = recv(c_sockets[i],&buffer,BUFFLEN,0);
					int w_len;
					
					if(strstr(buffer,usernameHandle)!=NULL)//if username save username to array
					{
						SaveUsername(buffer,i);
						printf("%s\n",hangman.username[i]);
					}
					
					else if (strstr(buffer,gameMoveHandle)!=NULL)
					{
						ProcessGameMove(buffer,c_sockets[i],i);
					}
					
					else if (buffer[0]=='/')//menu sequences
					{
						if(strstr(buffer,quitHandle)!=NULL)
						{
							//DisconnectUser(c_sockets[i]);
							send(c_sockets[i],disconnectHandle,strlen(disconnectHandle),0);
							/*segmentation fault(core dumped)
							hangman.lives[i] = 0;
							strcpy(hangman.word[i],NULL);
							strcpy(hangman.userString[i],NULL);
							strcpy(hangman.username[i],NULL);*/
							/*
							hangman.word[i] = NULL;
							hangman.userString[i] = NULL;
							hangman.username[i] = NULL;*/
							close(c_sockets[i]);
							c_sockets[i] = -1;
						}
					}
					else w_len = send(c_sockets[i], buffer, strlen(buffer),0);

					/*
					if(w_len<=0)
					{		
						DisconnectUser(c_sockets[i]);
						//close(c_sockets[i]);
						//c_sockets[i] = -1;
					}*/

                }
            }
        }
    }

    return 0;
}
