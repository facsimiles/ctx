
#ifdef CTX_ARMV7L

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <elf.h>


int ctx_arm_has_neon (int *armv)
{
  /* TODO : add or hardcode the other ways it can be on arm, where
   *        this info comes from the system and not from running cpu
   *        instructions
   */
  int has_neon = 0;
  int arm_level = 5;
  int fd = open ("/proc/self/auxv", O_RDONLY);
  Elf32_auxv_t auxv;
  if (fd >= 0)
  {
    while (read (fd, &auxv, sizeof (Elf32_auxv_t)) == sizeof (Elf32_auxv_t))
    {
      if (auxv.a_type == AT_HWCAP)
      {
        if (auxv.a_un.a_val & 4096)
          has_neon = 1;
      }
      else if (auxv.a_type == AT_PLATFORM)
      {
        if (!strncmp ((const char*)auxv.a_un.a_val, "v6l", 3))
          arm_level = 6;
        else if (!strncmp ((const char*)auxv.a_un.a_val, "v7l", 3))
          arm_level = 7;
        else if (!strncmp ((const char*)auxv.a_un.a_val, "v8l", 3))
          arm_level = 8;
      }
    }
    close (fd);
  }
  if (armv) *armv = arm_level;
  return has_neon;
}
#endif
