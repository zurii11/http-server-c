#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

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

  int server_fd, client_addr_len, new_socket;
  ssize_t read_length;
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

	new_socket = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	printf("Client connected\n");

  read_length = read(new_socket, buffer, 1024);
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
    if(createResponse(200, NULL, NULL, response) < 0)
      printf("NULL\n");
    printf("RESPONSE: %s\n", response);
    if(write(new_socket, response, strlen(response)) < 0)  {
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
    if(write(new_socket, response, strlen(response)) < 0) {
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
    printf("BODY: %s\n", body);
    char response[1024] = {0};
    createResponse(200, headers, body, response);
    if(write(new_socket, response, strlen(response)) < 0) {
      printf("Failed to send a response...\n");
      return 1;
    }
  } 
  else {
    char response[1024] = {0};
    printf("%s\n", path);
    createResponse(404, NULL, NULL, response);
    if(write(new_socket, response, strlen(response)) < 0) {
      printf("Failed to send a response...\n");
      return 1;
    }
  }

	close(server_fd);

	return 0;
}
