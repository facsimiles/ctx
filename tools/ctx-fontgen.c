#include <stdlib.h>
#include <libgen.h>

#define CTX_MAX_DRAWLIST_SIZE 4096000
#define CTX_BACKEND_TEXT 0 // we keep then non-backend code paths
                           // for code handling aroud, this should
                           // be run-time to permit doing text_to_path
#define CTX_RASTERIZER         0

#define CTX_BITPACK_PACKER     1  // pack vectors
#define CTX_BITPACK            1
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <sys/time.h>
#define CTX_EXTRAS 1
#define CTX_FONTS_FROM_FILE 1
#define CTX_AVX2 0
#define CTX_IMPLEMENTATION
#define CTX_PARSER 1
#include "ctx-nofont.h"

static int usage(){
  fprintf (stderr, "tool to generate native ctx embedded font format\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "usage: ctx-fontgen <file.ttf> [name [set1-set2-set3]]\n");
  fprintf (stderr, "\nrecognized sets: latin1, ascii, extra, all, emoji\n");
  fprintf (stderr, "\na final argument of \"binary\" might be appended, causing\nthe generated file to be binary ctx.\n");
  return -1;
}

CtxDrawlist output_font={NULL,};
uint32_t glyphs[65536];
unsigned int n_glyphs = 0;


void
add_glyph_real (Ctx *ctx, uint32_t glyph)
{
  for (unsigned int i = 0; i < n_glyphs; i++)
  {
    if (glyphs[i] == glyph)
      return;
  }
  ctx_start_frame (ctx);
  ctx_font_size (ctx, CTX_BAKE_FONT_SIZE);
  ctx_move_to (ctx, 0, 0);
  if (ctx_glyph (ctx, glyph, 0))
    return;
  glyphs[n_glyphs++] = glyph;
  ctx->drawlist.flags = CTX_TRANSFORMATION_BITPACK;
  ctx_drawlist_compact (&ctx->drawlist);

  char buf[44]={0,0,0,0,0};
  ctx_unichar_to_utf8 (glyph, (uint8_t*)buf);
  uint32_t args[2] = {glyph, ctx_glyph_width (ctx, glyph) * 256};
  ctx_drawlist_add_u32 (&output_font, CTX_DEFINE_GLYPH, args);

  for (unsigned int i = 3; i < ctx->drawlist.count - 1; i++)
  {
    CtxEntry *entry = &ctx->drawlist.entries[i];
    args[0] = entry->data.u32[0];
    args[1] = entry->data.u32[1];
    ctx_drawlist_add_u32 (&output_font, entry->code, &args[0]);
  }
}

uint32_t incoming_glyphs[65536];
unsigned int n_incoming_glyphs = 0;

void
add_glyph (Ctx *ctx, uint32_t glyph)
{
  for (unsigned int i = 0; i < n_incoming_glyphs; i++)
  {
    if (incoming_glyphs[i] == glyph)
      return;
  }
  incoming_glyphs[n_incoming_glyphs++] = glyph;
}

int compare_glyphs (const void*a, const void *b)
{
  uint32_t au = ((uint32_t*)a)[0];
  uint32_t bu = ((uint32_t*)b)[0];
  return au - bu;
}

void
real_add_glyphs (Ctx *ctx)
{
  qsort (incoming_glyphs, n_incoming_glyphs, sizeof (uint32_t),
         compare_glyphs);
  for (unsigned int i = 0; i < n_incoming_glyphs; i++)
    add_glyph_real (ctx, incoming_glyphs[i]);
}

static int find_glyph (CtxDrawlist *drawlist, uint32_t unichar)
{
  for (unsigned int i = 0; i < drawlist->count; i++)
  {
    if (drawlist->entries[i].code == CTX_DEFINE_GLYPH &&
        drawlist->entries[i].data.u32[0] == unichar)
    {
       return i;
       // XXX this could be prone to insertion of valid header
       // data in included bitmaps.. is that an issue?
    }
  }
  fprintf (stderr, "Eeeek %i\n", unichar);
  return -1;
}

int main (int argc, char **argv)
{
  int binary = 0;
  const char *path;
  const char *name = "regular";
  const char *subsets = "latin1";

  Ctx *ctx;
  path = argv[1];
  if (!path)
  {
    return usage();
  }
  if (argv[2])
  {
    name = argv[2];
    if (argv[3])
     subsets = argv[3];
    if (argv[4] &&  !strcmp(argv[4], "binary"))
     binary=1;
  }
  int font_no = ctx_load_font_ttf_file ("import", argv[1]);

  ctx = ctx_new (1000, 1000, "drawlist");
  _ctx_set_transformation (ctx, CTX_TRANSFORMATION_RELATIVE);
  ctx_font (ctx, "import");

  const char *font_name = ctx_get_font_name (NULL, font_no);

  if (strstr (subsets, "all"))
  for (int glyph = 0; glyph < 65536*8; glyph++) add_glyph (ctx, glyph);

  if (strstr (subsets, "latin1"))
  for (int glyph = 0; glyph < 256; glyph++) add_glyph (ctx, glyph);
  if (strstr (subsets, "ascii"))
  for (int glyph = 0; glyph < 127; glyph++) add_glyph (ctx, glyph);


  if (strstr (subsets, "terminal"))
  {
char* string = "☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼! #$%&'()*+,-.🔒"
"⌂ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒáíóúñÑªº¿⌐¬½¼¡«»░▒▓│┤╡╢╖╕╣║╗╝╜╛┐"
"└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■"
"!#$%&'()*+,-./◆▒␉␌␍␊°±␤␋┘┐┌└┼⎺⎻─⎼⎽├┤┴┬│≤≥π≠£·";

  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1))
    add_glyph (ctx, ctx_utf8_to_unichar (utf8));
  }

  if (strstr (subsets, "cp437"))
  {
char* string = " ☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼!\"#$%&'()*+,-./0123456789:;<=>?"
"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^,`abcdefghijklmnopqrstuvwxyz{|}~⌂"
"ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒáíóúñÑªº¿⌐¬½¼¡«»"
"░▒▓│┤╡╢╖╕╣║╗╝╜╛┐└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀"
"αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■";

  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1))
    add_glyph (ctx, ctx_utf8_to_unichar (utf8));
  }

  if (strstr (subsets, "vt100"))
  {
char* string = 
" !\"#$%&'()*+,-./0123456789:;<=>? @ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_◆▒␉␌␍␊°±␤␋┘┐┌└┼⎺⎻─⎼⎽├┤┴┬│≤≥π≠£· ";

  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1))
    add_glyph (ctx, ctx_utf8_to_unichar (utf8));
  }

  if (strstr (subsets, "extras"))
  {
  char *string =
	
"éñÑßæøåö£ÆÖØÅ€§π°üÜﬀﬁﬂﬃﬄﬅ…Ł”““”«»©®™⭾⏎⌫·←↑↓→☀☁☂☢☭☮☯☽✉⚙⚠␣²◆♥♦♣♠÷≈±╴−╶▶▷▽▼•☑☒☐📁📄";
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1))
    add_glyph (ctx, ctx_utf8_to_unichar (utf8));
  }

  if (strstr (subsets, "emoji"))
  {
  char *string = "๛ ☬☣☠☀☁☂☃☢☭☮☯☼☽✉✓❤№℃∞◌◎☎☐☑♉♩♪♫♬☆◦☄☉☏☤☥☹☺☻♲♨♼♽♾⚀⚁🥕⚂⚃⚄⚅⚙⚠␣⚡⌨⌚⏏⋯•‥․‣•↺↻⌘☕✈✉✓✔✕✖✗✘☑☒☐☓✶❄❍❢❣❤❥❤❥❦❧➲㎏㎆㎅㎇㎤㎥㎦㎝㎧㎨㎞㎐㎏㎑㎒㎓㎠㎡㎢★⁈ ⁉ ⁂✵✺✽✾✿❀❁❂❉❆✡✠🤐🤑🤒🤓🤔🤕🤖🤗🤘🤙🤚🤛🤜🤝🤞🤟🤠🤡🤢🤣🤤🤥🤦🤧🤨🤩🤪🤫🤬🤭🤮🤯🤷🤸🤹🤺🥂🥇🥈🥉🥤🥨🥬🥰🥱🥳🥴🥵🥶🥺🦁🦄🦉🦊🦋🦎🦓🦔🦕🦖🦘🦙🦚🦜🦝🦟🦴🦽🦾🦿🧁🧉🧐🧙🧚🧛🧜🧟🧠🧡🧥🧤🧦🧪🧬🧭🧮🧯🧰🧱🧲🧵🧷🧸🧺🧻🛹🛸🛵🛴🛰🛬🛫🛩🛠🛣🛤🛥🛒🛏🛌🛂🛁🚶🚴🚲🚬🚩🚗🚜🚴🚘🚖🚔🚚🚑🚍🚆🚀🙈🙉🙊🙄🙃🙂🙁🙀😿😾😽😼😻😺😹😸😷😶😵😴😳😲😱😰😯😮😭😬😫😪😩😨😧😦😥😤😣😢😡😠😟😞😝😜😛😚😙😘😗😖😕😔😓😒😑😐😏😎😍😌😋😊😉😈😇😆😅😄😃😂😁😀🗝🗜🗞🗓🗒🗑🗄🖼🖤🖥🖨🖖🖕🖐🖍🖌🖋🖊🖇🕺🕹🕸🕷🕶🕵🕴🕰🕯🔭🔬🔫🔪🔩🔨🔧🔦🔥🔤🔢🔣🔡🔠🔗🔕🔔🔓🔒🔑🔐🔏🔌🔋🔊🔉🔈🔇🔅🔆📽📻📺📹📸📷📳📱📰📯📭📬📫📪📨📦📡📞📚📖📐📏📎📍📌📉📈📄📃📂📁💿💾💻💸💶💳💰💯💫💩💧💦💤💣💢💡💞💖💕💔💓💊💃💀👾👽👻👺👹👸👷👶👵👴👯👭👬👫👣👓👋👍👎🐾🐼🐻🐺🐹🐸🐷🐶🐵🐳🐲🐱🐰🐯🐮🐭🐬🐧🐦🐥🐤🐣🐢🐡🐠🐟🐞🐝🐜🐛🐙🐘🐔🐓🐒🐑🐐🐏🐎🐍🐌🐋🐊🐉🐈🐇🐆🐅🐄🐃🐂🐁🐀🏺🏹🏸🏷🏴🏳🏵🏰🏭🏬🏫🏪🏧🏢🏠🏡🏐🏍🏆🏅🏁🎼🎶🎵🎲🎰🎮🎬🎨🎧🎥🎞🎃🎄🍸🍷🍄🌿🍀🍁🍒🌵🌳🌲🌩🌪🌨🌧🌦🌥🌤🌡🌠🌟🌞🌝🌜🌛🌚🌙🌘🌗🌖🌕🌔🌓🌒🌑🌐🌏🌎🌍🌌🌋🌊🌅🌄🌂🌀";
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1))
    add_glyph (ctx, ctx_utf8_to_unichar (utf8));
  }

  ctx_drawlist_add_data (&output_font, name, strlen(name));

  real_add_glyphs (ctx);

  for (unsigned int i = 0; i < n_glyphs; i++)
    for (unsigned int j = 0; j < n_glyphs; j++)
    {
      float kerning = ctx_glyph_kern (ctx, glyphs[i], glyphs[j]);
      if (kerning > 0.2)
      {
        CtxCommand command;
        unsigned int pos = find_glyph (&output_font, glyphs[i]);
        pos ++;
        while (pos < output_font.count &&
               output_font.entries[pos].code != CTX_DEFINE_GLYPH)
          pos++;

        command.code = CTX_KERNING_PAIR;
        command.kern.glyph_before = glyphs[i];
        command.kern.glyph_after = glyphs[j];
        command.kern.amount = kerning * 256;
        ctx_drawlist_insert_entry (&output_font, pos, (CtxEntry*)&command);
      }
    }

  ctx_destroy (ctx);

  if (!binary)
  {
  printf ("#ifndef CTX_FONT_%s\n", name);
  printf ("/* this is a ctx encoded font based on %s */\n", basename (argv[1]));

  int apache = 0;

  if (strstr (argv[1], "Roboto-"))
  {
    apache = 1;
    printf ("/* Copyright 2014 Christian Robertson\n");
  }

  if (strstr (argv[1], "Carlito-"))
  {
    apache = 1;
    printf ("/* Copyright 2013 Łukasz Dziedzic \n");
  }

  if (strstr (argv[1], "Arimo-") ||
      strstr (argv[1], "Tinos-") ||
      strstr (argv[1], "Cousine-"))
  {
    apache = 1;
    printf ("/* Copyright 2013 Steve Matteson \n");
  }

  if (strstr (argv[1], "Caladea-"))
  {
    apache = 1;
    printf ("/* Copyright 2014 Carolina Giovagnoli and Andres Torresi \n");
  }

  if (apache) printf (
" \n"
" Licensed under the Apache License, Version 2.0 (the \"License\");\n"
" you may not use this file except in compliance with the License.\n"
" You may obtain a copy of the License at\n"
" \n"
" http://www.apache.org/licenses/LICENSE-2.0\n"
" \n"
" Unless required by applicable law or agreed to in writing, software\n"
" distributed under the License is distributed on an \"AS IS\" BASIS,\n"
" WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
" See the License for the specific language governing permissions and\n"
" limitations under the License.\n"
" */\n");
  printf ("/* CTX_SUBDIV:%i  CTX_BAKE_FONT_SIZE:%i */\n", CTX_SUBDIV, CTX_BAKE_FONT_SIZE);

  printf ("/* glyphs covered: \n\n");
  int col = 0;
  for (unsigned int i = 0; i < output_font.count; i++)
  {
    CtxEntry *entry = &output_font.entries[i];
    if (entry->code == '@')
    {
       char buf[44]={0,0,0,0,0};
       ctx_unichar_to_utf8 (entry->data.u32[0], (uint8_t*)buf);
       switch (buf[0])
       {
         case '\\':
           printf ("\\");
         break;
         default:
           printf ("%s", buf);
       }
       col++;
       if (col > 73)
       {
         col = 0;
         printf ("\n  ");
       }
    }
  }

  printf ("  */\n");

  printf ("static const struct __attribute__ ((packed)) {uint8_t code; uint32_t a; uint32_t b;}\nctx_font_%s[]={\n", name);

  for (unsigned int i = 0; i < output_font.count; i++)
  {
    CtxEntry *entry = &output_font.entries[i];
    if (entry->code > 32 && entry->code < 127)
    {
      printf ("{'%c', 0x%08x, 0x%08x},", entry->code,
                                         entry->data.u32[0],
                                         entry->data.u32[1]);
    }
    else
    {
      printf ("{%i, 0x%08x, 0x%08x},", entry->code,
                                       entry->data.u32[0],
                                       entry->data.u32[1]);
    }
    if (entry->code == '@')
    {
       char buf[44]={0,0,0,0,0};
       ctx_unichar_to_utf8 (entry->data.u32[0], (uint8_t*)buf);
       switch (buf[0])
       {
         case '\\':
           printf ("/*       \\         x-advance: %f */", entry->data.u32[1]/256.0);
         break;
         default:
           printf ("/*        %s        x-advance: %f */", buf, entry->data.u32[1]/256.0);
       }
    }
    else
    {
    }
    printf ("\n");
  }
  printf ("};\n");
  printf ("#define ctx_font_%s_name \"%s\"\n", name, font_name);
  printf ("#endif\n");
  }
  else
  {
  for (unsigned int i = 0; i < output_font.count; i++)
  {
    CtxEntry *entry = &output_font.entries[i];
    for (int c = 0; c <  (int)sizeof (CtxEntry); c++)
      printf ("%c",((uint8_t*)(entry))[c]);
  }
  }

  return 0;
}
