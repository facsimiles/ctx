import canvas
o=canvas.ctx
 
import os
import gc

# immediate mode user interfaces on a microcontroller with ctx and micropython on rp2040

o.flags = o.HASH_CACHE | o.GRAY4 #| o.REDUCED # the UI is grayscale allow grayscale

clientflags = o.HASH_CACHE | o.RGB332

light_red=(255,80,80)
white=(255,255,255)
black=(0,0,0)
dark_gray=(80,80,80)
light_gray=(170,170,170)

wallpaper_bg    = black
document_bg     = white
document_fg     = black
toolbar_bg      = dark_gray
toolbar_fg      = white
button_bg       = dark_gray
button_fg       = white
dir_selected_bg = white
dir_selected_fg = black
dir_entry_fg    = white
scrollbar_fg    = dark_gray

button_height_vh = 0.33
button_width_vh  = 0.3
font_size_vh     = 0.11

if o.width < o.height:
  font_size_vh = 0.06
  button_width_vh = 0.2

cur = 0
def set_cur(event, no):
    global cur
    cur=no
    return 0

maxframe=10
def linear(start_val,end_val):
  return (frame/maxframe)*(end_val-start_val)+start_val

def mbutton(o, x, y, label, cb, user_data):
   o.save()
   o.font_size *= 0.75
   o.translate(x, y)
   o.begin_path().rectangle(0,0, o.height*button_width_vh * 0.95, o.height * button_height_vh * 0.95)
   o.listen_stop_propagate(o.PRESS, lambda e:cb(e, e.user_data), user_data)
   o.color(button_bg).fill()
   o.color(button_fg).move_to(o.font_size/3, o.height * button_height_vh / 2).text(label)
   o.restore()

def mbutton_thin(o, x, y, label, cb, user_data):
   o.save()
   o.font_size *= 0.8
   o.translate(x, y)
   o.begin_path().rectangle(0,0, o.height*button_width_vh * 0.95, o.font_size * 3)
   o.listen_stop_propagate(o.PRESS, lambda e:cb(e, e.user_data), user_data)
   o.begin_path().rectangle(0,0, o.height*button_width_vh * 0.95, o.font_size)

   o.color(button_bg).fill()
   o.color(button_fg).move_to(o.font_size/3, o.font_size*0.8).text(label)
   o.restore()

response = False

def respond(val):
    global response
    response = val

more_actions=False
def show_more_cb(event, userdata):
    global more_actions
    more_actions = True
    #event.stop_propagate=1

def hide_more_cb(userdata):
    global more_actions
    more_actions = False
    #event.stop_propagate=1


def remove_cb(event, path):
    global more_actions
    os.remove(path)
    more_actions = False
    #event.stop_propagate=1

offset=0

def drag_cb(event):
    global offset
    offset -=event.delta_y/(font_size_vh*o.height) #20.0

view_file=False
run_file=False
frame_no = 0

def run_cb(event, path):
    global run_file
    run_file = path

def view_cb(event, path):
    global view_file, offset
    offset = 0
    view_file = path
    #event.stop_propagate=1

def space_cb(event):
    run_cb(None, current_file)
    print(event.string)

def up_cb(event):
    global cur
    cur -= 1
    if cur <= 0:
        cur = 0

def down_cb(event):
    global cur
    cur += 1

import micropython

def dir_view(o):
   global cur,frame_no,current_file,offset
   frame_no += 1

#   if cur > 4:
       

   #gc.collect()
   o.start_frame()
   #micropython.mem_info()
   o.add_key_binding("space", "", "", space_cb)
   o.add_key_binding("up", "", "", up_cb)
   o.add_key_binding("down", "", "", down_cb)

   o.font_size=o.height*font_size_vh#32

   if (offset ) < cur - (o.height/o.font_size) * 0.8:
     offset = cur - (o.height/o.font_size)*0.2
   if (offset ) > cur - (o.height/o.font_size) * 0.2:
     offset = cur - (o.height/o.font_size)*0.8
   if offset < 0:
     offset = 0



   y = o.font_size - offset * o.font_size
   no = 0 
      
   #o.rectangle(0,0,o.width,o.height)
   #o.listen(o.MOTION, drag_cb, False)
   #o.begin_path()
   
   current_file = ""

   for file in os.listdir('/'):        
      o.rectangle(o.height * button_width_vh * 1.2,y-o.font_size*0.75,
                  o.width - o.height * button_width_vh * 1.2, o.font_size)
      if no == cur:
        o.color(dir_selected_bg)
        current_file = file
        o.fill()
        o.color(dir_selected_fg)
        if False:
          o.save()
          o.text_align=o.RIGHT
          o.move_to(o.width, y)
          o.text(str(os.stat(current_file)[6]))
          o.restore()
      else:
        o.listen(o.PRESS|o.DRAG_MOTION, lambda e:set_cur(e, e.user_data), no)
        # registering multiple times costs heap, refactor this or use uimui
        o.begin_path()
        o.color(dir_entry_fg)
      o.move_to(o.height * button_width_vh * 1.2 + o.font_size * 0.1,y)
      
      o.text(file)
      o.restore()
      
      
      y += o.font_size
      no += 1

   mbutton(o, 0, 0,
          "view", view_cb, current_file)
   mbutton(o, 0, o.height * button_height_vh,
          "run", run_cb, current_file)
   mbutton(o, 0, o.height - o.height * button_height_vh * 1,
          "...", show_more_cb, current_file)
   if more_actions:
     o.rectangle(60,0,o.width,o.height)
     o.listen_stop_propagate(o.PRESS, hide_more_cb, False)
     mbutton(o, o.width-o.height * button_width_vh, o.height - o.height * button_height_vh * 1, "remove", remove_cb, current_file)

   if frame_no > 1000:
     #o.color([255,0,0])
     #o.rectangle(40,40,40,40).fill()
     o.save()
     global_alpha=((frame_no-32)/50.0)
     if global_alpha > 1.0:
         global_alpha = 1.0
     elif global_alpha < 0:
         global_alpha = 0.0
     o.global_alpha = global_alpha
     o.logo(o.width - 32,o.height-32,64)
     o.restore()
   o.end_frame()
   #cur+=1
   
   #gc.collect()
   
def scrollbar_cb(event):
    global offset
    factor = (event.y - o.font_size * 1.5) / (o.height-o.font_size*2);
    if factor < 0.0:
        factor = 0.0
    if factor > 1.0:
        factor = 1.0
    offset = factor * event.user_data

def prev_page_cb(event):
    global offset
    offset -= ((o.height / o.font_size) - 2)

def next_page_cb(event):
    global offset
    offset += ((o.height / o.font_size) - 2)


def file_view(o):
   global offset
   gc.collect()
   o.start_frame()
   o.save()

   o.font_size = o.height * font_size_vh

   #offset += 0.25
   o.rectangle(0,0,o.width,o.height)
   #o.listen(o.MOTION, drag_cb, False)
   o.color(document_bg).fill()
   
   o.color(document_fg)
   o.translate(0,(int(offset)-offset) * o.font_size)
   o.move_to(0, o.font_size*0.8 * 2)
   line_no = 0
   o.font="mono";
   y = o.font_size * 0.8 * 2
   with open(view_file,'r') as file:
     for line in file:
       if line_no > offset and y - o.font_size < o.height:
          o.move_to (0, int(y))
          for word in line.split():
            o.move_to(int(o.x), int(o.y))
            o.text(word + ' ')
          y+=o.font_size
            
       line_no += 1
   o.restore()

   o.save()
   o.color(toolbar_bg)
   o.rectangle(0,0,o.width, o.font_size).clip()
   o.paint()
   o.text_align=o.RIGHT
   o.color(toolbar_fg)
   o.move_to(o.width, o.font_size*0.8)
   o.text(view_file)
   o.restore()
   mbutton_thin(o, 0, o.height * button_height_vh * 0, "close", lambda e,d:view_cb(e,False), -3)

   o.color(scrollbar_fg)
   
   o.move_to(o.width - o.font_size * 1.2, o.font_size + o.font_size)
   
   if True:#draw scrollbar
    o.line_to(o.width - o.font_size * 1.2, o.height - o.font_size)
    o.line_width=1
    o.stroke()
    o.arc(o.width - o.font_size * 1.2,
          o.font_size + o.font_size + (offset / line_no) * (o.height - o.font_size * 2),
          o.font_size*0.8, 0.0, 3.14152*2, 0).stroke()
    o.rectangle(o.width - o.font_size * 2, 0, o.font_size * 2, o.height)
    o.listen(o.PRESS|o.DRAG_MOTION, scrollbar_cb, line_no)
    o.begin_path()
    
   o.rectangle(0, o.font_size,
               o.width, (o.height - o.font_size)/2-1)
   o.listen(o.PRESS, prev_page_cb, False)
   o.begin_path()
   o.rectangle(0, o.font_size + (o.height - o.font_size)/2,
               o.width, (o.height - o.font_size)/2-1)
   o.listen(o.PRESS, next_page_cb, False)
   o.begin_path()
   o.end_frame()



while True:
    if view_file:
        file_view(o)
    else:
        dir_view(o)
        if run_file != False:
            backupflags = o.flags
            # we remove any scratch format from flags
            o.flags = backupflags - (o.flags&(o.GRAY2|o.GRAY4|o.RGB332))
            # and add in low res
            o.flags = clientflags

            gc.collect()
            o.start_frame()    
            o.color(wallpaper_bg).paint()
            o.end_frame()
            o.start_frame()    
            o.color(wallpaper_bg).paint()
            o.font_size=o.height*font_size_vh#32
            o.color(white).move_to(0,o.font_size).text(run_file)
            o.end_frame()
            gc.collect()


            
            try:
              exec(open(run_file).read())
            except Exception as e:
              string_res=io.StringIO(256)
              sys.print_exception(e, string_res)
              for frame in range(0,2):
               o.start_frame()
               o.color(wallpaper_bg).paint()
               o.font_size=o.height*font_size_vh#32
               o.color(white).move_to(0,o.font_size).text(run_file)
               #o.color([255,0,0]).move_to(0,o.font_size*2).text(str(e))
               o.color(light_red).move_to(0,o.font_size*3).text(string_res.getvalue())

               o.end_frame()
              time.sleep(5)
     
            o.flags = backupflags
            run_file = False
            gc.collect()



