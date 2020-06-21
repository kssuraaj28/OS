struct inode{
  char name [20]; //Name of file
  uint32_t flags;
  uint32_t refcount; //The number of C pointers we have to the inode. If 0, free it
  uint32_t nlink; //Number of directory links
  struct inode* parent;
};

struct node* inode_get( what is this)
{

  return pointer to inode

}

bool inode_put (struct inode* waht)
{


}
