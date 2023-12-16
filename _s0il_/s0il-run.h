#ifndef _RUN_H
#define _RUN_H


int   runv (char *pathname, char **argv);
int   runvp (char *file, char **argv);

void  s0il_bundle_main (const char *name, int(*main)(int argc, char **argv));

int   runs (const char *cmdline);

typedef enum {
  RUN_READONLY = (1<<0),
  RUN_DIR      = (1<<1)
} s0il_file_flag;

void s0il_add_file(const char *path, const char *contents, size_t size, s0il_file_flag flags);

// returns pid or -1 on fail // XXX : needs work - not fully working
int   spawnp (char **argv);


void s0il_output_state_reset (void);
int  s0il_output_state (void); // 1 text 2 graphics 3 both


#endif
