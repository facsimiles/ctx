#if EMSCRIPTEN
#include "emscripten.h"
#endif

#ifndef _MP_VFS_H
#define _MP_VFS_H

#include <stdint.h>
#include <errno.h>

enum mp_vfs_stat_type {
	/**
	 * Basically ``ENOENT``. Although :c:func:`mp_vfs_file_stat` returns an
	 * error for 'none', the type will still be set to none additionally.
	 *
	 * This is also used internally to track open FS objects, where we use
	 * ``EPICSTAT_NONE`` to mark free objects.
	 */
	EPICSTAT_NONE,
	/** normal file */
	EPICSTAT_FILE,
	/** directory */
	EPICSTAT_DIR,
};

/**
 * Maximum length of a path string (=255).
 */
#define EPICSTAT_MAX_PATH        255
/* conveniently the same as FF_MAX_LFN */

/** */
struct mp_vfs_stat {
	/** Entity Type: file, directory or none */
	enum mp_vfs_stat_type type;

	/*
	 * Note about padding & placement of uint32_t size:
	 *
	 *   To accomodate for future expansion, we want padding at the end of
	 *   this struct. Since sizeof(enum mp_vfs_stat_type) can not be assumed
	 *   to be have a certain size, we're placing uint32_t size here so we
	 *   can be sure it will be at offset 4, and therefore the layout of the
	 *   other fields is predictable.
	 */

	/** Size in bytes. */
	uint32_t size;

	/** File Name. */
	char name[EPICSTAT_MAX_PATH + 1];
	uint8_t _reserved[12];
};


int mp_vfs_file_stat (const char* path, struct mp_vfs_stat* stat);
int mp_vfs_file_write (int fd, const void *buf, size_t nbytes);
int mp_vfs_file_read (int fd, void *buf, size_t nbytes);
int mp_vfs_file_opendir (const char *path);
int mp_vfs_file_readdir (int fd, struct mp_vfs_stat *entry);
int mp_vfs_file_close (int fd);
int mp_vfs_file_mkdir (const char *path);
int mp_vfs_file_unlink(const char *path);
int mp_vfs_file_rename (const char *a, const char *b);
int mp_vfs_file_open (const char *path, const char *modeString);
int mp_vfs_file_tell (int fd);
int mp_vfs_file_seek (int fd, long offset, int whence);

#endif
