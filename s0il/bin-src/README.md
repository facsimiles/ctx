The files in this folder are each compiled to a ELF binary, they
are also structured to be possible to bundle into the firmware
and appear as inlined exectuables, found as a last resort in
the built-in PATH.

When bundled MAIN(foo) expands to
```
int foo\_main (int argc, char \*\*argv)
```
and otherwise to
```
int main (int argc, char \*\*argv)

```

To be fully bundled you also need to list the binary twice in
../bundled-programs.c

