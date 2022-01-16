#ifdef CTX_X86_64

enum
{
  ARCH_X86_INTEL_FEATURE_MMX      = 1 << 23,
  ARCH_X86_INTEL_FEATURE_XMM      = 1 << 25,
  ARCH_X86_INTEL_FEATURE_XMM2     = 1 << 26,
};

enum
{
  ARCH_X86_INTEL_FEATURE_PNI      = 1 << 0,
  ARCH_X86_INTEL_FEATURE_SSSE3    = 1 << 9,
  ARCH_X86_INTEL_FEATURE_FMA      = 1 << 12,
  ARCH_X86_INTEL_FEATURE_SSE4_1   = 1 << 19,
  ARCH_X86_INTEL_FEATURE_SSE4_2   = 1 << 20,
  ARCH_X86_INTEL_FEATURE_MOVBE    = 1 << 22,
  ARCH_X86_INTEL_FEATURE_POPCNT   = 1 << 23,
  ARCH_X86_INTEL_FEATURE_XSAVE    = 1 << 26,
  ARCH_X86_INTEL_FEATURE_OSXSAVE  = 1 << 27,
  ARCH_X86_INTEL_FEATURE_AVX      = 1 << 28,
  ARCH_X86_INTEL_FEATURE_F16C     = 1 << 29
};

enum
{
  ARCH_X86_INTEL_FEATURE_BMI1     = 1 << 3,
  ARCH_X86_INTEL_FEATURE_BMI2     = 1 << 8,
  ARCH_X86_INTEL_FEATURE_AVX2     = 1 << 5,
};

#define cpuid(a,b,eax,ebx,ecx,edx)                     \
  __asm__("cpuid"                                           \
           : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) \
           : "0" (a), "2" (b)  )

/* returns x86_64 microarchitecture level
 *   0
 */
int
ctx_x86_64_level (void)
{
  int level = 0;
  uint32_t eax, ebx, ecx, edx;
  cpuid (1, 0, eax, ebx, ecx, edx);

  if ((edx & ARCH_X86_INTEL_FEATURE_MMX) == 0)   return level;
  if ((edx & ARCH_X86_INTEL_FEATURE_XMM) == 0)   return level;
  level = 1;

  if ((ecx & ARCH_X86_INTEL_FEATURE_SSSE3)==0)   return level;
  if ((ecx & ARCH_X86_INTEL_FEATURE_SSE4_1)==0)  return level;
  if ((ecx & ARCH_X86_INTEL_FEATURE_SSE4_2)==0)  return level;
  if ((ecx & ARCH_X86_INTEL_FEATURE_POPCNT)==0)  return level;
  level = 2;

  if ((ecx & ARCH_X86_INTEL_FEATURE_AVX)==0)     return level;
  if ((ecx & ARCH_X86_INTEL_FEATURE_OSXSAVE)==0) return level;
  if ((ecx & ARCH_X86_INTEL_FEATURE_FMA)==0)     return level;
  if ((ecx & ARCH_X86_INTEL_FEATURE_F16C)==0)    return level;
  if ((ecx & ARCH_X86_INTEL_FEATURE_MOVBE)==0)   return level;

  cpuid (0, 0, eax, ebx, ecx, edx);
  if (eax >= 7)
  {
    cpuid (2, 0, eax, ebx, ecx, edx);
    if ((ebx & ARCH_X86_INTEL_FEATURE_AVX2)==0)  return level;
    if ((ebx & ARCH_X86_INTEL_FEATURE_BMI1)==0)  return level;
    if ((ebx & ARCH_X86_INTEL_FEATURE_BMI2)==0)  return level;
    level = 3; 
  }
  return level;
}

#endif
