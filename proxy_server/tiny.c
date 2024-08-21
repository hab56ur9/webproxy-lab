/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

//user part
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
  char filename[MAXLINE],cgiargs[MAXLINE];
  rio_t rio;

  //Read request line and headers
  Rio_readinitb(&rio,fd);
  Rio_readlineb(&rio,buf,MAXLINE);

  printf("\n\nRequest headers:\n");
  printf("%s",buf); 

  sscanf(buf,"%s %s %s",method,uri,version); // 파일에서 읽어옴
  if (strcasecmp(method,"GET")) // 같다면 0리턴하므로 실행 되지 않음.
  {
    clienterror(fd,method,"501","Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio); // 값을 읽어옴 

  //Parse URI from GET request
  is_static = parse_uri(uri, filename, cgiargs); // 정적 파일 여부 구분 1이면 정적 1아니면 동적
  if(stat(filename,&sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }
  if(is_static) // 정적 파일 요청하는 경우
  {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))//이 파일이 보통파일이고, 읽기 권한이 있는지 검사
    {
      clienterror(fd,filename,"403","Forbidden","Tiny couldn't read the file");
      return;
    }
    serve_static(fd,filename,sbuf.st_size);// 정적 파일 읽기 시작.
  }
  else // 동적 파일 요청하는 경우
  {
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) //이 파일이 보통파일이고, 실행가능한지 검사.
    {
      clienterror(fd,filename,"403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd,filename,cgiargs); // 동적 컨텐츠 시작.
  }
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

void read_requesthdrs(rio_t *rp) 
// rp의 값을 읽기만 하고 다른처리 없음 \r\n으로 한줄씩 읽음 동작에 전혀 영향이 없는 함수.
{
  char buf[MAXLINE];
  Rio_readlineb(rp,buf,MAXLINE);
  while(strcmp(buf,"\r\n"))
  {
    if( Rio_readlineb(rp,buf,MAXLINE) == 0){
      printf("request without buf\n");
      break;
    }
    printf("%s", buf);
  }
  return;
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

void serve_static(int fd, char *filename, int filesize)
{ // fd는 현재 연결상태인 파일 디스크립터
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  //Send response headers to client
  get_filetype(filename,filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n",buf);
  sprintf(buf, "%sConnection: close\r\n",buf);
  sprintf(buf, "%sContent-length: %d\r\n",buf,filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n",buf, filetype); // 빈줄 한개가 헤더 종료를 의미함 \r\n\r\n 여기부분 에 빈줄하나가 포함되어있음

  Rio_writen(fd,buf,strlen(buf)); // 연결파일에 헤더 작성

  printf("Response headers:\n");
  printf("%s",buf);

  //Send response body to client
  srcfd = Open(filename, O_RDONLY, 0 ); // filename의 파일을 readonly로 열기 
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 요청한 파일을 가상 메모리 영역으로 매핑
  Close(srcfd); //mmap으로 매핑시켰으니 파일디스크립터 닫기
  Rio_writen(fd, srcp, filesize); // 연결파일에 쓰기 
  Munmap(srcp, filesize);  // mmap 닫기 
}
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  //Return first part of HTTP response
  sprintf(buf,"HTTP/1.0 200 OK\r\n");
  Rio_writen(fd,buf,strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(Fork() == 0)
  {
    // Real server would set all CGI vars here
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    printf("%s",filename);
    Execve(filename, emptylist, environ);
  }
  Wait(NULL); // Parent waits for and reaps child
}
void get_filetype(char *filename, char* filetype)
{
  if(strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype,"image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename,".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}


