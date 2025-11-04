#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: server PORT\n");
    return 2;
  }

  int port = atoi(argv[1]);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("socket");
    return 1;
  }

  static const int uno = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &uno, sizeof uno)) {
    perror("setsockopt");
    return 1;
  }

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr = { INADDR_ANY }
  };

  if (bind(sock, (const struct sockaddr *)&addr, sizeof addr)) {
    perror("bind");
    return 1;
  }

  if (listen(sock, 16)) {
    perror("listen");
    return 1;
  }

  int csock = accept(sock, NULL, NULL);
  if (csock == -1) {
    perror("accept");
    return 1;
  }

  char buf[1024];
  ssize_t amount;
  while ((amount = recv(csock, buf, sizeof buf, 0)) > 0) {
    if (write(1, buf, amount) == -1) {
      perror("write");
      return 1;
    }
  }

  if (amount) {
    perror("recv");
    return 1;
  }

  static const char message[] = "Welcome to the server running on REPTILIAN\n";
  if (send(csock, message, sizeof message - 1, 0) == -1) {
    perror("send");
    return 1;
  }

  close(csock);
  close(sock);
  return 0;
}
