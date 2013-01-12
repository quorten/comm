all: winserver.exe client.exe owinserver.exe
winserver.exe: winserver.c
	gcc -D_WINDOWS -I. -L. winserver.c -lws2_32 -lpthreadGC2 -o winserver.exe
client.exe: client.c
	gcc -D_WINDOWS client.c -lws2_32 -o client.exe
owinserver.exe: owinserver.c
	gcc -D_WINDOWS owinserver.c -lws2_32 -o owinserver.exe
