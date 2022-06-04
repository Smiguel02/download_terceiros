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
		printf("User->%s\nPassword.>%s\nHost->%s\nPath->%s\n", url.user, url.password, url.host, url.path);
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

	while ((string[i] != parameter) || string[i] == '\0')
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

	char string[256];
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
	strcpy(url_string, &url_string[BEGGINING]);

	// if not is 0 by default
	if (url_string[0] == '[')
	{
		authentication = TRUE;
		strcpy(url_string, &url_string[1]);
	}

	switch (authentication)
	{
	case TRUE:
		read_substring(url_string, ':', url.user);
		read_substring(url_string, '@', url.password);
		strcpy(url_string, &url_string[1]);
		read_substring(url_string, '/', url.host);
		read_substring(url_string, '/', url.path); // resolver issue reading path
		break;

	case FALSE:
		printf("Entered no auth\n");
		read_substring(url_string, '/', url.host);
		read_substring(url_string, '/', url.path);
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
	strcpy(url.filename, DOWNLOAD);
	print_url();

	return 0;
}

int begin_connection(int port, char *ip)
{
	struct sockaddr_in server_addr;

	/*server address handling*/
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);			 /*server TCP port must be network byte ordered */

	int sockfd;

	printf("Opening Socket\n");
	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket()");
		exit(0);
	}

	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("connect()");
		exit(0);
	}

	return sockfd;
}

int passive_mode(int com_socket)
{
	char buf[256], aux[256];
	int cnt = 0, data_socket;

	memset(buf, '\0', sizeof(buf)); // reinicia buffer
	strcpy(buf, "pasv");
	send(com_socket, buf, sizeof(buf), 0);
	sleep(0.001);
	recv(com_socket, buf, sizeof(buf), 0);
	printf("Pas buf->%s\n", buf);
	printf("Sent pasv successfully\n");

	// Reads port that passive mode is connected to
	while (cnt < 5)
	{
		read_substring(buf, ',', aux);
		cnt++;
	}
	printf("First value -> %s\n", aux);
	data_socket = atoi(aux) * 256;
	read_substring(buf, ')', aux);
	data_socket += atoi(aux);

	data_socket = begin_connection(data_socket, url.ip);

	return data_socket;
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
	int com_socket, data_socket, sockfd, res;

	com_socket = begin_connection(url.port, url.ip);

	// LOGIN
	// sends user
	memset(buf, '\0', sizeof(buf));	   // reinicia buffer
	sprintf(buf, "user %s", url.user); // correct
	res = send(com_socket, buf, sizeof(buf), 0);

	// sends password
	memset(buf, '\0', sizeof(buf)); // reinicia buffer
	sprintf(buf, "pass %s", url.password);
	send(com_socket, buf, sizeof(buf), 0);

	// Opens file to print to
	FILE *f = fopen(url.filename, "w");

	// goes to path file
	memset(buf, '\0', sizeof(buf)); // reinicia buffer
	sprintf(buf, "cwd %s/%s", url.path, url.filename);
	send(com_socket, buf, sizeof(buf), 0);

	printf("Entering passive mode\n");
	data_socket = passive_mode(com_socket);
	memset(buf, '\0', sizeof(buf)); // reinicia buffer
	sprintf(buf, "retr %s", url.filename);
	send(com_socket, buf, sizeof(buf), 0);

	printf("Receiving data\n");
	// receives everything
	while (recv(data_socket, buf, sizeof(buf), 0))
	{
		fprintf(f, "%s", buf);
		printf("%s", buf);
	}

	// closes file
	free(f); // might need to delete this lol
	fclose(f);

	// close connection
	close(data_socket);
	close(com_socket);
}