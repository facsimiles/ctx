#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "squoze.h"

const char *strings[]={"0",
  "\n",
  "☺",
  "foo", "oof", "bar", "abc" "foo", "bar", "FOO", "Foo", "ooF",
  "lineTo","line_to","moveTo","curveTo","reset",
  "TEST",
  "abc",
  "a-_.b",
  "ᛖᚴ","ᚷᛖᛏ","ᛖᛏᛁ","ᚧᚷ","ᛚᛖᚱ","ᛘᚾᚦ","ᛖᛋᛋᚨ","ᚧᚡᛖ","ᚱᚧᚨ","ᛋᚨᚱ",
"আ","মি"," ","কাঁচ"," খেতে পারি",", তাতে আ","মার কোনো ক্ষতি হয় না।" ,
"मी काच खा","ऊ शकतो,","मला ते दुख","त नाही." ,
"ನನಗೆ"," ಹಾನಿ ","ಆಗದೆ","ನಾನು ಗ","ನ್ನು ","ನಬಹುದು" ,
"मैंकाँच ","सकता","औरमुझेउससे","कोई ","ट नहीं पहुंचती." ,
"Malayalam: എനിക്ക് ഗ്ലാസ് തിന്നാം. അതെന്നെ വേദനിപ്പിക്കില്ല." ,
  "abcxyz",
  "ABCXYZ",
  "3.141",
  "01234",
  "012345",
  "0123456",
  "01234567",
  "012345678",
  "0123456789",
  "eeeeeeeeeez",
  "eeeeeeeeeel",
  "eeeeeeeeeeek",
  "zzk",
  "zzzk",
  "zzzzk",
  "zzzzzk",
  "zzzzzzk",
  "zzzzzzzk",
  "margin_top_x6",
  "zzzzzzzzk",
  "zzzzzzzzzk",
  "zzzzzzzzzzk",
  "56789",
  "01259",
  "CamelCase",
  "test",
  "Test",
  "TEST",
  "TeST",
  "TesT",
  "tesT",
  "the",
  "this",
  "zzz",
  "Abcdefghijklmn",
  "abcdefghijkl",
  "abcdefghijklmn",
  "/pBwHor0FpO",
  "123",
  "1234",
  "12345",
  "123456",
  "1234567",
  "12345678",
  "123456789",
  "1234567890",
  "12345678901",
  "eek",
  "eeek",
  "eeeek",
  "Eeeeek",
  "Eeeeeek",
  "Eeeeeeek",
  "Eeeeeeeek",
  "Eeeeeeeeek",
  "Eeeeeeeeeek",
  "Eeeeeeeeeeek",
  "Eeeeeeeeeeeek",
  "Eeeeeeeeeeeeek",
  "EeEEEEeeeeeeeek",
  "eeeeeeeeeeeeeeek",
  "eeeeeeeeeeeeeeeek",
  "eeeeeeeeeeeeeeeeek",
  "eeeeeeeeeeeeeeeeeek",
  "eeeeeeeeeeeeeeeeeeek",
  "eeeeeeeeeeeeeeeeeeeek",
  "aaaaaaaaaaaaaaaaaaaaaa a1 100A A10 1a 11 111 aa 1a1 a1a 1a1a1 a1a1a b> 100b abc10 abc2 1000b 10000b 120a 52bt a"
  "100",
  "It is time for some longer words, I wonder if I can come up with some, perpendicular is a good one, the escaping can be incorporated back on track, I wonder.",
  "foo/a",
  "pBwHor as \n cd \n abc",
  "abcdefghijklmnopqrstuvwxyz",
  "abcdefghijkl",
  "Øyvind Kolås",
  "Øyvind","Kolås",
  "pippin",
  "Pippin",
  "PIPPIN",
  "pippin@gimp.org",
  "+479762898",
  "https://",
  "https://c",
  "https://ct",
  "https://ctx",
  "https://ctx.graphic",
  "https://ctx.graphics",
  "https://ctx.graphics/",
  "https://ctx.graphics/a",
  "abcdefghijklmn",
  "FOO BAR foo bar",
  ",",
  " ",
  "n ",
  "n",
  "ø ",
  /*
  " ø",
  " ø ",
  " n",
  */
  "if this is",
  "if the ",
  "if the n",
  "if the  ",
  "if this ",
  "if this n",
  "\\",
  "_\\.",
  ".",
  "0",
  "Ab",
  "a",
  "1",
  "_",
  "\\",
  ",.",
  "1",
  "10",
  "æøå",
  "ÆØÅ",
  "bjørn",
  "Bjørn",
  "Bjørnd",
  "bjørnd",
  "Bjørnda",
  "bjørnda",
  "Bjørndal",
  "bjørndal",
  "Hi Mom ☺!",
  NULL
};

int main (int argc, char **argv)
{
  int wrong = 0;
  int dim = 6;

  for (int i = 0; strings[i]; i++)
  {
    uint64_t hash;
    const char *decoded;

    char input[4096];
    for (int j = 0; j < 100; j++)
    {
      if (j)
        sprintf (input, "%s-%i", strings[i], j);
      else
        sprintf (input, "%s", strings[i]);

    switch (dim)
    {
       case 6:
        hash = squoze32 (input);
        decoded = squoze32_decode (hash);
        break;
       case 12:
        hash = squoze62 (input);
        decoded = squoze62_decode (hash);
        break;
       case 10:
       default:
        hash = squoze52 (input);
        decoded = squoze52_decode (hash);
        break;
    }
    if (decoded && strcmp (input, decoded))
    {
      printf ("%s = %lu = %s\n", input, hash, decoded);
      wrong ++;
    }
    }
    fprintf (stderr, "\r%.1f%% %i ", (100.0*i) / ((sizeof(strings)/sizeof(strings[0]))-1), i);
  }
  fprintf (stderr, "\r            ");
  if (wrong)
  {
    printf ("%i WRONG\n", wrong);
    return 1;
  }
  return 0;
}
