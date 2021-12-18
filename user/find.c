#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"
////////////////////////////////////////////////////////////////////////////////
typedef struct node {
  struct node *prev;
  struct node *next;
  void *value;
} node;

typedef struct queue {
  node *head;
  node *tail;
  int length;
} queue;

queue *initQueue();
void freeQueue(queue *q);
void enQueue(queue *q, void *value);
void *deQueue(queue *q);
int hasElem(const queue *q);
int match(char *path, char *pattern);

#define BUF_SIZE 512
////////////////////////////////////////////////////////////////////////////////
void find(char *root, char *filename) {
  queue *q = initQueue();
  if (q == 0)
    exit(-1);

  char buf[BUF_SIZE], *path, *p;
  int fd, len = strlen(root);
  struct dirent de;
  struct stat st;

  path = malloc(len + 1);
  strcpy(path, root);
  enQueue(q, path);

  while (hasElem(q)) {
    path = deQueue(q);
    strcpy(buf, path);

    if ((fd = open(path, 0)) < 0) {
      fprintf(2, "find: cannot open %s\n", path);
      free(path);
      continue;
    }

    if (fstat(fd, &st) < 0) {
      fprintf(2, "find: cannot stat %s\n", path);
      free(path);
      close(fd);
      continue;
    }

    if (match(path, filename))
      printf("%s\n", path);

    switch (st.type) {
    case T_DEVICE:
      if (path[0] != '.' || path[1] != 0)
        break;
    case T_DIR:
      while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0)
          continue;
        if ('.' == de.name[0]) {
          if (0 == de.name[1])
            continue;
          if ('.' == de.name[1] && 0 == de.name[2])
            continue;
        }
        p = buf + strlen(path);
        *p++ = '/';
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        p = malloc(strlen(buf) + 1);
        strcpy(p, buf);
        enQueue(q, p);
      }
    default:
      break;
    }
    free(path);
    close(fd);
  }
  freeQueue(q);
  exit(0);
}
int main(int argc, char *argv[]) {
  switch (argc) {
  case 2:
    find(".", argv[1]);
    break;
  case 3:
    find(argv[1], argv[2]);
    break;
  default:
    break;
  }
  exit(0);
}
////////////////////////////////////////////////////////////////////////////////
queue *initQueue() {
  queue *q = malloc(sizeof(queue));
  if (q == 0) {
    fprintf(2, "find: malloc failed\n");
    return 0;
  }
  q->head = 0;
  q->tail = 0;
  q->length = 0;
  return q;
}

void freeQueue(queue *q) {
  if (q == 0)
    return;
  node *cur, *next;
  if (q->length > 0) {
    for (cur = q->head; cur != 0; cur = next) {
      next = cur->next;
      free(cur);
    }
  }
  free(q);
}

void enQueue(queue *q, void *value) {
  node *new = malloc(sizeof(node));
  if (new == 0) {
    fprintf(2, "find: malloc failed\n");
    return;
  }
  new->prev = q->tail;
  new->next = 0;
  new->value = value;
  if (q->length == 0)
    q->head = new;
  else
    q->tail->next = new;
  q->tail = new;
  ++q->length;
}

void *deQueue(queue *q) {
  void *res = 0;
  node *p = q->head;
  if (q->length == 0)
    return 0;
  q->head = p->next;
  --q->length;
  if (q->length == 0) {
    q->head = 0;
    q->tail = 0;
  }
  res = p->value;
  free(p);
  return res;
}

int hasElem(const queue *q) { return (q->length == 0) ? 0 : -1; }

// Regexp matcher from Kernighan & Pike,
// The Practice of Programming, Chapter 9.

int matchhere(char *, char *);
int matchstar(int, char *, char *);

int match(char *path, char *re) {
  // get the filename after the last slash
  char *text = path + strlen(path);
  while (text >= path && *text != '/')
    --text;
  ++text;

  // Simple full text comparison
  // return !strcmp(re,text);

  if (re[0] == '^')
    return matchhere(re + 1, text);
  do { // must look at empty string
    if (matchhere(re, text))
      return 1;
  } while (*text++ != '\0');
  return 0;
}

// matchhere: search for re at beginning of text
int matchhere(char *re, char *text) {
  if (re[0] == '\0')
    return 1;
  if (re[1] == '*')
    return matchstar(re[0], re + 2, text);
  if (re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if (*text != '\0' && (re[0] == '.' || re[0] == *text))
    return matchhere(re + 1, text + 1);
  return 0;
}

// matchstar: search for c*re at beginning of text
int matchstar(int c, char *re, char *text) {
  do { // a * matches zero or more instances
    if (matchhere(re, text))
      return 1;
  } while (*text != '\0' && (*text++ == c || c == '.'));
  return 0;
}