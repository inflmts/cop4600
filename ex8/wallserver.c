#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define READ_MAX 80

struct message
{
  char content[81];
};

static const int uno = 1;

static void __attribute__((format(printf, 1, 2)))
msg(const char *format, ...)
{
  va_list ap;
  char buf[2048], *s;
  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);
  for (s = buf; *s; ++s)
    if (*s < 0x20 || *s == 0x7f)
      *s = '?';
  *(s++) = '\n';
  write(2, buf, s - buf);
}

#define err(...) msg("error: " __VA_ARGS__)
#define err_sys_(format, ...) err(format ": %s", __VA_ARGS__)
#define err_sys(...) err_sys_(__VA_ARGS__, strerror(errno))

static int
parse_uint(const char *s, int max)
{
  int value = 0;
  do {
    if (*s < '0' || *s > '9' || value > (max - (*s - '0')) / 10)
      return -1;
    value = value * 10 + (*s - '0');
  } while (*(++s));
  return value;
}

struct conn
{
  int sock;
  int rsize;
  char buf[READ_MAX];
};

static int
conn_write(struct conn *c, const char *format, ...)
{
  va_list ap;
  char buf[256];
  char *s = buf;
  int size;
  va_start(ap, format);
  size = vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);
  while (size) {
    ssize_t sent = send(c->sock, s, size, 0);
    if (sent < 0) {
      err_sys("send");
      return -1;
    }
    s += sent;
    size -= sent;
  }
  return 0;
}

static int
conn_read(struct conn *c, char *dest, int size)
{
  int received = c->rsize;
  int retsize = 0;
  int copysize;
  --size;

recv:
  retsize += received;
  copysize = size < received ? size : received;
  memcpy(dest, c->buf, copysize);
  dest += copysize;
  size -= copysize;

  received = recv(c->sock, c->buf, READ_MAX, 0);
  if (received <= 0) {
    if (received) {
      err_sys("recv");
    }
    return -1;
  }

  char *end = memchr(c->buf, '\n', received);
  if (!end) {
    goto recv;
  }

  retsize += end - c->buf;
  copysize = size < end - c->buf ? size : end - c->buf;
  memcpy(dest, c->buf, copysize);
  dest[copysize] = '\0';
  ++end;
  c->rsize = received - (end - c->buf);
  memmove(c->buf, end, c->rsize);
  return retsize;
}

int main(int argc, char **argv)
{
  int queue_size = 20;
  int port = 5514;

  if ((argc > 3) ||
      (argc > 1 && (queue_size = parse_uint(argv[1], 1000)) <= 0) ||
      (argc > 2 && (port = parse_uint(argv[2], 0xffff)) <= 0)) {
    msg("usage: wallserver [queue_size [port]]");
    return 2;
  }

  int ssock = socket(AF_INET, SOCK_STREAM, 0);
  if (ssock < 0) {
    err_sys("socket");
    return 1;
  }

  setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &uno, sizeof(uno));

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(port),
    .sin_addr = { INADDR_ANY }
  };

  if (bind(ssock, (const struct sockaddr *)&addr, sizeof(addr))) {
    err_sys("could not bind to port %d", port);
    return 1;
  }

  if (listen(ssock, 16)) {
    err_sys("listen");
    return 1;
  }

  msg("Wall server running on port %d with queue size %d.", port, queue_size);

  struct message *queue, *head, *tail, *end;
  queue = calloc(queue_size + 1, sizeof(*queue));
  head = queue;
  tail = queue;
  end = queue + queue_size + 1;

  struct conn c;
  char command[80];

accept:
  memset(&c, 0, sizeof(c));
  c.sock = accept(ssock, NULL, NULL);
  if (c.sock == -1) {
    err_sys("accept");
    return 1;
  }

wall:
  conn_write(&c, "Wall Contents\n-------------\n");

  if (head == tail) {
    conn_write(&c, "[NO MESSAGES - WALL EMPTY]\n\n");
  } else {
    struct message *m = head;
    while (m != tail) {
      conn_write(&c, "%s\n", m->content);
      if (++m == end) {
        m = queue;
      }
    }
    conn_write(&c, "\n");
  }

command:
  conn_write(&c, "Enter command: ");
  if (conn_read(&c, command, sizeof(command)) < 0) {
    goto close;
  }

  if (!strcmp(command, "clear")) {
    tail = head;
    conn_write(&c, "Wall cleared.\n\n");
    goto wall;
  }

  if (!strcmp(command, "post")) {
    conn_write(&c, "Enter name: ");
    if (conn_read(&c, command, sizeof(command)) < 0) {
      goto close;
    }
    int namelen = strlen(command);
    int maxlen = 78 - namelen;
    int messlen;
    command[namelen] = ':';
    command[namelen + 1] = ' ';
    conn_write(&c, "Post [Max length %d]: ", maxlen);
    messlen = conn_read(&c, command + namelen + 2, maxlen + 1);
    if (messlen < 0) {
      goto close;
    }
    if (messlen > maxlen) {
      conn_write(&c, "Error: message is too long!\n\n");
      goto command;
    }
    memcpy(tail->content, command, namelen + messlen + 3);
    if (++tail == end) {
      tail = queue;
    }
    if (head == tail && ++head == end) {
      head = queue;
    }
    conn_write(&c, "Successfully tagged the wall.\n\n");
    goto wall;
  }

  if (!strcmp(command, "kill")) {
    conn_write(&c, "Closing socket and terminating server. Bye!\n");
    goto terminate;
  }

  if (!strcmp(command, "quit")) {
    conn_write(&c, "Come back soon. Bye!\n");
    goto close;
  }

  conn_write(&c, "Error: bad command.\n\n");
  goto command;

close:
  close(c.sock);
  goto accept;

terminate:
  close(c.sock);
  close(ssock);
  return 0;
}
