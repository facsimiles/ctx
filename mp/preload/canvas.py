import ctx as modctx
 
def get_ctx():  # perhaps turn this into an attribute of the module?
  global _ctx
  if _ctx != None:
      return _ctx
  _ctx = modctx.get_context(23*1024)
  _ctx.flags = _ctx.HASH_CACHE|_ctx.RGB332|_ctx.INTRA_UPDATE
  return _ctx

_ctx = None

def __getattr__(name):
    if name == 'ctx2d':
        return get_ctx()
    elif name == 'ctx':
        return get_ctx()
    elif name == 'width':
        return get_ctx().width
    elif name == 'height':
        return get_ctx().height
      
    raise AttributeError("module " + __name__ + " has no attribute " + name)
    



if __name__=='__main__':
    ctx = get_ctx()
    frames=60
    
    for frame in range(0,frames):
      ctx.start_frame()
      ctx.color((50,50,30)).rectangle(0,0,ctx.width,ctx.height).fill()
      dim = ctx.height * frame / (frames-2.0)
      if frame >= frames-2:
        dim = ctx.height
      ctx.logo(ctx.width/2,ctx.height/2, dim)
      ctx.end_frame()


