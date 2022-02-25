typedef enum
{
  CTX_CPU_ACCEL_NONE        = 0x0,

  /* x86 accelerations */
  CTX_CPU_ACCEL_X86_MMX     = 0x80000000,
  CTX_CPU_ACCEL_X86_SSE     = 0x40000000,
  CTX_CPU_ACCEL_X86_SSE2    = 0x30000000,
  CTX_CPU_ACCEL_X86_SSE3    = 0x20000000,
  CTX_CPU_ACCEL_X86_SSSE3   = 0x10000000,
  CTX_CPU_ACCEL_X86_SSE4_1  = 0x08000000,
  CTX_CPU_ACCEL_X86_SSE4_2  = 0x04000000,
  CTX_CPU_ACCEL_X86_AVX     = 0x03000000,
  CTX_CPU_ACCEL_X86_POPCNT  = 0x02000000,
  CTX_CPU_ACCEL_X86_FMA     = 0x01000000,
  CTX_CPU_ACCEL_X86_MOVBE   = 0x00800000,
  CTX_CPU_ACCEL_X86_F16C    = 0x00400000,
  CTX_CPU_ACCEL_X86_XSAVE   = 0x00300000,
  CTX_CPU_ACCEL_X86_OSXSAVE = 0x00200000,
  CTX_CPU_ACCEL_X86_BMI1    = 0x00100000,
  CTX_CPU_ACCEL_X86_BMI2    = 0x00080000,
  CTX_CPU_ACCEL_X86_AVX2    = 0x00040000,

  CTX_CPU_ACCEL_X86_64_V2 =
    (CTX_CPU_ACCEL_X86_POPCNT|
     CTX_CPU_ACCEL_X86_SSE4_1|
     CTX_CPU_ACCEL_X86_SSE4_2|
     CTX_CPU_ACCEL_X86_SSSE3),

  CTX_CPU_ACCEL_X86_64_V3 =
    (CTX_CPU_ACCEL_X86_64_V2|
     CTX_CPU_ACCEL_X86_BMI1|
     CTX_CPU_ACCEL_X86_BMI2|
     CTX_CPU_ACCEL_X86_AVX|
     CTX_CPU_ACCEL_X86_AVX2|
     CTX_CPU_ACCEL_X86_OSXSAVE|
     CTX_CPU_ACCEL_X86_MOVBE),

} CtxCpuAccel;

CtxCpuAccel ctx_cpu_accel (void);

#if defined(ARCH_X86) && defined(USE_MMX) && defined(__GNUC__)

typedef enum
{
  ARCH_X86_VENDOR_NONE,
  ARCH_X86_VENDOR_INTEL,
  ARCH_X86_VENDOR_AMD,
  ARCH_X86_VENDOR_CENTAUR,
  ARCH_X86_VENDOR_CYRIX,
  ARCH_X86_VENDOR_NSC,
  ARCH_X86_VENDOR_TRANSMETA,
  ARCH_X86_VENDOR_NEXGEN,
  ARCH_X86_VENDOR_RISE,
  ARCH_X86_VENDOR_UMC,
  ARCH_X86_VENDOR_SIS,
  ARCH_X86_VENDOR_HYGON,
  ARCH_X86_VENDOR_UNKNOWN    = 0xff
} X86Vendor;

enum
{
  ARCH_X86_INTEL_FEATURE_MMX      = 1 << 23,
  ARCH_X86_INTEL_FEATURE_XMM      = 1 << 25,
  ARCH_X86_INTEL_FEATURE_XMM2     = 1 << 26,

  ARCH_X86_AMD_FEATURE_MMXEXT     = 1 << 22,
  ARCH_X86_AMD_FEATURE_3DNOW      = 1 << 31,

  ARCH_X86_CENTAUR_FEATURE_MMX    = 1 << 23,
  ARCH_X86_CENTAUR_FEATURE_MMXEXT = 1 << 24,
  ARCH_X86_CENTAUR_FEATURE_3DNOW  = 1 << 31,

  ARCH_X86_CYRIX_FEATURE_MMX      = 1 << 23,
  ARCH_X86_CYRIX_FEATURE_MMXEXT   = 1 << 24
};

enum
{
  ARCH_X86_INTEL_FEATURE_PNI      = 1 << 0
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

 // extended features

  ARCH_X86_INTEL_FEATURE_BMI1     = 1 << 3,
  ARCH_X86_INTEL_FEATURE_BMI2     = 1 << 8,
  ARCH_X86_INTEL_FEATURE_AVX2     = 1 << 5,
};

static X86Vendor
arch_get_vendor (void)
{
  guint32 eax, ebx, ecx, edx;
  guint32 id32[4];
  char *id = (char *) id32;

  cpuid (0, eax, ebx, ecx, edx);

  if (eax == 0)
    return ARCH_X86_VENDOR_NONE;

  id32[0] = ebx;
  id32[1] = edx;
  id32[2] = ecx;

  id[12] = '\0';

  if (ctx_strcmp (id, "AuthenticAMD") == 0)
    return ARCH_X86_VENDOR_AMD;
  else if (ctx_strcmp (id, "HygonGenuine") == 0)
    return ARCH_X86_VENDOR_HYGON;
  else if (ctx_strcmp (id, "GenuineIntel") == 0)
    return ARCH_X86_VENDOR_INTEL;

  return ARCH_X86_VENDOR_UNKNOWN;
}

static uint32_t
arch_accel_intel (void)
{
  uint32_t caps = 0;

  {
    guint32 eax, ebx, ecx, edx;

    cpuid (1, eax, ebx, ecx, edx);

    if ((edx & ARCH_X86_INTEL_FEATURE_MMX) == 0)
      return 0;

    caps = GEGL_CPU_ACCEL_X86_MMX;

    if (edx & ARCH_X86_INTEL_FEATURE_XMM)
      caps |= GEGL_CPU_ACCEL_X86_SSE | GEGL_CPU_ACCEL_X86_MMXEXT;

    if (edx & ARCH_X86_INTEL_FEATURE_XMM2)
      caps |= GEGL_CPU_ACCEL_X86_SSE2;

    if (ecx & ARCH_X86_INTEL_FEATURE_PNI)
      caps |= GEGL_CPU_ACCEL_X86_SSE3;

    if (ecx & ARCH_X86_INTEL_FEATURE_SSSE3)
      caps |= GEGL_CPU_ACCEL_X86_SSSE3;

    if (ecx & ARCH_X86_INTEL_FEATURE_SSE4_1)
      caps |= GEGL_CPU_ACCEL_X86_SSE4_1;

    if (ecx & ARCH_X86_INTEL_FEATURE_SSE4_2)
      caps |= GEGL_CPU_ACCEL_X86_SSE4_2;

    if (ecx & ARCH_X86_INTEL_FEATURE_AVX)
      caps |= GEGL_CPU_ACCEL_X86_AVX;

    if (ecx & ARCH_X86_INTEL_FEATURE_POPCNT)
      caps |= GEGL_CPU_ACCEL_X86_POPCNT;

    if (ecx & ARCH_X86_INTEL_FEATURE_XSAVE)
      caps |= GEGL_CPU_ACCEL_X86_XSAVE;

    if (ecx & ARCH_X86_INTEL_FEATURE_OSXSAVE)
      caps |= GEGL_CPU_ACCEL_X86_OSXSAVE;

    if (ecx & ARCH_X86_INTEL_FEATURE_FMA)
      caps |= GEGL_CPU_ACCEL_X86_FMA;

    if (ecx & ARCH_X86_INTEL_FEATURE_F16C)
      caps |= GEGL_CPU_ACCEL_X86_F16C;

    if (ecx & ARCH_X86_INTEL_FEATURE_MOVBE)
      caps |= GEGL_CPU_ACCEL_X86_MOVBE;

    cpuid (0, eax, ebx, ecx, edx);
    if (eax >= 7)
    {
      cpuid (7, eax, ebx, ecs, edx);
      if (ebx & ARCH_X86_INTEL_FEATURE_AVX2)
        caps |= GEGL_CPU_ACCEL_X86_AVX2;
      if (ebx & ARCH_X86_INTEL_FEATURE_BMI1)
        caps |= GEGL_CPU_ACCEL_X86_BMI1;
      if (ebx & ARCH_X86_INTEL_FEATURE_BMI2)
        caps |= GEGL_CPU_ACCEL_X86_BMI2;
    }
  }

  return caps;
}

static guint32
arch_accel_amd (void)
{
  guint32 caps;

  caps = arch_accel_intel ();

#ifdef USE_MMX
  {
    guint32 eax, ebx, ecx, edx;

    cpuid (0x80000000, eax, ebx, ecx, edx);

    if (eax < 0x80000001)
      return caps;

#ifdef USE_SSE
    cpuid (0x80000001, eax, ebx, ecx, edx);

    if (edx & ARCH_X86_AMD_FEATURE_3DNOW)
      caps |= GEGL_CPU_ACCEL_X86_3DNOW;

    if (edx & ARCH_X86_AMD_FEATURE_MMXEXT)
      caps |= GEGL_CPU_ACCEL_X86_MMXEXT;
#endif /* USE_SSE */
  }
#endif /* USE_MMX */

  return caps;
}

#define HAVE_ACCEL 1

static          sigjmp_buf   jmpbuf;
static volatile sig_atomic_t canjump = 0;

static void
sigill_handler (gint sig)
{
  if (!canjump)
    {
      signal (sig, SIG_DFL);
      raise (sig);
    }

  canjump = 0;
  siglongjmp (jmpbuf, 1);
}

static guint32
arch_accel (void)
{
  signal (SIGILL, sigill_handler);

  if (sigsetjmp (jmpbuf, 1))
    {
      signal (SIGILL, SIG_DFL);
      return 0;
    }

  canjump = 1;

  asm volatile ("mtspr 256, %0\n\t"
                "vand %%v0, %%v0, %%v0"
                :
                : "r" (-1));

  signal (SIGILL, SIG_DFL);

  return GEGL_CPU_ACCEL_PPC_ALTIVEC;
}
#endif /* __GNUC__ */

#endif /* ARCH_PPC && USE_ALTIVEC */


static GeglCpuAccelFlags
cpu_accel (void)
{
#ifdef HAVE_ACCEL
  static guint32 accel = ~0U;

  if (accel != ~0U)
    return accel;

  accel = arch_accel ();

  return (GeglCpuAccelFlags) accel;

#else /* !HAVE_ACCEL */
  return GEGL_CPU_ACCEL_NONE;
#endif
}
