#ifndef VFS_H
#define VFS_H

#include <stdint.h>
typedef enum { VFS_FILE, VFS_DIRECTORY } vfs_node_type_t;

typedef struct vfs_node {
  char name[128];
  vfs_node_type_t type;
  uint32_t size;
  void *data;

  struct vfs_node *parent;
  struct vfs_node *children;
  struct vfs_node *next;
} vfs_node_t;

extern vfs_node_t *vfs_root;

void init_vfs(void);

vfs_node_t *vfs_create_node(const char *name, vfs_node_type_t type);
void vfs_add_child(vfs_node_t *parent, vfs_node_t *child);
vfs_node_t *vfs_find_node(vfs_node_t *parent, const char *path);

vfs_node_t *create_directories(const char *path);

#endif // !VFS_H
