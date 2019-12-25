#include <stdlib.h>
#include <libgen.h>

#define CTX_MAX_JOURNAL_SIZE 4096000

#define CTX_GLYPH_CACHE 0
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <sys/time.h>
#define CTX_EXTRAS 1
#define CTX_IMPLEMENTATION
#include "ctx.h"

CtxRenderstream output_font={NULL,};
uint32_t glyphs[65536];
int n_glyphs = 0;

void
add_glyph (Ctx *ctx, uint32_t glyph)
{
  for (int i = 0; i < n_glyphs; i++)
  {
    if (glyphs[i] == glyph)
      return;
  }
  ctx_clear (ctx);
  ctx_set_font_size (ctx, CTX_BAKE_FONT_SIZE);
  ctx_move_to (ctx, 0, 0);
  if (ctx_glyph (ctx, glyph, 1)) /* we request stroking, since it is better to exclude strokes than fills from bitpacking  */
    return;
  glyphs[n_glyphs++] = glyph;
  ctx->renderstream.flags  = CTX_TRANSFORMATION_BITPACK;
  ctx_renderstream_refpack (&ctx->renderstream);

  char buf[44]={0,0,0,0,0};
  ctx_unichar_to_utf8 (glyph, buf);
  uint32_t args[2] = {glyph, ctx_glyph_width (ctx, glyph) * 256};
  ctx_renderstream_add_u32 (&output_font, CTX_DEFINE_GLYPH, args);

  for (int i = 2; i < ctx->renderstream.count - 1; i++)
  {
    CtxEntry *entry = &ctx->renderstream.entries[i];
    args[0] = entry->data.u32[0];
    args[1] = entry->data.u32[1];
    ctx_renderstream_add_u32 (&output_font, entry->code, &args[0]);
  }
}

int main (int argc, char **argv)
{
  const char *path;
  const char *name = "regular";
  const char *subsets = "latin1";

  Ctx *ctx;
  path = argv[1];
  if (!path)
  {
    fprintf (stderr, "usage: ctx-fontgen <file.ttf> [name [set1-set2-set3]]\n");
    fprintf (stderr, "\nrecognized sets: latin1, ascii, extra, all, emoji");
    return -1;
  }
  if (argv[2])
  {
    name = argv[2];
    if (argv[3])
     subsets = argv[3];
  }
  ctx_load_font_ttf_file (ctx, "import", argv[1]);
  ctx = ctx_new ();
  ctx_set_font (ctx, "import");

  if (strstr (subsets, "all"))
  for (int glyph = 0; glyph < 65536*8; glyph++) add_glyph (ctx, glyph);

  if (strstr (subsets, "latin1"))
  for (int glyph = 0; glyph < 256; glyph++) add_glyph (ctx, glyph);
  if (strstr (subsets, "ascii"))
  for (int glyph = 0; glyph < 127; glyph++) add_glyph (ctx, glyph);


  if (strstr (subsets, "terminal"))
  {
char* string = "☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼! #$%&'()*+,-."
"⌂ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒáíóúñÑªº¿⌐¬½¼¡«»░▒▓│┤╡╢╖╕╣║╗╝╜╛┐"
"└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■"
"!#$%&'()*+,-./◆▒␉␌␍␊°±␤␋┘┐┌└┼⎺⎻─⎼⎽├┤┴┬│≤≥π≠£·";

  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1))
    add_glyph (ctx, ctx_utf8_to_unichar (utf8));
  }


  if (strstr (subsets, "extras"))
  {
  char *string = "éñŇßæøåÆØÅ€§π…”““”‘’«»©®™·←↑↓→";
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1))
    add_glyph (ctx, ctx_utf8_to_unichar (utf8));
  }

  if (strstr (subsets, "emoji"))
  {
  char *string = "๛ ☬☣☠☀☁☂☃☢☭☮☯☼☽✉✓❤№℃∞◌◎☎☐☑♉♩♪♫♬☆◦☄☉☏☤☥☹☺☻♲♨♼♽♾⚀⚁🥕⚂⚃⚄⚅⚙⚠⚡␣⌨⌚⏏⋯•‥․‣•↺↻⌘☕✈✉✓✔✕✖✗✘☑☒☐☓✶❄❍❢❣❤❥❤❥❦❧➲㎏㎆㎅㎇㎤㎥㎦㎝㎧㎨㎞㎐㎏㎑㎒㎓㎠㎡㎢★⁈ ⁉ ⁂✵✺✽✾✿❀❁❂❉❆✡✠🤐🤑🤒🤓🤔🤕🤖🤗🤘🤙🤚🤛🤜🤝🤞🤟🤠🤡🤢🤣🤤🤥🤦🤧🤨🤩🤪🤫🤬🤭🤮🤯🤷🤸🤹🤺🥂🥇🥈🥉🥤🥨🥬🥰🥱🥳🥴🥵🥶🥺🦁🦄🦉🦊🦋🦎🦓🦔🦕🦖🦘🦙🦚🦜🦝🦟🦴🦽🦾🦿🧁🧉🧐🧙🧚🧛🧜🧟🧠🧡🧥🧤🧦🧪🧬🧭🧮🧯🧰🧱🧲🧵🧷🧸🧺🧻🛹🛸🛵🛴🛰🛬🛫🛩🛠🛣🛤🛥🛒🛏🛌🛂🛁🚶🚴🚲🚬🚩🚗🚜🚴🚘🚖🚔🚚🚑🚍🚆🚀🙈🙉🙊🙄🙃🙂🙁🙀😿😾😽😼😻😺😹😸😷😶😵😴😳😲😱😰😯😮😭😬😫😪😩😨😧😦😥😤😣😢😡😠😟😞😝😜😛😚😙😘😗😖😕😔😓😒😑😐😏😎😍😌😋😊😉😈😇😆😅😄😃😂😁😀🗝🗜🗞🗓🗒🗑🗄🖼🖤🖥🖨🖖🖕🖐🖍🖌🖋🖊🖇🕺🕹🕸🕷🕶🕵🕴🕰🕯🔭🔬🔫🔪🔩🔨🔧🔦🔥🔤🔢🔣🔡🔠🔗🔕🔔🔓🔒🔑🔐🔏🔌🔋🔊🔉🔈🔇🔅🔆📽📻📺📹📸📷📳📱📰📯📭📬📫📪📨📦📡📞📚📖📐📏📎📍📌📉📈📄📃📂📁💿💾💻💸💶💳💰💯💫💩💧💦💤💣💢💡💞💖💕💔💓💊💃💀👾👽👻👺👹👸👷👶👵👴👯👭👬👫👣👓👋👍👎🐾🐼🐻🐺🐹🐸🐷🐶🐵🐳🐲🐱🐰🐯🐮🐭🐬🐧🐦🐥🐤🐣🐢🐡🐠🐟🐞🐝🐜🐛🐙🐘🐔🐓🐒🐑🐐🐏🐎🐍🐌🐋🐊🐉🐈🐇🐆🐅🐄🐃🐂🐁🐀🏺🏹🏸🏷🏴🏳🏵🏰🏭🏬🏫🏪🏧🏢🏠🏡🏐🏍🏆🏅🏁🎼🎶🎵🎲🎰🎮🎬🎨🎧🎥🎞🎃🎄🍸🍷🍄🌿🍀🍁🍒🌵🌳🌲🌩🌪🌨🌧🌦🌥🌤🌡🌠🌟🌞🌝🌜🌛🌚🌙🌘🌗🌖🌕🌔🌓🌒🌑🌐🌏🌎🌍🌌🌋🌊🌅🌄🌂🌀";
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1))
    add_glyph (ctx, ctx_utf8_to_unichar (utf8));
  }

  for (int i = 0; i < n_glyphs; i++)
    for (int j = 0; j < n_glyphs; j++)
    {
      float kerning = ctx_glyph_kern (ctx, glyphs[i], glyphs[j]);
      if (kerning > 0.2)
      {
        uint16_t args[4]={glyphs[i],glyphs[j], 0, 0};
        int32_t *iargs = (void*)(&args[0]);
        iargs[1] = kerning * 256;
        ctx_renderstream_add_u32 (&output_font, CTX_KERNING_PAIR, (void*)&args[0]);
      }
    }

  ctx_free (ctx);

  printf ("/* this is a ctx encoded font based on %s */\n", basename (argv[1]));
  printf ("/* CTX_SUBDIV:%i  CTX_BAKE_FONT_SIZE:%i */\n", CTX_SUBDIV, CTX_BAKE_FONT_SIZE);

  printf ("/* glyphs covered: \n\n");
  int col = 0;
  for (int i = 0; i < output_font.count; i++)
  {
    CtxEntry *entry = &output_font.entries[i];
    if (entry->code == '@')
    {
       char buf[44]={0,0,0,0,0};
       ctx_unichar_to_utf8 (entry->data.u32[0], buf);
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

  for (int i = 0; i < output_font.count; i++)
  {
    CtxEntry *entry = &output_font.entries[i];
    printf ("{'%c', 0x%08x, 0x%08x},", entry->code,
                                       entry->data.u32[0],
                                       entry->data.u32[1]);
    if (entry->code == '@')
    {
       char buf[44]={0,0,0,0,0};
       ctx_unichar_to_utf8 (entry->data.u32[0], buf);
       switch (buf[0])
       {
         case '\\':
           printf ("/*       \\         x-advance: %f */", buf, entry->data.u32[1]/256.0);
         break;
         default:
           printf ("/*        %s        x-advance: %f */", buf, entry->data.u32[1]/256.0);
       }
    }
    else
    {
      printf ("/*  %s ", ctx_command_name (entry->code));
    printf ("*/");
    }
    printf ("\n");
  }
  printf ("};\n");
  printf ("#define CTX_FONT_%s 1\n", name);

  return 0;
}
