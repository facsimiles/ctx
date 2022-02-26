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
   //   add_paths (fullpath);
    }
    else
      paths[paths.length]=fullpath;
          }
  })

  FS.readdir(base).forEach(function(e){
    if (e[0] != '.' && e!= 'dev' && e != 'proc')// && e != 'lib')
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
  add_paths('/')
  html="";
  paths.forEach(function(e){
    html+="<option value='"+e+"'>"+e+"</option>";
  })
  select.innerHTML=html;
}


var mainProgram = function()
{
  FS.rmdir("/home/web_user")
  FS.rmdir("/home")
  FS.rmdir("/tmp")

  repopulate_file_picker()
  console.log(paths)

  //console.log(FS.readdir("/"))
  mp_js_init = Module.cwrap('mp_js_init', 'null', ['number']);
  mp_js_do_str = Module.cwrap('mp_js_do_str', 'number', ['string'], {async:true});
  mp_js_init_repl = Module.cwrap('mp_js_init_repl', 'null', ['null']);
  mp_js_process_char = Module.cwrap('mp_js_process_char', 'number', ['number']);

  MP_JS_EPOCH = (new Date()).getTime();

  if (typeof window === 'undefined' && require.main === module) {
      var fs = require('fs');
      var stack_size = 64 * 1024;
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
  //mp_js_init(64 * 1024);
  mp_js_do_str(editor.getValue());
  }, 500);
}

window.dosave = function ()
{
  var target_path = window.current_path;
  target_path = prompt("Path to save", target_path);

  FS.mkdirTree(target_path);
  try{FS.rmdir(target_path)}catch{};
  FS.writeFile(target_path,
          editor.getValue())

  document.getElementById('mp_js_stdout').innerText = 'saved ' + target_path;
  repopulate_file_picker();
  document.getElementById('file_list').value=target_path;
}


window.windowToCanvas = function(canvas, x, y)
{
  var bbox = canvas.getBoundingClientRect();
  return { x: (x - bbox.left) * (canvas.width  / bbox.width),
           y: (y - bbox.top)  * (canvas.height / bbox.height)}
}


    document.getElementById('mp_js_stdout').addEventListener('print', function(e) {
        document.getElementById('mp_js_stdout').innerText += e.data;
    }, false);
    dorun();


        editor = CodeMirror.fromTextArea(document.getElementById('script'), {
        lineNumbers: true,
        mode: 'text/x-python',
        theme:'cobalt'
     });



window.editor_load = function (path)
{
  var text = '';
  var raw = FS.readFile(path);
  for (var i = 0; i < raw.length; i++)
    text += String.fromCharCode(raw[i]);
  editor.setValue(text)
  window.current_path=path
}

  window.editor_load ("/ctxdemo.py");

            document.onkeydown = function(e)
            {
              if (e.ctrlKey && e.which == 13){ dorun();
                e.preventDefault();
              }
              else if (e.ctrlKey && e.which == 83) 
              {
                e.preventDefault();
                window.dosave();
              }

            }


}

Module["onRuntimeInitialized"] = mainProgram;
