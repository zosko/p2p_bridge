#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <stdbool.h>
#include <signal.h>


#define BUF_BEGIN 0
#define BUF_MAX 256
#define PORT 3311
#define MAX_USERS 5

typedef struct PeerClient{
    struct sockaddr_in cli_socket;
    char data[BUF_MAX];
}Peer;
struct sockaddr_in pair_socket;

void diep(char *s){
    perror(s);
    exit(1);
}
bool checkChannel(char a[], char b[]){
    int c = 0;
    while( a[c] == b[c] ){
        if(a[c] == '\0' || b[c] == '\0'){
            break;
        }
        c++;
    }
    if(a[c] == '\0' && b[c] == '\0'){
        return true;
    }
    else{
        return false;
    }
}
char *getCurrentTime(){
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char *returnTime = (char *)malloc(50);
    sprintf(returnTime, "%d.%d.%d %d:%d:%d", t->tm_mday, t->tm_mon+1, t->tm_year+1900, t->tm_hour, t->tm_min, t->tm_sec);
    return returnTime;
}
void logger(const char* format, ... ) {
    FILE *logDat = fopen("logDat.txt","ab+");
    fprintf(logDat,"[%s] ",getCurrentTime());
    
    va_list args;
    va_start(args, format);
    vfprintf(logDat,format,args);
    va_end(args);
    
    fprintf(logDat,"\n");
    fclose(logDat);
}
bool isChannelActive(Peer peer[],char data[],int placeNum){
    bool isChannelAlive = false;
    for(int j=0;j<MAX_USERS;j++){
        if(checkChannel(peer[j].data,data) && j!=placeNum){
            isChannelAlive = true;
            pair_socket = peer[j].cli_socket; //get socket of first peer
            memset(peer[j].data, '\0', BUF_MAX); //remove first peer data
        }
    }
    
    return isChannelAlive;
}
char *getMsg(const char* format, ... ){
    char *pairMsg = (char *)malloc(BUF_MAX);

    va_list args;
    va_start(args, format);
    vsnprintf(pairMsg, 255, format, args);
    va_end(args);
    
    return pairMsg;
}
void signal_callback(int signum){
    logger("[!!!!] P2P BRIDGE SERVER STOPPED [!!!!]");
    exit(signum);
}
int main(void){
    signal(SIGINT, signal_callback);

    struct sockaddr_in serv_socket;
    socklen_t slen = sizeof(struct sockaddr_in);
    int s;
    
    Peer peer[MAX_USERS];
    
    if((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) diep("socket");
    memset((char *) &serv_socket, 0, sizeof(serv_socket));
    serv_socket.sin_family = AF_INET;
    serv_socket.sin_port = htons(PORT);
    serv_socket.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(s, (struct sockaddr *)&serv_socket, sizeof(serv_socket))==-1) diep("bind");
    
    logger("==== P2P BRIDGE SERVER STARTED ====");
    
    for (int i=0; ;i = (i+1) % MAX_USERS) {

        if(recvfrom(s, peer[i].data, BUF_MAX, BUF_BEGIN, (struct sockaddr *)&peer[i].cli_socket, &slen)== -1) diep("recvfrom()");
        
        if(isChannelActive(peer,peer[i].data,i)){
            char *msgToPeer1 = getMsg("%s:%d",inet_ntoa(peer[i].cli_socket.sin_addr),ntohs(peer[i].cli_socket.sin_port));
            if(sendto(s, msgToPeer1, BUF_MAX, BUF_BEGIN, (struct sockaddr *)&pair_socket, slen) == -1) diep("sendto()");
            
            char *msgToPeer2 = getMsg("%s:%d",inet_ntoa(pair_socket.sin_addr),ntohs(pair_socket.sin_port));
            if(sendto(s, msgToPeer2, BUF_MAX, BUF_BEGIN, (struct sockaddr *)&peer[i].cli_socket, slen) == -1) diep("sendto()");
            
            logger("[PAIR] %s:%d <> %s [CHANNEL] %s ",inet_ntoa(peer[i].cli_socket.sin_addr),ntohs(peer[i].cli_socket.sin_port),msgToPeer2,peer[i].data);
            memset(peer[i].data, '\0', BUF_MAX);  //remove second peer data
        }
        else{
            logger("[WAITING] %s:%d [CHANNEL] %s ",inet_ntoa(peer[i].cli_socket.sin_addr),ntohs(peer[i].cli_socket.sin_port),peer[i].data);
        }
        
    }
    
    close(s);
    return 0;
}
