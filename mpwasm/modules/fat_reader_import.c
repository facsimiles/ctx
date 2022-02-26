#include "epicardium.h"

#include <py/runtime.h>
#include <py/reader.h>
#include <py/lexer.h>

#define EPICFAT_READER_BUFFER_SIZE 128

typedef struct _mp_reader_epicfat_t {
	int fd;
	size_t len;
	size_t pos;
	byte buf[EPICFAT_READER_BUFFER_SIZE];
} mp_reader_epicfat_t;

STATIC mp_uint_t mp_reader_epicfat_readbyte(void *data)
{
	mp_reader_epicfat_t *reader = (mp_reader_epicfat_t *)data;
	if (reader->pos >= reader->len) {
		if (reader->len == 0) {
			return MP_READER_EOF;
		} else {
			int n = epic_file_read(
				reader->fd, reader->buf, sizeof(reader->buf)
			);
			if (n <= 0) {
				reader->len = 0;
				return MP_READER_EOF;
			}
			reader->len = n;
			reader->pos = 0;
		}
	}
	return reader->buf[reader->pos++];
}

STATIC void mp_reader_epicfat_close(void *data)
{
	mp_reader_epicfat_t *reader = (mp_reader_epicfat_t *)data;
	epic_file_close(reader->fd);
	m_del_obj(mp_reader_epicfat_t, reader);
}

void mp_reader_new_file(mp_reader_t *reader, const char *filename)
{
	int fd = epic_file_open(filename, "r");
	if (fd < 0) {
		mp_raise_OSError(-fd);
	}
	mp_reader_epicfat_t *rp = m_new_obj(mp_reader_epicfat_t);
	rp->fd                  = fd;
	int n = epic_file_read(rp->fd, rp->buf, sizeof(rp->buf));
	if (n < 0) {
		epic_file_close(fd);
	}
	rp->len          = n;
	rp->pos          = 0;
	reader->data     = rp;
	reader->readbyte = mp_reader_epicfat_readbyte;
	reader->close    = mp_reader_epicfat_close;
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename)
{
	mp_reader_t reader;
	mp_reader_new_file(&reader, filename);
	return mp_lexer_new(qstr_from_str(filename), reader);
}

mp_import_stat_t mp_import_stat(const char *path)
{
	struct epic_stat stat;

	if (epic_file_stat(path, &stat) == 0) {
		if (stat.type == EPICSTAT_FILE) {
			return MP_IMPORT_STAT_FILE;
		} else {
			return MP_IMPORT_STAT_DIR;
		}
	}
	return MP_IMPORT_STAT_NO_EXIST;
}
