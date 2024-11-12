#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

int read_socket(int sfd, char *buffer)
{
  ssize_t read_length = read(sfd, buffer, 1024);
  if(read_length < 0) {
    printf("Failed to read from a connection...\n");
    return 1;
  }

  char path[100];
  int path_len = 1;
  int path_start = 3;

  char *user_agent_header = strtok(buffer, "\r\n");
  char *user_agent_value;

  while(user_agent_header != NULL) {
    if(strncmp(user_agent_header, "User-Agent", 10) == 0)
      break;

    user_agent_header = strtok(NULL, "\r\n");
  }

  if(user_agent_header != NULL) {
    user_agent_value = strtok(user_agent_header, " ");
    user_agent_value = strtok(NULL, " ");
  }

  while(buffer[path_start+path_len] != ' ') {
    path[path_len-1] = buffer[path_start+path_len];
    path_len++;
  }

  path[path_len-1] = '\0';
  if(strcmp(path, "/") == 0) {
    char response[1024] = {0};
    char headers[128] = "Content-Type: text/plain\r\n";
    char content_length[64];
    char body[10] = {0};
    sprintf(content_length, "Content-Length: %d\r\n\r\n", 4);
    strcat(headers, content_length);
    strcat(body, "Test\n");
    strcat(body, "\r\n\r\n");
    if(createResponse(200, headers, body, response) < 0)
      printf("NULL\n");
    printf("RESPONSE: %s\n", response);
    if(write(sfd, response, strlen(response)) < 0)  {
      printf("Failed to send a response...\n");
      return 1;
    }  
  }
  else if(strcmp(path, "/host") == 0) {
    char response[1024] = {0};
    char headers[128] = "Content-Type: text/html\r\n";
    char content_length[64];

    int file_fd = open("index.html", O_RDONLY);
    if(file_fd < 0) printf("Error reading file\n");

    struct stat file_stat;
    fstat(file_fd, &file_stat);
    ssize_t file_size = file_stat.st_size;
    char body[file_size];

    ssize_t read_size = 0;
    ssize_t bytes_read;
    while((bytes_read = read(file_fd, body, file_size)) > 0) {
      read_size += bytes_read;
    }

    sprintf(content_length, "Content-Length: %ld\r\n\r\n", read_size);
    strcat(headers, content_length);
    if(createResponse(200, headers, body, response) < 0)
      printf("NULL\n");
    printf("RESPONSE: %s\n", response);
    if(write(sfd, response, strlen(response)) < 0)  {
      printf("Failed to send a response...\n");
      return 1;
    }  
  }
  else if(strcmp(path, "/test") == 0) {
    char response[1024] = {0};
    char headers[128] = "Content-Type: text/html\r\n";
    char content_length[64];
    char body[256] = {0};
    sprintf(content_length, "Content-Length: %d\r\n\r\n", 84);
    strcat(headers, content_length);
    strcat(body, "<html><head><title>Test title</title></head><body><p>test p</p></body></html>\n");
    strcat(body, "\r\n\r\n");
    if(createResponse(200, headers, body, response) < 0)
      printf("NULL\n");
    printf("RESPONSE: %s\n", response);
    if(write(sfd, response, strlen(response)) < 0)  {
      printf("Failed to send a response...\n");
      return 1;
    }  
  }
  else if(strncmp(path, "/user-agent", 11) == 0) {
    char response[1024] = {0};
    char headers[128] = "Content-Type: text/plain\r\n";
    char content_length[64];
    char body[sizeof(user_agent_value) + 4];
    sprintf(content_length, "Content-Length: %zu\r\n\r\n", strlen(user_agent_value));
    strcat(headers, content_length);
    strcpy(body, user_agent_value);
    strcat(body, "\r\n\r\n");

    if(createResponse(200, headers, body, response) < 0)
      printf("NULL\n");
    printf("RESPONSE: %s\n", response);
    if(write(sfd, response, strlen(response)) < 0) {
      printf("Failed to send a response...\n");
      return 1;
    }
  }
  else if(strncmp(path, "/echo", 5) == 0) {
    char substr[128];
    char headers[128] = "Content-Type: text/plain\r\n";
    char content_length[64];
    sprintf(content_length, "Content-Length: %d\r\n\r\n", path_len);
    strcat(headers, content_length);
    char body[sizeof(path)+5];
    strncpy(substr, path+5, path_len-5);
    strcpy(body, substr);
    strcat(body, "\r\n\r\n");

    char response[1024] = {0};
    if(createResponse(200, headers, body, response) < 0)
      printf("NULL\n");
    if(write(sfd, response, strlen(response)) < 0) {
      printf("Failed to send a response...\n");
      return 1;
    }
  } 
  else {
    printf("PATH: %s\n", path);
    printf("PATH LEN: %d\n", path_len);
    char response[1024] = {0};
    char ext[10] = {0};
    char headers[128] = {0};
    char content_length[64] = {0};
    char filename[path_len-1];

    strcpy(filename, path+1);
    printf("FILENAME: %s\n", filename);

    // Get the file extension
    for(int i = path_len-2; i >=0; i--) {
      if(path[i] == (int)'.') {
        for(int j = i+1; j < path_len-1; j++) {
          //printf("Got after: %c\n", path[j-(i+1)]);
          ext[j-(i+1)] = path[j];
        }
      }
    }
    if(strcmp(ext, "js") == 0) {
      sprintf(headers, "Content-Type: text/javascript\r\n");
    }

    int file_fd = open(filename, O_RDONLY);
    if(file_fd < 0) printf("Error reading file\n");

    struct stat file_stat;
    fstat(file_fd, &file_stat);
    ssize_t file_size = file_stat.st_size;
    char body[file_size];

    ssize_t read_size = 0;
    ssize_t bytes_read;
    while((bytes_read = read(file_fd, body, file_size)) > 0) {
      read_size += bytes_read;
    }

    sprintf(content_length, "Content-Length: %ld\r\n\r\n", read_size);
    strcat(headers, content_length);
    if(createResponse(200, headers, body, response) < 0)
      printf("NULL\n");
    printf("RESPONSE: %s\n", response);
    if(write(sfd, response, strlen(response)) < 0)  {
      printf("Failed to send a response...\n");
      return 1;
    }
  }

  return 0;
}

int createResponse(int response_code, char* headers, char* body, char* buffer)
{
  strcat(buffer, "HTTP/1.1 ");

  if(response_code == 200) {
    strcat(buffer, "200 OK\r\n");
  }
  else if(response_code == 404) {
    strcat(buffer, "404 Not Found\r\n");
  }

  if(headers != NULL) {
    strcat(buffer, headers);
  }

  if(body != NULL) {
    strcat(buffer, body);
  }

  strcat(buffer, "\r\n");

  return 0;
}

int main() {
  // Disable output buffering
  setbuf(stdout, NULL);

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  printf("Logs from your program will appear here!\n");

  int server_fd, client_addr_len;
  fd_set read_fds, active_fds;
  char buffer[1024];
  struct sockaddr_in client_addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    printf("Socket creation failed: %s...\n", strerror(errno));
    return 1;
  }

  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    printf("SO_REUSEPORT failed: %s \n", strerror(errno));
    return 1;
  }

  struct sockaddr_in serv_addr = { 
    .sin_family = AF_INET,
    .sin_port = htons(4221),
    .sin_addr = { htonl(INADDR_ANY) },
  };

  if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  printf("Waiting for a client to connect...\n");
  client_addr_len = sizeof(client_addr);

  // Initialize fd sets
  FD_ZERO(&active_fds);
  FD_SET(server_fd, &active_fds);

  while(1) {
    read_fds = active_fds;

    if(select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0){
      printf("Select failed: %s \n", strerror(errno));
      return 1;
    }

    for(int i = 0; i < FD_SETSIZE; i++) {
      if(FD_ISSET(i, &read_fds)) {
        // Server socket available for read operation, means there is a connection request on that socket
        if(i == server_fd) {
          int new_socket = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
          if(new_socket < 0) {
            printf("Error accepting a connection: %s\n", strerror(errno));
            return 1;
          }
          printf("Client connected: %d\n", inet_ntoa(client_addr.sin_addr));

          FD_SET(new_socket, &active_fds);
        } else {
          if(read_socket(i, buffer) != 0) {
            close(i);
            FD_CLR(i, &active_fds);
          }
        }
      }
    }
  }

  close(server_fd);

  return 0;
}
