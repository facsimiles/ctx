
/* utility to strip out elf_pie flag, which hinders using dlopen on
   pic excutables in recent version. Some things work, and this allows
   having a simulator where the executables also work outside the shell.

   ISC licensed (c) 2023 Øyvind Kolås <pippin@gimp.org>
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#if 0
#include <elf.h>
#else
#define DT_FLAGS_1  0x6ffffffb
#define DF_1_PIE    0x08000000
#endif

int main (int argc, char **argv)
{
  if (!argv[1])
  {
    printf ("Usage: %s <elf1> [elf2 [elf3 ..]]\n", argv[0]);
    printf ("  removes DF_1_PIE flag from elf binaries\n");
  }
  for (int i = 1; argv[i]; i++)
  {
     int fd = open (argv[i], O_RDWR);
     if (fd)
     {
       int size = 0;
       size = lseek (fd, 0, SEEK_END);
       int16_t *contents = mmap (NULL, size,
                                 PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
       int32_t *ptr = (void*)contents;
       for (int i = 0; i < size/4 ;i++)
       {
          if (ptr[i] == DT_FLAGS_1 && ptr[i+2] == DF_1_PIE)
          {
            ptr[i+2] = 0;
          }
       }
       munmap (contents, size);
       close (fd);
     }
  }
}
