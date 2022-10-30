// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define HASHNUM 13

struct {
  struct spinlock lock[HASHNUM];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[HASHNUM];
} bcache;


uint bucket_size;

void add_bucket(struct buf *b, int bucket_num){                         //将b添加到头结点下一个，该函数内部未加锁
  b->next = bcache.head[bucket_num].next;
  b->prev = &bcache.head[bucket_num];
  bcache.head[bucket_num].next->prev = b;
  bcache.head[bucket_num].next = b;
}


void
binit(void)
{
  struct buf *b;

  bucket_size = (NBUF/HASHNUM)*sizeof(struct buf);

  for(int i=0; i<HASHNUM; i++){                             //初始化头结点
    initlock(&bcache.lock[i], "bcache");
    bcache.head[i].prev = &(bcache.head[i]);
    bcache.head[i].next = &(bcache.head[i]);
  }
  for(b = bcache.buf; b<bcache.buf+NBUF; b++){              //将实际存储块添加到对应桶中
    add_bucket(b, (b-bcache.buf)/bucket_size);
    initsleeplock(&b->lock, "buffer");
  }

}

static struct buf*
bget_bucket(uint dev, uint blockno, int bucket, int takeout){                   //从指定桶号bucket获取缓存块， takeout起bool类型作用，代表是否从桶中取出
  struct buf *b;
  acquire(&bcache.lock[bucket]);
  if(!takeout){                                                                 //若需要取出则桶中一定没有对应映射，否则需查找是否已存在相应缓存块
    for(b = bcache.head[bucket].next; b != &(bcache.head[bucket]); b = b->next){    //从头部访问
      if(b->dev == dev && b->blockno == blockno){
        b->refcnt++;
        release(&bcache.lock[bucket]);
        acquiresleep(&b->lock);
        return b;
      }
    }
  }

  for(b = bcache.head[bucket].prev; b!= &bcache.head[bucket]; b = b->prev){   //桶中没有映射，需寻找缓存块替换     倒序访问
    if(b->refcnt == 0){                                                       //缓存块未使用，修改为当前的映射
      b -> dev = dev;     
      b -> blockno = blockno;
      b -> valid = 0;
      b -> refcnt = 1;
      if(takeout){                                                            //取出则将其前后直接相连
        b->next->prev = b->prev;
        b->prev->next = b->next;
      }
      release(&bcache.lock[bucket]);                                          //先释放该桶锁避免takeout再申请其它桶造成死锁
      if(takeout){
        int bucket_tar = blockno % HASHNUM;                                   //将取来的块放入对应桶
        acquire(&bcache.lock[bucket_tar]);
        add_bucket(b, bucket_tar);
        release(&bcache.lock[bucket_tar]);
      }
      acquiresleep(&b -> lock);
      return b;
    }
  }
  release(&bcache.lock[bucket]);                                             //未找到，释放锁
  return 0;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int bucket = blockno % HASHNUM;
  if((b = bget_bucket(dev, blockno, bucket, 0))){                               //查询对应哈希桶
    return b;
  }
  for(int i=0; i<HASHNUM; i++){                                                 //寻找其它桶
    if(i == bucket){
      continue;
    }
    if((b = bget_bucket(dev, blockno, i, 1))){
      return b;
    }
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bucket_real = (b->blockno) % HASHNUM;

  acquire(&bcache.lock[bucket_real]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    
    add_bucket(b, bucket_real);
    
  }
  release(&bcache.lock[bucket_real]);
}

void
bpin(struct buf *b) {
  int bucket = b->blockno % HASHNUM;
  acquire(&bcache.lock[bucket]);
  b->refcnt++;
  release(&bcache.lock[bucket]);
}

void
bunpin(struct buf *b) {
  int bucket = b->blockno % HASHNUM;
  acquire(&bcache.lock[bucket]);
  b->refcnt--;
  release(&bcache.lock[bucket]);
}