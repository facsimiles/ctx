#ifndef _RUN_H
#define _RUN_H


int   runv (char *pathname, char **argv);
int   runvp (char *file, char **argv);

void  run_bundle_main (const char *name, int(*main)(int argc, char **argv));

int   runs (const char *cmdline);

typedef enum {
  RUN_READONLY = (1<<0),
  RUN_DIR      = (1<<1)
} run_file_flag;

void run_add_file(const char *path, const char *contents, size_t size, run_file_flag flags);

// returns pid or -1 on fail // XXX : needs work - not fully working
int   spawnp (char **argv);


void run_output_state_reset (void);
int  run_output_state (void); // 1 text 2 graphics 3 both


#endif
