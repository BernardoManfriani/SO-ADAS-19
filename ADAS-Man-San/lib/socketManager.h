// look: ifndef DA METTERE ?
#define DEFAULT_PROTOCOL 0

extern int readSocket(int fd, char *str);
extern void writeSocket(int socketFd, char *data);
extern int connectClient(char *socketName);