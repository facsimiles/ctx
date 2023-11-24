#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>

int main (int argc, char **argv)
{
  for (int i = 1; argv[i]; i++)
  {
     int fd = open (argv[i], O_RDWR);
     if (fd)
     {
       int size = 0;
       size = lseek (fd, 0, SEEK_END);
       int16_t *contents = mmap (NULL, size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
       printf ("%s %p %i\n", argv[i], contents, size);
       
       int32_t *ptr = (void*)contents;
       for (int i = 0; i < size/4 ;i++)
       {
          if (ptr[i] == DT_FLAGS_1 &&
              ptr[i+2] == 0x8000000)
          {
            printf ("FOUND!\n");
            printf ("%i: %x\n", i, ptr[i+2]);
            ptr[i+2] = 0;
          }
       }

//     printf ("phoff: %i\n", elf_header->e_phoff);

       munmap (contents, size);
       close (fd);
     }
  }
}
