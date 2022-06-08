CC=gcc
FILE=ftp_download.c
NAME=download
HOST=mirrors.up.pt
PATH=pub/debian
USER=anonymous
PASSWORD=anonymous
PROTOCOL=ftp

default:
	$(CC) $(FILE) -o $(NAME)

auth:
	./$(NAME) $(PROTOCOL)://[$(USER):$(PASSWORD)@]$(HOST)/$(PATH)/

no_auth:
	./$(NAME) $(PROTOCOL)://$(HOST)/$(PATH)/

rm:
	rm README.html