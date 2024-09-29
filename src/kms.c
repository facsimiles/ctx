#include "ctx-split.h"

#if !__COSMOPOLITAN__
#include <fcntl.h>
#if CTX_PTY
#include <sys/ioctl.h>
#endif
#include <signal.h>
#endif

#if CTX_KMS
#ifdef __linux__
  #include <linux/kd.h>
#endif
  //#include <linux/fb.h>
  //#include <linux/vt.h>
  #include <sys/mman.h>
  //#include <threads.h>
  #include <libdrm/drm.h>
  #include <libdrm/drm_mode.h>


typedef struct _CtxKMS CtxKMS;
struct _CtxKMS
{
   int           key_balance;
   int           key_repeat;
   int           lctrl;
   int           lalt;
   int           rctrl;

   int          fb_fd;
   char        *fb_path;
   int          fb_bits;
   int          fb_bpp;
   int          fb_mapped_size;
   //struct       fb_var_screeninfo vinfo;
   //struct       fb_fix_screeninfo finfo;
   int          vt;
   int          tty;
   int          is_kms;
   cnd_t        cond;
   mtx_t        mtx;
   struct drm_mode_crtc crtc;
};


#if UINTPTR_MAX == 0xffFFffFF
  #define fbdrmuint_t uint32_t
#elif UINTPTR_MAX == 0xffFFffFFffFFffFF
  #define fbdrmuint_t uint64_t
#endif

static void *ctx_fbkms_new_int (CtxKMS *fb, int *width, int *height, const char *path)
{
   struct drm_mode_modeinfo *conn_mode_buf = NULL;
   fb->fb_fd = open(path, O_RDWR | O_CLOEXEC);
   if (!fb->fb_fd)
     return NULL;
   static fbdrmuint_t res_conn_buf[20]={0}; // this is static since its contents
                                         // are used by the flip callback
   fbdrmuint_t res_fb_buf[20]={0};
   fbdrmuint_t res_crtc_buf[20]={0};
   fbdrmuint_t res_enc_buf[20]={0};
   struct   drm_mode_card_res res={0};

   if (ioctl(fb->fb_fd, DRM_IOCTL_SET_MASTER, 0))
   {
     fb->fb_fd = 0;
     return NULL;
   }

   if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
     goto cleanup;
   res.fb_id_ptr=(fbdrmuint_t)res_fb_buf;
   res.crtc_id_ptr=(fbdrmuint_t)res_crtc_buf;
   res.connector_id_ptr=(fbdrmuint_t)res_conn_buf;
   res.encoder_id_ptr=(fbdrmuint_t)res_enc_buf;
   if(ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
      goto cleanup;


   unsigned int i;
   conn_mode_buf = ctx_calloc(20, sizeof(struct drm_mode_modeinfo));
   for (i=0;i<res.count_connectors;i++)
   {
     fbdrmuint_t conn_prop_buf[20]={0},
                     conn_propval_buf[20]={0},
                     conn_enc_buf[20]={0};

     struct drm_mode_get_connector conn={0};

     conn.connector_id=res_conn_buf[i];

     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
       goto cleanup;

     conn.modes_ptr=(fbdrmuint_t)conn_mode_buf;
     conn.props_ptr=(fbdrmuint_t)conn_prop_buf;
     conn.prop_values_ptr=(fbdrmuint_t)conn_propval_buf;
     conn.encoders_ptr=(fbdrmuint_t)conn_enc_buf;

     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
       goto cleanup;

     //Check if connector is connected to a display
     if (conn.count_encoders<1 || conn.count_modes<1 || !conn.encoder_id || !conn.connection)
       continue;

     struct drm_mode_create_dumb create_dumb={0};
     struct drm_mode_map_dumb    map_dumb={0};
     struct drm_mode_fb_cmd      cmd_dumb={0};
     create_dumb.width  = conn_mode_buf[0].hdisplay;
     create_dumb.height = conn_mode_buf[0].vdisplay;
     create_dumb.bpp   = 32;
     create_dumb.flags = 0;
     create_dumb.pitch = 0;
     create_dumb.size  = 0;
     create_dumb.handle = 0;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb) ||
         !create_dumb.handle)
       goto cleanup;

     cmd_dumb.width =create_dumb.width;
     cmd_dumb.height=create_dumb.height;
     cmd_dumb.bpp   =create_dumb.bpp;
     cmd_dumb.pitch =create_dumb.pitch;
     cmd_dumb.depth =24;
     cmd_dumb.handle=create_dumb.handle;
     if (ioctl(fb->fb_fd,DRM_IOCTL_MODE_ADDFB,&cmd_dumb))
       goto cleanup;

     map_dumb.handle=create_dumb.handle;
     if (ioctl(fb->fb_fd,DRM_IOCTL_MODE_MAP_DUMB,&map_dumb))
       goto cleanup;

     void *base = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fb->fb_fd, map_dumb.offset);
     if (!base)
     {
       goto cleanup;
     }
     *width  = create_dumb.width;
     *height = create_dumb.height;

     struct drm_mode_get_encoder enc={0};
     enc.encoder_id=conn.encoder_id;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETENCODER, &enc))
        goto cleanup;

     fb->crtc.crtc_id=enc.crtc_id;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCRTC, &fb->crtc))
        goto cleanup;

     fb->crtc.fb_id=cmd_dumb.fb_id;
     fb->crtc.set_connectors_ptr=(fbdrmuint_t)&res_conn_buf[i];
     fb->crtc.count_connectors=1;
     fb->crtc.mode=conn_mode_buf[0];
     fb->crtc.mode_valid=1;
     if (conn_mode_buf) ctx_free (conn_mode_buf);
     return base;
   }
cleanup:
   if (conn_mode_buf)
     ctx_free (conn_mode_buf);
   ioctl(fb->fb_fd, DRM_IOCTL_DROP_MASTER, 0);
   fb->fb_fd = 0;
   return NULL;
}

void *ctx_fbkms_new (CtxKMS *fb, int *width, int *height)
{
   void *ret = ctx_fbkms_new_int (fb, width, height, "/dev/dri/card0");
   if (!ret)
     ret = ctx_fbkms_new_int (fb, width, height, "/dev/dri/card1");
   return ret;
}

void ctx_fbkms_flip (CtxKMS *fb)
{
  if (!fb->fb_fd)
    return;
  ioctl(fb->fb_fd, DRM_IOCTL_MODE_SETCRTC, &fb->crtc);
}

void ctx_fbkms_close (CtxKMS *fb)
{
  if (!fb->fb_fd)
    return;
  ioctl(fb->fb_fd, DRM_IOCTL_DROP_MASTER, 0);
  close (fb->fb_fd);
  fb->fb_fd = 0;
}

static void ctx_kms_flip (CtxKMS *fb)
{
  if (fb->is_kms)
    ctx_fbkms_flip (fb);
#if 0
  else
    ioctl (fb->fb_fd, FBIOPAN_DISPLAY, &fb->vinfo);
#endif
}

void ctx_kms_destroy (CtxKMS *fb)
{
  if (fb->is_kms)
  {
    ctx_fbkms_close (fb);
  }
#ifdef __linux__
  ioctl (0, KDSETMODE, KD_TEXT);
#endif
  if (system("stty sane")){};
  //ctx_free (fb);
}

#endif
