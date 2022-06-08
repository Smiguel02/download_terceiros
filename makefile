CC=gcc
FILE=ftp_download.c
NAME=download
HOST=mirrors.up.pt
CAMINHO=pub/debian/README.html
USER=anonymous
PASSWORD=anonymous
PROTOCOL=ftp

default:
	$(CC) $(FILE) -o $(NAME)

auth:
	./$(NAME) $(PROTOCOL)://[$(USER):$(PASSWORD)@]$(HOST)/$(CAMINHO)

no_auth:
	./$(NAME) $(PROTOCOL)://$(HOST)/$(CAMINHO)

rm:
	rm README.html

test_1:
	./$(NAME) $(PROTOCOL)://$(HOST)/pub/kodi/screenshots/kodi-epg.jpg
