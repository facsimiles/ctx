#include "vfs.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include "emscripten.h"

#include "py/obj.h"
#include "py/runtime.h"

#define MP_VFS_VIRTUAL_ROOT "/sd"

static char _temp_path[1024];
static char _temp_pathB[1024];
static const char *make_abs_path (const char *path)
{
  if (!strcmp (path, "/")) return MP_VFS_VIRTUAL_ROOT;
  if (path[0]=='/')
    snprintf (&_temp_path[0], 1023, MP_VFS_VIRTUAL_ROOT "%s", path);
  else
    snprintf (&_temp_path[0], 1023, MP_VFS_VIRTUAL_ROOT "/%s", path);
  return _temp_path;
}
static const char *make_abs_pathB (const char *path)
{
  if (!strcmp (path, "/")) return MP_VFS_VIRTUAL_ROOT;
  if (path[0]=='/')
    snprintf (_temp_pathB, 1023, MP_VFS_VIRTUAL_ROOT "%s", path);
  else
    snprintf (_temp_pathB, 1023, MP_VFS_VIRTUAL_ROOT"/%s", path);
  return _temp_pathB;
}

int mp_vfs_file_stat (const char* path, struct mp_vfs_stat* stat)
{
  path = make_abs_path (path);
  struct stat native_stat;
  int ret = lstat (path, &native_stat);
  stat->type = EPICSTAT_NONE;
  if (!ret)
  {
    switch (native_stat.st_mode & S_IFMT)
    {
       case S_IFREG:
         stat->size = native_stat.st_size;
         /* FALLTHROUGH */
       case S_IFDIR:
         if ((native_stat.st_mode & S_IFMT) == S_IFDIR)
           stat->type = EPICSTAT_DIR;
         else
           stat->type = EPICSTAT_FILE;
         if (strrchr (path, '/'))
         {
           strncpy (stat->name, strrchr (path, '/')+1, EPICSTAT_MAX_PATH);
         }
         else
         {
           strncpy (stat->name, path, EPICSTAT_MAX_PATH);
         }
         break;
    }
  }
  return ret;
}

int mp_vfs_file_write (int fd, const void *buf, size_t nbytes)
{
  return write (fd, buf, nbytes);
}

int mp_vfs_file_read (int fd, void *buf, size_t nbytes)
{
  return read (fd, buf, nbytes);
}

#define MP_VFS_MAX_OPENDIRS 32
DIR *open_dirs[MP_VFS_MAX_OPENDIRS];


int mp_vfs_file_opendir (const char *path)
{
  path = make_abs_path (path);
  for (int i = 0; i < MP_VFS_MAX_OPENDIRS; i++)
  {
    if (open_dirs[i] == NULL)
    {
      open_dirs[i] = opendir (path);
      return 9000 + i;
    }
  }
  return -1;
}

int mp_vfs_file_readdir (int fd, struct mp_vfs_stat *entry)
{
  fd -= 9000;
  if (fd < 0 || fd >= MP_VFS_MAX_OPENDIRS || open_dirs[fd] == NULL) return -1;
  struct dirent *native = readdir (open_dirs[fd]);
  entry->type = EPICSTAT_NONE;
  if (!native)
  {
    return 0;
  }
    switch (native->d_type)
    {
       case DT_REG:
       case DT_LNK:
         entry->type = EPICSTAT_FILE;
         break;
       case DT_DIR:
         entry->type = EPICSTAT_DIR;
         break;
       default:
         break;
    }
  strncpy (entry->name, native->d_name, EPICSTAT_MAX_PATH-1);
  return 0;
}

int mp_vfs_file_close (int fd)
{
  if (fd >= 9000)
  {
    fd-=9000;
    if (fd >= MP_VFS_MAX_OPENDIRS) return -1;
    if (!open_dirs[fd]) return -2;
    closedir (open_dirs[fd]);
    open_dirs[fd]=NULL;
    return 0;
  }
  return close (fd);
}

int mp_vfs_file_mkdir (const char *path)
{
  path = make_abs_path (path);
  return mkdir (path, 0777);
}

int mp_vfs_file_unlink(const char *path)
{
  path = make_abs_path (path);
  return unlink (path);
}

int mp_vfs_file_rename (const char *a, const char *b)
{
  a = make_abs_path (a);
  b = make_abs_pathB (b);
  return rename (a, b);
}

void mp_vfs_system_reset (void) { }

void mp_vfs_exit (int val)
{
  mp_raise_type(&mp_type_SystemExit);
}

int mp_vfs_file_open (const char *path, const char *modeString)
{
  path = make_abs_path (path);
  int mode = O_RDONLY;
  if (strchr (modeString, 'r')) mode |= O_RDONLY;

  if (strchr (modeString, 'w'))
  {
     mode |= O_WRONLY;
     if (!access (path, R_OK))
     {
       unlink (path);
     }
     mode |= O_CREAT;
  }
  return open (path, mode);
}

int mp_vfs_file_tell (int fd)
{
  return lseek (fd, 0, SEEK_CUR);
}

int mp_vfs_file_seek (int fd, long offset, int whence)
{
  return lseek (fd, offset, whence);
}
