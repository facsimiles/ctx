card10sim
=========

pycardium transmuted into Javascript using emscriptem, and an epicardium to
emscriten (posix) shim, this should also be possible to use as a console
simulator that uses unicode characters for the display.

Dependencies
------------

Building micropython.js bears the same requirements as the standard MicroPython
ports with the addition of Emscripten (and uglify-js for the minified file).

Build instructions
------------------

In order to build card10sim make sure micropython-1.18 is a sibiling of the
parent folder. and run:

    $ make

To generate the minified file micropython.min.js, run:

    $ make min

Running with HTML
-----------------

The included index.html is intended to be served by a webserver, the
simplest way to get this is to run 

    $ python2 -m SimpleHTTPServer 8000

And open http://localhost:8000/ in a webbrowser.

Testing
-------

Run the test suite using:

    $ make test

API
---

The following functions have been exposed to javascript.

```
mp_js_init(stack_size)
```

Initialize MicroPython with the given stack size in bytes. This must be
called before attempting to interact with MicroPython.

```
mp_js_do_str(code)
```

Execute the input code. `code` must be a `string`.

```
mp_js_init_repl()
```

Initialize MicroPython repl. Must be called before entering characters into
the repl.

```
mp_js_process_char(char)
```

Input character into MicroPython repl. `char` must be of type `number`. This
will execute MicroPython code when necessary.
