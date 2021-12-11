#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user.h"
////////////////////////////////////////////////////////////////////////////////
typedef struct node {
  struct node *prev;
  struct node *next;
  void * value;
} node;

typedef struct queue {
  node *head;
  node *tail;
  int length;
} queue;

queue *initQueue();
void freeQueue(queue *q);
void enQueue(queue *q, const void * value);
void * deQueue(queue *q);
int hasElem(const queue *q);

#define BUF_SIZE 512
////////////////////////////////////////////////////////////////////////////////
int match(const char * path, const char * filename)
{
  char * p = (char*)path + strlen(path);
  while(p >= path && *p != '/')
    --p;
  ++p;
  return !strcmp(p, filename);
}
void find(char * root, char * filename)
{
  queue * q = initQueue();
  if(q == 0)
    exit(-1);

  char buf[BUF_SIZE], *p, *path;
  int fd, len = strlen(root);
  struct dirent de;
  struct stat st;

  path = malloc(len+1);
  strcpy(path, root);
  enQueue(q, path);

  while(hasElem(q))
  {
    path = deQueue(q);
    strcpy(buf, path);
    p = buf + strlen(path);

    if((fd = open(path, 0)) < 0)
    {
      fprintf(2, "find: cannot open %s\n", path);
      free(path);
      continue;
    }

    if(fstat(fd, &st) < 0)
    {
      fprintf(2, "find: cannot stat %s\n", path);
      free(path);
      close(fd);
      continue;
    }

    if(match(path, filename))
      printf("%s\n", path);

    switch(st.type)
    {
      case T_DEVICE:
        if(!match(path, root))
          break;
      case T_DIR:
        *p++ = '/';
        while(read(fd, &de, sizeof(de)) == sizeof(de))
        {
          if(de.inum == 0)
            continue;
          if('.' == de.name[0])
          {
            if(0 == de.name[1])
              continue;
            if('.' == de.name[1] && 0 == de.name[2])
              continue;
          }
          memmove(p, de.name, DIRSIZ);
          p[DIRSIZ] = 0;
          path = malloc(strlen(buf)+1);
          strcpy(path, buf);
          enQueue(q, path);
        }
        break;
    }
    free(path);
    close(fd);
  }
  freeQueue(q);
  exit(0);
}
int main(int argc, char *argv[]) {
  switch(argc)
  {
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
  if(q == 0)
  {
    fprintf(2, "find: malloc failed\n");
    return 0;
  }
  q->head = 0;
  q->tail = 0;
  q->length = 0;
  return q;
}

void freeQueue(queue *q) {
  if(q == 0)
    return;
  node *cur, *next;
  if (q->length > 0) {
    for (cur = q->head; cur != 0; cur = next)
    {
      next = cur->next;
      free(cur);
    }
  }
  free(q);
}

void enQueue(queue *q, const void * value) {
  //printf("enQueue: %s\n", value);
  node *new = malloc(sizeof(node));
  if(new == 0)
  {
    fprintf(2, "find: malloc failed\n");
    return;
  }
  new->prev = q->tail;
  new->next = 0;
  new->value = (void*)value;
  if (q->length == 0)
    q->head = new;
  else
    q->tail->next = new;
  q->tail = new;
  ++q->length;
}

void * deQueue(queue *q) {
  void * res = 0;
  node *p = q->head;
  if (q->length == 0)
    return 0;
  q->head = p->next;
  --q->length;
  res = p->value;
  free(p);
  return res;
}

int hasElem(const queue *q) { return (q->length == 0) ? 0 : -1; }