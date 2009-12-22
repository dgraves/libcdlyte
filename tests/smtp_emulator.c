#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define socket_t int
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define closesocket close
#else
#include <winsock2.h>
#include <windows.h>
#define socket_t SOCKET
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#define snprintf _snprintf
#endif

int live=1;
jmp_buf env;

void sighandler(int s)
{
  if(s==SIGINT)
  {
    live=0;
    longjmp(env,1);
  }
}

int readline(socket_t client,char *line,int size)
{
  char c;
  int bytes=0;
  do
  {
    if(recv(client,&c,1,0)!=1)
      return SOCKET_ERROR;
    if(bytes<size-1) line[bytes++]=c;
  } while(c!='\n');

  line[bytes]='\0';
  return bytes;
}

int main(int argc,char **argv)
{
  char line[BUFSIZ],data[BUFSIZ],*p,*e;
  short port;
  int size,bytes,result;
  socket_t server,client;
  struct sockaddr_in sin;

  /* Listen for connections on user supplied port or port 8080 */
  if(argc>2)
  {
    printf("usage: %s [port]\n",argv[0]);
    return 0;
  }

  port=(argc==2)?atoi(argv[1]):2525;  /* port 25 can only be bound by root */

  sin.sin_family=AF_INET;
  sin.sin_addr.s_addr=htonl(INADDR_ANY);
  sin.sin_port=htons(port);

  if((server=socket(AF_INET,SOCK_STREAM,0))==INVALID_SOCKET)
  {
    printf("Could not create socket.\n");
    return 0;
  }

  if(bind(server,(struct sockaddr *)&sin,sizeof(sin))==SOCKET_ERROR)
  {
    printf("Could not bind socket to port %d.\n",port);
    return 0;
  }

  if(listen(server,5)==SOCKET_ERROR)
  {
    printf("Unable to listen for incoming connections.\n");
    return 0;
  }

  signal(SIGINT,sighandler);
  setjmp(env);
  while(live)
  {
    printf("\nemulated smtp server listening on port %d\n",port);

    size=sizeof(sin);
    if((client=accept(server,(struct sockaddr *)&sin,&size))==SOCKET_ERROR)
    {
      printf("Error accepting connection.\n");
      continue;
    }

    printf("accepted connection from %s on port %d\n",inet_ntoa(sin.sin_addr),sin.sin_port);
    printf("receiving data:\n\n");

    /* Hand shake */
    strncpy(line,"220 SMTP emulator\r\n",BUFSIZ);
    printf("<-- %s",line);
    if(send(client,line,strlen(line),0)==SOCKET_ERROR)
    {
      printf("\n\nError sending data to client.\n");
      goto readerror;
    }
    if(readline(client,line,BUFSIZ)==SOCKET_ERROR)
    {
      printf("\n\nError reading data from client.\n");
      goto readerror;
    }
    if(strncmp(line,"HELO ",5)!=0)
    {
      printf("\n\nInvalid SMTP command.\n");
      goto readerror;
    }
    printf("--> %s",line);

    p=strchr(line,' ');
    if((e=strrchr(line,'\r'))==NULL) e=strrchr(line,'\n');
    *e='\0';
    strncpy(data,++p,BUFSIZ);
    snprintf(line,BUFSIZ,"250 Hello %s, pleased to meet you\r\n",data);
    printf("<-- %s",line);
    if(send(client,line,strlen(line),0)==SOCKET_ERROR)
    {
      printf("\n\nError sending data to client.\n");
      goto readerror;
    }
    if(readline(client,line,BUFSIZ)==SOCKET_ERROR)
    {
      printf("\n\nError reading data from client.\n");
      goto readerror;
    }
    if(strncmp(line,"MAIL FROM:",10)!=0)
    {
      printf("\n\nInvalid SMTP command.\n");
      goto readerror;
    }
    printf("--> %s",line);

    p=strchr(line,'<');
    if((e=strrchr(line,'\r'))==NULL) e=strrchr(line,'\n');
    *e='\0';
    strncpy(data,p,BUFSIZ);
    snprintf(line,BUFSIZ,"250 2.1.0 %s... Sender ok\r\n",data);
    printf("<-- %s",line);
    if(send(client,line,strlen(line),0)==SOCKET_ERROR)
    {
      printf("\n\nError sending data to client.\n");
      goto readerror;
    }
    if(readline(client,line,BUFSIZ)==SOCKET_ERROR)
    {
      printf("\n\nError reading data from client.\n");
      goto readerror;
    }
    if(strncmp(line,"RCPT TO:",8)!=0)
    {
      printf("\n\nInvalid SMTP command.\n");
      goto readerror;
    }
    printf("--> %s",line);

    p=strchr(line,'<');
    if((e=strrchr(line,'\r'))==NULL) e=strrchr(line,'\n');
    *e='\0';
    strncpy(data,p,BUFSIZ);
    snprintf(line,BUFSIZ,"250 2.1.5 %s... Recipient ok\r\n",data);
    printf("<-- %s",line);
    if(send(client,line,strlen(line),0)==SOCKET_ERROR)
    {
      printf("\n\nError sending data to client.\n");
      goto readerror;
    }
    if(readline(client,line,BUFSIZ)==SOCKET_ERROR)
    {
      printf("\n\nError reading data from client.\n");
      goto readerror;
    }
    if(strncmp(line,"DATA",4)!=0)
    {
      printf("\n\nInvalid SMTP command.\n");
      goto readerror;
    }
    printf("--> %s",line);

    strncpy(line,"354 Enter mail, end with \".\" on a line by itself\r\n",BUFSIZ);
    printf("<-- %s",line);
    if(send(client,line,strlen(line),0)==SOCKET_ERROR)
    {
      printf("\n\nError sending data to client.\n");
      goto readerror;
    }

    /* Read header */
    do
    {
      if(readline(client,line,BUFSIZ)==SOCKET_ERROR)
      {
        printf("\n\nError reading data from client.\n");
        goto readerror;
      }
      printf("%s",line);
    } while(strcmp(line,"\r\n")!=0);

    /* Read data */
    bytes=0;
    do
    {
      if((result=readline(client,line,BUFSIZ))==SOCKET_ERROR)
      {
        printf("\n\nError reading data from client.\n");
        goto readerror;
      }
      bytes+=result;
      printf("%s",line);
    } while(strcmp(line,".\n")!=0);

    printf("\nsubmission received (%d bytes); sending respone:\n\n",bytes);

    strncpy(line,"250 Message accepted for delivery\r\n",BUFSIZ);
    printf("<-- %s",line);
    if(send(client,line,strlen(line),0)==SOCKET_ERROR)
    {
      printf("\n\nError sending data to client.\n");
      goto readerror;
    }
    if(readline(client,line,BUFSIZ)==SOCKET_ERROR)
    {
      printf("\n\nError reading data from client.\n");
      goto readerror;
    }
    if(strncmp(line,"QUIT",4)!=0)
    {
      printf("\n\nInvalid SMTP command.\n");
      goto readerror;
    }
    printf("--> %s",line);

    strncpy(line,"221 SMTP emulator closing connection\r\n",BUFSIZ);
    printf("<-- %s",line);
    if(send(client,line,strlen(line),0)==SOCKET_ERROR)
    {
      printf("\n\nError sending data to client.\n");
      goto readerror;
    }

readerror:
    closesocket(client);
  }

  printf("Stoping server\n");
  closesocket(server);
  return 0;
}
