/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017, 2018 Rami Ali
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

var Module = {};

var paths=[]

function add_paths (base)
{
  FS.readdir(base).forEach(function(e){
    if (e[0] != '.' && e!= 'dev' && e != 'proc'
            && !e.endsWith('.png')
            && !e.endsWith('.tvg')
            && !e.endsWith('.jpg')
            && !e.endsWith('.gif')
    )
          {
    var fullpath = base + '/' + e;
    if (base == '/') fullpath = base + e;
    if (FS.isDir(FS.stat(fullpath).mode))
    {
      add_paths (fullpath);
    }
    else
      paths[paths.length]=fullpath.substr(3);
          }
  })

  FS.readdir(base).forEach(function(e){
    if (e[0] != '.' && e!= 'dev' && e != 'proc' && e != 'lib')
          {
    var fullpath = base + '/' + e;
    if (base == '/') fullpath = base + e;
    if (FS.isDir(FS.stat(fullpath).mode))
    {
      add_paths (fullpath);
    }
  //else
  //    paths[paths.length]=fullpath;
          }
  })
}

function repopulate_file_picker()
{
  var select = document.getElementById('file_list');

  paths=[]
  add_paths('/sd')
  html="";
  paths.forEach(function(e){
    html+="<option value='"+e+"'>"+e+"</option>";
  })
  select.innerHTML=html;

  document.getElementById('file_list').value=window.current_path;
}


var mainProgram = function()
{
  FS.rmdir("/home/web_user")
  FS.rmdir("/home")
  FS.mkdir("/sd");
  FS.mount(IDBFS, {}, '/sd');
  FS.syncfs(true, function (err) { });

  setTimeout(repopulate_file_picker, 550);
  setTimeout(function(){

          try { a=FS.stat("/sd/main.py");
          } catch (e){
            document.getElementById('mp_js_stdout').innerText = 'installing main';
            FS.writeFile('/sd/main.py', FS.readFile('/main.py'));
          }
          try { a=FS.stat("/sd/canvas.py");
          } catch (e){
            document.getElementById('mp_js_stdout').innerText = 'installing main';
            FS.writeFile('/sd/canvas.py', FS.readFile('/canvas.py'));
            FS.writeFile('/sd/README', FS.readFile('/README'));
          }

          FS.syncfs(false, function (err) { });
          repopulate_file_picker();
          window.editor_load ("/main.py");
    dorun();
  }, 600);

  mp_js_init = Module.cwrap('mp_js_init', 'null', ['number']);
  mp_js_do_str = Module.cwrap('mp_js_do_str', 'number', ['string'], {async:true});
  mp_js_init_repl = Module.cwrap('mp_js_init_repl', 'null', ['null']);
  mp_js_process_char = Module.cwrap('mp_js_process_char', 'number', ['number']);

  MP_JS_EPOCH = (new Date()).getTime();

  if (typeof window === 'undefined' && require.main === module) {
      var fs = require('fs');
      var stack_size = window.heap;
      var contents = '';
      var repl = true;

      for (var i = 0; i < process.argv.length; i++) {
          if (process.argv[i] === '-X' && i < process.argv.length - 1) {
              if (process.argv[i + 1].includes('stack=')) {
                  stack_size = parseInt(process.argv[i + 1].split('stack=')[1]);
                  if (process.argv[i + 1].substr(-1).toLowerCase() === 'k') {
                      stack_size *= 1024;
                  } else if (process.argv[i + 1].substr(-1).toLowerCase() === 'm') {
                      stack_size *= 1024 * 1024;
                  }
              }
          } else if (process.argv[i].includes('.py')) {
              contents += fs.readFileSync(process.argv[i], 'utf8');
              repl = false;;
          }
      }
      mp_js_init(stack_size);

      if (repl) {
          mp_js_init_repl();
          process.stdin.setRawMode(true);
          process.stdin.on('data', function (data) {
              for (var i = 0; i < data.length; i++) {
                  if (mp_js_process_char(data[i])) {
                      process.exit()
                  }
              }
          });
      } else {
          process.exitCode = mp_js_do_str(contents);
      }
  }

window.running = 0;
window.heap = 192;

window.dorun = function ()
{
  if (window.running != 0)
  {
    _mp_quit();
  }
  else
    window.running = 1;
  setTimeout(function(){

  document.getElementById('mp_js_stdout').innerText = '';
  mp_js_do_str(editor.getValue());
  }, 500);
}

window.dosave = function ()
{
  var target_path = window.current_path;
  var abs_path = make_abs (target_path);
  FS.mkdirTree(abs_path);
  try{FS.rmdir(abs_path)}catch{};
  FS.writeFile(abs_path,
          editor.getValue())
  FS.syncfs(false, function (err) { });

  document.getElementById('mp_js_stdout').innerText = 'saved ' + target_path;
  repopulate_file_picker();
  document.getElementById('file_list').value=target_path;
}

window.dosaveas = function ()
{
  var target_path = window.current_path;
  target_path = prompt("Path to save", target_path);

  var abs_path = make_abs (target_path);
  FS.mkdirTree(abs_path);
  try{FS.rmdir(abs_path)}catch{};
  FS.writeFile(abs_path,
          editor.getValue())
  FS.syncfs(false, function (err) { });

  document.getElementById('mp_js_stdout').innerText = 'saved ' + target_path;
  repopulate_file_picker();
  document.getElementById('file_list').value=target_path;
}

window.dounlink = function ()
{
  var target_path = window.current_path;
  //var response = prompt("really remove?", "no");
  //if (response == "yes")
  {
    FS.unlink(make_abs(target_path));
    FS.syncfs(false, function (err) { });
    document.getElementById('mp_js_stdout').innerText = 'removed file ' + target_path;
    repopulate_file_picker();
  }
}


window.windowToCanvas = function(canvas, x, y)
{
  var bbox = canvas.getBoundingClientRect();
  return { x: (x - bbox.left) * (canvas.width  / bbox.width),
           y: (y - bbox.top)  * (canvas.height / bbox.height)}
}

  window.in_escape    = false;
  window.in_sbracket  = false;
  window.rel_cur_pos = 0;

    document.getElementById('mp_js_stdout').addEventListener('print', function(e) {
     var stdout = document.getElementById('mp_js_stdout');
        if (window.in_escape)
        {
          if (window.in_sbracket)
          {
            switch (e.data)
            {
              case 'K':
                {
                  var string = stdout.innerText;
                stdout.innerText = string.substring(
                        0, string.length-1);
                }
              break;
              default:
                stdout.innerText = stdout.innerText + '[CSI ' + e.data +']';
            }
            window.in_escape = false;
          }
          else
          {
            if (e.data == '[')
            {
              window.in_sbracket = true;
            }
            else
            {
              window.in_escape = false;
            }
          }
        }
        else
        {
          if (e.data.charCodeAt(0) == 8)
          {
//          window.rel_cur_pos -= 1;
          }
          else if (e.data.charCodeAt(0) == 10)
          {
          }
          else if (e.data.charCodeAt(0) == 27)
          {
            window.in_escape = true;
            window.in_sbracket = false;
          }
          else
          {
            if (window.rel_cur_pos == 0)
            {
            stdout.innerText += e.data;
            }
            else
            {
              var string = stdout.innerText;
              stdout.innerText =
                 string.substring(0, string.length + window.rel_cur_pos) +
                 e.data
                 + string.substring(string.length + window.rel_cur_pos + 2, string.length - 1);
              //window.rel_cur_pos ++;

            }
            stdout.scrollTo(0, 10000000);
          }
        }
    }, false);


        editor = CodeMirror.fromTextArea(document.getElementById('script'), {
        lineNumbers: true,
        mode: 'text/x-python',
        theme:'cobalt'
     });


window.heap_set = function (newheap)
{
   window.heap = parseInt(newheap) * 1024;
   _mp_js_set_heap_size (window.heap);
}

function make_abs (path)
{
  if (path=='/') return '/sd';
  if (path[0]=='/') return '/sd'+path;
  return '/sd/'+path;
}

window.editor_load = function (path)
{
  var text = '';
  var raw = FS.readFile(make_abs(path));
  for (var i = 0; i < raw.length; i++)
    text += String.fromCharCode(raw[i]);
  editor.setValue(text)
  window.current_path=path
  document.getElementById('file_list').value=window.current_path;
}


            document.onkeydown = function(e)
            {
              if (e.ctrlKey && e.which == 13){ dorun();
                e.preventDefault();
              }
              else if (e.shiftKey && e.ctrlKey && e.which == 83) 
              {
                e.preventDefault();
                window.dosaveas();
              }
              else if (e.ctrlKey && e.which == 83) 
              {
                e.preventDefault();
                window.dosave();
              }

            }
  document.getElementById('mp_js_stdout').onkeydown=function(e)
  {
     if (e.ctrlKey)
     {
        if (e.which >= 65 && e.which <= 75){
          e.preventDefault();
          mp_js_process_char (e.which - 64);
                console.log('foo!')
        }
     }
     else
     {
        e.preventDefault();

     if (e.which == 32 || e.which == 13 || e.which == 8 || e.which == 9) {
        mp_js_process_char (e.which);
     }
     else if (e.which >= 65 && e.which <= 90)
     {
        mp_js_process_char (e.shiftKey?e.which:e.which+32);
     }
     else if (e.which >=48 && e.which <= 57)
     {
        /* TODO : replace with a keymap  */
        if (e.shiftKey)
          switch (e.which)
          {
            case 48: mp_js_process_char (')'.charCodeAt(0)); break;
            case 49: mp_js_process_char ('!'.charCodeAt(0)); break;
            case 50: mp_js_process_char ('@'.charCodeAt(0)); break;
            case 51: mp_js_process_char ('#'.charCodeAt(0)); break;
            case 52: mp_js_process_char ('$'.charCodeAt(0)); break;
            case 53: mp_js_process_char ('%'.charCodeAt(0)); break;
            case 54: mp_js_process_char ('^'.charCodeAt(0)); break;
            case 55: mp_js_process_char ('&'.charCodeAt(0)); break;
            case 56: mp_js_process_char ('*'.charCodeAt(0)); break;
            case 57: mp_js_process_char ('('.charCodeAt(0)); break;
          }
        else
          mp_js_process_char (e.which);
     }
     else
     {
        switch (e.which)
        {
          
/*
          case 38://up
            mp_js_process_char (27);
            mp_js_process_char ('['.charCodeAt(0));
            mp_js_process_char ('A'.charCodeAt(0));
            break;
          case 40://down
            mp_js_process_char (27);
            mp_js_process_char ('['.charCodeAt(0));
            mp_js_process_char ('B'.charCodeAt(0));
            break;
          case 37://left
            mp_js_process_char (27);
            mp_js_process_char ('['.charCodeAt(0));
            mp_js_process_char ('D'.charCodeAt(0));
            break;
          case 39://right
            mp_js_process_char (27);
            mp_js_process_char ('['.charCodeAt(0));
            mp_js_process_char ('C'.charCodeAt(0));
            break;
                        */
          case 188: mp_js_process_char (e.shiftKey?60:44);break;
          case 190: mp_js_process_char (e.shiftKey?62:46);break;
          case 222: mp_js_process_char (e.shiftKey?34:39);break;
          case 59:  mp_js_process_char (e.shiftKey?59:58);break;
          case 191: mp_js_process_char (e.shiftKey?63:47);break;
          case 219: mp_js_process_char (e.shiftKey?123:91);break;
          case 221: mp_js_process_char (e.shiftKey?125:93);break;
          case 220: mp_js_process_char (e.shiftKey?124:92);break;
          case 173: mp_js_process_char (e.shiftKey?95:45);break;
          case 61: mp_js_process_char (e.shiftKey?43:61);break;
          case 192: mp_js_process_char (e.shiftKey?126:96);break;
          default:
           console.log('unhandled key' + e.which);
        }
     }
     }
  }

}

Module["onRuntimeInitialized"] = mainProgram;
