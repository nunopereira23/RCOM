#! FTP Client
ftpDownloader: urlParser.c urlParser.h ftpDownloader.c ftpDownloader.h
	gcc -Wall urlParser.c ftpDownloader.c -o ftpDownloader
debug: urlParser.c urlParser.h ftpDownloader.c ftpDownloader.h
	gcc -Wall -g urlParser.c ftpDownloader.c -o ftpDownloader

clean:
	rm -f ftpDownloader
