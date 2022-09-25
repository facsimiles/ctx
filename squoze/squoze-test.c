#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define ITERATIONS 33

#define SQUOZE_IMPLEMENTATION
#define SQUOZE_IMPLEMENTATION_32 1
#define SQUOZE_IMPLEMENTATION_62 1
#define SQUOZE_IMPLEMENTATION_52 1

#include "squoze.h"
#include <sys/time.h>
#define usecs(time)    ((uint64_t)(time.tv_sec - start_time.tv_sec) * 1000000 + time.     tv_usec)


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

#define INNER_ITERATIONS 1

static float do_test_round (int words, int create, int lookup, int decode)
{
  Squoze *refs[words];
  long start = ticks();
  squoze_atexit ();

  {
   int i = 0;
  for (StringRef *str = dict; str; str=str->next,i++)
  {
    char input[4096];
    for (int j = 0; j < INNER_ITERATIONS; j++)
    {
      if (j)
        sprintf (input, "%s%i", str->str, j);
      else
        sprintf (input, "%s", str->str);
      refs [i] = squoze (input);
    }
  }
  }

  if (!create) start = ticks();

  if (lookup)
  {
  for (StringRef *str = dict; str; str=str->next)
  {
    char input[4096];
    for (int j = 0; j < INNER_ITERATIONS; j++)
    {
      if (j)
        sprintf (input, "%s%i", str->str, j);
      else
        sprintf (input, "%s", str->str);
      squoze (input);
    }
  }
  }

  long end = ticks();

  if(decode){
  int i = 0;
  char temp[1024];
  for (StringRef *str = dict; str; str=str->next, i++)
  {
    const char *decoded;
    for (int j = 0; j < INNER_ITERATIONS; j++)
    {
      decoded = squoze_peek (refs[i]);
      if (decoded)
        strcpy (temp, decoded);
    }
  }

     end = ticks ();

  }
  return (end-start);
}

static float do_test (int words, int iterations, int create, int lookup, int decode)
{
  float best_ms = 10000000.0f;
  for (int i = 0; i < iterations; i ++)
  {
    float ms = do_test_round (words, create, lookup, decode);
    if (ms < best_ms) best_ms = ms;
  }
  return best_ms;
}


int main (int argc, char **argv)
{
  int wrong = 0;

  int iterations = ITERATIONS;
  FILE* f = fopen("/usr/share/dict/words", "r");
  //FILE* f = fopen("words.txt", "r");
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
    printf ("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/><title>squoze - reversible unicode string hashes</title><style>th {font-weight:normal;text-align:left;} td { text-align:right;border-right: 1px solid gray;border-bottom:1px solid gray;}  p{text-align: justify;} h2,h3,table,td,tr,th{font-size:1em;} body{font-family:monospace; max-width:50em;margin-left:auto;margin-right:auto;background:#fff;padding:1em;} html{background:#234;font-size:1.2em;} dt { color: #832} body { hyphens: auto; hyphenate-limit-chars: 6 3 2; }h2 { border-top: 1px solid black; padding-top: 0.5em; } table {margin-left:auto;margin-right:auto;} h1 { font-size:1.33em;}</style></head>");


    printf ("<h1>Squoze - reversible unicode string hashes.</h2><div style='font-style:italic; text-align:right;'>embedding text in integers</div>");
    printf ("<p>Squoze is a type of unicode string hashes designed for use in content addressed storage. The hashes trade the least significant bit of digest data for being able to embed digest_size-1 bits of payload data in the hash.</p>");


    printf("<p>An important use of content addressed storage is interned strings. This embedding of words/tokens that fit in registers directly can speed up many tasks involving text processing like parsing and even runtime dispatch in many programming languages.</p>");

    printf ("<p>squoze64-utf8 achieves <b>7x speedup</b> over murmurhash one-at-a-time used for initial string interning and <b>2.5x speedup</b> for subsequent lookups of the same string when the strings are shorther than 8bytes of utf8, see <a href='#benchmarks'>the benchmarks</a> for details.</p>");

    printf ("<dl>");

    printf ("<dt>squoze-bignum</dt><dd><a href='#utf5'>UTF5+</a> encoding, supporting arbitrary length unicode strings stored in bignums.</dd>");

    printf ("<dt>squoze32</dt><dd><a href='#utf5'>UTF5+</a> embed encoding, supporting up to 6 lower-case ascii of embedded data</dd>");
    printf ("<dt>squoze52</dt><dd><a href='#utf5'>UTF5+</a> embed encoding, supporting up to 10 lower-case ascii of embedded data, 52bit integers can be stored without loss in a double.</dd>");
    printf ("<dt>squoze62</dt><dd><a href='#utf5'>UTF5+</a> embed encoding, supporting up to 12 unicode code points.</dd>");
    printf ("<dt>squoze64</dt><dd>UTF-8 embed encoding, supporting up to 8 ASCII chars of embedded data.</dd>");
    printf ("<dt>squoze256</dt><dd><a href='#utf5'>UTF5+</a> embed encoding, supporting up to 50 unicode code points</dd>");
    printf ("</dl>\n");

    printf ("<p>squoze is still under development, preliminary variants for <a href='squoze.py'>python3</a> and <a href='squoze.h'>C</a> are available, the C code is the hashes tested in the <a href='#benchmarks'>benchmarks</a>.</p>\n");

    printf ("<h2>squoze64 implementation</h2>");
    printf ("<p>the squoze-64 encoding is a small bitmanipulation of UTF-8 in-memory encoding, for strings that will fit only the first byte is manipulated and only if it ascii and not <em>@</em> the value is double and 1 is added. When the first byte does not match our constraints we store 129 - which means the following 7bytes directly encode the value</p>");

    printf (
"<pre>uint64_t squoze64(const char *utf8, size_t len)\n"
"{\n"
"  size_t   squoze_dim = 64;\n"
"  uint64_t hash       = 0;\n"
"  uint8_t *encoded    = (uint8_t*)&hash;\n"
"  uint8_t  first_byte = ((uint8_t*)utf8)[0];\n"
"\n"
"  if (first_byte<128\n"
"      && first_byte != '@'\n"
"      && (len &lt;= (squoze_dim/8)))\n"
"  {\n"
"    for (int i = 0; utf8[i]; i++)\n"
"      encoded[i] = utf8[i];\n"
"    encoded[0] = encoded[0]*2+1;\n"
"    return *((uint64_t*)&encoded[0]);\n"
"  }\n"
"  else if (len &lt;= (squoze_dim/8)-1)\n"
"  {\n"
"    for (int i = 0; utf8[i]; i++)\n"
"      encoded[i+1] = utf8[i];\n"
"    encoded[0] = 129;\n"
"    return *((uint64_t*)&encoded[0]);\n"
"  }\n"
"\n"
"  // murmurhash one-at-a-time\n"
"  hash = 3323198485ul;\n"
"  for (unsigned int i = 0; i &lt; len; i++)\n"
"  { \n"
"    uint8_t key = utf8[i];\n"
"    hash ^= key;\n"
"    hash *= 0x5bd1e995;\n"
"    hash ^= hash &gt;&gt; 15;\n"
"  }\n"
"  return hash & ~1; // make even\n"
"}</pre>\n");


    printf ("<pre>const char *squoze64_decode (uint64_t hash)\n"
"{\n"
"  static uint8_t buf[10];\n"
"  buf[8] = 0;\n"
"  ((uint64_t*)buf)[0]= hash; // or memcpy (buf, hash, 8);\n"
"  if ((buf[0] & 1) == 0)\n"
"     return NULL;\n"
"  if (buf[0] == 129)\n"
"     return buf+1;\n"
"  buf[0] /= 2;\n"
"  return buf;\n"
"}</pre>");

    printf ("<h2 id='utf5'>UTF5+ and squoze-bignum implementation</h2>\n");
    printf ("<p>The first stage of this encoding is encoding to UTF5+ which extends <a href='https://datatracker.ietf.org/doc/html/draft-jseng-utf5-01.txt'>UTF-5</a>. The symbol 'G' with value 16 does not occur in normal UTF-5 and is used to change encoding mode to a sliding window, valid UTF5 strings are correctly decoded by a UTF5+ decoder.</p>");
    printf ("<p>In squeeze mode the initial offset is set based on the last encoded unicode codepoint in UTF5 mode. Start offsets for a code point follow the pattern 19 + 26 * N, which makes a-z fit in one window. In sliding window mode the following quintets have special meaning:</p>");
    printf ("<table><tr><td>0</td><td>0</td><td>emit SPACE</td></tr>\n");
    printf ("<tr><td>1</td><td>1</td><td>codepoint at offset + 0</td></tr>\n");
    printf ("<tr><td>2</td><td>2</td><td>codepoint at offset + 1</td></tr>\n");
    printf ("<tr><td></td><td>..</td><td></td></tr>\n");
    printf ("<tr><td>10</td><td>A</td><td>codepoint at offset + 9</td></tr>\n");
    printf ("<tr><td>11</td><td>B</td><td>codepoint at offset + 10</td></tr>\n");
    printf ("<tr><td>12</td><td>C</td><td>codepoint at offset + 11</td></tr>\n");
    printf ("<tr><td></td><td>..</td><td></td></tr>\n");
    printf ("<tr><td>26</td><td>Q</td><td>codepoint at offset + 25</td></tr>\n");
    printf ("<tr><td>27</td><td>R</td><td>offset += 26 *1</td></tr>\n");
    printf ("<tr><td>28</td><td>S</td><td>offset += 26 *1</td></tr>\n");
    printf ("<tr><td>29</td><td>T</td><td>offset += 26 *1</td></tr>\n");
    printf ("<tr><td>30</td><td>U</td><td>offset += 26 *1</td></tr>\n");
    printf ("<tr><td>31</td><td>V</td><td>switch to UTF-5 mode</td></tr></table>\n");

    printf ("<p>For compatibility with UTF-5 we start out in UTF-5 mode rather than window mode.</p>");
    printf ("<p>The encoder decides if the current mode is kept or not for every codepoint. The cost in output quintets is computed for UTF-5 and windowed is computed for both this and the next codepoint. We switch from UTF-5 to windowed when the cost of switching considering this and the next code points is equal or smaller, in the other direction we only switch if there is a gain to be had.</p>");

    printf ("<p>For example the string <em>Hello World</em> is encoded as follows:</p>");
    printf ("<pre>H   e  l l o   W  o  r l d                             11 bytes\n"
"GT2 U5 C C F 0 TH UF I C 4     16 quintets = 80 bits = 10 bytes\n"
"\n"
"h  e l l o   w o r l d     11 bytes\n"
"G8 5 C C F 0 N F I C 4     12 quintets = 60 bits = 7.5bytes padded to 8 bytes</pre>");

    printf ("<p>When transforming a quintet sequence into an integer the initial mode is encoded as a bit of 1 if we are starting out in UTF-5 mode, allowing us to skip the G. To create an integer we start with 0, add the integer value of the first quintet. If there are more quintets, multiply by 32 and continue adding quintets. The resulting value is multipled by 4, the second lowest bit set according to windowed or utf-5 initial mode and the lowest bit set.</p>");

    printf ("<h2>squoze32, squoze52 and squoze62 implementation</h2>\n");
    printf ("<p>These hashes are just like squoze-bignum if they as UTF5+ encode as fewer than 6, 10 or 12 quintets. If this is not the case a murmurhash is computed and the lowest bit stripped.</p>");

    printf ("<h2 id='benchmarks'>benchmarks</h2>\n");
    printf ("<p>The implementation benchmarked is an open adressing hashtable storing heap allocated chunks containing both the hash, length and raw byte data for the UTF8 string.</p>");

    printf ("<p>The <em>alwaysintern</em> variants of squoze are using the squoze hashes without their embedding capability.</p>");

    printf ("<p>The <em>create</em> column is the microseconds taken on average to intern a word, <em>lookup</em> is the time taken the second and subsequent times a string is referenced. For comparisons the handle/pointer of the interned string would normally be used and be the same for all cases, <em>decode</em> is the time taken for getting a copy of the interned string, for interned strings all we need to do is dereference a pointer, in most uses decoding of strings is not where the majority of time is spent.</p>");
    printf ("<p>The embed%% column shows how many of the words got embedded instead of interned.</p>");
    printf ("<p>The <em>RAM use</em> column shows the amount of bytes used by the allocations for interned strings as well as the size taken by the hash table used, without the size taken by tempty slots in the hash-table to be comparable with what a more compact optimizing structure used when targeting memory constrained systems.</p>");

    printf ("<p>The hashes with the direct UTF8 embedding are the most reliable optimization when only runtime/energy use is considered. The UTF5 embeddings save more RAM and allow more guarantee collision free strings but are more expensive to compute.</p>");

    printf ("<p>On embedded platforms not having the strings consume heap space can be a significant saving, this should however be weighed against the overhead of needing 32bit values to store/pass around sometimes being able to use 16bit references to strings is a more significant overall saving.</p>");

    /*
    printf ("<p>The large amount of time taken for <em>squoze52 alwaysintern</em> can be attributed to the dataset no longer fitting in caches when using 64bit quantities in the struct backing each interned string.</p>");
    */

    return 0;
#endif
#ifdef HEADER
    printf ("<h3>%i words, max-word-len: %i</h3>\n", words, max_word_len);
	printf ("<table><tr><td></td><td>create</td><td>lookup</td><!--<td>create+decode</td>--><td>decode</td><td>embed%%</td><td>RAM use</td></tr>\n");
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

	if (SQUOZE_ALWAYS_INTERN)
	{
	if (SQUOZE_EMBEDDED_UTF5)
	{
	  name = malloc(256);
	  sprintf (name, "squoze%i-utf5 alwaysintern", SQUOZE_ID_BITS);
	}
	else
	{
          if (SQUOZE_ID_BITS==32)
	    name = "murmurhash OAAT 32bit";
	  else
	    name = "squoze64-utf8 alwaysintern";
	}
	}
	else
	{
	if (SQUOZE_EMBEDDED_UTF5)
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


#if 0 
  {
    unsigned char str[4]=" ";
    for (int i = 0; i < 256; i++)
    {
	str[0]=i;
	fprintf (stdout, "%i :", i);
	fprintf (stdout, "%i\n", squoze32 (str, 1));
    }
  }
#endif

#if 0
  {
     char *strs[] = {"foo", "abcdefghijklmnopqrst",
	                    "abcdefghijklmnopqrst", "foo", "bar", "baz", "boo", "", NULL};
     for (int i =0; strs[i+2]; i++)
     {
       const char *str= strs[i];
       Squoze *intern = squoze (str);
       printf ("%p %s -> %p ", str, str, intern);
       printf (" -> %s\n", squoze_peek (intern));
       squoze_unref(intern);
       //printf ("%p -> %s\n", intern, squoze_peek (intern));
       intern = NULL;
     }
  }
#endif


  float embed_percentage = 0.0f;
  { // warumup round
  for (StringRef *str = dict; str; str=str->next)
  {
    const char *decoded;
    Squoze *squozed;
    char input[4096];
    for (int j = 0; j < INNER_ITERATIONS; j++)
    {
      if (j)
        sprintf (input, "%s%i", str->str, j);
      else
        strcpy (input, str->str);

      squozed = squoze (input);
      decoded = squoze_peek (squozed);
      if (decoded && strcmp (input, decoded))
      {
        uint64_t hash = squoze_id (squozed);
        printf ("!%s = %lu = %s\n", input, hash, decoded);
        wrong ++;
      }
    }
  }
    embed_percentage = (100.0f * global_pool.count_embedded) / words;
  }

  printf ("%.3f</td><td>", do_test (words, iterations, 1, 0, 0)/words);
  printf ("%.3f</td><td>", do_test (words, iterations, 0, 1, 0)/words);
  printf ("%.3f</td><td>", do_test (words, iterations, 0, 0, 1)/words);
  printf ("%.0f%%</td>", embed_percentage);
  size_t ht, ht_slack, ht_entries;
  squoze_pool_mem_stats (NULL, &ht, &ht_slack, &ht_entries);
  //printf ("<td>%li</td><td>%li</td><td>%li</td>", ht, ht_slack, ht_entries);
  printf ("<td>%li</td>", ht - ht_slack + ht_entries);
  printf ("</tr>\n");

  if (wrong)
  {
    printf ("%i WRONG\n", wrong);
    squoze_atexit ();
    return 1;
  }
  squoze_atexit ();


  return 0;
}
