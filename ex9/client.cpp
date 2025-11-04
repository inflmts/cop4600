#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: client PORT\n");
    return 2;
  }

  int port = atoi(argv[1]);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr = { htonl(INADDR_LOOPBACK) }
  };

  if (connect(sock, (const struct sockaddr *)&addr, sizeof addr)) {
    perror("connect");
    return 1;
  }

  static const char message[] = "Daniel Li: 99157575\n";
  if (send(sock, message, sizeof message - 1, 0) == -1) {
    perror("send");
    return 1;
  }

  shutdown(sock, SHUT_WR);

  char buf[1024];
  ssize_t amount;
  while ((amount = recv(sock, buf, sizeof buf, 0)) > 0) {
    if (write(1, buf, amount) == -1) {
      perror("write");
      return 1;
    }
  }

  if (amount) {
    perror("recv");
    return 1;
  }

  close(sock);
  return 0;
}
