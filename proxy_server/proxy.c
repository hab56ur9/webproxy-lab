#include <stdio.h>
#include <csapp.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct {
  int called_cnt;
  char* filename;
  char* buf;
  int obj_size;
}object_w;


/************************************************/
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
void echo( int fd); // 연결할 아이디와 값을 저장할 버퍼 
int parse_uri(char *uri, char *filename, char *cgiargs); // filename은 여기서 최초로 값이 할당됨.
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,char *longmsg);


int cache(char* filename);
void move_a2b(int fd_a, int fd_b);

static int cached_size = 0;
int add_cache();

/************************************************/



int main(int argc, char**argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  /************************************************/
  char buf[MAX_CACHE_SIZE];

  /************************************************/
  /* Check command line args */
  // if (argc != 2) {
  //   fprintf(stderr, "usage: %s <port>\n", argv[0]);
  //   exit(1);
  // }

  //listenfd = Open_listenfd(argv[1]);
  listenfd = Open_listenfd("4000");
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,&clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    echo(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

void echo( int client_fd) // 연결할 아이디와 값을 저장할 버퍼 
{
  int is_static;
  char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
  char filename[MAXLINE],cgiargs[MAXLINE];
  rio_t rio;

  /****************************************************************/
  // 클라이언트의 요청에서 첫번째 줄을 읽어와 uri와 기타 정보 추출
  // 
  //Read request line and headers
  // Rio_readinitb(&rio,client_fd); //클라이언트의 읽기 초기화
  // Rio_readlineb(&rio,buf,MAXLINE); // 한줄 읽기 
  // printf("\n\nRequest headers:\n");
  // printf("%s",buf); 
  // sscanf(buf,"%s %s %s",method,uri,version); // 파일에서 읽어옴
  // if (strcasecmp(method,"GET")){ // 같다면 0리턴하므로 실행 되지 않음.
  //   clienterror(client_fd,method,"501","Not implemented", "Tiny does not implement this method");
  //   return;
  // }
  /****************************************************************/

  // is_static = parse_uri(uri, filename, cgiargs);
  // if(is_static & cache(filename)) // 정적파일이고 캐시되어있다면 함수 종료.
  //   return;

  /****************************************************************/
  int server_fd = Open_clientfd("localhost","5000");
  printf("try to connect server... \n");
  move_a2b(client_fd,server_fd);
  
  // move_a2b(server_fd,client_fd);
  Close(server_fd);
  //add_cache();
  /****************************************************************/
}
void move_a2b(int fd_a, int fd_b){ // a to b
  size_t n;
  char buf[MAXLINE];
  rio_t rio;
  
  Rio_readinitb(&rio,fd_a);
  // Rio_readnb(&rio,buf,MAXLINE);
  // Rio_writen(fd_b,buf,n);
  while((n=Rio_readlineb(&rio,buf,MAXLINE)) != 0)
  {
    if(n == -1 )
      break;
    printf("server received %s %d\n", buf,n);
    Rio_writen(fd_b,buf,n);
  }
  printf("test2test2test2\n");
}


int parse_uri(char *uri, char *filename, char *cgiargs) // filename은 여기서 최초로 값이 할당됨.
{
  char *ptr;

  if(!strstr(uri, "cgi-bin")) // uri에서 substring "cgi-bin" 을 찾지 못한다면
  {
    strcpy(cgiargs, ""); 
    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri)-1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  {
    ptr = index(uri, '?'); 
    if(ptr)
    {
      strcpy(cgiargs,ptr+1); // ?다음 문자열을 cgi args에 저장
      *ptr = '\0'; // ptr null값으로 바꾸어주면 url에서 query가 제거됨
    }
    else 
      strcpy(cgiargs,""); 
    strcpy(filename, ".");
    strcat(filename, uri); // query가 제거된 filename 할당.
    // printf("흠... uri 는 다음 과같다, %s",filename);
    return 0;
  }
}

int cache(char* filename){
  return 0; // 현재는 항상 캐시없음으로 작동 
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{ // 에러 메세지를 버퍼에 적기만 하고 마무리 파일을 닫으면서 클라이언트에 전송하는 역할은 메인에서 처리.
  char buf[MAXLINE], body[MAXBUF];
  // Build the HTTP response body
  sprintf(body,"<html><title>Tiny Error</title>");
  sprintf(body,"%s<body bgcolor=""ffffff"">\r\n",body);
  sprintf(body,"%s%s: %s\r\n",body,errnum,shortmsg);
  sprintf(body,"%s<p>%s: %s\r\n",body,longmsg,cause);
  sprintf(body,"%s<hr><em>The Tiny Webserver</em>\r\n",body);

  //Print the HTTP response
  sprintf(buf,"HTTP/1.0 %s %s\r\n",errnum,shortmsg);
  Rio_writen(fd,buf,strlen(buf));
  sprintf(buf,"Content-type: text/html\r\n", (int)strlen(body));
  Rio_writen(fd, buf,strlen(buf));
  Rio_writen(fd, body, strlen(body));
}