#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define ITERATIONS                  20
#define SQUOZE_IMPLEMENTATION
#define SQUOZE_INITIAL_POOL_SIZE   (1<<20)

#include "squoze.h"
#include <sys/time.h>
#define usecs(time)    ((uint64_t)(time.tv_sec - start_time.tv_sec) * 1000000 + time.     tv_usec)

typedef enum {
	TEST_CREATE,
	TEST_LOOKUP,
	TEST_DECODE,
	TEST_CONCAT,
} TestType;

static struct timeval start_time;
static void
init_ticks (void)
{
  static int done = 0;
  if (done)
    return;
  done = 1;
  gettimeofday (&start_time, NULL);
}

static inline unsigned long
ticks (void)
{
  struct timeval measure_time;
  init_ticks ();
  gettimeofday (&measure_time, NULL);
  return usecs (measure_time) - usecs (start_time);
}


typedef struct _StringRef StringRef;
struct _StringRef
{
  StringRef *next;
  char *str;
};

StringRef *dict = NULL;

static float do_test_round (int words, TestType type)
{
  Sqz *refs[words];
  sqz_atexit (); // drop existing interned strings
  long start = ticks();
  {
   int i = 0;
   for (StringRef *str = dict; str; str=str->next,i++)
   {
    char input[4096];
    sprintf (input, "%s", str->str);
    refs [i] = sqz (input);
   }
  }

  switch (type)
  {
    case TEST_CREATE:
      return ticks()-start;
      break;
    case TEST_LOOKUP:
      start = ticks();
      for (StringRef *str = dict; str; str=str->next)
      {
        sqz (str->str);
      }
      return ticks()-start;
    case TEST_DECODE:
      start = ticks ();
      {
        int i = 0;
        char temp[1024];
        for (StringRef *str = dict; str; str=str->next, i++)
        {
          char tmp[16];
	  strcpy (temp, sqz_decode (refs[i], tmp));
        }
      }
      return ticks ()-start;
    case TEST_CONCAT:
      {
      Sqz *str_b = sqz("s");
      start = ticks ();
        int i = 0;
        for (StringRef *str = dict; str; str=str->next, i++)
        {
	  Sqz *res = sqz_cat (refs[i], str_b);
	  if (res) {};
        }
      return ticks ()-start;
      }
  }

  return 0.1;
}

static float do_test (int words, int iterations, TestType type)
{
  float best_ms = 10000000.0f;
  for (int i = 0; i < iterations; i ++)
  {
    float ms = do_test_round (words, type);
    if (ms < best_ms) best_ms = ms;
  }
  return best_ms;
}

#if 0
void sqz_test (void)
{
  Sqz *a = sqz ("æøådefghi");
  char temp[16];
  printf ("[%s]\n", sqz_decode (a, temp));

  for (int i = -1; i < 8;i ++)
    printf ("unichar at %i:%u\n", i, sqz_char_at (a, i));

  printf ("substring(2, 3) [%s]\n",  sqz_decode (sqz_substring (a, 2, 3), temp));
  printf ("substring(2, 13) [%s]\n", sqz_decode (sqz_substring (a, 2, 13), temp));

  sqz_append(&a, sqz("!"));
  printf ("[%s]\n",   sqz_decode (a, temp));
  sqz_remove_char (&a, 2);
  printf ("[%s]\n",   sqz_decode (a, temp));
  sqz_insert_char (&a, 3, 'd');
  printf ("[%s]\n",   sqz_decode (a, temp));

}
#endif

int main (int argc, char **argv)
{
  int wrong = 0;
  //sqz_test();
  //return 0;
  int iterations = ITERATIONS;
  FILE* f;
  f  = fopen("words.txt", "r");
  if (!f)
    f = fopen("/usr/share/dict/words", "r");
  if (!f)
  {
    fprintf (stderr, "no word list found words.txt in current dir or /usr/share/dict/words expected\n");
    return -1;
  }
  int words = 0;
    // Read file line by line, calculate hash
    char line[1024];
    int max_word_len = 10;
    if (getenv ("MAX_WORD_LEN"))
      max_word_len = atoi(getenv("MAX_WORD_LEN"));
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';   // strip newline
	if (strlen (line) <= max_word_len)
	{
	StringRef *ref = calloc (1, sizeof (StringRef));
	ref->str = strdup (line);
	ref->next = dict;
	dict = ref;
	words++;
	}
    }
    fclose(f);
#ifdef HEAD
    printf ("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/><title>squoze - reversible unicode string hashes</title><style>\n"
"th {font-weight:normal;text-align:left;}\n"
"td { text-align:right;border-right: 1px solid gray;border-bottom:1px solid gray;}\n"
"p{text-align: justify;}\n"
"h2,h3,table,td,tr,th{font-size:1em;}\n"
"body{font-family:monospace; max-width:40em;margin-left:auto;margin-right:auto;background:#fff;padding:1em; hyphens: auto; hyphenate-limit-chars: 6 3 2; }\n"
"html{background:#234;font-size:1.2em;}\n"
"em, dt { color: #832}\n"
"h1 { font-size:1.33em;}\n"
"h2 { border-top: 1px solid black; padding-top: 0.5em; }\n"
"table {margin-left:auto;margin-right:auto;}\n"

"</style></head><body>");


    printf ("<h1>Squoze - reversible unicode string hashes.</h2>\n");
    printf ("<div style='font-style:italic; text-align:right;'>compute more, save energy, parse faster;<br/>with strings squozed to fit in computer words</div>");
    printf ("<p>Storing strings in computer words is an <a href='https://en.wikipedia.org/wiki/SQUOZE'>old practice</a>, here this optimization technique is modernized for unicode and combined with string interning. The least significant bit of pointers (or hash) is used to indicate if a string is embedded or not - even values are interpreted as embedded strings. By storing short strings directly  we avoid cache and lock contention involved in hash-table and RAM overhead..</p>\n");
    printf("<p>Calling it a hash might seem like a bit of a misnomer, but the same optimization can be used for with larger hashes in content addressed storage systems like git/IPFS, as well as in parsers with a limited vocabulary so the resulting behavior is that of a perfect hash.</p>");

    printf("<p>The embedded data is stored in either UTF-8 or transcoded to <a href='utf5+/'>UTF5+</a>, a 5bit variable length and dynamic window unicode coding scheme.</p>");

    printf ("<p>The benefits of embedding strings in pointers is dataset dependent - but note that in the english language the average word length is 5. If all data fits in caches the added computational overhead might only slightly reduce cache contention. As I understand it microcontrollers have no L1/L2 cache, but there can still be benefits from RAM savings</p>");

    printf ("<p>A series of subvariants have been specified as part of parameterizing the benchmarks, and definining the encoding: </p>");
    
    printf ("<p>squoze64-utf8 achieves <b>7x speedup</b> over murmurhash one-at-a-time used for initial string interning and <b>3x speedup</b> for subsequent lookups of the same string when the strings are shorther than 8bytes of utf8, see <a href='#benchmarks'>the benchmarks</a> for details.</p>");


    printf ("<dl>");

    printf ("<dt>squoze64-utf8</dt><dd>UTF-8 embed encoding, supporting up 8 UTF-8 bytes of embedded data.</dd>\n");
    //printf ("<dt>squoze-bignum-utf5</dt><dd><a href='utf5+/'>UTF5+</a> encoding, supporting arbitrary length unicode strings stored in bignums.</dd>\n");
    printf ("<dt>squoze32-utf8</dt><dd>UTF-8 embed encoding, supporting up 4 UTF-8 bytes of embedded data</dd>\n");
    printf ("<dt>squoze32-utf5</dt><dd><a href='utf5+/'>UTF5+</a> embed encoding, supporting up to 6 lower-case ascii of embedded data</dd>\n");
    printf ("<dt>squoze52-utf5</dt><dd><a href='utf5+/'>UTF5+</a> embed encoding, supporting up to 10 lower-case ascii of embedded data, 52bit integers can be stored without loss in a double.</dd>\n");
    printf ("<dt>squoze62-utf5</dt><dd><a href='utf5+/'>UTF5+</a> embed encoding, supporting up to 12 unicode code points.</dd>\n");
    printf ("</dl>\n");



    printf ("<h2>squoze64 implementation</h2>");
    printf ("<p>the squoze-64 encoding is a small bitmanipulation of UTF-8 in-memory encoding, for strings that will fit only the first byte is manipulated and only if it ascii and not vertical-tab (ASCII 11) the value is doubled and 1 is added. When the first byte does not match our constraints we store 23 - which means the following 7bytes directly encode the value. </p>");

    printf ("<p>The following example code is an illustration based on the benchmarked code.</p>\n");

    printf (
"<pre>void *squoze64(const char *utf8, size_t len)\n"
"{\n"
"  size_t   squoze_dim = 64;\n"
"  uint64_t hash       = 0;\n"
"  uint8_t *encoded    = (uint8_t*)&amp;hash;\n"
"  uint8_t  first_byte = ((uint8_t*)utf8)[0];\n"
"\n"
"  if (first_byte &lt; 128\n"
"      &amp;&amp; first_byte != 11\n"
"      &amp;&amp; (len &lt;= (squoze_dim/8)))\n"
"  {\n"
"    for (int i = 0; utf8[i]; i++)\n"
"      encoded[i] = utf8[i];\n"
"    encoded[0] = encoded[0] * 2 + 1;\n"
"    return (void*)*((uint64_t*)&amp;encoded[0]);\n"
"  }\n"
"  else if (len &lt;= (squoze_dim/8)-1)\n"
"  {\n"
"    for (int i = 0; utf8[i]; i++)\n"
"      encoded[i+1] = utf8[i];\n"
"    encoded[0] = 23;\n"
"    return (void*)*((uint64_t*)&amp;encoded[0]);\n"
"  }\n"
"\n"
"  return intern_string(utf8); // fall back to\n"
"                              // regular interning\n"
"                              // and rely on an aligned\n"
"                              // pointer for the interned\n"
"                              // string\n"
"}</pre>\n");

    printf ("<pre>\n"
"// caller provides a temporary buffer of at least 9 bytes \n"
"const char *squoze64_decode (void *squozed, uint8_t *buf)\n"
"{\n"
"  uint64_t bits = (uint64_t)squozed;\n"
"  if ((bits & 1) == 0)\n"
"     return (char*)squozed;\n"
"\n"
"  buf[8] = 0;\n"
"  ((uint64_t*)buf)[0] = bits;\n"
"  if (buf[0] == 23)\n"
"    return buf + 1;\n"
"  buf[0] /= 2;\n"
"  return buf;\n"
"}</pre>");

    printf ("<h2>squoze32, squoze52 and squoze62 implementation</h2>\n");
    printf ("<p>These encodings use <a href='utf5+/'>UTF5+</a> to encode strings of up to 6, 10 or 12 characters.</p>");

    printf ("<h2 id='benchmarks'>benchmarks</h2>\n");
    printf ("<p>The implementation benchmarked is an open adressing hashtable storing heap allocated chunks containing both the hash, length and raw byte data for the UTF-8 string. The benchmarking code can be downloaded here: <a href='squoze.tar.xz'>squoze.tar.xz</a> it is a small C project that should build on a 64bit linux system, with /usr/share/dict/words available.</p>");

    printf ("<p>The benchmark is goes through all the words in /usr/share/dict/words and respectively creating them for the first time, a second time, and finally having an array of IDs get a copy of the string. The benchmark selects the quickest runtimes out of 10 (or more) runs to average out effects of caches and other tasks on the system. For the shorter strings, the whole datastructure fitting in L2 cache and there being no concurrent cache contention has impact on the results.</p>");

    printf ("<p>The <em>create</em> column is the microseconds taken on average to intern a word, <em>lookup</em> is the time taken the second and subsequent times a string is referenced. For comparisons the handle/pointer of the interned string would normally be used and be the same for all cases, <em>decode</em> is the time taken for getting a copy of the interned string, for interned strings all we need to do is dereference a pointer, in most uses decoding of strings is not where the majority of time is spent.");
    printf (" <em>concat</em> is the time taken to concatenate \"s\" at tje end of each word, getting some matches due to pluralisation.\n");

    printf (" The <em>embed%%</em> column shows how many of the words got embedded instead of interned. This can also be read as the percentage of possible cache-misses that are avoided for the workload.");
    printf ("The <em>RAM use</em> column shows the amount of bytes used by the allocations for interned strings as well as the size taken by the hash table used, without the size taken by tempty slots in the hash-table to be comparable with what a more compact optimizing structure used when targeting memory constrained systems.</p>");


    printf ("<p>The first line in each set of benchmarks is a baseline string-interning implementation using the same infrastructure as the others, but without embedding.</p>\n");

    return 0;
#endif
#ifdef HEADER
    printf ("<h3>%i words, max-word-len: %i</h3>\n", words, max_word_len);
	printf ("<table><tr><td></td><td>create</td><td>lookup</td><!--<td>create+decode</td>--><td>decode</td><td>concat</td><td>embed%%</td><td>RAM use</td></tr>\n");
	return 0;
#endif
#ifdef FOOTER
	printf ("</table>\n");
	return 0;
#endif
#ifdef FOOT



    printf ("<h2>Funding</h2>\n");
    printf ("<p>This work has been done with funding from patrons on <a href='https://patreon.com/pippin'>Patreon</a> and <a href='https://liberapay.com/pippin'>liberapay</a>. To join in to fund similar and unrelated independent research and development.</p>\n");

    printf ("<h2>Licence</h2>\n");
    printf ("<p>Copyright (c) 2021-2022 Øyvind Kolås &lt;pippin@gimp.org&gt;</p>");

printf ("<p>Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.</p>\n");

printf ("<p>THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.</p>");


	printf ("</body></html>\n");
	return 0;
#endif

#if 1
	char *name = "squoze32";

	if (SQUOZE_ID_MURMUR)
	{
	   name = "murmurhash";
	}
	else
	{
	if (SQUOZE_ID_UTF5)
	{
           name = malloc(256);
	   sprintf (name, "squoze%i-utf5", SQUOZE_ID_BITS);
	}
	else
	{
          if (SQUOZE_ID_BITS==32)
		  name = "squoze32-utf8";
	  else
		  name = "squoze64-utf8";
	}
	}

	printf ("<tr><th>%s</th><td>", name);
#endif


  float embed_percentage = 0.0f;
  size_t ht, ht_slack, ht_entries;
  { // warumup round
  for (StringRef *str = dict; str; str=str->next)
  {
    const char *decoded;
    Sqz *squozed;
    char input[4096];
    {
      squozed = sqz (str->str);
      char temp[16];
      decoded = sqz_decode (squozed, temp);
      if (decoded && strcmp (str->str, decoded))
      {
        uint64_t hash = sqz_id (squozed);
        printf ("!%s = %lu = %s\n", input, hash, decoded);
        wrong ++;
      }
    }
  }
    embed_percentage = (100.0f * (words - global_pool.count)) / (words);
  sqz_pool_mem_stats (NULL, &ht, &ht_slack, &ht_entries);
  }

  printf ("%.3f</td><td>", do_test (words, iterations, TEST_CREATE)/(words));
  printf ("%.3f</td><td>", do_test (words, iterations, TEST_LOOKUP)/(words));
  printf ("%.3f</td><td>", do_test (words, iterations, TEST_DECODE)/(words));
  printf ("%.3f</td><td>", do_test (words, iterations, TEST_CONCAT)/(words));
  printf ("%.0f%%</td>", embed_percentage);
  printf ("<td>%li</td>", ht - ht_slack + ht_entries);
  printf ("</tr>\n");

  if (wrong)
  {
    printf ("%i WRONG\n", wrong);
    sqz_atexit ();
    return 1;
  }
  sqz_atexit ();
  return 0;
}
