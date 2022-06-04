#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define STARTPOS 6 // ftp:// ocupa 6 pos
#define h_addr h_addr_list[0]

int divideURL(const char *string);
int getHost(char *host);
int connectToServer(char *ip, int port);
int Login(int sockcom);
int readfromSocket(int sockcom, char *res);
int passMode(int sockcom);
int sendPath(int sockdata);
int downloadFile(int sockdata);
int closeConnection(int sockcom);

typedef struct
{
    char user[256];
    char password[256];
    char host[256];
    char ip[256];
    char path[256];
    char filename[256];
    int port;
} URL;

URL url;

int divideURL(const char *string)
{
    char aux[256];
    int pos, i;

    if (strncpy(aux, string, STARTPOS) == NULL)
    {
        return -1;
    }

    if (strcmp(aux, "ftp://") != 0)
    {
        return -1;
    }

    for (pos = STARTPOS; pos < 256; pos++)
    {
        if (string[pos] == '@')
        {
            printf("found\n");
            sscanf(string, "%*[^:]%*[:/]%[^:]:%[^@]@/%[^/]/%[^\n]", url.user, url.password, url.host, url.path);
            strcpy(aux, strchr(url.path, '/'));
            strcpy(url.filename, aux + 1);
            printf("User: %s\nPass: %s\nHost: %s\nPath: %s\nFile Name: %s\n", url.user, url.password, url.host, url.path, url.filename);
            return 0;
        }
    }

    strcpy(url.user, "anonymous");
    strcpy(url.password, "anonymous");
    sscanf(string, "%*[^:]%*[:/]%[^/]/%[^\n]", url.host, url.path);

    strcpy(aux, strrchr(url.path, '/'));

    strcpy(url.filename, aux + 1);
    printf("User: %s\nPass: %s\nHost: %s\nPath: %s\nFile Name: %s\n", url.user, url.password, url.host, url.path, url.filename);

    return 0;
}

int getHost(char *host)
{
    struct hostent *h = gethostbyname(host);

    if (h == NULL)
    {
        herror("gethostbyname");
        exit(-1);
    }
    strcpy(url.ip, inet_ntoa(*((struct in_addr *)h->h_addr)));
    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", url.ip);

    return 0;
}

int connectToServer(char *ip, int port)
{
    struct sockaddr_in server_addr;

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("socket()");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect()");
        return -1;
    }

    return sockfd;
}

int readfromSocket(int sockcom, char *res)
{
    FILE *file = fdopen(sockcom, "r");

    do
    {
        memset(res, '\0', sizeof(*res));
        res = fgets(res, 1024, file);
        printf("%s", res);

    } while (!('1' <= res[0] && res[0] <= '5') || res[3] != ' ');

    return 0;
}

int writeToSocket(int sockcom, char *buf)
{
    int bytes;
    if ((bytes = write(sockcom, buf, strlen(buf))) < strlen(buf))
    {
        printf("Error writing to socket\n");
        return -1;
    }

    return bytes;
}

int Login(int sockcom)
{
    char buf[512];
    int bytes;

    sprintf(buf, "user %s\n", url.user);

    if (writeToSocket(sockcom, buf) < 0)
    {
        return -1;
    }

    if (readfromSocket(sockcom, buf) < 0)
    {
        return -1;
    }

    memset(buf, '\0', sizeof(buf));

    sprintf(buf, "pass %s\n", url.password);

    if ((bytes = writeToSocket(sockcom, buf)) < 0)
    {
        printf("Bytes written: %d\n", bytes);
        return -1;
    }
    memset(buf, '\0', sizeof(buf));

    if (readfromSocket(sockcom, buf) < 0)
    {
        return -1;
    }
    if (buf[0] != '2' || buf[1] != '3' || buf[2] != '0')
    {
        return -1;
    }
    return 0;
}

int passMode(int sockcom)
{
    char pasv[] = "pasv\n";
    int ip1, ip2, ip3, ip4, port1, port2;
    char buf[1024];
    char IP[256];
    int newPort = 0, sockdata = 0;

    if (writeToSocket(sockcom, pasv) < 0)
    {
        return -1;
    }

    if (readfromSocket(sockcom, buf) < 0)
    {
        return -1;
    }

    if (buf[0] != '2' || buf[1] != '2' || buf[2] != '7')
    {
        return -1;
    }
    else
    {
        sscanf(buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip1, &ip2, &ip3, &ip4, &port1, &port2);

        sprintf(IP, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

        newPort = port1 * 256 + port2;

        printf("IP: %s\n", IP);
        printf("PORT: %d\n", newPort);

        sockdata = connectToServer(IP, newPort);

        return sockdata;
    }
}

int sendPath(int sockcom)
{

    char buf[1024];

    sprintf(buf, "retr ./%s\n", url.path);

    if (writeToSocket(sockcom, buf) < 0)
    {
        return -1;
    }
    memset(buf, '\0', sizeof(buf));

    if (readfromSocket(sockcom, buf) < 0)
    {
        return -1;
    }
    if (buf[0] != '1' || buf[1] != '5' || buf[2] != '0')
    {
        return -1;
    }

    return 0;
}

int downloadFile(int sockdata)
{

    FILE *file;
    int bytes;

    if ((file = fopen(url.filename, "w")) < 0)
    {
        printf("ERROR: Cannot open file.\n");
        return -1;
    }

    char buf[1024];

    while ((bytes = read(sockdata, buf, 1024)) > 0)
    {
        if ((fwrite(buf, bytes, 1, file)) < 0)
        {
            printf("ERROR: Cannot write data in file.\n");
            return -1;
        }
    }

    if (bytes < 0)
    {
        printf("ERROR: Nothing was received from data socket fd.\n");
        return -1;
    }

    fclose(file);
    close(sockdata);

    return 0;
}

int closeConnection(int sockcom)
{
    char buf[1024];
    sprintf(buf, "quit\r\n");

    if (writeToSocket(sockcom, buf) < 0)
    {
        printf("Error Sending QUIT Command.\n");
        return -1;
    }

    memset(buf, '\0', sizeof(buf));

    if (readfromSocket(sockcom, buf) < 0)
    {
        printf("Error Disconnecting Account.\n");
        return -1;
    }

    if (buf[0] != '2' || buf[1] != '2' || buf[2] != '6')
    {
        printf("Erro Transfering File");
        return -1;
    }

    memset(buf, '\0', sizeof(buf));

    if (readfromSocket(sockcom, buf) < 0)
    {
        return -1;
    }

    close(sockcom);

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Error no argv");
        return -1;
    }

    if (divideURL(argv[1]) < 0)
    {
        printf("Error getting URL");
        return -1;
    }

    if (getHost(url.host) < 0)
    {
        printf("Error IP from Host");
        return -1;
    }

    url.port = 21;
    int sockcom, sockdata;

    if ((sockcom = connectToServer(url.ip, url.port)) < 0)
    {
        printf("Error Connecting");
        return -1;
    }
    char buf[256];

    if (readfromSocket(sockcom, buf) < 0)
    {
        return -1;
    }

    if (Login(sockcom) < 0)
    {
        printf("Error Login");
        return -1;
    }

    if ((sockdata = passMode(sockcom)) < 0)
    {
        printf("Error Entering Passive Mode");
        return -1;
    }

    if (sendPath(sockcom) < 0)
    {
        printf("Error Sending Path");
        return -1;
    }
    printf("Send Path Done\n");

    if ((downloadFile(sockdata) < 0))
    {
        printf("Error Downloading File");
        return -1;
    }

    if (closeConnection(sockcom) < 0)
    {
        printf("Error Closing Connection");
        return -1;
    }
    printf("---CLOSED---\n");
}