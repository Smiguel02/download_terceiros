/**
 * @file ftp_download.c
 * @author Miguel&Maria
 * @brief
 * @version 0.1
 * @date 2022-06-05
 *
 * @copyright Copyright (c) 2022
 *
 */

/***
 * Learnings:
 *  - To send stuff, need to put \n to simulate ENTER
 *  - Need to sleep before receiving, 0.1s isnt enouh
 *  - We dont receive the things we send, just in case, it was verificated for fun ahaha
 *
 *
 * */

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

#define TRUE 1
#define FALSE 0

#define PORT 21
#define USER "anonymous"
#define PASSWORD "anonymous"
#define BEGGINING 6
#define DOWNLOAD "README.html"

typedef struct
{
	char user[256];
	char password[256];
	char host[256];
	char ip[256];
	char path[256];
	char filename[256];
	int port;
	char host_name[256];
} URL;

URL url;
int authentication = FALSE;

void print_url()
{
	switch (authentication)
	{
	case TRUE:
		printf("URL read:\n");
		printf("User->%s\nPassword->%s\nHost->%s\nPath->%s\n", url.user, url.password, url.host, url.path);
		break;

	case FALSE:
		printf("URL read:\n");
		printf("Host->%s\nPath->%s\n", url.host, url.path);
		break;
	}
	printf("FileName  : %s\n", url.filename);
	printf("Host name  : %s\n", url.host_name);
	printf("IP Address : %s\n", url.ip);
	printf("Port  : %d\n", url.port);
	return;
}

// Reads desirable to new and updates string
void read_substring(char *string, char parameter, char *new)
{

	int i = 0;
	while ((string[i] != parameter) && (string[i] != '\0'))
	{
		new[i] = string[i];
		i++;
	}
	if (parameter != '\0')
	{
		strcpy(&string[0], &string[i + 1]);
		return;
	}

	strcpy(&string[0], &string[i]);
	return;
}

int define_url(char *url_string)
{
	// to make sure last byte is '\0'
	url_string[strlen(url_string)] = '\0';

	char string[512];
	int beggining = 0, aux = 0;
	int i = 0;

	// verify url
	strncpy(string, url_string, BEGGINING);
	if (strcmp(string, "ftp://") != 0)
	{
		printf("No ftp:// included\n");
		return -1;
	}

	// works just fine

	url_string += 6;

	// if not is 0 by default
	if (url_string[0] == '[')
	{
		authentication = TRUE;
		url_string++;
	}

	switch (authentication)
	{
	case TRUE:
		read_substring(url_string, ':', url.user);
		read_substring(url_string, '@', url.password);
		strcpy(url_string, &url_string[1]);
		read_substring(url_string, '/', url.host);
		read_substring(url_string, '\0', url.path); // resolver issue reading path

		break;

	case FALSE:
		read_substring(url_string, '/', url.host);
		read_substring(url_string, '\0', url.path);
		strcpy(url.user, "anonymous");
		strcpy(url.password, "anonymous");

		break;
	}

	struct hostent *h = gethostbyname(url.host);

	if (h == NULL)
	{
		printf("Failed to gethostbyname\n");
		return -1;
	}
	strcpy(url.ip, inet_ntoa(*((struct in_addr *)h->h_addr_list[0])));
	strcpy(url.host_name, h->h_name);
	url.port = PORT;
	memset(url.filename, '\0', sizeof(url.filename)); // reinicia buffer
	strcpy(url.filename, DOWNLOAD);
	print_url();

	return 0;
}

int begin_connection(int port, char *ip)
{
	struct sockaddr_in server_addr;

	int sockfd = 0;

	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);			 /*server TCP port must be network byte ordered */
	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket()");
		return -1;
	}
	/*connect to the server*/
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("connect()");
		return -1;
	}
	printf("Connected successfuly\n");
	return sockfd;
}

int read_socket(int sockfd, char *buf, int n, char *command)
{

	do
	{
		sleep(0.1);
		memset(buf, '\0', sizeof(buf)); // reinicia buffer
		recv(sockfd, buf, n, MSG_DONTWAIT);
		printf("%s", buf);
	} while (strstr(buf, command) == NULL);
}

int passive_mode(int com_socket)
{
	char buf[256], aux[256];
	int cnt = 0, data_socket;
	int port, lenght;

	memset(buf, '\0', sizeof(buf)); // reinicia buffer
	strcpy(buf, "pasv\r\n");
	lenght = sizeof(buf);
	send(com_socket, buf, lenght, 0);
	printf("%s", buf);

	memset(buf, '\0', sizeof(buf)); // reinicia buffer
	while (!recv(com_socket, buf, sizeof(buf), 0))
	{
		printf("Did not receive Passive mode info\n");
		sleep(1);
	}

	if (strstr(buf, "227") == NULL)
	{
		printf("Error reading passive mode\n");
		return -1;
	}
	printf("%s", buf);

	// Reads port that passive mode is connected to, working
	while (cnt < 5)
	{
		memset(aux, '\0', sizeof(aux)); // reinicia buffer
		read_substring(buf, ',', aux);
		cnt++;
	}
	printf("First value : %s\n", aux);
	port = atoi(aux) * 256;
	memset(aux, '\0', sizeof(aux)); // reinicia buffer
	read_substring(buf, ')', aux);
	printf("Second value : %s\n", aux);
	port += atoi(aux);
	printf("Data socket Port: %d\n", port);

	return begin_connection(port, url.ip);
}

int passive_mode_2(int com_socket)
{
	char buf[256], aux[256];
	int cnt = 0, data_socket;
	int port, lenght;

	memset(buf, '\0', sizeof(buf)); // reinicia buffer
	while (!recv(com_socket, buf, sizeof(buf), 0))
	{
		printf("Did not receive Passive mode info\n");
		sleep(1);
	}

	if (strstr(buf, "227") == NULL)
	{
		printf("Error reading passive mode\n");
		return -1;
	}
	printf("%s", buf);

	// Reads port that passive mode is connected to, working
	while (cnt < 5)
	{
		memset(aux, '\0', sizeof(aux)); // reinicia buffer
		read_substring(buf, ',', aux);
		cnt++;
	}
	printf("First value : %s\n", aux);
	port = atoi(aux) * 256;
	memset(aux, '\0', sizeof(aux)); // reinicia buffer
	read_substring(buf, ')', aux);
	printf("Second value : %s\n", aux);
	port += atoi(aux);
	printf("Data socket Port: %d\n", port);

	return begin_connection(port, url.ip);
}

int read_file(int sockfd, char *filename)
{
	char buf[1024] = {0};
	int lenght = 0, cnt = 0;
	char new_buf[1024] = {0};

	// Opens file to print to
	FILE *f = fopen(filename, "w");

	if (f == NULL)
	{
		printf("Failed to open file\n");
		return -1;
	}

	while (lenght = recv(sockfd, buf, sizeof(buf), 0))
	{
		fprintf(f, "%s", buf);
		cnt += lenght;
		memset(buf, '\0', sizeof(buf));
	}
	free(f);
	fclose(f);
	printf("Read %d Bytes from file\n", cnt);
	return 0;
}

int main(int argc, char *argv[])
{

	char buf[1024];

	if (argc != 2)
	{
		printf("Error receiving from compilation");
		return -1;
	}

	if (define_url(argv[1]) < 0)
	{
		printf("What\n");
		return -1;
	}

	// initiate socket files
	int com_socket, data_socket, res, lenght;

	com_socket = begin_connection(url.port, url.ip);

	// sends user
	sleep(1); // otherwise doesnt work
	read_socket(com_socket, buf, sizeof(buf), "220");
	memset(buf, '\0', sizeof(buf));		   // reinicia buffer
	sprintf(buf, "user %s\r\n", url.user); // correct
	lenght = strlen(buf);
	res = send(com_socket, buf, lenght, 0);
	printf("%s", buf);
	read_socket(com_socket, buf, sizeof(buf), "331");

	// sends password
	memset(buf, '\0', sizeof(buf)); // reinicia buffer
	sprintf(buf, "pass %s\r\n", url.password);
	lenght = strlen(buf);
	res = send(com_socket, buf, lenght, 0);
	printf("%s", buf);
	read_socket(com_socket, buf, sizeof(buf), "230");

	// goes to path file
	memset(buf, '\0', sizeof(buf)); // reinicia buffer
	sprintf(buf, "cwd %s\r\n", url.path);
	lenght = strlen(buf);
	send(com_socket, buf, lenght, 0);
	printf("%s", buf);
	read_socket(com_socket, buf, sizeof(buf), "250");

	// enters passive mode
	data_socket = passive_mode(com_socket);

	memset(buf, '\0', sizeof(buf)); // reinicia buffer
	sprintf(buf, "retr %s\r\n", url.filename);
	lenght = strlen(buf);
	send(com_socket, buf, lenght, 0);
	printf("%s", buf);
	read_socket(com_socket, buf, sizeof(buf), "\n");

	memset(buf, '\0', sizeof(buf)); // reinicia buffer
	sprintf(buf, "retr %s\r\n", url.filename);
	lenght = strlen(buf);
	send(com_socket, buf, lenght, 0);
	printf("%s", buf);

	// receives everything
	read_file(data_socket, url.filename);

	// close connection
	close(com_socket);
}