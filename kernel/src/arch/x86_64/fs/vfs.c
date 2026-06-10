#include "slab.h"
#include <arch/x86_64/fs/vfs.h>
#include <stddef.h>
#include <string.h>

vfs_node_t *vfs_root = NULL;

static int strncmp_path(const char *s1, const char *s2, size_t n) {
  while (n-- && *s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  if (n == (size_t)-1) {
    return 0;
  }
  return *s1 - *s2;
}

void init_vfs(void) { vfs_root = vfs_create_node("/", VFS_DIRECTORY); }

vfs_node_t *vfs_create_node(const char *name, vfs_node_type_t type) {
  vfs_node_t *node = kmalloc(sizeof(vfs_node_t));
  memset(node, 0, sizeof(vfs_node_t));

  size_t i = 0;
  while (name[i] && name[i] != '/' && i < 127) {
    node->name[i] = name[i];
    i++;
  }
  node->name[i] = '\0';
  node->type = type;
  return node;
}

void vfs_add_child(vfs_node_t *parent, vfs_node_t *child) {
  child->parent = parent;
  child->next = parent->children;
  parent->children = child;
}

vfs_node_t *vfs_find_node(vfs_node_t *parent, const char *path) {
  if (!parent || !path || *path == '\0') {
    return parent;
  }

  while (*path == '/') {
    path++;
  }
  if (*path == '\0') {
    return parent;
  }

  const char *next_slash = path;
  while (*next_slash && *next_slash != '/') {
    next_slash++;
  }
  size_t segment_len = next_slash - path;

  vfs_node_t *child = parent->children;
  while (child) {
    if (strncmp_path(child->name, path, segment_len) == 0 &&
        child->name[segment_len] == '\0') {
      return child;
    } else {
      return vfs_find_node(child, next_slash + 1);
    }
    child = child->next;
  }
  return NULL;
}

vfs_node_t *create_directories(const char *path) {
  vfs_node_t *current = vfs_root;
  char buffer[128];

  const char *p = path;
  while (*p) {
    const char *next_slash = p;
    while (*next_slash && *next_slash != '/') {
      next_slash++;
    }
    if (*next_slash == '\0') {
      break;
    }
    size_t len = next_slash - p;
    int _copied = memcmp(buffer, p, len);

    buffer[len] = '\0';

    vfs_node_t *child = vfs_find_node(current, buffer);

    if (!child) {
      child = vfs_create_node(buffer, VFS_DIRECTORY);
      vfs_add_child(current, child);
    }
    current = child;
    p = next_slash + 1;
  }
  return current;
}
