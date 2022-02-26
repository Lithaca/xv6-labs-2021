struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  int ts;      // timestamp
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  uchar data[BSIZE];
};

