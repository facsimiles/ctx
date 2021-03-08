/* This file is not licensed (c) Øyvind Kolås pippin@gimp.org,
 * based on micro-raptor GUI
  */

#if CTX_EVENTS

#define MRG_MAX_STYLE_DEPTH  CTX_MAX_STATES
#define MRG_MAX_STATE_DEPTH  CTX_MAX_STATES
#define MRG_MAX_FLOATS           16
#define MRG_MAX_SELECTOR_LENGTH  64
#define MRG_MAX_CSS_STRINGLEN    512
#define MRG_MAX_CSS_RULELEN      32   // XXX doesnt have overflow protection
#define MRG_MAX_CSS_RULES        128

/* other important maximums */
#define MRG_MAX_TEXT_LISTEN      256
#define CTX_XML_INBUF_SIZE       1024

#define PROP(a)          (ctx_get_float(mrg->ctx, CTX_##a))
#define PROPS(a)         (ctx_get_string(mrg->ctx, CTX_##a))
#define SET_PROPh(a,v)   (ctx_set_float(mrg->ctx, a, v))
#define SET_PROP(a,v)    SET_PROPh(CTX_##a, v)
#define SET_PROPS(a,v)   (ctx_set_string(mrg->ctx, CTX_##a, v))
#define SET_PROPSh(a,v)  (ctx_set_string(mrg->ctx, a, v))

#if CTX_XML
#define CTX_1           CTX_STRH('1',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_a           CTX_STRH('a',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_absolute 	CTX_STRH('a','b','s','o','l','u','t','e',0,0,0,0,0,0)
#define CTX_aelig 	CTX_STRH('a','e','l','i','g',0,0,0,0,0,0,0,0,0)
#define CTX_Aelig 	CTX_STRH('A','e','l','i','g',0,0,0,0,0,0,0,0,0)
#define CTX_AElig 	CTX_STRH('A','E','l','i','g',0,0,0,0,0,0,0,0,0)
#define CTX_alias 	CTX_STRH('a','l','i','a','s',0,0,0,0,0,0,0,0,0)
#define CTX_all_scroll 	CTX_STRH('a','l','l','-','s','c','r','o','l','l',0,0,0,0)
#define CTX_alpha 	CTX_STRH('a','l','p','h','a',0,0,0,0,0,0,0,0,0)
#define CTX_amp 	CTX_STRH('a','m','p',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_apos 	CTX_STRH('a','p','o','s',0,0,0,0,0,0,0,0,0,0)
#define CTX_aring 	CTX_STRH('a','r','i','n','g',0,0,0,0,0,0,0,0,0)
#define CTX_Aring 	CTX_STRH('A','r','i','n','g',0,0,0,0,0,0,0,0,0)
#define CTX_auto 	CTX_STRH('a','u','t','o',0,0,0,0,0,0,0,0,0,0)
#define CTX_align       CTX_STRH('a','l','i','g','n',0,0,0,0,0,0,0,0)
#define CTX_align_content  CTX_STRH('a','l','i','g','n','-','c','o','n','t','e','n','t',0)
#define CTX_align_items    CTX_STRH('a','l','i','g','n','-','i','t','e','m','s',0,0,0)
#define CTX_align_self     CTX_STRH('a','l','i','g','n','-','s','e','l','f',0,0,0,0)
#define CTX_aspect_ratio CTX_STRH('a','s','p','e','c','t','-','r','a','t','i','o',0,0)
#define CTX_avoid       CTX_STRH('a','v','o','i','d',0,0,0,0,0,0,0,0)
#define CTX_background 	CTX_STRH('b','a','c','k','g','r','o','u','n','d',0,0,0,0)
#define CTX_background_color 	CTX_STRH('b','a','c','k','g','r','o','u','n','d','-','c','o','l')
#define CTX_background_clip     CTX_STRH('b','a','c','k','g','r','o','u','n','d','-','c','l','i')
#define CTX_background_attachment       CTX_STRH('b','a','c','k','g','r','o','u','n','d','-','a','t','t')
#define CTX_background_image    CTX_STRH('b','a','c','k','g','r','o','u','n','d','-','i','m','a')
#define CTX_background_origin   CTX_STRH('b','a','c','k','g','r','o','u','n','d','-','o','r','i')
#define CTX_background_size     CTX_STRH('b','a','c','k','g','r','o','u','n','d','-','s','i','z')
#define CTX_background_position CTX_STRH('b','a','c','k','g','r','o','u','n','d','-','p','o','s')
#define CTX_background_position_x CTX_STRH('b','a','c','k','g','r','o','u','n','d','-','p','o','s') // XXX
#define CTX_background_position_y CTX_STRH('b','a','c','k','g','r','o','u','n','d','-','p','o','s') // XXX
#define CTX_background_repeat   CTX_STRH('b','a','c','k','g','r','o','u','n','d','-','r','e','p')
#define CTX_base  	CTX_STRH('b','a','s','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_bidi_override 	CTX_STRH('b','i','d','i','-','o','v','e','r','r','i','d','e',0)
#define CTX_blink 	CTX_STRH('b','l','i','n','k',0,0,0,0,0,0,0,0,0)
#define CTX_block 	CTX_STRH('b','l','o','c','k',0,0,0,0,0,0,0,0,0)
#define CTX_body 	CTX_STRH('b','o','d','y',0,0,0,0,0,0,0,0,0,0)
#define CTX_bold 	CTX_STRH('b','o','l','d',0,0,0,0,0,0,0,0,0,0)
#define CTX_bolder 	CTX_STRH('b','o','l','d','e','r',0,0,0,0,0,0,0,0)
#define CTX_border 	CTX_STRH('b','o','r','d','e','r',0,0,0,0,0,0,0,0)
#define CTX_border_bottom 	CTX_STRH('b','o','r','d','e','r','-','b','o','t','t','o','m',0)
#define CTX_border_bottom_width CTX_STRH('b','o','r','d','e','r','-','b','o','t','t','o','m','-')
#define CTX_border_bottom_color CTX_STRH('b','o','r','d','e','r','-','b','o','t','t','o','m','c') // XXX
#define CTX_border_box 	CTX_STRH('b','o','r','d','e','r','-','b','o','x',0,0,0,0)
#define CTX_border_collapse CTX_STRH('b','o','r','d','e','r','-','c','o','l','l','a','p','s') // XXX
#define CTX_border_color 	CTX_STRH('b','o','r','d','e','r','-','c','o','l','o','r',0,0)
#define CTX_border_image 	CTX_STRH('b','o','r','d','e','r','-','i','m','a','g','e',0,0)
#define CTX_border_left 	CTX_STRH('b','o','r','d','e','r','-','l','e','f','t',0,0,0)
#define CTX_border_left_width 	CTX_STRH('b','o','r','d','e','r','-','l','e','f','t','-','w','i')
#define CTX_border_left_color 	CTX_STRH('b','o','r','d','e','r','-','l','e','f','t','-','c','o')
#define CTX_border_radius    	CTX_STRH('b','o','r','d','e','r','-','r','a','d','i','u','s',0)
#define CTX_border_right 	CTX_STRH('b','o','r','d','e','r','-','r','i','g','h','t',0,0)
#define CTX_border_right_width 	CTX_STRH('b','o','r','d','e','r','-','r','i','g','h','t','-','w')
#define CTX_border_right_color 	CTX_STRH('b','o','r','d','e','r','-','r','i','g','h','t','-','c')
#define CTX_border_style  	CTX_STRH('b','o','r','d','e','r','-','s','t','y','l','e',0,0)
#define CTX_border_spacing   	CTX_STRH('b','o','r','d','e','r','-','s','p','a','c','i','n','g')
#define CTX_border_top 	CTX_STRH('b','o','r','d','e','r','-','t','o','p',0,0,0,0)
#define CTX_border_top_color 	CTX_STRH('b','o','r','d','e','r','-','t','o','p','-','c','o','l')
#define CTX_border_top_width 	CTX_STRH('b','o','r','d','e','r','-','t','o','p','-','w','i','d')
#define CTX_border_width 	CTX_STRH('b','o','r','d','e','r','-','w','i','d','t','h',0,0)
#define CTX_both 	CTX_STRH('b','o','t','h',0,0,0,0,0,0,0,0,0,0)
#define CTX_bottom 	CTX_STRH('b','o','t','t','o','m',0,0,0,0,0,0,0,0)
#define CTX_box_decoration_break CTX_STRH('b','o','x','-','d','e','c','o','r','a','t','i','o','n')
#define CTX_box_shadow 	CTX_STRH('b','o','x','-','s','h','a','d','o','w',0,0,0,0)
#define CTX_box_sizing 	CTX_STRH('b','o','x','-','s','i','z','i','n','g',0,0,0,0)
#define CTX_br 	CTX_STRH('b','r',0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_bull 	CTX_STRH('b','u','l','l',0,0,0,0,0,0,0,0,0,0)
#define CTX_butt 	CTX_STRH('b','u','t','t',0,0,0,0,0,0,0,0,0,0)
#define CTX_button      CTX_STRH('b','u','t','t','o','n',0,0,0,0,0,0,0,0)
#define CTX_caption_side	CTX_STRH('c','a','p','t','i','o','n','-','s','i','d','e',0,0)
#define CTX_cedil 	CTX_STRH('c','e','d','i','l',0,0,0,0,0,0,0,0,0)
#define CTX_cell 	CTX_STRH('c','e','l','l',0,0,0,0,0,0,0,0,0,0)
#define CTX_cent 	CTX_STRH('c','e','n','t',0,0,0,0,0,0,0,0,0,0)
#define CTX_class 	CTX_STRH('c','l','a','s','s',0,0,0,0,0,0,0,0,0)
#define CTX_clear 	CTX_STRH('c','l','e','a','r',0,0,0,0,0,0,0,0,0)
#define CTX_color 	CTX_STRH('c','o','l','o','r',0,0,0,0,0,0,0,0,0)
#define CTX_copy  	CTX_STRH('c','o','p','y',0,0,0,0,0,0,0,0,0,0)
#define CTX_color_index 	CTX_STRH('c','o','l','o','r','-','i','n','d','e','x',0,0,0)
#define CTX_content     CTX_STRH('c','o','n','t','e','n','t',0,0,0,0,0,0,0)
#define CTX_col_resize 	CTX_STRH('c','o','l','-','r','e','s','i','z','e',0,0,0,0)
#define CTX_counter_increment    CTX_STRH('c','o','u','n','t','e','r','-','i','n','c','r','e','m')
#define CTX_counter_reset        CTX_STRH('c','o','u','n','t','e','r','-','r','e','s','e','t',0)
#define CTX_context_menu 	CTX_STRH('c','o','n','t','e','x','t','-','m','e','n','u',0,0)
#define CTX_crosshair 	CTX_STRH('c','r','o','s','s','h','a','i','r',0,0,0,0,0)
#define CTX_curren 	CTX_STRH('c','u','r','r','e','n',0,0,0,0,0,0,0,0)
#define CTX_cursor 	CTX_STRH('c','u','r','s','o','r',0,0,0,0,0,0,0,0)
#define CTX_cursor_wait 	CTX_STRH('c','u','r','s','o','r','-','w','a','i','t',0,0,0)
#define CTX_d 	CTX_STRH('d',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_dd 	CTX_STRH('d','d',0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_default 	CTX_STRH('d','e','f','a','u','l','t',0,0,0,0,0,0,0)
#define CTX_device_width CTX_STRH('d','e','v','i','c','e','-','w','i','d','t','h',0,0)
#define CTX_device_height CTX_STRH('d','e','v','i','c','e','-','h','e','i','g','h','t',0)
#define CTX_device_aspect_ratio CTX_STRH('d','e','v','i','c','e','-','a','s','p','e','c','t','-')
#define CTX_deg 	CTX_STRH('d','e','g',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_dim 	CTX_STRH('d','i','m',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_div 	CTX_STRH('d','i','v',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_dir 	CTX_STRH('d','i','r',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_direction 	CTX_STRH('d','i','r','e','c','t','i','o','n',0,0,0,0,0)
#define CTX_display 	CTX_STRH('d','i','s','p','l','a','y',0,0,0,0,0,0,0)
#define CTX_dotted 	CTX_STRH('d','o','t','t','e','d',0,0,0,0,0,0,0,0)
#define CTX_dt 	CTX_STRH('d','t',0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_embed 	CTX_STRH('e','m','b','e','d',0,0,0,0,0,0,0,0,0)
#define CTX_empty_cells CTX_STRH('e','m','p','t','y','-','c','e','l','l','s',0,0,0)
#define CTX_e_resize 	CTX_STRH('e','-','r','e','s','i','z','e',0,0,0,0,0,0)
#define CTX_euro 	CTX_STRH('e','u','r','o',0,0,0,0,0,0,0,0,0,0)
#define CTX_even 	CTX_STRH('e','v','e','n',0,0,0,0,0,0,0,0,0,0)
#define CTX_evenodd 	CTX_STRH('e','v','e','n','o','d','d',0,0,0,0,0,0,0)
#define CTX_ew_resize 	CTX_STRH('e','w','-','r','e','s','i','z','e',0,0,0,0,0)
#define CTX_fill 	CTX_STRH('f','i','l','l',0,0,0,0,0,0,0,0,0,0)
#define CTX_file 	CTX_STRH('f','i','l','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_fill_color 	CTX_STRH('f','i','l','l','-','c','o','l','o','r',0,0,0,0)
#define CTX_first_child CTX_STRH('f','i','r','s','t','-','c','h','i','l','d',0,0,0)
#define CTX_fixed 	CTX_STRH('f','i','x','e','d',0,0,0,0,0,0,0,0,0)
#define CTX_flex  	CTX_STRH('f','l','e','x',0,0,0,0,0,0,0,0,0,0)
#define CTX_flex_direction CTX_STRH('f','l','e','x','-','d','i','r','e','c','t','i','o','n')
#define CTX_flex_flow   CTX_STRH('f','l','e','x','-','f','l','o','w',0,0,0,0,0)
#define CTX_flex_grow   CTX_STRH('f','l','e','x','-','g','r','o','w',0,0,0,0,0)
#define CTX_flex_wrap   CTX_STRH('f','l','e','x','-','w','r','a','p',0,0,0,0,0)
#define CTX_flex_shrink CTX_STRH('f','l','e','x','-','s','h','r','i','n','k',0,0,0)
#define CTX_float 	CTX_STRH('f','l','o','a','t',0,0,0,0,0,0,0,0,0)
#define CTX_font_family 	CTX_STRH('f','o','n','t','-','f','a','m','i','l','y',0,0,0)
#define CTX_font_size_adjust 	CTX_STRH('f','o','n','t','-','s','i','z','e','-','a','d','j','u')
#define CTX_font_style 	CTX_STRH('f','o','n','t','-','s','t','y','l','e',0,0,0,0)
#define CTX_font_stretch 	CTX_STRH('f','o','n','t','-','s','t','r','e','t','c','h',0,0)
#define CTX_font_feature_settings CTX_STRH('f','o','n','t','-','f','e','a','t','u','r','-','s','e')
#define CTX_font_kerning          CTX_STRH('f','o','n','t','-','k','e','r','n','i','n','g',0,0)
#define CTX_font_language_override CTX_STRH('f','o','n','t','-','l','a','n','g','u','a','g','e','-')
#define CTX_font_synthesis CTX_STRH('f','o','n','t','-','s','y','n','t','h','e','s','i','s')
#define CTX_font_variant_alternates CTX_STRH('f','o','n','t','-','v','a','r','i','a','n','t','-','a')
#define CTX_font_variant_caps CTX_STRH('f','o','n','t','-','v','a','r','i','a','n','t','-','c')
#define CTX_font_variant_east_asian CTX_STRH('f','o','n','t','-','v','a','r','i','a','n','t','-','e')
#define CTX_font_variant_ligatures CTX_STRH('f','o','n','t','-','v','a','r','i','a','n','t','-','l')
#define CTX_font_variant_numeric CTX_STRH('f','o','n','t','-','v','a','r','i','a','n','t','-','n')
#define CTX_font_variant_positoin CTX_STRH('f','o','n','t','-','v','a','r','i','a','n','t','-','p')
#define CTX_font_weight CTX_STRH('f','o','n','t','-','w','e','i','g','h','t',0,0,0)
#define CTX_font_variant CTX_STRH('f','o','n','t','-','v','a','r','i','a','n','t',0,0)
#define CTX_g           CTX_STRH('g',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_grid  	CTX_STRH('g','r','i','d',0,0,0,0,0,0,0,0,0,0)
#define CTX_gt          CTX_STRH('g','t',0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_hanging_punctuation     CTX_STRH('h','a','a','n','g','i','n','g','-','p','u','n','c','t')
#define CTX_head 	CTX_STRH('h','e','a','d',0,0,0,0,0,0,0,0,0,0)
#define CTX_height 	CTX_STRH('h','e','i','g','h','t',0,0,0,0,0,0,0,0)
#define CTX_hellip 	CTX_STRH('h','e','l','l','i','p',0,0,0,0,0,0,0,0)
#define CTX_help 	CTX_STRH('h','e','l','p',0,0,0,0,0,0,0,0,0,0)
#define CTX_hidden 	CTX_STRH('h','i','d','d','e','n',0,0,0,0,0,0,0,0)
#define CTX_hr          CTX_STRH('h','r',0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_href 	CTX_STRH('h','r','e','f',0,0,0,0,0,0,0,0,0,0)
#define CTX_http 	CTX_STRH('h','t','t','p',0,0,0,0,0,0,0,0,0,0)
#define CTX_hyphens	CTX_STRH('h','y','p','h','e','n','s',0,0,0,0,0,0,0)
#define CTX_id          CTX_STRH('i','d',0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_iexcl 	CTX_STRH('i','e','x','c','l',0,0,0,0,0,0,0,0,0)
#define CTX_image_orientation 	CTX_STRH('i','m','a','g','e','-','o','r','i','e','n','t','a','t')
#define CTX_img 	CTX_STRH('i','m','g',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_inline_block 	CTX_STRH('i','n','l','i','n','e','-','b','l','o','c','k',0,0)
#define CTX_input 	CTX_STRH('i','n','p','u','t',0,0,0,0,0,0,0,0,0)
#define CTX_inset 	CTX_STRH('i','n','s','e','t',0,0,0,0,0,0,0,0,0)
#define CTX_italic 	CTX_STRH('i','t','a','l','i','c',0,0,0,0,0,0,0,0)
#define CTX_justify 	CTX_STRH('j','u','s','t','i','f','y',0,0,0,0,0,0,0)
#define CTX_justify_content 	CTX_STRH('j','u','s','t','i','f','y','-','c','o','n','t','e','n')
#define CTX_justify_items   	CTX_STRH('j','u','s','t','i','f','y','-','i','t','e','m','s',0)
#define CTX_justify_self    	CTX_STRH('j','u','s','t','i','f','y','-','s','e','l','f',0,0)
#define CTX_label 	CTX_STRH('l','a','b','e','l',0,0,0,0,0,0,0,0,0)
#define CTX_lang  	CTX_STRH('l','a','n','g',0,0,0,0,0,0,0,0,0,0)
#define CTX_laquo 	CTX_STRH('l','a','q','u','o',0,0,0,0,0,0,0,0,0)
#define CTX_letter_spacing 	CTX_STRH('l','e','t','t','e','r','-','s','p','a','c','i','n','g')
#define CTX_li 	CTX_STRH('l','i',0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_linethrough 	CTX_STRH('l','i','n','e','t','h','r','o','u','g','h',0,0,0)
#define CTX_line_break   CTX_STRH('l','i','n','e','-','b','r','e','a','k',0,0,0,0)
#define CTX_link 	CTX_STRH('l','i','n','k',0,0,0,0,0,0,0,0,0,0)
#define CTX_list_item 	CTX_STRH('l','i','s','t','-','i','t','e','m',0,0,0,0,0)
#define CTX_list_style	CTX_STRH('l','i','s','t','-','s','t','y','l','e',0,0,0,0)
#define CTX_list_style_image	CTX_STRH('l','i','s','t','-','s','t','y','l','e','-','i','m','a')
#define CTX_list_style_position CTX_STRH('l','i','s','t','-','s','t','y','l','e','-','p','o','s')
#define CTX_list_style_type     CTX_STRH('l','i','s','t','-','s','t','y','l','e','-','t','y','p')
#define CTX_lt 	CTX_STRH('l','t',0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_ltr 	CTX_STRH('l','t','r',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_margin 	CTX_STRH('m','a','r','g','i','n',0,0,0,0,0,0,0,0)
#define CTX_margin_bottom 	CTX_STRH('m','a','r','g','i','n','-','b','o','t','t','o','m',0)
#define CTX_margin_left 	CTX_STRH('m','a','r','g','i','n','-','l','e','f','t',0,0,0)
#define CTX_margin_right 	CTX_STRH('m','a','r','g','i','n','-','r','i','g','h','t',0,0)
#define CTX_margin_top 	CTX_STRH('m','a','r','g','i','n','-','t','o','p',0,0,0,0)
#define CTX_matrix 	CTX_STRH('m','a','t','r','i','x',0,0,0,0,0,0,0,0)
#define CTX_max_height	CTX_STRH('m','a','x','-','h','e','i','g','h','t',0,0,0,0)
#define CTX_max_lines 	CTX_STRH('m','a','x','-','l','i','n','e','s',0,0,0,0,0)
#define CTX_max_width 	CTX_STRH('m','a','x','-','w','i','d','t','h',0,0,0,0,0)
#define CTX_mdash 	CTX_STRH('m','d','a','s','h',0,0,0,0,0,0,0,0,0)
#define CTX_meta 	CTX_STRH('m','e','t','a',0,0,0,0,0,0,0,0,0,0)
#define CTX_middot 	CTX_STRH('m','i','d','d','o','t',0,0,0,0,0,0,0,0)
#define CTX_min_height 	CTX_STRH('m','i','n','-','h','e','i','g','h','t',0,0,0,0)
#define CTX_min_width 	CTX_STRH('m','i','n','-','w','i','d','t','h',0,0,0,0,0)
#define CTX_monochrome  CTX_STRH('m','o','n','o','c','h','r','o','m','e',0,0,0,0)
#define CTX_move 	CTX_STRH('m','o','v','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_nbsp 	CTX_STRH('n','b','s','p',0,0,0,0,0,0,0,0,0,0)
#define CTX_ne_resize 	CTX_STRH('n','e','-','r','e','s','i','z','e',0,0,0,0,0)
#define CTX_nesw_resize 	CTX_STRH('n','e','s','w','-','r','e','s','i','z','e',0,0,0)
#define CTX_no_drop 	CTX_STRH('n','o','-','d','r','o','p',0,0,0,0,0,0,0)
#define CTX_nonzero 	CTX_STRH('n','o','n','z','e','r','o',0,0,0,0,0,0,0)
#define CTX_normal 	CTX_STRH('n','o','r','m','a','l',0,0,0,0,0,0,0,0)
#define CTX_not_allowed 	CTX_STRH('n','o','t','-','a','l','l','o','w','e','d',0,0,0)
#define CTX_nowrap 	CTX_STRH('n','o','w','r','a','p',0,0,0,0,0,0,0,0)
#define CTX_n_resize 	CTX_STRH('n','-','r','e','s','i','z','e',0,0,0,0,0,0)
#define CTX_ns_resize 	CTX_STRH('n','s','-','r','e','s','i','z','e',0,0,0,0,0)
#define CTX_nth_child 	CTX_STRH('n','t','h','-','c','h','i','l','d',0,0,0,0,0)
#define CTX_nw_resize 	CTX_STRH('n','w','-','r','e','s','i','z','e',0,0,0,0,0)
#define CTX_object   t  CTX_STRH('o','b','j','e','c','t',0,0,0,0,0,0,0,0)
#define CTX_object_fit  CTX_STRH('o','b','j','e','c','t','-','f','i','t',0,0,0,0)
#define CTX_object_position  CTX_STRH('o','b','j','e','c','t','-','p','o','s','i','t','i','o')
#define CTX_oblique 	CTX_STRH('o','b','l','i','q','u','e',0,0,0,0,0,0,0)
#define CTX_odd 	CTX_STRH('o','d','d',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_omega 	CTX_STRH('o','m','e','g','a',0,0,0,0,0,0,0,0,0)
#define CTX_opacity 	CTX_STRH('o','p','a','c','i','t','y',0,0,0,0,0,0,0)
#define CTX_ordm 	CTX_STRH('o','r','d','m',0,0,0,0,0,0,0,0,0,0)
#define CTX_order	CTX_STRH('o','r','d','m','e','r',0,0,0,0,0,0,0,0)
#define CTX_oslash 	CTX_STRH('o','s','l','a','s','h',0,0,0,0,0,0,0,0)
#define CTX_Oslash 	CTX_STRH('O','s','l','a','s','h',0,0,0,0,0,0,0,0)
#define CTX_orphans  	CTX_STRH('o','r','p','h','a','n','s',0,0,0,0,0,0,0)
#define CTX_outline  	CTX_STRH('o','u','t','l','i','n','e',0,0,0,0,0,0,0)
#define CTX_outline_color  	CTX_STRH('o','u','t','l','i','n','e','-','c','o','l','o','r',0)
#define CTX_outline_style  	CTX_STRH('o','u','t','l','i','n','e','-','s','t','y','l','e',0)
#define CTX_outline_width  	CTX_STRH('o','u','t','l','i','n','e','-','w','i','d','t','h',0)
#define CTX_overflow 	CTX_STRH('o','v','e','r','f','l','o','w',0,0,0,0,0,0)
#define CTX_overflow_x  CTX_STRH('o','v','e','r','f','l','o','w','-','x',0,0,0,0)
#define CTX_overflow_y  CTX_STRH('o','v','e','r','f','l','o','w','-','y',0,0,0,0)
#define CTX_overflow_wrap  CTX_STRH('o','v','e','r','f','l','o','w','-','w','r','a','p',0)
#define CTX_overline 	CTX_STRH('o','v','e','r','l','i','n','e',0,0,0,0,0,0)
#define CTX_orientation CTX_STRH('o','r','i','e','n','t','a','t','i','o','n',0,0,0)
#define CTX_p 	CTX_STRH('p',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_page     	CTX_STRH('p','a','g','e',0,0,0,0,0,0,0,0,0,0)

#define CTX_page_break_before  CTX_STRH('p','a','g','e','-','b','r','e','a','k','-','b','e','f')
#define CTX_page_break_after  CTX_STRH('p','a','g','e','-','b','r','e','a','k','-','a','f','t')
#define CTX_page_break_inside CTX_STRH('p','a','g','e','-','b','r','e','a','k','-','i','n','s')
#define CTX_padding 	CTX_STRH('p','a','d','d','i','n','g',0,0,0,0,0,0,0)
#define CTX_padding_bottom 	CTX_STRH('p','a','d','d','i','n','g','-','b','o','t','t','o','m')
#define CTX_padding_left 	CTX_STRH('p','a','d','d','i','n','g','-','l','e','f','t',0,0)
#define CTX_padding_right 	CTX_STRH('p','a','d','d','i','n','g','-','r','i','g','h','t',0)
#define CTX_padding_top 	CTX_STRH('p','a','d','d','i','n','g','-','t','o','p',0,0,0)
#define CTX_para 	CTX_STRH('p','a','r','a',0,0,0,0,0,0,0,0,0,0)
#define CTX_path 	CTX_STRH('p','a','t','h',0,0,0,0,0,0,0,0,0,0)
#define CTX_phi 	CTX_STRH('p','h','i',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_plusmn 	CTX_STRH('p','l','u','s','m','n',0,0,0,0,0,0,0,0)
#define CTX_pointer 	CTX_STRH('p','o','i','n','t','e','r',0,0,0,0,0,0,0)
#define CTX_polygon 	CTX_STRH('p','o','l','y','g','o','n',0,0,0,0,0,0,0)
#define CTX_position 	CTX_STRH('p','o','s','i','t','i','o','n',0,0,0,0,0,0)
#define CTX_pound 	CTX_STRH('p','o','u','n','d',0,0,0,0,0,0,0,0,0)
#define CTX_pre 	CTX_STRH('p','r','e',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_pre_line 	CTX_STRH('p','r','e','-','l','i','n','e',0,0,0,0,0,0)
#define CTX_pre_wrap 	CTX_STRH('p','r','e','-','w','r','a','p',0,0,0,0,0,0)
#define CTX_print_symbols 	CTX_STRH('p','r','i','n','t','-','s','y','m','b','o','l','s',0)
#define CTX_progress 	CTX_STRH('p','r','o','g','r','e','s','s',0,0,0,0,0,0)
#define CTX_quot 	CTX_STRH('q','u','o','t',0,0,0,0,0,0,0,0,0,0)
#define CTX_quotes      CTX_STRH('q','u','o','t','e','s',0,0,0,0,0,0,0,0,0)
#define CTX_raquo 	CTX_STRH('r','a','q','u','o',0,0,0,0,0,0,0,0,0)
#define CTX_rect 	CTX_STRH('r','e','c','t',0,0,0,0,0,0,0,0,0,0)
#define CTX_rel 	CTX_STRH('r','e','l',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_reg 	CTX_STRH('r','e','g',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_relative 	CTX_STRH('r','e','l','a','t','i','v','e',0,0,0,0,0,0)
#define CTX_reverse 	CTX_STRH('r','e','v','e','r','s','e',0,0,0,0,0,0,0)
#define CTX_resize 	CTX_STRH('r','e','s','i','z','e',0,0,0,0,0,0,0,0)
#define CTX_resolution  CTX_STRH('r','e','s','o','l','u','t','i','o','n',0,0,0,0)
#define CTX_rotate 	CTX_STRH('r','o','t','a','t','e',0,0,0,0,0,0,0,0)
#define CTX_row_resize 	CTX_STRH('r','o','w','-','r','e','s','i','z','e',0,0,0,0)
#define CTX_rtl 	CTX_STRH('r','t','l',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_scale 	CTX_STRH('s','c','a','l','e',0,0,0,0,0,0,0,0,0)
#define CTX_scan  	CTX_STRH('s','c','a','n',0,0,0,0,0,0,0,0,0,0)
#define CTX_scroll 	CTX_STRH('s','c','r','o','l','l',0,0,0,0,0,0,0,0)
#define CTX_sans_serif 	CTX_STRH('s','a','n','s','-','s','e','r','i','f',0,0,0,0)
#define CTX_serif 	CTX_STRH('s','e','r','i','f',0,0,0,0,0,0,0,0,0)
#define CTX_sect 	CTX_STRH('s','e','c','t',0,0,0,0,0,0,0,0,0,0)
#define CTX_select      CTX_STRH('s','e','l','e','c','t',0,0,0,0,0,0,0,0)
#define CTX_se_resize 	CTX_STRH('s','e','-','r','e','s','i','z','e',0,0,0,0,0)
#define CTX_shy 	CTX_STRH('s','h','y',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_size  	CTX_STRH('s','i','z','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_solid 	CTX_STRH('s','o','l','i','d',0,0,0,0,0,0,0,0,0)
#define CTX_span  	CTX_STRH('s','p','a','n',0,0,0,0,0,0,0,0,0,0)
#define CTX_src 	CTX_STRH('s','r','c',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_svg 	CTX_STRH('s','v','g',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_s_resize 	CTX_STRH('s','-','r','e','s','i','z','e',0,0,0,0,0,0)
#define CTX_static 	CTX_STRH('s','t','a','t','i','c',0,0,0,0,0,0,0,0)
#define CTX_stroke 	CTX_STRH('s','t','r','o','k','e',0,0,0,0,0,0,0,0)
#define CTX_strong 	CTX_STRH('s','t','r','o','n','g',0,0,0,0,0,0,0,0)
#define CTX_stroke_color 	CTX_STRH('s','t','r','o','k','e','-','c','o','l','o','r',0,0)
#define CTX_stroke_linecap 	CTX_STRH('s','t','r','o','k','e','-','l','i','n','e','c','a','p')
#define CTX_stroke_linejoin 	CTX_STRH('s','t','r','o','k','e','-','l','i','n','e','j','o','i')
#define CTX_stroke_miterlimit 	CTX_STRH('s','t','r','o','k','e','-','m','i','t','e','r','l','i')
#define CTX_stroke_width 	CTX_STRH('s','t','r','o','k','e','-','w','i','d','t','h',0,0)
#define CTX_style 	CTX_STRH('s','t','y','l','e',0,0,0,0,0,0,0,0,0)
#define CTX_sub 	CTX_STRH('s','u','b',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_sup1 	CTX_STRH('s','u','p','1',0,0,0,0,0,0,0,0,0,0)
#define CTX_sup2 	CTX_STRH('s','u','p','2',0,0,0,0,0,0,0,0,0,0)
#define CTX_sup3 	CTX_STRH('s','u','p','3',0,0,0,0,0,0,0,0,0,0)
#define CTX_super 	CTX_STRH('s','u','p','e','r',0,0,0,0,0,0,0,0,0)
#define CTX_sw_resize 	CTX_STRH('s','w','-','r','e','s','i','z','e',0,0,0,0,0)
#define CTX_syntax_highlight 	CTX_STRH('s','y','n','t','a','x','-','h','i','g','h','l','i','g')
#define CTX_table    	CTX_STRH('t','a','b','l','e',0,0,0,0,0,0,0,0,0)
#define CTX_table_cell  CTX_STRH('t','a','b','l','e','-','c','e','l','l',0,0,0,0)
#define CTX_table_layout        CTX_STRH('t','a','b','l','e','-','l','a','y','o','u','t',0,0)
#define CTX_tab_size 	CTX_STRH('t','a','b','-','s','i','z','e',0,0,0,0,0,0)
#define CTX_td          CTX_STRH('t','d',0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_text        CTX_STRH('t','e','x','t',0,0,0,0,0,0,0,0,0,0)
#define CTX_textarea        	CTX_STRH('t','e','x','t','a','r','e','a',0,0,0,0,0,0)
#define CTX_text_align_last 	CTX_STRH('t','e','x','t','-','a','l','i','g','n','l','a','s','t')
#define CTX_text_combine_horizontal CTX_STRH('t','e','x','t','-','c','o','m','b','i','n','e','-','h')
#define CTX_text_emphasis   	CTX_STRH('t','e','x','t','-','e','m','p','h','a','s','i','s',0)
#define CTX_text_indent    CTX_STRH('t','e','x','t','_','i','n','d','e','n','t', 0,0,0)

#define CTX_text_justify    	CTX_STRH('t','e','x','t','-','j','u','s','t','i','f','y',0,0)
#define CTX_text_decoration 	CTX_STRH('t','e','x','t','-','d','e','c','o','r','a','t','i','o')
#define CTX_text_orientation	CTX_STRH('t','e','x','t','-','o','r','i','e','n','t','a','t','i')
#define CTX_text_shadow 	CTX_STRH('t','e','x','t','-','s','h','a','d','o','w',0,0,0)
#define CTX_text_stroke_color 	CTX_STRH('t','e','x','t','-','s','t','r','o','k','e','-','c','o')
#define CTX_text_stroke_width 	CTX_STRH('t','e','x','t','-','s','t','r','o','k','e','-','w','i')
#define CTX_text_transform    	CTX_STRH('t','e','x','t','-','t','r','a','n','s','f','o','r','m')
#define CTX_title       CTX_STRH('t','i','t','l','e',0,0,0,0,0,0,0,0,0)
#define CTX_thead       CTX_STRH('t','h','e','a','d',0,0,0,0,0,0,0,0,0)
#define CTX_tbody       CTX_STRH('t','b','o','d','y',0,0,0,0,0,0,0,0,0)
#define CTX_tfoot       CTX_STRH('t','f','o','o','t',0,0,0,0,0,0,0,0,0)
#define CTX_thin        CTX_STRH('t','h','i','n',0,0,0,0,0,0,0,0,0,0)
#define CTX_tr          CTX_STRH('t','r',0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_trade       CTX_STRH('t','r','a','d','e',0,0,0,0,0,0,0,0,0)
#define CTX_transform   CTX_STRH('t','r','a','n','s','f','o','r','m',0,0,0,0,0)
#define CTX_transform_origin   CTX_STRH('t','r','a','n','s','f','o','r','m','-''o','r','i','g')
#define CTX_transform_style    CTX_STRH('t','r','a','n','s','f','o','r','m','-','s','t','y','l')
#define CTX_backface_visibility CTX_STRH('b','a','c','k','f','a','c','e','-','v','i','s','i','b')
#define CTX_translate   CTX_STRH('t','r','a','n','s','l','a','t','e',0,0,0,0,0)
#define CTX_true        CTX_STRH('t','r','u','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_underline   CTX_STRH('u','n','d','e','r','l','i','n','e',0,0,0,0,0)
#define CTX_unicode_bidi    CTX_STRH('u','n','i','c','o','d','e','-','b','i','d','i',0,0)
#define CTX_unicode_range CTX_STRH('u','n','i','c','o','d','e','-','r','a','n','g','e',0)
#define CTX_vertical_align  CTX_STRH('v','e','r','t','i','c','a','l','-','a','l','i','g','n')
#define CTX_vertical_text   CTX_STRH('v','e','r','t','i','c','a','l','-','t','e','x','t',0)
#define CTX_viewbox     	CTX_STRH('v','i','e','w','b','o','x',0,0,0,0,0,0,0)
#define CTX_visibility 	CTX_STRH('v','i','s','i','b','i','l','i','t','y',0,0,0,0)
#define CTX_visible 	CTX_STRH('v','i','s','i','b','l','e',0,0,0,0,0,0,0)
#define CTX_white_space CTX_STRH('w','h','i','t','e','-','s','p','a','c','e',0,0,0)
#define CTX_width 	CTX_STRH('w','i','d','t','h',0,0,0,0,0,0,0,0,0)
#define CTX_word_break   CTX_STRH('w','o','r','d','-','b','r','e','a','k',0,0,0,0)
#define CTX_word_wrap    CTX_STRH('w','o','r','d','-','w','r','a','p',0,0,0,0,0)
#define CTX_word_spacing CTX_STRH('w','o','r','d','-','s','p','a','c','i','n','g',0,0)
#define CTX_writing_mode CTX_STRH('w','r','i','t','i','n','g','-','m','o','d','e',0,0)
#define CTX_w_resize 	CTX_STRH('w','-','r','e','s','i','z','e',0,0,0,0,0,0)
#define CTX_x           CTX_STRH('x',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_y 	        CTX_STRH('y',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_yen 	CTX_STRH('y','e','n',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_yes 	CTX_STRH('y','e','s',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_z_index	CTX_STRH('z','-','i','n','d','e','x',0,0,0,0,0,0,0)
#define CTX_zoom_in 	CTX_STRH('z','o','o','m','-','i','n',0,0,0,0,0,0,0)
#define CTX_zoom_out 	CTX_STRH('z','o','o','m','-','o','u','t',0,0,0,0,0,0)


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif



/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <math.h>

typedef struct _Mrg Mrg;
typedef struct _MrgHtml      MrgHtml; 
typedef struct _MrgHtmlState MrgHtmlState;


void mrg_clear (Mrg *mrg);

static void mrg_queue_draw (Mrg *mrg, CtxRectangle *rect)
{
}

typedef struct _MrgStyleNode MrgStyleNode;
typedef struct _MrgHtmlState MrgHtmlState;

#define MRG_STYLE_MAX_CLASSES 8
#define MRG_STYLE_MAX_PSEUDO  8

struct _MrgStyleNode
{
  int         is_direct_parent; /* for use in selector chains with > */
  const char *id;
  uint32_t id_hash;
  uint32_t element_hash;
  uint32_t classes_hash[MRG_STYLE_MAX_CLASSES];
  const char *pseudo[MRG_STYLE_MAX_PSEUDO];
              // TODO : to hash pseudos we need to store
              //   argument (like for nth-child)
  uint32_t pseudo_hash[MRG_STYLE_MAX_PSEUDO];
};

typedef enum {
  MRG_FLOAT_NONE = 0,
  MRG_FLOAT_LEFT,
  MRG_FLOAT_RIGHT,
  MRG_FLOAT_FIXED
} MrgFloat;


typedef struct MrgFloatData {
  MrgFloat  type;
  float     x;
  float     y;
  float     width;
  float     height;
} MrgFloatData;

struct _MrgHtmlState
{
  float        original_x;
  float        original_y;
  float        block_start_x;
  float        block_start_y;
  float        ptly;
  float        vmarg;
  MrgFloatData float_data[MRG_MAX_FLOATS];
  int          floats;
};


typedef void (*MrgNewText)       (const char *new_text, void *data);
typedef void (*UiRenderFun)      (Mrg *mrg, void *ui_data);

struct _MrgHtml
{
  Mrg *mrg;
  int foo;
  MrgHtmlState  states[MRG_MAX_STYLE_DEPTH];
  MrgHtmlState *state;
  int state_no;
  CtxList *geo_cache;
  //char  attribute[MRG_XML_MAX_ATTRIBUTES][MRG_XML_MAX_ATTRIBUTE_LEN];
  //char  value[MRG_XML_MAX_ATTRIBUTES][MRG_XML_MAX_VALUE_LEN];
  //int   attributes;
};

typedef struct _Mrg Mrg;
typedef struct _MrgGeoCache MrgGeoCache;
struct _MrgGeoCache
{
  void *id_ptr;
  float height;
  float width;
  int   hover;
  int gen;
};

struct _MrgColor {
  float red;
  float green;
  float blue;
  float alpha;
};

typedef enum {
  MRG_DISPLAY_INLINE = 0,
  MRG_DISPLAY_BLOCK,
  MRG_DISPLAY_LIST_ITEM,
  MRG_DISPLAY_NONE,
  MRG_DISPLAY_INLINE_BLOCK,
  MRG_DISPLAY_FLEX,
  MRG_DISPLAY_GRID,
  MRG_DISPLAY_INLINE_FLEX,
  MRG_DISPLAY_INLINE_GRID,
  MRG_DISPLAY_INLINE_TABLE,
  MRG_DISPLAY_RUN_IN,
  MRG_DISPLAY_TABLE,
  MRG_DISPLAY_TABLE_CAPTION,
  MRG_DISPLAY_TABLE_COLUMN_GROUP,
  MRG_DISPLAY_TABLE_HEADER_GROUP,
  MRG_DISPLAY_TABLE_FOOTER_GROUP,
  MRG_DISPLAY_TABLE_ROW_GROUP,
  MRG_DISPLAY_TABLE_CELL,
  MRG_DISPLAY_TABLE_COLUMN
} MrgDisplay;

/* matches cairo order */
typedef enum
{
  MRG_FONT_WEIGHT_NORMAL = 0,
  MRG_FONT_WEIGHT_BOLD
} MrgFontWeight;

typedef enum
{
  MRG_FILL_RULE_NONZERO = 0,
  MRG_FILL_RULE_EVEN_ODD
} MrgFillRule;

/* matches cairo order */
typedef enum
{
  MRG_FONT_STYLE_NORMAL = 0,
  MRG_FONT_STYLE_ITALIC,
  MRG_FONT_STYLE_OBLIQUE
} MrgFontStyle;

typedef enum
{
  MRG_BOX_SIZING_CONTENT_BOX = 0,
  MRG_BOX_SIZING_BORDER_BOX
} MrgBoxSizing;

/* matching nchanterm definitions */

typedef enum {
  MRG_REGULAR     = 0,
  MRG_BOLD        = (1 << 0),
  MRG_DIM         = (1 << 1),
  MRG_UNDERLINE   = (1 << 2),
  MRG_REVERSE     = (1 << 3),
  MRG_OVERLINE    = (1 << 4),
  MRG_LINETHROUGH = (1 << 5),
  MRG_BLINK       = (1 << 6)
} MrgTextDecoration;

typedef enum {
  MRG_POSITION_STATIC = 0,
  MRG_POSITION_RELATIVE,
  MRG_POSITION_FIXED,
  MRG_POSITION_ABSOLUTE
} MrgPosition;

typedef enum {
  MRG_OVERFLOW_VISIBLE = 0,
  MRG_OVERFLOW_HIDDEN,
  MRG_OVERFLOW_SCROLL,
  MRG_OVERFLOW_AUTO
} MrgOverflow;

typedef enum {
  MRG_CLEAR_NONE = 0,
  MRG_CLEAR_LEFT,
  MRG_CLEAR_RIGHT,
  MRG_CLEAR_BOTH
} MrgClear;

typedef enum {
  MRG_TEXT_ALIGN_LEFT = 0,
  MRG_TEXT_ALIGN_RIGHT,
  MRG_TEXT_ALIGN_JUSTIFY,
  MRG_TEXT_ALIGN_CENTER
} MrgTextAlign;

typedef enum {
  MRG_WHITE_SPACE_NORMAL = 0,
  MRG_WHITE_SPACE_NOWRAP,
  MRG_WHITE_SPACE_PRE,
  MRG_WHITE_SPACE_PRE_LINE,
  MRG_WHITE_SPACE_PRE_WRAP
} MrgWhiteSpace;

typedef enum {
  MRG_VERTICAL_ALIGN_BASELINE = 0,
  MRG_VERTICAL_ALIGN_MIDDLE,
  MRG_VERTICAL_ALIGN_BOTTOM,
  MRG_VERTICAL_ALIGN_TOP,
  MRG_VERTICAL_ALIGN_SUB,
  MRG_VERTICAL_ALIGN_SUPER
} MrgVerticalAlign;

typedef enum {
  MRG_CURSOR_AUTO = 0,
  MRG_CURSOR_ALIAS,
  MRG_CURSOR_ALL_SCROLL,
  MRG_CURSOR_CELL,
  MRG_CURSOR_CONTEXT_MENU,
  MRG_CURSOR_COL_RESIZE,
  MRG_CURSOR_COPY,
  MRG_CURSOR_CROSSHAIR,
  MRG_CURSOR_DEFAULT,
  MRG_CURSOR_E_RESIZE,
  MRG_CURSOR_EW_RESIZE,
  MRG_CURSOR_HELP,
  MRG_CURSOR_MOVE,
  MRG_CURSOR_N_RESIZE,
  MRG_CURSOR_NE_RESIZE,
  MRG_CURSOR_NESW_RESIZE,
  MRG_CURSOR_NS_RESIZE,
  MRG_CURSOR_NW_RESIZE,
  MRG_CURSOR_NO_DROP,
  MRG_CURSOR_NONE,
  MRG_CURSOR_NOT_ALLOWED,
  MRG_CURSOR_POINTER,
  MRG_CURSOR_PROGRESS,
  MRG_CURSOR_ROW_RESIZE,
  MRG_CURSOR_S_RESIZE,
  MRG_CURSOR_SE_RESIZE,
  MRG_CURSOR_SW_RESIZE,
  MRG_CURSOR_TEXT,
  MRG_CURSOR_VERTICAL_TEXT,
  MRG_CURSOR_W_RESIZE,
  MRG_CURSOR_WAIT,
  MRG_CURSOR_ZOOM_IN,
  MRG_CURSOR_ZOOM_OUT
} MrgCursor;


typedef enum {
  MRG_UNICODE_BIDI_NORMAL = 0,
  MRG_UNICODE_BIDI_EMBED,
  MRG_UNICODE_BIDI_BIDI_OVERRIDE
} MrgUnicodeBidi;

typedef enum {
  MRG_DIRECTION_LTR = 0,
  MRG_DIRECTION_RTL
} MrgDirection;

typedef enum {
  MRG_VISIBILITY_VISIBLE = 0,
  MRG_VISIBILITY_HIDDEN
} MrgVisibility;

typedef enum {
  MRG_LIST_STYLE_OUTSIDE = 0,
  MRG_LIST_STYLE_INSIDE
} CtxListStyle;

/* This style class should be able to grow to contain some color names with
 * semantic meaning.
 */
struct _MrgStyle {
  /* text-related, we could potentially store *all* variables in keydb
   * that would both be slower and more bloatful than tightly packed bits,
   * some things currently in keydb should maybe be moved out for
   * performance.
   */
  float               font_size; // used for mrg_em() should be direct
  float               line_height;
  MrgVisibility       visibility:1;
  MrgFillRule         fill_rule:1;
  MrgFontStyle        font_style:3;
  MrgFontWeight       font_weight:4;
  CtxLineCap          stroke_linecap:2;
  CtxLineJoin         stroke_linejoin:2;
  MrgTextAlign        text_align:2;
  MrgPosition         position:2;
  MrgBoxSizing        box_sizing:1;
  MrgVerticalAlign    vertical_align:3;
  MrgWhiteSpace       white_space:3;
  MrgUnicodeBidi      unicode_bidi:2;
  MrgDirection        direction:2;
  CtxListStyle        list_style:1;
  MrgClear            clear:2;
  unsigned char       fill:1;
  MrgCursor           cursor:6;
  MrgTextDecoration   text_decoration:7;
  unsigned char       width_auto:1;
  unsigned char       margin_left_auto:1;
  unsigned char       margin_right_auto:1;
  unsigned char       print_symbols:1;
  MrgFloat            float_:2;
  unsigned char       stroke:1;
  MrgOverflow         overflow:2;
  MrgDisplay          display:5;
  void               *id_ptr;
};

typedef struct _MrgString MrgString;
typedef struct _MrgStyle MrgStyle;

typedef struct MrgState {
  float      (*wrap_edge_left)  (Mrg *mrg, void *data);
  float      (*wrap_edge_right) (Mrg *mrg, void *data);
  void        *wrap_edge_data;
  void       (*post_nl)  (Mrg *mrg, void *post_nl_data, int last);
  void        *post_nl_data;
  float        edge_top;
  float        edge_left;
  float        edge_right;
  float        edge_bottom;

  int          skip_lines;  /* better with an em offset? */
  int          max_lines;   /* better with max-y in ems? ? */

  char        *style_id;
  MrgStyleNode style_node;
  MrgStyle     style;

  int          children;
  int          overflowed:1;
  int          span_bg_started:1;
} MrgState;;

struct _Mrg {
  Ctx            *ctx;


  float            rem;
  MrgHtml          html;
  float            ddpx;
  CtxList         *stylesheet;
  void            *css_parse_state;
  MrgString       *style;
  MrgString       *style_global;
  //int          is_press_grabbed;
  int              quit;
  float            x; /* in px */
  float            y; /* in px */
  CtxRectangle     dirty;
  CtxRectangle     dirty_during_paint; // queued during painting
  MrgState        *state;
  CtxList         *geo_cache;
  MrgState         states[MRG_MAX_STATE_DEPTH];
  int              state_no;
  int              in_paint;
  void            *backend_data;
  int              do_clip;
  int (*mrg_get_contents) (const char  *referer,
                           const char  *input_uri,
                           char       **contents,
                           long        *length,
                           void        *get_contents_data);
  void *get_contents_data;

  void (*ui_update)(Mrg *mrg, void *user_data);
  void *user_data;

  //MrgBackend *backend;
  char      *title;

    /** text editing state follows **/
  int              text_edited;
  int              got_edit;
  MrgString       *edited_str;
  char           **edited;

  int              text_edit_blocked;
  MrgNewText       update_string;
  void            *update_string_user_data;

  CtxDestroyNotify update_string_destroy_notify;
  void            *update_string_destroy_data;

  int              cursor_pos;
  float            e_x;
  float            e_y;
  float            e_ws;
  float            e_we;
  float            e_em;
  float            offset_x;
  float            offset_y;
  //cairo_scaled_font_t *scaled_font;

  CtxEventType          text_listen_types[MRG_MAX_TEXT_LISTEN];
  CtxCb            text_listen_cb[MRG_MAX_TEXT_LISTEN];
  void            *text_listen_data1[MRG_MAX_TEXT_LISTEN];
  void            *text_listen_data2[MRG_MAX_TEXT_LISTEN];

  void     (*text_listen_finalize[MRG_MAX_TEXT_LISTEN])(void *listen_data, void *listen_data2, void *finalize_data);
  void      *text_listen_finalize_data[MRG_MAX_TEXT_LISTEN];
  int        text_listen_count;
  int        text_listen_active;

  int        printing;
  //cairo_t *printing_cr;
};

static Ctx *mrg_cr (Mrg *mrg)
{
  return mrg->ctx;
}


/* XXX: stopping sibling grabs should be an addtion to stop propagation,
 * this would permit multiple events to co-register, and use that
 * to signal each other,.. or perhaps more coordination is needed
 */
void _mrg_clear_text_closures (Mrg *mrg)
{
  int i;
  for (i = 0; i < mrg->text_listen_count; i ++)
  {
    if (mrg->text_listen_finalize[i])
       mrg->text_listen_finalize[i](
         mrg->text_listen_data1[i],
         mrg->text_listen_data2[i],
         mrg->text_listen_finalize_data[i]);
  }
  mrg->text_listen_count  = 0;
  mrg->text_listen_active = 0;
}

static CtxList *interns = NULL;

const char * mrg_intern_string (const char *str)
{
  CtxList *i;
  for (i = interns; i; i = i->next)
  {
    if (!strcmp (i->data, str))
      return i->data;
  }
  str = strdup (str);
  ctx_list_append (&interns, (void*)str);
  return str;
}

/*
 * Copyright (c) 2002, 2003, Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#if 0
void _ctX_bindings_key_down (CtxEvent *event, void *data1, void *data2)
{
  Ctx *ctx = event->ctx;
  CtxEvents *mrg = &ctx->events;
  int i;
  int handled = 0;

  for (i = mrg->n_bindings-1; i>=0; i--)
    if (!strcmp (mrg->bindings[i].nick, event->string))
    {
      if (mrg->bindings[i].cb)
      {
        mrg->bindings[i].cb (event, mrg->bindings[i].cb_data, NULL);
        if (event->stop_propagate)
          return;
        handled = 1;
      }
    }
  if (!handled)
  for (i = mrg->n_bindings-1; i>=0; i--)
    if (!strcmp (mrg->bindings[i].nick, "unhandled"))
    {
      if (mrg->bindings[i].cb)
      {
        mrg->bindings[i].cb (event, mrg->bindings[i].cb_data, NULL);
        if (event->stop_propagate)
          return;
      }
    }
}
#endif

#ifndef XMLTOK_H
#define XMLTOK_H

#include <stdio.h>


typedef struct _Mrg Mrg;

typedef struct _MrgXml MrgXml;

enum
{
  t_none = 0,
  t_whitespace,
  t_prolog,
  t_dtd,
  t_comment,
  t_word,
  t_tag,
  t_closetag,
  t_closeemptytag,
  t_endtag,
  t_att = 10,
  t_val,
  t_eof,
  t_entity,
  t_error
};

MrgXml *xmltok_new     (FILE *file_in);
MrgXml *xmltok_buf_new (char *membuf);
void    xmltok_free    (MrgXml *t);
int     xmltok_lineno  (MrgXml *t);
int     xmltok_get     (MrgXml *t, char **data, int *pos);

#endif /*XMLTOK_H */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */



struct _MrgString
{
  char *str;
  int   length;
  int   utf8_length;
  int   allocated_length;
}  __attribute((packed));

MrgString   *mrg_string_new_with_size  (const char *initial, int initial_size);
MrgString   *mrg_string_new            (const char *initial);
MrgString   *mrg_string_new_printf     (const char *format, ...);
void         mrg_string_free           (MrgString  *string, int freealloc);
char        *mrg_string_dissolve       (MrgString  *string);
const char  *mrg_string_get            (MrgString  *string);
int          mrg_string_get_length     (MrgString  *string);
void         mrg_string_set            (MrgString  *string, const char *new_string);
void         mrg_string_clear          (MrgString  *string);
void         mrg_string_append_str     (MrgString  *string, const char *str);
void         mrg_string_append_byte    (MrgString  *string, char  val);
void         mrg_string_append_string  (MrgString  *string, MrgString *string2);
void         mrg_string_append_data    (MrgString  *string, const char *data, int len);
void         mrg_string_append_printf  (MrgString  *string, const char *format, ...);


/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void mrg_string_init (MrgString *string, int initial_size)
{
  string->allocated_length = initial_size;
  string->length = 0;
  string->utf8_length = 0;
  string->str = malloc (string->allocated_length);
  string->str[0]='\0';
}

static void mrg_string_destroy (MrgString *string)
{
  if (string->str)
  {
    free (string->str);
    string->str = NULL;
  }
}

void mrg_string_clear (MrgString *string)
{
  string->length = 0;
  string->utf8_length = 0;
  string->str[string->length]=0;
}

static inline void _mrg_string_append_byte (MrgString *string, char  val)
{
  if ((val & 0xC0) != 0x80)
    string->utf8_length++;
  if (string->length + 1 >= string->allocated_length)
    {
      char *old = string->str;
      string->allocated_length *= 2;
      string->str = malloc (string->allocated_length);
      memcpy (string->str, old, string->allocated_length/2);
      free (old);
    }
  string->str[string->length++] = val;
  string->str[string->length] = '\0';
}
void mrg_string_append_byte (MrgString *string, char  val)
{
  _mrg_string_append_byte (string, val);
}

static inline void _mrg_string_append_str (MrgString *string, const char *str)
{
  if (!str) return;
  while (*str)
    {
      _mrg_string_append_byte (string, *str);
      str++;
    }
}
void mrg_string_append_str (MrgString *string, const char *str)
{
  _mrg_string_append_str (string, str);
}

MrgString *mrg_string_new_with_size (const char *initial, int initial_size)
{
  MrgString *string = calloc (sizeof (MrgString), 1);
  mrg_string_init (string, initial_size);
  if (initial)
    _mrg_string_append_str (string, initial);
  return string;
}

MrgString *mrg_string_new (const char *initial)
{
  return mrg_string_new_with_size (initial, 8);
}

void mrg_string_append_data (MrgString *string, const char *str, int len)
{
  int i;
  for (i = 0; i<len; i++)
    _mrg_string_append_byte (string, str[i]);
}

void mrg_string_append_string (MrgString *string, MrgString *string2)
{
  const char *str = mrg_string_get (string2);
  while (str && *str)
    {
      _mrg_string_append_byte (string, *str);
      str++;
    }
}
const char *mrg_string_get (MrgString *string)
{
  return string->str;
}
int mrg_string_get_length (MrgString *string)
{
  return string->length;
}

/* dissolving a string, means destroying it, but returning
 * the string, that should be manually freed.
 */
char *mrg_string_dissolve   (MrgString *string)
{
  char *ret = string->str;
  string->str = NULL;
  free (string);
  return ret;
}

void
mrg_string_free (MrgString *string, int freealloc)
{
  if (freealloc)
    {
      mrg_string_destroy (string);
    }
  free (string);
}

void
mrg_string_append_printf (MrgString *string, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  _mrg_string_append_str (string, buffer);
  free (buffer);
}

MrgString *mrg_string_new_printf (const char *format, ...)
{
  MrgString *string = mrg_string_new_with_size ("", 8);
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  _mrg_string_append_str (string, buffer);
  free (buffer);
  return string;
}

void
mrg_string_set (MrgString *string, const char *new_string)
{
  mrg_string_clear (string);
  _mrg_string_append_str (string, new_string);
}

struct _MrgXml
{
  FILE      *file_in;
  int        state;
  MrgString *curdata;
  MrgString *curtag;
  int        c;
  int        c_held;

  unsigned char *inbuf;
  int            inbuflen;
  int            inbufpos;
  int            line_no;
};

enum
{
  s_null = 0,
  s_start,
  s_tag,
  s_tagnamestart,
  s_tagname,
  s_tagnamedone,
  s_intag,
  s_attstart,
  s_attname,
  s_attdone,
  s_att,
  s_atteq,
  s_eqquot,
  s_eqvalstart,
  s_eqapos,
  s_eqaposval,
  s_eqaposvaldone,
  s_eqval,
  s_eqvaldone,
  s_eqquotval,
  s_eqquotvaldone,
  s_tagend,
  s_empty,
  s_inempty,
  s_emptyend,
  s_whitespace,
  s_whitespacedone,
  s_entitystart,
  s_entity,
  s_entitydone,
  s_word,
  s_worddone,
  s_tagclose,
  s_tagclosenamestart,
  s_tagclosename,
  s_tagclosedone,
  s_tagexcl,
  s_commentdash1,
  s_commentdash2,
  s_incomment,
  s_commentenddash1,
  s_commentenddash2,
  s_commentdone,
  s_dtd,
  s_prolog,
  s_prologq,
  s_prologdone,
  s_eof,
  s_error
};

char     *c_ws = " \n\r\t";

enum
{
  c_nil = 0,
  c_eat = 1,                    /* request that another char be used for the next state */
  c_store = 2                   /* store the current char in the output buffer */
};

typedef struct
{
  int           state;
  char         *chars;
  unsigned char r_start;
  unsigned char r_end;
  int           next_state;
  int           resetbuf;
  int           charhandling;
  int           return_type;        /* if set return current buf, with type set to the type */
}
state_entry;

#define max_entries 20

static state_entry state_table[s_error][max_entries];

static void
a (int state,
   char *chars,
   unsigned char r_start,
   unsigned char r_end, int charhandling, int next_state)
{
  int       no = 0;
  while (state_table[state][no].state != s_null)
    no++;
  state_table[state][no].state = state;
  state_table[state][no].r_start = r_start;
  if (chars)
    state_table[state][no].chars = chars;
  state_table[state][no].r_end = r_end;
  state_table[state][no].charhandling = charhandling;
  state_table[state][no].next_state = next_state;
}

static void
r (int state, int return_type, int next_state)
{
  state_table[state][0].state = state;
  state_table[state][0].return_type = return_type;
  state_table[state][0].next_state = next_state;
}

/* *INDENT-OFF* */

static void
init_statetable (void) {
    static int inited=0;
    if(inited)
        return;
    inited=1;
    memset(state_table,0,sizeof(state_table));
    a(s_start,        "<",  0,0,              c_eat,            s_tag);
    a(s_start,        c_ws, 0,0,              c_eat+c_store,    s_whitespace);
    a(s_start,        "&",  0,0,              c_eat,            s_entitystart);
    a(s_start,        NULL, 0,255,            c_eat+c_store,    s_word);
    a(s_tag,          c_ws, 0,0,              c_eat,            s_tag);
    a(s_tag,          "/",  0,0,              c_eat,            s_tagclose);
    a(s_tag,          "!",  0,0,              c_eat,            s_tagexcl);
    a(s_tag,          "?",  0,0,              c_eat,            s_prolog);
    a(s_tag,          NULL, 0,255,            c_eat+c_store,    s_tagnamestart);
    a(s_tagclose,     NULL,    0,255,         c_eat+c_store,    s_tagclosenamestart);
    a(s_tagclosenamestart,    ">",    0,0,    c_eat,            s_tagclosedone);
    a(s_tagclosenamestart,    NULL,    0,255, c_eat+c_store,    s_tagclosename);
    a(s_tagclosename,    ">",    0,0,         c_eat,            s_tagclosedone);
    a(s_tagclosename,    NULL,    0,255,      c_eat+c_store,    s_tagclosename);
    r(s_tagclosedone,    t_closetag,                            s_start);

    a(s_whitespace,        c_ws,    0,0,      c_eat+c_store,    s_whitespace);
    a(s_whitespace,        NULL,    0,255,    c_nil,            s_whitespacedone);
    r(s_whitespacedone,    t_whitespace,                        s_start);

    a(s_entitystart,";",    0,0,              c_eat,            s_entitydone);
    a(s_entitystart,NULL,    0,255,           c_eat+c_store,    s_entity);
    a(s_entity,        ";",    0,0,           c_eat,            s_entitydone);
    a(s_entity,NULL,        0,255,            c_eat+c_store,    s_entity);
    r(s_entitydone,    t_entity,                                s_start);

    a(s_word,        c_ws,    0,0,            c_nil,            s_worddone);
    a(s_word,        "<&",    0,0,            c_nil,            s_worddone);
    a(s_word,        NULL,    0,255,          c_eat+c_store,    s_word);
    r(s_worddone,    t_word,                                    s_start);

    a(s_tagnamestart,c_ws,    0,0,            c_nil,            s_tagnamedone);
    a(s_tagnamestart,    "/>",    0,0,        c_nil,            s_tagnamedone);
    a(s_tagnamestart,NULL,    0,255,          c_eat+c_store,    s_tagname);
    a(s_tagname,    c_ws,    0,0,             c_nil,            s_tagnamedone);
    a(s_tagname,    "/>",    0,0,             c_nil,            s_tagnamedone);
    a(s_tagname,    NULL,    0,255,           c_eat+c_store,    s_tagname);
    r(s_tagnamedone,    t_tag,                                s_intag);

    a(s_intag,        c_ws,    0,0,           c_eat,            s_intag);
    a(s_intag,        ">",    0,0,            c_eat,            s_tagend);
    a(s_intag,        "/",    0,0,            c_eat,            s_empty);
    a(s_intag,        NULL,    0,255,         c_eat+c_store,    s_attstart);

    a(s_attstart,    c_ws,    0,0,            c_eat,            s_attdone);
    a(s_attstart,    "=/>",    0,0,           c_nil,            s_attdone);
    a(s_attstart,    NULL,    0,255,          c_eat+c_store,    s_attname);
    a(s_attname,    "=/>",    0,0,            c_nil,            s_attdone);
    a(s_attname,    c_ws,    0,0,             c_eat,            s_attdone);
    a(s_attname,    NULL,    0,255,           c_eat+c_store,    s_attname);
    r(s_attdone,    t_att,                                    s_att);
    a(s_att,        c_ws,    0,0,             c_eat,            s_att);
    a(s_att,        "=",    0,0,              c_eat,            s_atteq);
    a(s_att,        NULL,    0,255,           c_eat,            s_intag);
    a(s_atteq,        "'",    0,0,            c_eat,            s_eqapos);
    a(s_atteq,        "\"",    0,0,           c_eat,            s_eqquot);
    a(s_atteq,        c_ws,    0,0,           c_eat,            s_atteq);
    a(s_atteq,        NULL,    0,255,         c_nil,            s_eqval);

    a(s_eqapos,        "'",    0,0,           c_eat,            s_eqaposvaldone);
    a(s_eqapos,        NULL,    0,255,        c_eat+c_store,    s_eqaposval);
    a(s_eqaposval,        "'",    0,0,        c_eat,            s_eqaposvaldone);
    a(s_eqaposval,        NULL,    0,255,     c_eat+c_store,    s_eqaposval);
    r(s_eqaposvaldone,    t_val,                                    s_intag);

    a(s_eqquot,        "\"",    0,0,          c_eat,            s_eqquotvaldone);
    a(s_eqquot,        NULL,    0,255,        c_eat+c_store,    s_eqquotval);
    a(s_eqquotval,     "\"",    0,0,          c_eat,            s_eqquotvaldone);
    a(s_eqquotval,     NULL,    0,255,        c_eat+c_store,    s_eqquotval);
    r(s_eqquotvaldone, t_val,                                    s_intag);

    a(s_eqval,        c_ws,    0,0,          c_nil,            s_eqvaldone);
    a(s_eqval,        "/>",    0,0,          c_nil,            s_eqvaldone);
    a(s_eqval,        NULL,    0,255,        c_eat+c_store,    s_eqval);

    r(s_eqvaldone,    t_val,                 s_intag);

    r(s_tagend,       t_endtag,              s_start);

    r(s_empty,          t_endtag,                               s_inempty);
    a(s_inempty,        ">",0,0,             c_eat,            s_emptyend);
    a(s_inempty,        NULL,0,255,          c_eat,            s_inempty);
    r(s_emptyend,    t_closeemptytag,        s_start);

    a(s_prolog,        "?",0,0,              c_eat,            s_prologq);
    a(s_prolog,        NULL,0,255,           c_eat+c_store,    s_prolog);

    a(s_prologq,    ">",0,0,                 c_eat,            s_prologdone);
    a(s_prologq,    NULL,0,255,              c_eat+c_store,    s_prolog);
    r(s_prologdone,    t_prolog,             s_start);

    a(s_tagexcl,    "-",0,0,                 c_eat,            s_commentdash1);
    a(s_tagexcl,    "D",0,0,                 c_nil,            s_dtd);
    a(s_tagexcl,    NULL,0,255,              c_eat,            s_start);

    a(s_commentdash1,    "-",0,0,            c_eat,            s_commentdash2);
    a(s_commentdash1,    NULL,0,255,         c_eat,            s_error);

    a(s_commentdash2,    "-",0,0,            c_eat,            s_commentenddash1);
    a(s_commentdash2,    NULL,0,255,         c_eat+c_store,    s_incomment);

    a(s_incomment,       "-",0,0,            c_eat,            s_commentenddash1);
    a(s_incomment,       NULL,0,255,         c_eat+c_store,    s_incomment);

    a(s_commentenddash1, "-",0,0,            c_eat,            s_commentenddash2);
    a(s_commentenddash1, NULL,0,255,         c_eat+c_store,    s_incomment);

    a(s_commentenddash2, ">",0,0,            c_eat,            s_commentdone);
    a(s_commentenddash2, NULL,0,255,         c_eat+c_store,    s_incomment);

    r(s_commentdone,     t_comment,          s_start);

}

/* *INDENT-ON* */

static int
is_oneof (char c, char *chars)
{
  while (*chars)
    {
      if (c == *chars)
        return 1;
      chars++;
    }
  return 0;
}

static int
nextchar (MrgXml *t)
{
  int       ret;

  if (t->file_in)
    {
      if (t->inbufpos >= t->inbuflen)
        {
          t->inbuflen = fread (t->inbuf, 1, CTX_XML_INBUF_SIZE, t->file_in);
          t->inbufpos = 0;
          if (!t->inbuflen)
            return -1;
        }

      ret = (int) t->inbuf[t->inbufpos++];

      if (ret == '\n')
        t->line_no++;
    }
  else
    {
      if (t->inbufpos >= t->inbuflen)
        {
          return -1;
        }
      ret = (int) t->inbuf[t->inbufpos++];
      if (ret == '\n')
        t->line_no++;
    }
  return ret;
}

int
xmltok_get (MrgXml *t, char **data, int *pos)
{
  state_entry *s;

  init_statetable ();
  mrg_string_clear (t->curdata);
  while (1)
    {
      if (!t->c_held)
        {
          t->c = nextchar (t);
          if (t->c == -1)
          {
            if (pos)*pos = t->inbufpos;
            return t_eof;
          }
          t->c_held = 1;
        }
      if (t->state == s_dtd)
        {     /* FIXME: should make better code for skipping DTD */

          /*            int angle = 0; */
          int       squote = 0;
          int       dquote = 0;
          int       abracket = 1;

          /*            int sbracket = 0; */

          mrg_string_append_byte (t->curdata, t->c);

          while (abracket)
            {
              switch (t->c = nextchar (t))
                {
                case -1:
                  return t_eof;
                case '<':
                  if ((!squote) && (!dquote))
                    abracket++;
                  mrg_string_append_byte (t->curdata, t->c);
                  break;
                case '>':
                  if ((!squote) && (!dquote))
                    abracket--;
                  if (abracket)
                    mrg_string_append_byte (t->curdata, t->c);
                  break;
                case '"':
                case '\'':
                case '[':
                case ']':
                default:
                  mrg_string_append_byte (t->curdata, t->c);
                  break;
                }
            }
          t->c_held = 0;
          t->state = s_start;

          if (pos)*pos = t->inbufpos;
          return t_dtd;
        }
      s = &state_table[t->state][0];
      while (s->state)
        {
          if (s->return_type != t_none)
            {
              *data = (char *) mrg_string_get (t->curdata);
              t->state = s->next_state;
              if (s->return_type == t_tag)
                mrg_string_set (t->curtag, mrg_string_get (t->curdata));
              if (s->return_type == t_endtag)
                *data = (char *) mrg_string_get (t->curtag);
              if (s->return_type == t_closeemptytag)
                *data = (char *) mrg_string_get (t->curtag);
              if (pos)
                *pos = t->inbufpos;
              return s->return_type;
            }
          if ((s->chars && is_oneof (t->c, s->chars))
              || ((s->r_start + s->r_end)
                  && (t->c >= s->r_start && t->c <= s->r_end)))
            {
              if (s->charhandling & c_store)
                {
                  mrg_string_append_byte (t->curdata, t->c);
                }
              if (s->charhandling & c_eat)
                {
                  t->c_held = 0;
                }
              t->state = s->next_state;
              break;
            }
          s++;
        }
    }
  if (pos)
    *pos = t->inbufpos;
  return t_eof;
}

MrgXml *
xmltok_new (FILE * file_in)
{
  MrgXml *ret;

  ret = calloc (1, sizeof (MrgXml));
  ret->file_in = file_in;
  ret->state = s_start;
  ret->curtag = mrg_string_new ("");
  ret->curdata = mrg_string_new ("");
  ret->inbuf = calloc (1, CTX_XML_INBUF_SIZE);
  return ret;
}

MrgXml *
xmltok_buf_new (char *membuf)
{
  MrgXml *ret;

  ret = calloc (1, sizeof (MrgXml));
  ret->file_in = NULL;
  ret->state = s_start;
  ret->curtag = mrg_string_new ("");
  ret->curdata = mrg_string_new ("");
  ret->inbuf = (void*)membuf;
  ret->inbuflen = strlen (membuf);
  ret->inbufpos = 0;
  return ret;
}

void
xmltok_free (MrgXml *t)
{
  mrg_string_free (t->curtag, 1);
  mrg_string_free (t->curdata, 1);

  if (t->file_in)
    {
      /*        fclose (t->file_in); */
      free (t->inbuf);
    }
  free (t);
}

char     *empty_tags[] = {
  "img", "IMG", "br", "BR", "hr", "HR", "META", "meta", "link", "LINK",
  NULL
};

char     *endomission_tags[] = {
  "li", "LI", "p", "P", "td", "TD", "tr", "TR", NULL
};

int
xmltok_lineno (MrgXml *t)
{
  return t->line_no;
}


void ctx_events_clear (Ctx *ctx)
{
  if (ctx_events_frozen (ctx))
    return;

  ctx_events_clear_items (ctx);
  //if (mrg->backend->mrg_clear)
  //  mrg->backend->mrg_clear (mrg);

  ctx_clear_bindings (ctx);
}

void mrg_clear (Mrg *mrg)
{
  ctx_events_clear (mrg->ctx);
  _mrg_clear_text_closures (mrg);
}



static void
_mrg_draw_background_increment (Mrg *mrg, void *data, int last);

MrgGeoCache *_mrg_get_cache (MrgHtml *htmlctx, void *id_ptr)
{
  CtxList *l;
  for (l = htmlctx->geo_cache; l; l = l->next)
  {
    MrgGeoCache *item = l->data;
    if (item->id_ptr == id_ptr)
    {
      item->gen++;
      return item;
    }
  }
  {
    MrgGeoCache *item = calloc (sizeof (MrgGeoCache), 1);
    item->id_ptr = id_ptr;
    ctx_list_prepend_full (&htmlctx->geo_cache, item, (void*)free, NULL);
    return item;
  }
  return NULL;
}

void mrg_set_edge_right (Mrg *mrg, float val);
void mrg_set_edge_left (Mrg *mrg, float val);
void mrg_set_edge_top (Mrg *mrg, float val);
void mrg_set_edge_bottom (Mrg *mrg, float val);
float mrg_edge_right (Mrg *mrg);
float mrg_edge_left (Mrg *mrg);
float mrg_y (Mrg *mrg);
float mrg_x (Mrg *mrg);

float mrg_em (Mrg *mrg);
void mrg_set_xy (Mrg *mrg, float x, float y);




static float _mrg_dynamic_edge_right2 (Mrg *mrg, MrgHtmlState *state)
{
  float ret = mrg_edge_right (mrg);
  float y = mrg_y (mrg);
  float em = mrg_em (mrg);
  int i;

  if (state->floats)
    for (i = 0; i < state->floats; i++)
    {
      MrgFloatData *f = &state->float_data[i];
      if (f->type == MRG_FLOAT_RIGHT &&
          y >= f->y  &&
          y - em < f->y + f->height &&

          f->x < ret)
          ret = f->x;
    }
  return ret;
}

static float _mrg_dynamic_edge_left2 (Mrg *mrg, MrgHtmlState *state)
{
  float ret = mrg_edge_left (mrg);
  float y = mrg_y (mrg);
  float em = mrg_em (mrg);
  int i;

  if (state->floats)
    for (i = 0; i < state->floats; i++)
    {
      MrgFloatData *f = &state->float_data[i];
      if (f->type == MRG_FLOAT_LEFT &&
          y >= f->y &&
          y - em < f->y + f->height &&
          f->x + f->width > ret)
          ret = f->x + f->width;
    }
  return ret;
}

static float _mrg_parent_dynamic_edge_left (MrgHtml *ctx)
{
  MrgHtmlState *state = ctx->state;
  if (ctx->state_no)
    state = &ctx->states[ctx->state_no-1];
  return _mrg_dynamic_edge_left2 (ctx->mrg, state);
}

static float _mrg_parent_dynamic_edge_right (MrgHtml *ctx)
{
  MrgHtmlState *state = ctx->state;
  if (ctx->state_no)
    state = &ctx->states[ctx->state_no-1];
  return _mrg_dynamic_edge_right2 (ctx->mrg, state);
}

static float _mrg_dynamic_edge_left (Mrg *mrg)
{
  if (mrg->state->wrap_edge_left)
    return mrg->state->wrap_edge_left (mrg, mrg->state->wrap_edge_data);
  return mrg->state->edge_left;
}
int  mrg_width (Mrg *mrg)
{
  if (!mrg) return 640;
  return ctx_events_width (mrg->ctx) / mrg->ddpx;
}

int  mrg_height (Mrg *mrg)
{
  if (!mrg) return 480;
  return ctx_events_height (mrg->ctx) / mrg->ddpx;
}

static float _mrg_dynamic_edge_right (Mrg *mrg)
{
  if (mrg->state->wrap_edge_right)
    return mrg->state->wrap_edge_right (mrg, mrg->state->wrap_edge_data);
  return mrg->state->edge_right;
}

static float wrap_edge_left (Mrg *mrg, void *data)
{
  MrgHtml *ctx = data;
  MrgHtmlState *state = ctx->state;
  return _mrg_dynamic_edge_left2 (mrg, state);
}

static float wrap_edge_right (Mrg *mrg, void *data)
{
  MrgHtml *ctx = data;
  MrgHtmlState *state = ctx->state;
  return _mrg_dynamic_edge_right2 (mrg, state);
}

static void clear_left (MrgHtml *ctx)
{
  Mrg *mrg = ctx->mrg;
  float y = mrg_y (mrg);
  int i;

  if (ctx->state->floats)
  {
    for (i = 0; i < ctx->state->floats; i++)
      {
        MrgFloatData *f = &ctx->state->float_data[i];
        {
          if (f->type == MRG_FLOAT_LEFT)
          {
            if (f->y + f->height > y)
              y = f->y + f->height;
          }
        }
      }
  }
  mrg_set_xy (mrg, mrg_x (mrg), y);
}

static void clear_right (MrgHtml *ctx)
{
  Mrg *mrg = ctx->mrg;
  float y = mrg_y (mrg);
  int i;

  if (ctx->state->floats)
  {
    for (i = 0; i < ctx->state->floats; i++)
      {
        MrgFloatData *f = &ctx->state->float_data[i];
        {
          if (f->type == MRG_FLOAT_RIGHT)
          {
            if (f->y + f->height > y)
              y = f->y + f->height;
          }
        }
      }
  }
  mrg_set_xy (mrg, mrg_x (mrg), y);
}

/**
 * mrg_style:
 * @mrg the mrg-context
 *
 * Returns the currently 
 *
 */
MrgStyle *mrg_style (Mrg *mrg)
{
  return &mrg->state->style;
}

static void clear_both (MrgHtml *ctx)
{
  Mrg *mrg = ctx->mrg;
#if 0
  clear_left (mrg);
  clear_right (mrg);
#else
  float y = mrg_y (mrg);
  int i;

  if (ctx->state->floats)
  {
    for (i = 0; i < ctx->state->floats; i++)
      {
        MrgFloatData *f = &ctx->state->float_data[i];
        {
          if (f->y + f->height > y)
            y = f->y + f->height;
        }
      }
  }
  y += mrg_em (mrg) * mrg_style(mrg)->line_height;
  mrg_set_xy (mrg, mrg_x (mrg), y);
  //_mrg_draw_background_increment (mrg, &mrg->html, 0);
#endif
}

static float mrg_edge_bottom  (Mrg *mrg)
{
  return mrg->state->edge_bottom;
}

static float mrg_edge_top  (Mrg *mrg)
{
  return mrg->state->edge_top;
}

float mrg_edge_left  (Mrg *mrg)
{
  return mrg->state->edge_left;
}

float mrg_edge_right (Mrg *mrg)
{
  return mrg->state->edge_right;
}

float _mrg_dynamic_edge_right (Mrg *mrg);
float _mrg_dynamic_edge_left (Mrg *mrg);

void  mrg_set_edge_top (Mrg *mrg, float val)
{
  mrg->state->edge_top = val;
  mrg_set_xy (mrg, _mrg_dynamic_edge_left (mrg) + ctx_get_float (mrg_cr(mrg), CTX_text_indent)
      , mrg->state->edge_top + mrg_em (mrg));
}

void  mrg_set_edge_left (Mrg *mrg, float val)
{
  mrg->state->edge_left = val;
}

void  mrg_set_edge_bottom (Mrg *mrg, float val)
{
  mrg->state->edge_bottom = val;
}

void  mrg_set_edge_right (Mrg *mrg, float val)
{
  mrg->state->edge_right = val;
}

float mrg_rem (Mrg *mrg)
{
  return mrg->rem;
}


void mrg_start            (Mrg *mrg, const char *class_name, void *id_ptr);
void mrg_start_with_style (Mrg        *mrg,
                           const char *style_id,
                           void       *id_ptr,
                           const char *style);
void mrg_start_with_stylef (Mrg *mrg, const char *style_id, void *id_ptr,
                            const char *format, ...);
static void mrg_parse_style_id (Mrg          *mrg,
                                const char   *style_id,
                                MrgStyleNode *node)
{
  const char *p;
  char temp[128] = "";
  int  temp_l = 0;
  if (!style_id)
  {
    return; // XXX: why does this happen?
  }

  memset (node, 0, sizeof (MrgStyleNode));

  for (p = style_id; ; p++)
  {
    switch (*p)
    {
      case '.':
      case ':':
      case '#':
      case 0:
        if (temp_l)
        {
          switch (temp[0])
          {
            case '.':
              {
                int i = 0;
                for (i = 0; node->classes_hash[i]; i++);
                node->classes_hash[i] = ctx_strhash (&temp[1], 0);
              }
              break;
            case ':':
              {
                int i = 0;
                for (i = 0; node->pseudo[i]; i++);
                node->pseudo[i] = mrg_intern_string (&temp[1]);
                for (i = 0; node->pseudo_hash[i]; i++);
                node->pseudo_hash[i] = ctx_strhash (&temp[1], 0);
              }
              break;
            case '#':
              node->id = mrg_intern_string (&temp[1]);
              node->id_hash = ctx_strhash (&temp[1], 0);
              break;
            default:
              node->element_hash = ctx_strhash (temp, 0);
              break;
          }
          temp_l = 0;
        }
        if (*p == 0)
          return;
        temp[temp_l++] = *p;  // XXX: added to make reported fallthrough
        temp[temp_l]=0;       //      not be reported - butexplicit
        break;
      default:
        temp[temp_l++] = *p;
        temp[temp_l]=0;
    }
  }
}

void _mrg_init_style (Mrg *mrg)
{
  MrgStyle *s = mrg_style (mrg);

  /* things not set here, are inherited from the parent style context,
   * more properly would be to rig up a fresh context, and copy inherited
   * values over, that would permit specifying inherit on any propery.
   */

  s->text_decoration= 0;
  s->display  = MRG_DISPLAY_INLINE;
  s->float_   = MRG_FLOAT_NONE;
  s->clear    = MRG_CLEAR_NONE;
  s->overflow = MRG_OVERFLOW_VISIBLE;
  s->position = MRG_POSITION_STATIC;
#if 0
  s->border_top_color.alpha = 0;
  s->border_left_color.alpha = 0;
  s->border_right_color.alpha = 0;
  s->border_bottom_color.alpha = 0;
#endif

  SET_PROP(border_top_width, 0);
  SET_PROP(border_left_width, 0);
  SET_PROP(border_right_width, 0);
  SET_PROP(border_bottom_width, 0);
  SET_PROP(margin_top, 0);
  SET_PROP(margin_left, 0);
  SET_PROP(margin_right, 0);
  SET_PROP(margin_bottom, 0);
  SET_PROP(padding_top, 0);
  SET_PROP(padding_left, 0);
  SET_PROP(padding_right, 0);
  SET_PROP(padding_bottom, 0);
  SET_PROP(top, 0);
  SET_PROP(left, 0);
  SET_PROP(right, 0);
  SET_PROP(bottom, 0);

  ctx_set_float (mrg->ctx, CTX_stroke_width, 0.2);
#if 0
  s->stroke_color.red = 1;
  s->stroke_color.green = 0;
  s->stroke_color.blue = 1;
  s->stroke_color.alpha = 1;
  s->fill_color.red = 1;
  s->fill_color.green = 1;
  s->fill_color.blue = 0;
  s->fill_color.alpha = 1;
#endif

  //ctx_color_set_rgba (&mrg->ctx->state, &s->fill_color, 1, 0, 1, 0);
  //ctx_color_set_rgba (&mrg->ctx->state, &s->stroke_color, 1, 1, 0, 0);
  //ctx_color_set_rgba (&mrg->ctx->state, &s->background_color, 1, 1, 1, 0);
  /* this shouldn't be inherited? */
  //s->background_color.red = 1;
  //s->background_color.green = 1;
  //s->background_color.blue = 1;
  //s->background_color.alpha = 0;
}


/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

const char * html_css =
"html,address,\n"
"blockquote,\n"
"body,dd, div,\n"
"dl,dt,fieldset, form,\n"
"frame,frameset,\n"
"h1,h2,h3, h4,\n"
"h5,h6,noframes,\n"
"ol,p,ul,center,\n"
"dir,hr,menu,pre{display:block;unicode-bidi:embed}\n"
"h1,h2,h3,h4,h5{page-break-after:avoid}\n"
"li{display:list-item}\n"
"head{display:none}\n"
"table{display:table}\n"
"tr{display:table-row}\n"
"thead{display:table-header-group }\n"
"tbody{display:table-row-group }\n"
"tfoot{display:table-footer-group }\n"
"col{display:table-column}\n"
"img{display:inline-block}\n"
"colgroup{display:table-column-group}\n"
"td,th{display:table-cell}\n"
"caption{display:table-caption}\n"
"th{font-weight:bolder;text-align:center}\n"
"caption{text-align:center}\n"
"body{margin:0.5em}\n"
"h1{font-size:2em;margin:.67em 0}\n"
"h2{font-size:1.5em;margin:.75em 0}\n"
"h3{font-size:1.17em;margin:.83em 0}\n"
"h4,p,"
"blockquote,ul,"
"fieldset,form,"
"ol,dl,dir,"
"menu{margin:1.12em 0}\n"
"h5{font-size:.83em;margin: 1.5em 0}\n"
"h6{font-size:.75em;margin: 1.67em 0}\n"
"h1,h2,h3,h4,\n"
"h5,h6,b,\n"
"strong{font-weight:bolder}\n"
"blockquote{margin-left:4em;margin-right:4em}\n"
"i,cite,em,\n"
"var,address{font-style:italic}\n"
"pre,tt,code,\n"
"kbd,samp{font-family:monospace}\n"
"pre{white-space:pre}\n"
"button,textarea,\n"
"input,select{display:inline-block}\n"
"big{font-size:1.17em}\n"
"small,sub,sup{font-size:.83em}\n"
"sub{vertical-align:sub}\n"
"sup{vertical-align:super}\n"
"table{border-spacing:2px;}\n"
"thead,tbody,\n"
"tfoot{vertical-align:middle}\n"
"td,th,tr{vertical-align:inherit}\n"
"s,strike,del{text-decoration:line-through}\n"
"hr{border:1px inset black;margin-bottom: 0.5em;margin-top:0.5em;}\n"
"ol,ul,dir,"
"menu,dd{padding-left:2.5em}\n"
"ol{list-style-type:decimal}\n"
"ol ul,ul ol,"
"ul ul,ol ol{margin-top: 0;margin-bottom: 0}\n"
"u,ins{text-decoration:underline}\n"
//"br:before{content:\"\\A\";white-space:pre-line}\n"
"center{text-align:center}\n"
":link,:visited{text-decoration:underline}\n"
":focus{outline:thin dotted invert}\n"
".cursor{color:white;background: black;} \n"
"br{display:block;}\n"
"document{color:black;font-weight:normal;background-color:white;}\n"
"body{background-color:transparent;}\n"
"a{color:blue;text-decoration: underline;}\n"
"a:hover{background:black;color:white;text-decoration:underline; }\n"
"style,script{display:none;}\n"
"hr{margin-top:16px;font-size:1px;}\n"  /* hack that works in one way, but shrinks top margin too much */
;


typedef struct StyleEntry {
  char        *selector;
  MrgStyleNode parsed[MRG_MAX_SELECTOR_LENGTH];
  int          sel_len;
  char        *css;
  int          specificity;
} StyleEntry;

static void free_entry (StyleEntry *entry)
{
  free (entry->selector);
  free (entry->css);
  free (entry);
}

static int compute_specificity (const char *selector, int priority)
{
  const char *p;
  int n_id = 0;
  int n_class = 0;
  int n_tag = 0;
  int n_pseudo = 0;
  int in_word = 0;

  for (p = selector; *p; p++)
  {
    switch (*p)
    {
      case ' ':
        in_word = 0;
        break;
      case ':':
        in_word = 1;
        n_pseudo++;
        break;
      case '.':
        in_word = 1;
        n_class++;
        break;
      case '#':
        in_word = 1;
        n_id++;
        break;
      default:
        if (!in_word)
        {
          in_word = 1;
          n_tag++;
        }
    }
  }
  return priority * 100000 + n_pseudo * 10000 + n_id * 1000 + n_class * 100 + n_tag * 10;
}

static void mrg_parse_selector (Mrg *mrg, const char *selector, StyleEntry *entry)
{
  const char *p = selector;
  char section[256];
  int sec_l = 0;

  char type = ' ';

  for (p = selector; ; p++)
  {
    switch (*p)
    {
      case '.': case ':': case '#': case ' ': case 0:
        if (sec_l)
        {
          switch (type)
          {
            case ' ':
              entry->parsed[entry->sel_len].element_hash = ctx_strhash (section, 0);
              break;
            case '#':
              entry->parsed[entry->sel_len].id = mrg_intern_string (section);
              entry->parsed[entry->sel_len].id_hash = ctx_strhash (section, 0);
              break;
            case '.':
              {
                int i = 0;
                for (i = 0; entry->parsed[entry->sel_len].classes_hash[i]; i++);
                entry->parsed[entry->sel_len].classes_hash[i] = ctx_strhash (section, 0);
              }
              break;
            case ':':
              {
                int i = 0;
                for (i = 0; entry->parsed[entry->sel_len].pseudo[i]; i++);
                entry->parsed[entry->sel_len].pseudo[i] = mrg_intern_string (section);
                for (i = 0; entry->parsed[entry->sel_len].pseudo_hash[i]; i++);
                entry->parsed[entry->sel_len].pseudo_hash[i] = ctx_strhash (section, 0);
              }
              break;
          }
        if (*p == ' ' || *p == 0)
        entry->sel_len ++;
        }
        if (*p == 0)
        {
          return;
        }
        section[(sec_l=0)] = 0;
        type = *p;
        break;
      default:
        section[sec_l++] = *p;
        section[sec_l] = 0;
        break;
    }
  }
}

static void mrg_stylesheet_add_selector (Mrg *mrg, const char *selector, const char *css, int priority)
{
  StyleEntry *entry = calloc (sizeof (StyleEntry), 1);
  entry->selector = strdup (selector);
  entry->css = strdup (css);
  entry->specificity = compute_specificity (selector, priority);
  mrg_parse_selector (mrg, selector, entry);
  ctx_list_prepend_full (&mrg->stylesheet, entry, (void*)free_entry, NULL);
}


#define MAKE_ERROR \
 if (error)\
 {\
   char errbuf[128];\
   sprintf (errbuf, "%i unexpected %c at %i'  %c%c%c", __LINE__, *p, (int)(p-css),\
     p[0], p[1], p[2]);\
   *error = strdup (errbuf);\
 }


typedef struct _MrgCssParseState MrgCssParseState;

struct _MrgCssParseState {
  int   state;
  char  rule[MRG_MAX_CSS_RULES][MRG_MAX_CSS_RULELEN];
  int   rule_no ;
  int   rule_l[MRG_MAX_CSS_RULES];
  char  val[MRG_MAX_CSS_STRINGLEN];
  int   val_l;
};

/* XXX: this funciton has no buffer bounds checking */
/* doesnt balance {} () [] and quotes */

enum
{
  NEUTRAL = 0,
  NEUTRAL_COMMENT,
  IN_RULE,
  RULE_COMMENT,
  IN_ARULE,
  ARULE_COMMENT,
  IN_IMPORT,
  IMPORT_COMMENT,
  EXPECT_COLON,
  COLON_COMMENT,
  EXPECT_VAL,
  EXPECT_VAL_COMMENT,
  IN_VAL,
  VAL_COMMENT,
};

void mrg_set_mrg_get_contents (Mrg *mrg,
  int (*mrg_get_contents) (const char  *referer,
                      const char  *input_uri,
                      char       **contents,
                      long        *length,
                      void        *get_contents_data),
  void *get_contents_data)

{
  mrg->mrg_get_contents = mrg_get_contents;
  mrg->get_contents_data = get_contents_data;
}


int
mrg_get_contents (Mrg         *mrg,
                  const char  *referer,
                  const char  *input_uri,
                  char       **contents,
                  long        *length)
{
  if (mrg->mrg_get_contents)
  {
    int ret;
    ret = mrg->mrg_get_contents (referer, input_uri, contents, length, mrg->get_contents_data);
    return ret;
  }
  else
  {
    *contents = NULL;
    *length = 0;
    return -1;
  }
}

static void _mrg_stylesheet_add (MrgCssParseState *ps, Mrg *mrg, const char *css, const char *uri_base,
                         int priority, char **error)
{
  const char *p;
  if (!ps)
    ps = mrg->css_parse_state = calloc (sizeof (MrgCssParseState), 1);

  if (!css)
    return;

  for (p = css; *p; p++)
  {
    switch (ps->state)
    {
      case NEUTRAL_COMMENT:
      case RULE_COMMENT:
      case ARULE_COMMENT:
      case IMPORT_COMMENT:
      case VAL_COMMENT:
      case EXPECT_VAL_COMMENT:
      case COLON_COMMENT:      if (p[0] == '*' && p[1] == '/') { p++; ps->state--; } break;
      case NEUTRAL:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = NEUTRAL_COMMENT; } break;
          case ' ':
          case '\n':
          case '\t':
          case ';':
            break;
          case '{':
          case '}':
            MAKE_ERROR;
            return;
          case '@':
            ps->state = IN_ARULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
          default:
            ps->state = IN_RULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
        }
        break;
      case IN_ARULE:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = ARULE_COMMENT; } break;
          case '{':
            ps->state = IN_VAL; // should be AVAL for media sections...
            break;
          case '\n':
          case '\t':
          case ' ':
            if (!strcmp (ps->rule[0], "import"))
            {
              MAKE_ERROR;
            }
            else
            {
              ps->state = IN_IMPORT;
            }
            break;
          case ';':
          case '}':
            MAKE_ERROR;
            return;
          default:
            ps->state = IN_ARULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
        }
        break;
      case IN_IMPORT:
        switch (*p)
        {
          int no;
          case '/': if (p[1] == '*') { p++; ps->state = IMPORT_COMMENT; } break;
          case ';':
            while (ps->val_l && (
                ps->val[ps->val_l-1] == ' ' ||
                ps->val[ps->val_l-1] == '\n' ||
                ps->val[ps->val_l-1] == '\t'))
              ps->val_l--;
            while (ps->val_l && ps->val[ps->val_l-1] != ')')
              ps->val_l--;
            if (ps->val[ps->val_l-1] == ')')
              ps->val_l--;
            if (ps->val[ps->val_l-1] == '"')
              ps->val_l--;
            ps->val[ps->val_l]=0;

            if(mrg->mrg_get_contents){
              char *contents;
              long length;
              char *uri = ps->val;

              /* really ugly string trimming to get to import uri.. */
              while (uri[0]==' ') uri++;
              if (!memcmp (uri, "url", 3)) uri+=3;
              if (uri[0]=='(') uri++;
              if (uri[0]=='"') uri++;
      
              /* XXX: should parse out the media part, and check if we should
               * really include this file
               */

              mrg_get_contents (mrg, uri_base, uri, &contents, &length);
              if (contents)
              {
                MrgCssParseState child_parser = {0,};
                _mrg_stylesheet_add (&child_parser, mrg, contents, uri, priority, error);
                free (contents);
              }
            }

            for (no = 0; no < ps->rule_no+1; no ++)
              ps->rule[no][ps->rule_l[no]=0] = 0;
            ps->val_l = 0;
            ps->val[0] = 0;
            ps->rule_no = 0;
            ps->state = NEUTRAL;
            break;
          default:
            ps->state = IN_IMPORT;
            ps->val[ps->val_l++] = *p;
            ps->val[ps->val_l] = 0;
            break;

        }
        break;
      case IN_RULE:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = RULE_COMMENT; } break;
          case '{':
            ps->state = IN_VAL;
            break;
          case '\n':
          case '\t':
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = ' ';
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
          case ',':
            ps->state = NEUTRAL;
            ps->rule_no++;
            break;
          case ';':
          case '}':
            MAKE_ERROR;
            return;
          default:
            ps->state = IN_RULE;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]++] = *p;
            ps->rule[ps->rule_no][ps->rule_l[ps->rule_no]] = 0;
            break;
        }
        break;
      case EXPECT_COLON:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = COLON_COMMENT; } break;
          case ' ':
          case '\n':
          case '\t':
            break;
          case ':':
            ps->state = EXPECT_VAL;
            break;
          default:
            MAKE_ERROR;
            return;
        }
        break;
      case EXPECT_VAL:
        switch (*p)
        {
          case '/': if (p[1] == '*') { p++; ps->state = EXPECT_VAL_COMMENT; } break;
          case ' ':
          case '\n':
          case '\t':
          case ';':
            break;
          case '{':
            ps->state = IN_VAL;
            break;
          default:
            MAKE_ERROR;
            return;
        }
        break;
      case IN_VAL:
        switch (*p)
        {
          int no;

          case '/': if (p[1] == '*') { p++; ps->state = VAL_COMMENT; } break;
          case '}':
            while (ps->val_l && (
                ps->val[ps->val_l-1] == ' ' ||
                ps->val[ps->val_l-1] == '\n' ||
                ps->val[ps->val_l-1] == '\t'))
              ps->val_l--;
            ps->val[ps->val_l]=0;

            for (no = 0; no < ps->rule_no+1; no ++)
            {
              while (ps->rule_l[no] && (
                  ps->rule[no][ps->rule_l[no]-1] == ' ' ||
                  ps->rule[no][ps->rule_l[no]-1] == '\n' ||
                  ps->rule[no][ps->rule_l[no]-1] == '\t'))
                ps->rule_l[no]--;
              ps->rule[no][ps->rule_l[no]]=0;

              mrg_stylesheet_add_selector (mrg, ps->rule[no], ps->val, priority);
              ps->rule[no][ps->rule_l[no]=0] = 0;
            }

            ps->val_l = 0;
            ps->val[0] = 0;
            ps->rule_no = 0;
            ps->state = NEUTRAL;
            break;
          default:
            ps->state = IN_VAL;
            ps->val[ps->val_l++] = *p;
            ps->val[ps->val_l] = 0;
            break;

        }
        break;
    }
  }
}

void mrg_stylesheet_add (Mrg *mrg, const char *css, const char *uri_base,
                         int priority, char **error)
{
  MrgCssParseState *ps = mrg->css_parse_state;
  _mrg_stylesheet_add (ps, mrg, css, uri_base, priority, error);
}
#define MRG_STYLE_INTERNAL 10
#define MRG_STYLE_GLOBAL   15
#define MRG_STYLE_XML      20
#define MRG_STYLE_APP      20
#define MRG_STYLE_INLINE   25
#define MRG_STYLE_CODE     30

void mrg_css_default (Mrg *mrg)
{
  char *error = NULL;
  mrg_stylesheet_add (mrg, html_css, NULL, MRG_STYLE_INTERNAL, &error);
  if (error)
  {
    fprintf (stderr, "Mrg css parsing error: %s\n", error);
  }

  mrg_stylesheet_add (mrg,
"bold{font-weight:bold;}"
"dim*,dim{opacity:0.5;}"
"underline*,underline{text-decoration:underline;}"
"reverse*,selected*,reverse,selected{text-decoration:reverse;}"
"unhandled{color:cyan;}"
"binding:key{background-color:white;color:black;}"
"binding:label{color:cyan;}"
      
      ,NULL, MRG_STYLE_INTERNAL, &error);

  if (error)
  {
    fprintf (stderr, "mrg css parsing error: %s\n", error);
  }
}

void mrg_stylesheet_clear (Mrg *mrg)
{
  if (mrg->stylesheet)
    ctx_list_free (&mrg->stylesheet);
  mrg_css_default (mrg);
}

typedef struct StyleMatch
{
  StyleEntry *entry;
  int score;
} StyleMatch;

static int compare_matches (const void *a, const void *b, void *d)
{
  const StyleMatch *ma = a;
  const StyleMatch *mb = b;
  return ma->score - mb->score;
}

static inline int _mrg_nth_match (const char *selector, int child_no)
{
  const char *tmp = selector + 10;
  int a = 0;
  int b = 0;

  if (!strcmp (tmp, "odd)"))
  {
    a = 2; b = 1;
  }
  else if (!strcmp (tmp, "even)"))
  {
    a = 2; b = 0;
  }
  else
  {
    if (ctx_strchr (tmp, 'n'))
    {
      a = atoi (tmp);
      b = atoi (ctx_strchr (tmp, 'n')+1);
    }
    else
    {
      b = atoi (tmp);
    }
  }

  if (!a)
    return child_no == b;
  else
    if (child_no == b ||
       ((child_no - b > 0) == (a>0)))
      return !((child_no - b) % a);

  return 0;
}

int _mrg_child_no (Mrg *mrg)
{
  return mrg->states[mrg->state_no-1].children;
}

static inline int match_nodes (Mrg *mrg, MrgStyleNode *sel_node, MrgStyleNode *subject)
{
  int j, k;

  if (sel_node->element_hash &&
      sel_node->element_hash != subject->element_hash)
    return 0;

  if (sel_node->id &&
      sel_node->id != subject->id)
    return 0;

  for (j = 0; sel_node->classes_hash[j]; j++)
  {
    int found = 0;
    for (k = 0; subject->classes_hash[k] && !found; k++)
    {
      if (sel_node->classes_hash[j] == subject->classes_hash[k])
        found = 1;
    }
    if (!found)
      return 0;
  }
  for (j = 0; sel_node->pseudo[j]; j++)
  {
    if (ctx_strhash (sel_node->pseudo[j], 0) == CTX_first_child)
    {
      if (!(_mrg_child_no (mrg) == 1))
        return 0;
    }
    else if (!strncmp (sel_node->pseudo[j], "nth-child(", 10))
    {
      if (!_mrg_nth_match (sel_node->pseudo[j], _mrg_child_no (mrg)))
        return 0;
    }
    else
    {
      int found = 0;

      for (k = 0; subject->pseudo[k] && !found; k++)
      {
        if (sel_node->pseudo_hash[j] == subject->pseudo_hash[k])
          found = 1;
      }
      if (!found)
        return 0;
    }
  }
  return 1;
}

static int mrg_selector_vs_ancestry (Mrg *mrg,
                                     StyleEntry *entry,
                                     MrgStyleNode **ancestry,
                                     int a_depth)
{
  int s = entry->sel_len - 1;

  /* right most part of selector must match */
  if (!match_nodes (mrg, &entry->parsed[s], ancestry[a_depth-1]))
    return 0;

  s--;
  a_depth--;

  if (s < 0)
    return 1;

  while (s >= 0)
  {
    int ai;
    int found_node = 0;

  /* XXX: deal with '>' */
    // if (entry->parsed[s].direct_ancestor) //
    for (ai = a_depth-1; ai >= 0 && !found_node; ai--)
    {
      if (match_nodes (mrg, &entry->parsed[s], ancestry[ai]))
        found_node = 1;
    }
    if (found_node)
    {
      a_depth = ai;
    }
    else
    {
      return 0;
    }
    s--;
  }

  return 1;
}

static int mrg_css_selector_match (Mrg *mrg, StyleEntry *entry, MrgStyleNode **ancestry, int a_depth)
{
  if (entry->selector[0] == '*' &&
      entry->selector[1] == 0)
    return entry->specificity;

  if (a_depth == 0)
    return 0;

  if (mrg_selector_vs_ancestry (mrg, entry, ancestry, a_depth))
    return entry->specificity;

  return 0;
}

static char *_mrg_css_compute_style (Mrg *mrg, MrgStyleNode **ancestry, int a_depth)
{
  CtxList *l;
  CtxList *matches = NULL;
  int totlen = 2;
  char *ret = NULL;

  for (l = mrg->stylesheet; l; l = l->next)
  {
    StyleEntry *entry = l->data;
    int score = 0;
    score = mrg_css_selector_match (mrg, entry, ancestry, a_depth);

    if (score)
    {
      StyleMatch *match = malloc (sizeof (StyleMatch));
      match->score = score;
      match->entry = entry;
      ctx_list_prepend_full (&matches, match, (void*)free, NULL);
      totlen += strlen (entry->css) + 1;
    }
  }

  if (matches)
  {
    CtxList *l;
    char *p;

    p = ret = malloc (totlen);

    ctx_list_sort (&matches, compare_matches, NULL);
    for (l = matches; l; l = l->next)
    {
      StyleMatch *match = l->data;
      StyleEntry *entry = match->entry;
      strcpy (p, entry->css);
      p += strlen (entry->css);
      strcpy (p, ";");
      p ++;
    }
    ctx_list_free (&matches);
  }
  return ret;
}

static int _mrg_get_ancestry (Mrg *mrg, MrgStyleNode **ancestry)
{
  int i, j;
  for (i = 0, j = 0; i <= mrg->state_no; i++)
    if (mrg->states[i].style_id)
    {
      ancestry[j++] = &(mrg->states[i].style_node);
    }
  ancestry[j] = NULL;
  return j;
}

char *_mrg_stylesheet_collate_style (Mrg *mrg)
{
  MrgStyleNode *ancestry[MRG_MAX_STYLE_DEPTH];
  int ancestors = _mrg_get_ancestry (mrg, ancestry);
  char *ret = _mrg_css_compute_style (mrg, ancestry, ancestors);
  return ret;
}

void  mrg_set_line_height (Mrg *mrg, float line_height)
{
  mrg_style (mrg)->line_height = line_height;
}

float mrg_line_height (Mrg *mrg)
{
  return mrg_style (mrg)->line_height;
}

void  mrg_set_rem         (Mrg *mrg, float em)
{
  mrg->rem = em;
}

float mrg_em (Mrg *mrg)
{
  return mrg->state->style.font_size;
}

void  mrg_set_em (Mrg *mrg, float em)
{
  mrg->state->style.font_size = em;
}

void mrg_css_set (Mrg *mrg, const char *css)
{
  mrg_string_set (mrg->style, css);
}

void mrg_css_add (Mrg *mrg, const char *css)
{
  mrg_string_append_str (mrg->style, css);
}

void _mrg_layout_pre (Mrg *mrg, MrgHtml *ctx);
void _mrg_layout_post (Mrg *mrg, MrgHtml *ctx);

void mrg_set_style (Mrg *mrg, const char *style);

void mrg_start_with_style (Mrg        *mrg,
                           const char *style_id,
                           void       *id_ptr,
                           const char *style)
{
  mrg->states[mrg->state_no].children++;
  if (mrg->state_no+1 >= CTX_MAX_STATES)
          return;
  mrg->state_no++;
  mrg->state = &mrg->states[mrg->state_no];
  *mrg->state = mrg->states[mrg->state_no-1];
  mrg->states[mrg->state_no].children = 0;

  mrg->state->style_id = style_id ? strdup (style_id) : NULL;

  mrg_parse_style_id (mrg, mrg->state->style_id, &mrg->state->style_node);

  mrg->state->style.display = MRG_DISPLAY_INLINE;
  mrg->state->style.id_ptr = id_ptr;

  _mrg_init_style (mrg);

  if (mrg->in_paint)
    ctx_save (mrg_cr (mrg));

  {
    char *collated_style = _mrg_stylesheet_collate_style (mrg);
    if (collated_style)
    {
      mrg_set_style (mrg, collated_style);
      free (collated_style);
    }
  }
  if (style)
  {
    mrg_set_style (mrg, style);
  }
  _mrg_layout_pre (mrg, &mrg->html);
}

void
mrg_start_with_stylef (Mrg *mrg, const char *style_id, void *id_ptr,
                       const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  mrg_start_with_style (mrg, style_id, id_ptr, buffer);
  free (buffer);
}

void mrg_start (Mrg *mrg, const char *style_id, void *id_ptr)
{
  mrg_start_with_style (mrg, style_id, id_ptr, NULL);
}

void mrg_end (Mrg *mrg)
{
  _mrg_layout_post (mrg, &mrg->html);
  if (mrg->state->style_id)
  {
    free (mrg->state->style_id);
    mrg->state->style_id = NULL;
  }
  mrg->state_no--;
  if (mrg->state_no < 0)
    fprintf (stderr, "unbalanced mrg_start/mrg_end, enderflow\n");
  mrg->state = &mrg->states[mrg->state_no];
  if (mrg->in_paint)
    ctx_restore (mrg_cr (mrg));
}

void mrg_end       (Mrg *mrg);

void mrg_stylesheet_clear (Mrg *mrg);
void mrg_stylesheet_add (Mrg *mrg, const char *css, const char *uri,
                         int priority,
                         char **error);

void mrg_css_set (Mrg *mrg, const char *css);
void mrg_css_add (Mrg *mrg, const char *css);


static inline float mrg_parse_px_x (Mrg *mrg, const char *str, char **endptr)
{
  float result = 0;
  char *end = NULL;
#define PPI   96

  if (!str)
    return 0.0;

  result = _ctx_parse_float (str, &end);
  if (endptr)
    *endptr=end;

  //if (end[0]=='%v') /// XXX  % of viewport; regard less of stacking
  if (end[0]=='%')
  {
    result = result / 100.0 * (mrg_edge_right (mrg) - mrg_edge_left (mrg));

    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='v' && end[1] == 'h')
  {
    result = result / 100.0 * (mrg_edge_bottom (mrg) - mrg_edge_top (mrg));
    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='v' && end[1] == 'w')
  {
    result = result / 100.0 * (mrg_edge_right (mrg) - mrg_edge_left (mrg));
    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='r' && end[1]=='e' && end[2]=='m')
  {
    result *= mrg_rem (mrg);
    if (endptr)
      *endptr=end + 3;
  }
  else if (end[0]=='e' && end[1]=='m')
  {
    result *= mrg_em (mrg);
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='x')
  {
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='t')
  {
    result = (result / PPI) * 72;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='c')
  {
    result = (result / PPI) * 72 / 12;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='i' && end[1]=='n')
  {
    result = result / PPI;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='c' && end[1]=='m')
  {
    result = (result / PPI) * 2.54;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='m' && end[1]=='m')
  {
    result = (result / PPI) * 25.4;
    if (endptr)
      *endptr=end + 2;
  }
  return result;
}

static inline float mrg_parse_px_y (Mrg *mrg, const char *str, char **endptr)
{
  float result = 0;
  char *end = NULL;
  if (!str)
    return 0.0;

  result = _ctx_parse_float (str, &end);
  if (endptr)
    *endptr=end;

  if (end[0]=='%')
  {
    result = result / 100.0 * (mrg_edge_bottom (mrg) - mrg_edge_top (mrg));
    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='v' && end[1] == 'h')
  {
    result = result / 100.0 * (mrg_edge_bottom (mrg) - mrg_edge_top (mrg));
    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='v' && end[1] == 'w')
  {
    result = result / 100.0 * (mrg_edge_right (mrg) - mrg_edge_left (mrg));
    if (endptr)
      *endptr=end + 1;
  }
  else if (end[0]=='r' && end[1]=='e' && end[2]=='m')
  {
    result *= mrg_rem (mrg);
    if (endptr)
      *endptr=end + 3;
  }
  else if (end[0]=='e' && end[1]=='m')
  {
    result *= mrg_em (mrg);
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='x')
  {
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='t')
  {
    result = (result / PPI) * 72;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='p' && end[1]=='c')
  {
    result = (result / PPI) * 72 / 12;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='i' && end[1]=='n')
  {
    result = result / PPI;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='c' && end[1]=='m')
  {
    result = (result / PPI) * 2.54;
    if (endptr)
      *endptr=end + 2;
  }
  else if (end[0]=='m' && end[1]=='m')
  {
    result = (result / PPI) * 25.4;
    if (endptr)
      *endptr=end + 2;
  }
  return result;
}

static inline int mrg_parse_pxs (Mrg *mrg, const char *str, float *vals)
{
  int n_floats = 0;
  char *p =    (void*)str;
  char *prev = (void *)NULL;

  for (; p && p != prev && *p; )
  {
    float val;
    prev = p;
    val = n_floats%2==1?
      mrg_parse_px_x (mrg, p, &p):mrg_parse_px_y (mrg, p, &p);
    if (p != prev)
    {
      vals[n_floats++] = val;
    }
  }

  return n_floats;
}


static inline void mrg_css_handle_property_pass0 (Mrg *mrg, uint32_t key,
                                                  const char *value)
{
  /* pass0 deals with properties that parsing of many other property
   * definitions rely on */
  if (key == CTX_font_size)
  {
    float parsed;
    
    if (mrg->state_no)
    {
      mrg->state_no--;
      parsed = mrg_parse_px_y (mrg, value, NULL);
      mrg->state_no++;
    }
    else
    {
      parsed = mrg_parse_px_y (mrg, value, NULL);
    }
    mrg_set_em (mrg, parsed);
  }
  else if (key == CTX_color)
  {
    CtxColor *color = ctx_color_new ();
    ctx_color_set_from_string (mrg->ctx, color, value);
    ctx_set_color (mrg->ctx, CTX_color, color);
    ctx_color_free (color);
  }
}

static void mrg_css_handle_property_pass1 (Mrg *mrg, uint32_t key,
                                           const char *value)
{
  MrgStyle *s = mrg_style (mrg);
  uint32_t val_hash = ctx_strhash (value, 0);

  switch (key)
  {
    default:
      SET_PROPSh(key, value);
      break;
    case CTX_right:
    case CTX_bottom:
    case CTX_width:
    case CTX_color:
    case CTX_font_size:
      // handled in pass0
      break;
    case CTX_top:
    case CTX_height:
    case CTX_line_width:
    case CTX_text_indent:
    case CTX_letter_spacing:
    case CTX_word_spacing:
    case CTX_stroke_width:
    case CTX_text_stroke_width:
    case CTX_line_height:
    case CTX_border_top_width:
    case CTX_border_bottom_width:
    case CTX_margin_top:
    case CTX_margin_bottom:
    case CTX_padding_bottom:
    case CTX_padding_top:
    case CTX_min_height:
    case CTX_max_height:
      SET_PROPh(key, mrg_parse_px_y (mrg, value, NULL));
      break;
    case CTX_border_right_width:
    case CTX_border_left_width:
    case CTX_left:
    case CTX_tab_size:
    case CTX_min_width:
    case CTX_max_width:
    case CTX_padding_left:
    case CTX_padding_right:
      SET_PROPh(key, mrg_parse_px_x (mrg, value, NULL));
      break;
    case CTX_margin:
      {
        float vals[10];
        int    n_vals;
  
        n_vals = mrg_parse_pxs (mrg, value, vals);
        switch (n_vals)
        {
          case 1:
            SET_PROP(margin_top, vals[0]);
            SET_PROP(margin_right, vals[0]);
            SET_PROP(margin_bottom, vals[0]);
            SET_PROP(margin_left, vals[0]);
            break;
          case 2:
            SET_PROP(margin_top, vals[0]);
            SET_PROP(margin_right, vals[1]);
            SET_PROP(margin_bottom, vals[0]);
            SET_PROP(margin_left, vals[1]);
            break;
          case 3:
            SET_PROP(margin_top, vals[0]);
            SET_PROP(margin_right, vals[1]);
            SET_PROP(margin_bottom, vals[2]);
            SET_PROP(margin_left, vals[1]);
            break;
          case 4:
            SET_PROP(margin_top, vals[0]);
            SET_PROP(margin_right, vals[1]);
            SET_PROP(margin_bottom, vals[2]);
            SET_PROP(margin_left, vals[3]);
            break;
        }
      }
      break;
    case CTX_margin_left:
    {
      if (val_hash == CTX_auto)
      {
        s->margin_left_auto = 1;
      }
      else
      {
        SET_PROP(margin_left, mrg_parse_px_x (mrg, value, NULL));
        s->margin_left_auto = 0;
      }
    }
      break;
    case CTX_margin_right:
    {
      if (val_hash == CTX_auto)
      {
        s->margin_right_auto = 1;
      }
      else
      {
        SET_PROP(margin_right, mrg_parse_px_x (mrg, value, NULL));
        s->margin_right_auto = 0;
      }
    }
      break;
  
    case CTX_padding:
      {
        float vals[10];
        int   n_vals;
        n_vals = mrg_parse_pxs (mrg, value, vals);
        switch (n_vals)
        {
          case 1:
            SET_PROP(padding_top,    vals[0]);
            SET_PROP(padding_right,  vals[0]);
            SET_PROP(padding_bottom, vals[0]);
            SET_PROP(padding_left,   vals[0]);
            break;
          case 2:
            SET_PROP(padding_top,    vals[0]);
            SET_PROP(padding_right,  vals[1]);
            SET_PROP(padding_bottom, vals[0]);
            SET_PROP(padding_left,   vals[1]);
            break;
          case 3:
            SET_PROP(padding_top,    vals[0]);
            SET_PROP(padding_right,  vals[1]);
            SET_PROP(padding_bottom, vals[2]);
            SET_PROP(padding_left,   vals[1]);
            break;
          case 4:
            SET_PROP(padding_top,    vals[0]);
            SET_PROP(padding_right,  vals[1]);
            SET_PROP(padding_bottom, vals[2]);
            SET_PROP(padding_left,   vals[3]);
            break;
        }
      }
      break;
    case CTX_visibility:
    {
      if      (val_hash == CTX_visible) s->visibility = MRG_VISIBILITY_VISIBLE;
      else if (val_hash == CTX_hidden)  s->visibility = MRG_VISIBILITY_HIDDEN;
      else                              s->visibility = MRG_VISIBILITY_VISIBLE;
    }
    break;
  
    case CTX_border_width:
      {
        float valf = mrg_parse_px_y (mrg, value, NULL);
  
        SET_PROP(border_top_width, valf);
        SET_PROP(border_bottom_width, valf);
        SET_PROP(border_right_width, valf);
        SET_PROP(border_left_width, valf);
      }
      break;
    case CTX_border_color:
      {
        CtxColor *color = ctx_color_new ();
        ctx_color_set_from_string (mrg->ctx, color, value);
        ctx_set_color (mrg->ctx, CTX_border_top_color, color);
        ctx_set_color (mrg->ctx, CTX_border_left_color, color);
        ctx_set_color (mrg->ctx, CTX_border_right_color, color);
        ctx_set_color (mrg->ctx, CTX_border_bottom_color, color);
        ctx_color_free (color);
      }
      break;
    case CTX_border:
      {
        char word[64];
        int w = 0;
        const char *p;
        for (p = value; ; p++)
        {
          switch (*p)
          {
            case ' ':
            case '\n':
            case '\t':
            case '\r':
            case '\0':
              if (w)
              {
                uint32_t word_hash = ctx_strhash (word, 0);
                if ((word[0] >= '0' && word[0]<='9') || word[0] == '.')
                {
                  float valf = mrg_parse_px_y (mrg, word, NULL);
                  SET_PROP(border_top_width, valf);
                  SET_PROP(border_bottom_width, valf);
                  SET_PROP(border_right_width, valf);
                  SET_PROP(border_left_width, valf);
                } else if (word_hash == CTX_solid ||
                           word_hash == CTX_dotted ||
                           word_hash == CTX_inset) {
                } else {
                  CtxColor *color = ctx_color_new ();
                  ctx_color_set_from_string (mrg->ctx, color, word);
                  ctx_set_color (mrg->ctx, CTX_border_top_color, color);
                  ctx_set_color (mrg->ctx, CTX_border_left_color, color);
                  ctx_set_color (mrg->ctx, CTX_border_right_color, color);
                  ctx_set_color (mrg->ctx, CTX_border_bottom_color, color);
                  ctx_color_free (color);
                }
                word[0]=0;
                w=0;
              }
              break;
            default:
              word[w++]=*p;
              word[w]=0;
              break;
          }
          if (!*p)
            break;
        }
      }
      break;
    case CTX_border_right:
      {
        char word[64];
        int w = 0;
        const char *p;
        for (p = value; ; p++)
        {
          switch (*p)
          {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
            case '\0':
              if (w)
              {
                uint32_t word_hash = ctx_strhash (word, 0);
                if ((word[0] >= '0' && word[0]<='9') || (word[0] == '.'))
                {
                  float valf = mrg_parse_px_x (mrg, word, NULL);
                  SET_PROP(border_right_width, valf);
                } else if (word_hash == CTX_solid ||
                           word_hash == CTX_dotted ||
                           word_hash == CTX_inset) {
                } else {
                  CtxColor *color = ctx_color_new ();
                  ctx_color_set_from_string (mrg->ctx, color, word);
                  ctx_set_color (mrg->ctx, CTX_border_right_color, color);
                  ctx_color_free (color);
                }
                word[0]=0;
                w=0;
              }
              break;
            default:
              word[w++]=*p;
              word[w]=0;
              break;
          }
          if (!*p)
            break;
        }
      }
      break;
    case CTX_border_top:
      {
        char word[64];
        int w = 0;
        const char *p;
        for (p = value; ; p++)
        {
          switch (*p)
          {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
            case '\0':
              if (w)
              {
                uint32_t word_hash = ctx_strhash (word, 0);
                if ((word[0] >= '0' && word[0]<='9') || (word[0] == '.'))
                {
                  float valf = mrg_parse_px_x (mrg, word, NULL);
                  SET_PROP(border_top_width, valf);
                } else if (word_hash == CTX_solid ||
                           word_hash == CTX_dotted ||
                           word_hash == CTX_inset) {
                } else {
                  CtxColor *color = ctx_color_new ();
                  ctx_color_set_from_string (mrg->ctx, color, word);
                  ctx_set_color (mrg->ctx, CTX_border_top_color, color);
                  ctx_color_free (color);
                }
                word[0]=0;
                w=0;
              }
              break;
            default:
              word[w++]=*p;
              word[w]=0;
              break;
          }
          if (!*p)
            break;
        }
      }
      break;
    case CTX_border_left:
      {
        char word[64];
        int w = 0;
        const char *p;
        for (p = value; ; p++)
        {
          switch (*p)
          {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
            case '\0':
              if (w)
              {
                uint32_t word_hash = ctx_strhash (word, 0);
                if ((word[0] >= '0' && word[0]<='9') || (word[0] == '.'))
                {
                  float valf = mrg_parse_px_x (mrg, word, NULL);
                  SET_PROP(border_left_width, valf);
                } else if (word_hash == CTX_solid ||
                           word_hash == CTX_dotted ||
                           word_hash == CTX_inset) {
                } else {
                  CtxColor *color = ctx_color_new ();
                  ctx_color_set_from_string (mrg->ctx, color, word);
                  ctx_set_color (mrg->ctx, CTX_border_left_color, color);
                  ctx_color_free (color);
                }
                word[0]=0;
                w=0;
              }
              break;
            default:
              word[w++]=*p;
              word[w]=0;
              break;
          }
          if (!*p)
            break;
        }
      }
      break;
    case CTX_border_bottom:
      {
        char word[64];
        int w = 0;
        const char *p;
        for (p = value; ; p++)
        {
          switch (*p)
          {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
            case '\0':
              if (w)
              {
                uint32_t word_hash = ctx_strhash (word, 0);
                if ((word[0] >= '0' && word[0]<='9') || (word[0] == '.'))
                {
                  float valf = mrg_parse_px_x (mrg, word, NULL);
                  SET_PROP(border_bottom_width, valf);
                } else if (word_hash == CTX_solid ||
                           word_hash == CTX_dotted ||
                           word_hash == CTX_inset) {
                } else {
                  CtxColor *color = ctx_color_new ();
                  ctx_color_set_from_string (mrg->ctx, color, word);
                  ctx_set_color (mrg->ctx, CTX_border_bottom_color, color);
                  ctx_color_free (color);
                }
                word[0]=0;
                w=0;
              }
              break;
            default:
              word[w++]=*p;
              word[w]=0;
              break;
          }
          if (!*p)
            break;
        }
      }
      break;
  
    case CTX_background_color:
    case CTX_background:
    {
      CtxColor *color = ctx_color_new ();
      ctx_color_set_from_string (mrg->ctx, color, value);
      ctx_set_color (mrg->ctx, CTX_background_color, color);
      ctx_color_free (color);
    }
      break;
    case CTX_fill_color:
    case CTX_fill:
    {
      CtxColor *color = ctx_color_new ();
      ctx_color_set_from_string (mrg->ctx, color, value);
      ctx_set_color (mrg->ctx, CTX_fill_color, color);
      ctx_color_free (color);
    }
      break;
    case CTX_stroke_color:
    case CTX_stroke:
    {
      CtxColor *color = ctx_color_new ();
      ctx_color_set_from_string (mrg->ctx, color, value);
      ctx_set_color (mrg->ctx, CTX_stroke_color, color);
      ctx_color_free (color);
    }
      break;
    case CTX_text_stroke_color:
    {
      CtxColor *color = ctx_color_new ();
      ctx_color_set_from_string (mrg->ctx, color, value);
      ctx_set_color (mrg->ctx, CTX_text_stroke_color, color);
      ctx_color_free (color);
    }
      break;
    case CTX_text_stroke:
    {
      char *col = NULL;
      SET_PROP(text_stroke_width, mrg_parse_px_y (mrg, value, &col));
  
      if (col)
      {
        CtxColor *color = ctx_color_new ();
        ctx_color_set_from_string (mrg->ctx, color, col + 1);
        ctx_set_color (mrg->ctx, CTX_text_stroke_color, color);
        ctx_color_free (color);
      }
    }
      break;
    case CTX_opacity:
    {
      ctx_global_alpha (mrg->ctx, _ctx_parse_float (value, NULL));
      SET_PROP(opacity, _ctx_parse_float (value, NULL));
    }
    break;
    case CTX_print_symbols:
    switch (val_hash)
    {
        case CTX_true:
        case CTX_1:
        case CTX_yes:
          s->print_symbols = 1;
          break;
        default:
          s->print_symbols = 0;
    }
      break;
    case CTX_font_weight:
      switch (val_hash)
      {
        case CTX_bold:
        case CTX_bolder:
          s->text_decoration |= MRG_BOLD;
          s->font_weight = MRG_FONT_WEIGHT_BOLD;
          break;
        default:
          s->text_decoration ^= (s->text_decoration & MRG_BOLD);
          s->font_weight = MRG_FONT_WEIGHT_NORMAL;
      }
  #if 0 // XXX 
        cairo_select_font_face (mrg_cr (mrg),
            s->font_family,
            s->font_style,
            s->font_weight);
  #endif
      break;
    case CTX_white_space:
      {
        switch (val_hash)
        {
          default:
          case CTX_normal:   s->white_space = MRG_WHITE_SPACE_NORMAL; break;
          case CTX_nowrap:   s->white_space = MRG_WHITE_SPACE_NOWRAP; break;
          case CTX_pre:      s->white_space = MRG_WHITE_SPACE_PRE; break;
          case CTX_pre_line: s->white_space = MRG_WHITE_SPACE_PRE_LINE; break;
          case CTX_pre_wrap: s->white_space = MRG_WHITE_SPACE_PRE_WRAP; break;
        }
      }
      break;
    case CTX_box_sizing:
      {
        if (val_hash == CTX_border_box)
        {
          s->box_sizing = MRG_BOX_SIZING_BORDER_BOX;
          s->box_sizing = MRG_BOX_SIZING_CONTENT_BOX;
        }
      }
      break;
    case CTX_float:
      switch (val_hash)
      {
        case CTX_left:  s->float_ = MRG_FLOAT_LEFT; break;
        case CTX_right: s->float_ = MRG_FLOAT_RIGHT; break;
        default:        s->float_ = MRG_FLOAT_NONE;
      }
      break;
    case CTX_overflow:
      switch(val_hash)
      {
        case CTX_visible: s->overflow = MRG_OVERFLOW_VISIBLE; break;
        case CTX_hidden:  s->overflow = MRG_OVERFLOW_HIDDEN; break;
        case CTX_scroll:  s->overflow = MRG_OVERFLOW_SCROLL; break;
        case CTX_auto:    s->overflow = MRG_OVERFLOW_AUTO; break;
        default:          s->overflow = MRG_OVERFLOW_VISIBLE; break;
      }
      break;
    case CTX_clear:
      switch(val_hash)
      {
        case CTX_left:  s->clear = MRG_CLEAR_LEFT; break;
        case CTX_right: s->clear = MRG_CLEAR_RIGHT; break;
        case CTX_both:  s->clear = MRG_CLEAR_BOTH; break;
        default:        s->clear = MRG_CLEAR_NONE; break;
      }
      break;
    case CTX_font_style:
      switch(val_hash)
      {
        case CTX_italic:  s->font_style = MRG_FONT_STYLE_ITALIC; break;
        case CTX_oblique: s->font_style = MRG_FONT_STYLE_OBLIQUE; break;
        default:          s->font_style = MRG_FONT_STYLE_NORMAL;
  #if 0 // XXX
        cairo_select_font_face (mrg_cr (mrg),
            s->font_family,
            s->font_style,
            s->font_weight);
  #endif
      }
      break;
    case CTX_font_family:
      {
        SET_PROPS(font_family, value);
        ctx_font (mrg_cr (mrg), value);
      }
      break;
    case CTX_syntax_highlight:
      SET_PROPS(syntax_highlight, value);
      break;
    case CTX_fill_rule:
      switch (val_hash)
      { 
        default:
        case  CTX_evenodd: s->fill_rule = MRG_FILL_RULE_EVEN_ODD; break;
        case  CTX_nonzero: s->fill_rule = MRG_FILL_RULE_NONZERO; break;
      }
      if (s->fill_rule == MRG_FILL_RULE_EVEN_ODD)
        ctx_fill_rule (mrg_cr (mrg), CTX_FILL_RULE_EVEN_ODD);
      else
        ctx_fill_rule (mrg_cr (mrg), CTX_FILL_RULE_WINDING);
      break;
    case CTX_stroke_linejoin:
      switch (val_hash)
      { 
        case CTX_miter: s->stroke_linejoin = CTX_JOIN_MITER; break;
        case CTX_round: s->stroke_linejoin = CTX_JOIN_ROUND; break;
        case CTX_bevel: s->stroke_linejoin = CTX_JOIN_BEVEL; break;
        default:        s->stroke_linejoin = CTX_JOIN_MITER;
      }
      ctx_line_join (mrg_cr (mrg), (CtxLineJoin)s->stroke_linejoin);
      break;
    case CTX_stroke_linecap:
      switch (val_hash)
      { 
        case  CTX_butt:   s->stroke_linecap = CTX_CAP_NONE; break;
        case  CTX_round:  s->stroke_linecap = CTX_CAP_ROUND;break;
        case  CTX_square: s->stroke_linecap = CTX_CAP_SQUARE; break;
        default:          s->stroke_linecap = CTX_CAP_NONE;
      }
      ctx_line_cap (mrg_cr (mrg), s->stroke_linecap);
      break;
    case CTX_vertical_align:
      switch (val_hash)
      {
        case CTX_middle: s->vertical_align = MRG_VERTICAL_ALIGN_MIDDLE; break;
        case CTX_top:    s->vertical_align = MRG_VERTICAL_ALIGN_TOP; break;
        case CTX_sub:    s->vertical_align = MRG_VERTICAL_ALIGN_SUB;break;
        case CTX_super:  s->vertical_align = MRG_VERTICAL_ALIGN_SUPER;break;
        case CTX_bottom: s->vertical_align = MRG_VERTICAL_ALIGN_BOTTOM; break;
        default:         s->vertical_align = MRG_VERTICAL_ALIGN_BASELINE;
      }
      break;
    case CTX_cursor:
      switch (val_hash)
      {
        default:
        case CTX_default:       s->cursor = MRG_CURSOR_DEFAULT;break;
        case CTX_auto:          s->cursor = MRG_CURSOR_AUTO;break;
        case CTX_alias:         s->cursor = MRG_CURSOR_ALIAS;break;
        case CTX_all_scroll:    s->cursor = MRG_CURSOR_ALL_SCROLL;break;
        case CTX_cell:          s->cursor = MRG_CURSOR_CELL;break;
        case CTX_context_menu:  s->cursor = MRG_CURSOR_CONTEXT_MENU;break;
        case CTX_col_resize:    s->cursor = MRG_CURSOR_COL_RESIZE;break;
        case CTX_copy:          s->cursor = MRG_CURSOR_COPY;break;
        case CTX_crosshair:     s->cursor = MRG_CURSOR_CROSSHAIR;break;
        case CTX_e_resize:      s->cursor = MRG_CURSOR_E_RESIZE;break;
        case CTX_ew_resize:     s->cursor = MRG_CURSOR_EW_RESIZE;break;
        case CTX_help:          s->cursor = MRG_CURSOR_HELP;break;
        case CTX_move:          s->cursor = MRG_CURSOR_MOVE;break;
        case CTX_n_resize:      s->cursor = MRG_CURSOR_N_RESIZE;break;
        case CTX_ne_resize:     s->cursor = MRG_CURSOR_NE_RESIZE;break;
        case CTX_nesw_resize:   s->cursor = MRG_CURSOR_NESW_RESIZE;break;
        case CTX_ns_resize:     s->cursor = MRG_CURSOR_NS_RESIZE;break;
        case CTX_nw_resize:     s->cursor = MRG_CURSOR_NW_RESIZE;break;
        case CTX_no_drop:       s->cursor = MRG_CURSOR_NO_DROP;break;
        case CTX_none:          s->cursor = MRG_CURSOR_NONE;break;
        case CTX_not_allowed:   s->cursor = MRG_CURSOR_NOT_ALLOWED;break;
        case CTX_pointer:       s->cursor = MRG_CURSOR_POINTER;break;
        case CTX_progress:      s->cursor = MRG_CURSOR_PROGRESS;break;
        case CTX_row_resize:    s->cursor = MRG_CURSOR_ROW_RESIZE;break;
        case CTX_s_resize:      s->cursor = MRG_CURSOR_S_RESIZE;break;
        case CTX_se_resize:     s->cursor = MRG_CURSOR_SE_RESIZE;break;
        case CTX_sw_resize:     s->cursor = MRG_CURSOR_SW_RESIZE;break;
        case CTX_text:          s->cursor = MRG_CURSOR_TEXT;break;
        case CTX_vertical_text: s->cursor = MRG_CURSOR_VERTICAL_TEXT;break;
        case CTX_w_resize:      s->cursor = MRG_CURSOR_W_RESIZE;break;
        case CTX_cursor_wait:   s->cursor = MRG_CURSOR_WAIT;break;
        case CTX_zoom_in:       s->cursor = MRG_CURSOR_ZOOM_IN;break;
        case CTX_zoom_out:      s->cursor = MRG_CURSOR_ZOOM_OUT;break;
      }
      break;
    case CTX_display:
      switch (val_hash)
      {
        case CTX_hidden:       s->display = MRG_DISPLAY_NONE; break;
        case CTX_block:        s->display = MRG_DISPLAY_BLOCK; break;
        case CTX_list_item:    s->display = MRG_DISPLAY_LIST_ITEM; break;
        case CTX_inline_block: s->display = MRG_DISPLAY_INLINE_BLOCK; break;
        default:               s->display = MRG_DISPLAY_INLINE;
      }
      break;
    case CTX_position:
      switch (val_hash)
      {
        case CTX_relative:  s->position = MRG_POSITION_RELATIVE; break;
        case CTX_static:    s->position = MRG_POSITION_STATIC; break;
        case CTX_absolute:  s->position = MRG_POSITION_ABSOLUTE; break;
        case CTX_fixed:     s->position = MRG_POSITION_FIXED; break;
        default:            s->position = MRG_POSITION_STATIC;
      }
      break;
    case CTX_direction:
      switch (val_hash)
      {
        case CTX_rtl: s->direction = MRG_DIRECTION_RTL; break;
        case CTX_ltr: s->direction = MRG_DIRECTION_LTR; break;
        default:      s->direction = MRG_DIRECTION_LTR;
      }
      break;
    case CTX_unicode_bidi:
      switch (val_hash)
      {
        case CTX_normal: s->unicode_bidi = MRG_UNICODE_BIDI_NORMAL; break;
        case CTX_embed:  s->unicode_bidi = MRG_UNICODE_BIDI_EMBED; break;
        case CTX_bidi_override: s->unicode_bidi = MRG_UNICODE_BIDI_BIDI_OVERRIDE; break;
        default:         s->unicode_bidi = MRG_UNICODE_BIDI_NORMAL; break;
      }
      break;
    case CTX_text_align:
      switch (val_hash)
      {
        case CTX_left:    s->text_align = MRG_TEXT_ALIGN_LEFT; break;
        case CTX_right:   s->text_align = MRG_TEXT_ALIGN_RIGHT; break;
        case CTX_justify: s->text_align = MRG_TEXT_ALIGN_JUSTIFY; break;
        case CTX_center:  s->text_align = MRG_TEXT_ALIGN_CENTER; break;
        default:          s->text_align = MRG_TEXT_ALIGN_LEFT;
      }
      break;
    case CTX_text_decoration:
      switch (val_hash)
      {
        case CTX_reverse:     s->text_decoration|= MRG_REVERSE; break;
        case CTX_underline:   s->text_decoration|= MRG_UNDERLINE; break;
        case CTX_overline:    s->text_decoration|= MRG_OVERLINE; break;
        case CTX_linethrough: s->text_decoration|= MRG_LINETHROUGH; break;
        case CTX_blink:       s->text_decoration|= MRG_BLINK; break;
        case CTX_none:
          s->text_decoration ^= (s->text_decoration &
        (MRG_UNDERLINE|MRG_REVERSE|MRG_OVERLINE|MRG_LINETHROUGH|MRG_BLINK));
        break;
      }
      break;
  }
}

static void mrg_css_handle_property_pass1med (Mrg *mrg, uint32_t key,
                                              const char *value)
{
  MrgStyle *s = mrg_style (mrg);

  if (key == CTX_width)
  {
    if (ctx_strhash (value, 0) == CTX_auto)
    {
      s->width_auto = 1;
      SET_PROP(width, 42);
    }
    else
    {
      s->width_auto = 0;
      SET_PROP(width, mrg_parse_px_x (mrg, value, NULL));

      if (s->position == MRG_POSITION_FIXED) // XXX: seems wrong
      {
        //s->width -= s->border_left_width + s->border_right_width;
      }
    }
  }
}

enum
{
  MRG_CSS_PROPERTY_PARSER_STATE_NEUTRAL = 0,
  MRG_CSS_PROPERTY_PARSER_STATE_IN_NAME,
  MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_COLON,
  MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_VAL,
  MRG_CSS_PROPERTY_PARSER_STATE_IN_VAL
};

static void css_parse_properties (Mrg *mrg, const char *style,
  void (*handle_property) (Mrg *mrg, uint32_t key,
                           const char *value))
{
  const char *p;
  char name[MRG_MAX_CSS_STRINGLEN] = "";
  char string[MRG_MAX_CSS_STRINGLEN] = "";
  int name_l = 0;
  int string_l = 0;
  int state = MRG_CSS_PROPERTY_PARSER_STATE_NEUTRAL;
  if (!style)
    return;
  for (p = style; *p; p++)
  {
    switch (state)
    {
      case MRG_CSS_PROPERTY_PARSER_STATE_NEUTRAL:
        switch (*p)
        {
          case ' ':
          case '\t':
          case ';':
          case '\n':
          case '\r':
            break;
          default:
            name[name_l++]=*p;
            name[name_l]=0;
            state = MRG_CSS_PROPERTY_PARSER_STATE_IN_NAME;
            break;
        }
        break;
      case MRG_CSS_PROPERTY_PARSER_STATE_IN_NAME:
        switch (*p)
        {
          case ':':
            state = MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_VAL;
            break;
          case ' ':
          case '\n':
          case '\r':
          case '\t':
            state = MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_COLON;
            break;
          default:
            name[name_l++]=*p;
            name[name_l]=0;
            break;
        }
        break;
      case MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_COLON:
        switch (*p)
        {
          case ':':
            state = MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_VAL;
            break;
          default:
            break;
        }
        break;
      case MRG_CSS_PROPERTY_PARSER_STATE_EXPECT_VAL:
        switch (*p)
        {
          case ' ':
          case '\n':
          case '\r':
          case '\t':
            break;
          default:
            string[string_l++]=*p;
            string[string_l]=0;
            state = MRG_CSS_PROPERTY_PARSER_STATE_IN_VAL;
            break;
        }
        break;
      case MRG_CSS_PROPERTY_PARSER_STATE_IN_VAL:
        switch (*p)
        {
          case ';':
            handle_property (mrg, ctx_strhash (name, 0), string);
            state = MRG_CSS_PROPERTY_PARSER_STATE_NEUTRAL;
            name_l = 0;
            name[0] = 0;
            string_l = 0;
            string[0] = 0;
            break;
          default:
            string[string_l++]=*p;
            string[string_l]=0;
            break;
        }
        break;
    }
  }
  if (name[0])
  handle_property (mrg, ctx_strhash (name, 0), string);
}


static void mrg_css_handle_property_pass2 (Mrg *mrg, uint32_t key,
                                           const char *value)
{
  /* this pass contains things that might depend on values
   * generated by the previous pass.
   */
  MrgStyle *s = mrg_style (mrg);

  if (key == CTX_right)
  {
    float width = PROP(width);
    float right = mrg_parse_px_x (mrg, value, NULL);

    SET_PROP(right, right);

    if (width == 0)
    {
      MrgGeoCache *geo = _mrg_get_cache (&mrg->html, s->id_ptr);
      if (geo->gen)
        width = geo->width;
      else
      {
        width = 8 * s->font_size;
        mrg_queue_draw (mrg, NULL);
      }
    }
    SET_PROP(right, right);
    SET_PROP(left,
         (mrg_width(mrg)-right) - width - PROP(border_left_width) - PROP(padding_left) - PROP(padding_right) - PROP(border_right_width) - PROP(margin_right));
  }
  else if (key == CTX_bottom)
  {
    float height = PROP(height);

    SET_PROP (bottom, mrg_parse_px_y (mrg, value, NULL));

    if (height == 0)
    {
      MrgGeoCache *geo = _mrg_get_cache (&mrg->html, s->id_ptr);
      if (geo->gen)
        height = geo->height;
      else
      {
        height = 2 * s->font_size;
        mrg_queue_draw (mrg, NULL);
      }
    }
    SET_PROP(top, mrg_height(mrg) - PROP(bottom) - height - PROP(padding_top) - PROP(border_top_width) - PROP(padding_bottom) - PROP(border_bottom_width) - PROP(margin_bottom));
  }
}


static float deco_width (Mrg *mrg)
{
  return PROP (padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width);
}

void mrg_set_style (Mrg *mrg, const char *style)
{
  MrgStyle *s;

  css_parse_properties (mrg, style, mrg_css_handle_property_pass0);
  css_parse_properties (mrg, style, mrg_css_handle_property_pass1);
  css_parse_properties (mrg, style, mrg_css_handle_property_pass1med);

  s = mrg_style (mrg);

  if (s->position == MRG_POSITION_STATIC &&
      !s->float_)
  {
    if (s->width_auto && (s->margin_right_auto || s->margin_left_auto))
    {
      if (s->margin_left_auto && s->margin_right_auto)
      {
        s->margin_right_auto = 0;
      }
      else if (s->margin_left_auto)
        s->margin_left_auto = 0;
      else
        s->margin_right_auto = 0;
    }

    if ( s->margin_left_auto && !s->width_auto && !s->margin_right_auto)
    {
      SET_PROP (margin_left,
        (mrg->state->edge_right - mrg->state->edge_left)
        - deco_width (mrg) - PROP(margin_right) - PROP(width));
    }
    else if ( !s->margin_left_auto &&
              s->width_auto &&
              !s->margin_right_auto)
    {
      SET_PROP (width,
        (mrg->state->edge_right - mrg->state->edge_left)
        - deco_width (mrg) - PROP(margin_left) - PROP(margin_right));
    }
    else if ( !s->margin_left_auto && !s->width_auto && s->margin_right_auto)
    {
      SET_PROP (margin_right,
        (mrg->state->edge_right - mrg->state->edge_left)
        - deco_width (mrg) - PROP(margin_left) - PROP(width));
    }
    else if ( s->margin_left_auto && !s->width_auto && s->margin_right_auto)
    {
      float val = ((mrg->state->edge_right - mrg->state->edge_left)
        - deco_width (mrg) - PROP(width))/2;
      SET_PROP (margin_left, val);
      SET_PROP (margin_right, val);
    }
  }
  css_parse_properties (mrg, style, mrg_css_handle_property_pass2);
}

void _mrg_set_style_properties (Mrg *mrg, const char *style_properties)
{
  _mrg_init_style (mrg);

  if (style_properties)
  {
    mrg_set_style (mrg, style_properties);
  }
}

void
mrg_set_stylef (Mrg *mrg, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  mrg_set_style (mrg, buffer);
  free (buffer);
}


void  mrg_set_line_height (Mrg *mrg, float line_height);
float  mrg_line_height (Mrg *mrg);


static void
mrg_ctx_set_source_color (Ctx *ctx, CtxColor *color)
{
   float rgba[4];
   ctx_color_get_rgba (ctx_get_state (ctx), color, rgba);
   ctx_rgba (ctx, rgba[0], rgba[1], rgba[2], rgba[3]);
}

static void mrg_path_fill_stroke (Mrg *mrg)
{
  Ctx *ctx = mrg_cr (mrg);
  CtxColor *fill_color = ctx_color_new ();
  CtxColor *stroke_color = ctx_color_new ();

  ctx_get_color (ctx, CTX_fill_color, fill_color);
  ctx_get_color (ctx, CTX_stroke_color, stroke_color);

  if (!ctx_color_is_transparent (fill_color))
  {
    mrg_ctx_set_source_color (ctx, fill_color);
    if (PROP(stroke_width) > 0.001 && !ctx_color_is_transparent (stroke_color))
      ctx_preserve (ctx);
    ctx_fill (ctx);
  }

  if (PROP(stroke_width) > 0.001 && !ctx_color_is_transparent (stroke_color))
  {
    ctx_line_width (ctx, PROP(stroke_width));
    mrg_ctx_set_source_color (ctx, stroke_color);
    ctx_stroke (ctx);
  }
  ctx_color_free (fill_color);
  ctx_color_free (stroke_color);
}

void _mrg_border_top (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);

  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, CTX_border_top_color, color);

  if (PROP(border_top_width) > 0.01f &&
      !ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x - PROP(padding_left) - PROP(border_left_width),
                       y - PROP(padding_top) - PROP(border_top_width));
    ctx_rel_line_to (ctx, width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width), 0);
    ctx_rel_line_to (ctx, -PROP(border_right_width), PROP(border_top_width));
    ctx_rel_line_to (ctx, - (width + PROP(padding_right) + PROP(padding_left)), 0);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }
  ctx_color_free (color);
}

void _mrg_border_bottom (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, CTX_border_top_color, color);

  if (PROP(border_bottom_width) > 0.01 &&
      !ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width + PROP(padding_right), y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, PROP(border_right_width), PROP(border_bottom_width));
    ctx_rel_line_to (ctx, - (width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width)), 0);
    ctx_rel_line_to (ctx, PROP(border_left_width), -PROP(border_bottom_width));

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}

void _mrg_border_top_r (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *cr = mrg_cr (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (cr, CTX_border_top_color, color);

  if (PROP(border_top_width) > 0.01 &&
      !ctx_color_is_transparent (color))
  {
  ctx_save (cr);
    ctx_begin_path (cr);
    ctx_move_to (cr, x, y - PROP(padding_top) - PROP(border_top_width));
    ctx_rel_line_to (cr, width + PROP(padding_right) + PROP(border_right_width), 0);
    ctx_rel_line_to (cr, -PROP(border_right_width), PROP(border_top_width));
    ctx_rel_line_to (cr, - (width + PROP(padding_right)), 0);

    mrg_ctx_set_source_color (cr, color);
    ctx_fill (cr);
  ctx_restore (cr);
  }
  ctx_color_free (color);
}
void _mrg_border_bottom_r (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, CTX_border_bottom_color, color);

  if (PROP(border_bottom_width) &&
      !ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width + PROP(padding_right), y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, PROP(border_right_width), PROP(border_bottom_width));
    ctx_rel_line_to (ctx, - (width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width)), 0);
    ctx_rel_line_to (ctx, PROP(border_left_width), -PROP(border_bottom_width));

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}

void _mrg_border_top_l (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, CTX_border_top_color, color);

  if (PROP(border_top_width) > 0.01 &&
      !ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x - PROP(padding_left) - PROP(border_left_width),
                       y - PROP(padding_top) - PROP(border_top_width));
    ctx_rel_line_to (ctx, width + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width), 0);
    ctx_rel_line_to (ctx, 0, PROP(border_top_width));
    ctx_rel_line_to (ctx, - (width + PROP(padding_left)), 0);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }
  ctx_color_free (color);
}
void _mrg_border_bottom_l (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, CTX_border_bottom_color, color);

  if (PROP(border_bottom_width) > 0.01 &&
      !ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width, y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, 0, PROP(border_bottom_width));
    ctx_rel_line_to (ctx, - (width + PROP(padding_left) + PROP(border_left_width)), 0);
    ctx_rel_line_to (ctx, PROP(border_left_width), -PROP(border_bottom_width));

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}


void _mrg_border_top_m (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, CTX_border_top_color, color);

  if (PROP(border_top_width) &&
      !ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x,
                       y - PROP(padding_top) - PROP(border_top_width));
    ctx_rel_line_to (ctx, width, 0);
    ctx_rel_line_to (ctx, 0, PROP(border_top_width));
    ctx_rel_line_to (ctx, -width, 0);

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }
  ctx_color_free (color);
}
void _mrg_border_bottom_m (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, CTX_border_bottom_color, color);

  if (PROP(border_bottom_width) &&
      !ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width, y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, 0, PROP(border_bottom_width));
    ctx_rel_line_to (ctx, - width, 0);
    ctx_rel_line_to (ctx, 0, -PROP(border_bottom_width));

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}
void _mrg_border_left (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, CTX_border_left_color, color);

  if (PROP(border_left_width) &&
      !ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x - PROP(padding_left) - PROP(border_left_width),
                       y - PROP(padding_top) - PROP(border_top_width));
    ctx_rel_line_to (ctx, PROP(border_left_width), PROP(border_top_width));
    ctx_rel_line_to (ctx, 0, height + PROP(padding_top) + PROP(padding_bottom) );
    ctx_rel_line_to (ctx, -PROP(border_left_width), PROP(border_bottom_width));
    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}
void _mrg_border_right (Mrg *mrg, int x, int y, int width, int height)
{
  Ctx *ctx = mrg_cr (mrg);
  CtxColor *color = ctx_color_new ();
  ctx_get_color (ctx, CTX_border_right_color, color);

  if (PROP(border_right_width) > 0.01 &&
      !ctx_color_is_transparent (color))
  {
  ctx_save (ctx);
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x + width + PROP(padding_right), y + height + PROP(padding_bottom));
    ctx_rel_line_to (ctx, PROP(border_right_width), PROP(border_bottom_width));
    ctx_rel_line_to (ctx, 0, - (height + PROP(padding_top) + PROP(padding_bottom) + PROP(border_top_width) + PROP(border_bottom_width)));
    ctx_rel_line_to (ctx, -PROP(border_right_width), PROP(border_top_width));

    mrg_ctx_set_source_color (ctx, color);
    ctx_fill (ctx);
  ctx_restore (ctx);
  }

  ctx_color_free (color);
}

static void mrg_box (Mrg *mrg, int x, int y, int width, int height)
{
  _mrg_draw_background_increment (mrg, &mrg->html, 1);
  _mrg_border_top (mrg, x, y, width, height);
  _mrg_border_left (mrg, x, y, width, height);
  _mrg_border_right (mrg, x, y, width, height);
  _mrg_border_bottom (mrg, x, y, width, height);
}

static void mrg_box_fill (Mrg *mrg, MrgStyle *style, float x, float y, float width, float height)
{
  Ctx *ctx = mrg_cr (mrg);
  CtxColor *background_color = ctx_color_new ();
  ctx_get_color (ctx, CTX_background_color, background_color);
  if (ctx_color_is_transparent (background_color))
  {
    ctx_color_free (background_color);
    return;
  }

  height = ctx_floorf (y + height) - ctx_floorf(y);
  y = ctx_floorf (y);

  ctx_save (ctx);
  {
    ctx_begin_path (ctx);
    ctx_move_to (ctx, x,
                       y);
    ctx_rel_line_to (ctx, 0, height );
    ctx_rel_line_to (ctx, width, 0);
    ctx_rel_line_to (ctx, 0, -(height ));

    ctx_fill_rule (ctx, CTX_FILL_RULE_EVEN_ODD);
    mrg_ctx_set_source_color (ctx, background_color);
    ctx_fill (ctx);
  }
  ctx_restore (ctx);
  ctx_color_free (background_color);
}

/*
 *  each style state level needs to know how far down it has
 *  painted background,.. on background increment we do all of them..
 *  .. floats are problematic - maybe draw them in second layer.
 *
 */

static void
_mrg_draw_background_increment2 (Mrg *mrg, MrgState *state, 
    MrgHtmlState *html_state, void *data, int last)
{
  MrgHtml *html = &mrg->html;
  Ctx *ctx = mrg_cr (mrg);
  MrgStyle *style = &state->style;
  float gap = ctx_get_font_size (ctx) * mrg->state->style.line_height;

  int width = PROP(width);
#if ooops
  if (style->background_color.alpha <= 0.0001)
    return;
#endif
  if (style->display == MRG_DISPLAY_INLINE &&
      style->float_ == MRG_FLOAT_NONE)
    return;

  if (last)
    gap += PROP(padding_bottom);

  if (!width)
  {
    MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
    if (geo->width)
      width = geo->width;
    else
      width = mrg_edge_right (mrg) - mrg_edge_left (mrg); // XXX : err
  }

  if (html_state->ptly == 0)
  {
    mrg_box_fill (mrg, style,
      html_state->block_start_x - PROP(padding_left),
      (html_state->block_start_y - mrg_em (mrg) - PROP(padding_top)),
      width + PROP(padding_left) + PROP(padding_right),
      PROP(padding_top) + gap);
    
    html_state->ptly = 
      html_state->block_start_y - mrg_em (mrg) - PROP(padding_top) +
      (PROP(padding_top) + gap);
  }
  else
  {
    if (( (mrg_y (mrg) - style->font_size) - html_state->ptly) + gap > 0)
    {
      mrg_box_fill (mrg, style,
          html_state->block_start_x - PROP(padding_left),
          html_state->ptly,
          width + PROP(padding_left) + PROP(padding_right),
          ((mrg_y (mrg) - style->font_size) - html_state->ptly) + gap);

      html_state->ptly = mrg_y (mrg) - style->font_size  + gap;
    }
  }
}

static void
_mrg_draw_background_increment (Mrg *mrg, void *data, int last)
{
  MrgHtml *ctx = &mrg->html;
  int state;
  for (state = 0; state <= ctx->state_no; state++)
  {
    _mrg_draw_background_increment2 (mrg,
        &mrg->states[mrg->state_no - (ctx->state_no) + state],
        &ctx->states[state],
        data, last);
  }
}

float mrg_ddpx (Mrg *mrg)
{
  return 1;
}

/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/**************/

static float measure_word_width (Mrg *mrg, const char *word)
{
  return ctx_text_width (mrg->ctx, word);
}

void _mrg_get_ascent_descent (Mrg *mrg, float *ascent, float *descent)
{
#if 0 // MRG_CAIRO
  cairo_scaled_font_t *scaled_font = mrg->scaled_font;
  cairo_font_extents_t extents;
  if (mrg_is_terminal (mrg))
  {
    if (ascent) *ascent = 0;
    if (descent) *descent= 0;
    return;
  }

  if (mrg->in_paint)
  {
    cairo_set_font_size (mrg_cr(mrg), mrg_style(mrg)->font_size);
    scaled_font = cairo_get_scaled_font (mrg_cr (mrg));
  }
  cairo_scaled_font_extents (scaled_font, &extents);

  if (ascent)  *ascent  = extents.ascent;
  if (descent) *descent = extents.descent;
#else
  if (ascent)  *ascent  = mrg_style(mrg)->font_size;
  if (descent) *descent = 0.0;
#endif
}


static float _mrg_text_shift (Mrg *mrg)
{
  //MrgStyle *style = mrg_style (mrg);
  float ascent, descent;
  _mrg_get_ascent_descent (mrg, &ascent, &descent);
  return (descent * 0.9); // XXX
}

const char * hl_punctuation[] =
{";", ",", "(", ")", "{", "}", NULL};
const char * hl_operators [] =
{"-", "+", "=", "*", "/", "return", "<", ">", ":",
 "if", "else", "break", "case", NULL};
const char * hl_types[] =
// XXX anything ending in _t ?
{"int", "uint32_t", "uint64_t", "uint8_t", "Ctx", "cairo_t", "Mrg", "float", "double",
  "char", "const", "static", "void", "NULL",
  "#include", "#define", NULL};

static int is_one_of (const char *word, const char **words)
{
  int i;
  for (i = 0; words[i]; i++)
  {
    if (!strcmp (words[i], word))
      return 1;
  }
  return 0;
}

static int is_a_number (const char *word)
{
  int yep = 1;
  int i;
  for (i = 0; word[i]; i++)
  {
    if ((word[i] < '0' || word[i] > '9') && word[i] != '.')
      yep = 0;
  }
  return yep;
}

/* the syntax highlighting is done with static globals; deep in the text
 * rendering, this permits the editing code to recognize which string is
 * edited and directly work with pointer arithmetic on that instead of
 * marked up xml for the highlighting - it limits the syntax highlighting
 * context ability
 */
enum {
  MRG_HL_NEUTRAL      = 0,
  MRG_HL_NEXT_NEUTRAL = 1,
  MRG_HL_STRING       = 2,
  MRG_HL_STRING_ESC   = 3,
  MRG_HL_QSTRING      = 4,
  MRG_HL_QSTRING_ESC  = 5,
  MRG_HL_SLASH        = 6,
  MRG_HL_LINECOMMENT  = 7,
  MRG_HL_COMMENT      = 8,
  MRG_HL_COMMENT_STAR = 9,
};

static int hl_state_c = MRG_HL_NEUTRAL;

static void mrg_hl_token (Ctx *cr, const char *word)
{
  switch (hl_state_c)
  {
    case MRG_HL_NEUTRAL:
      if (!strcmp (word, "\""))
      {
        hl_state_c = MRG_HL_STRING;
      }
      else if (!strcmp (word, "'"))
      {
        hl_state_c = MRG_HL_QSTRING;
      }
      else if (!strcmp (word, "/"))
      {
        hl_state_c = MRG_HL_SLASH;
      }
      break;
    case MRG_HL_SLASH:
      if (!strcmp (word, "/"))
      {
        hl_state_c = MRG_HL_LINECOMMENT;
      } else if (!strcmp (word, "*"))
      {
        hl_state_c = MRG_HL_COMMENT;
      } else
      {
        hl_state_c = MRG_HL_NEUTRAL;
      }
      break;
    case MRG_HL_LINECOMMENT:
      if (!strcmp (word, "\n"))
      {
        hl_state_c = MRG_HL_NEXT_NEUTRAL;
      }
      break;
    case MRG_HL_COMMENT:
      if (!strcmp (word, "*"))
      {
        hl_state_c = MRG_HL_COMMENT_STAR;
      }
      break;
    case MRG_HL_COMMENT_STAR:
      if (!strcmp (word, "/"))
      {
        hl_state_c = MRG_HL_NEUTRAL;
      }
      else
      {
        hl_state_c = MRG_HL_COMMENT;
      }
      break;
    case MRG_HL_STRING:
      if (!strcmp (word, "\""))
      {
        hl_state_c = MRG_HL_NEXT_NEUTRAL;
      }
      else if (!strcmp (word, "\\"))
      {
        hl_state_c = MRG_HL_STRING_ESC;
      }
      break;
    case MRG_HL_STRING_ESC:
      hl_state_c = MRG_HL_STRING;
      break;
    case MRG_HL_QSTRING:
      if (!strcmp (word, "'"))
      {
        hl_state_c = MRG_HL_NEXT_NEUTRAL;
      }
      else if (!strcmp (word, "\\"))
      {
        hl_state_c = MRG_HL_QSTRING_ESC;
      }
      break;
    case MRG_HL_QSTRING_ESC:
      hl_state_c = MRG_HL_QSTRING;
      break;
    case MRG_HL_NEXT_NEUTRAL:
      hl_state_c = MRG_HL_NEUTRAL;
      break;
  }

  switch (hl_state_c)
  {
    case MRG_HL_NEUTRAL:
      if (is_a_number (word))
        ctx_rgb (cr, 0.5, 0.0, 0.0);
      else if (is_one_of (word, hl_punctuation))
        ctx_rgb (cr, 0.4, 0.4, 0.4);
      else if (is_one_of (word, hl_operators))
        ctx_rgb (cr, 0, 0.5, 0);
      else if (is_one_of (word, hl_types))
        ctx_rgb (cr, 0.2, 0.2, 0.5);
      else 
        ctx_rgb (cr, 0, 0, 0);
      break;
    case MRG_HL_STRING:
    case MRG_HL_QSTRING:
        ctx_rgb (cr, 1, 0, 0.5);
      break;
    case MRG_HL_COMMENT:
    case MRG_HL_COMMENT_STAR:
    case MRG_HL_LINECOMMENT:
        ctx_rgb (cr, 0.4, 0.4, 1);
      break;
  }

  ctx_text (cr, word);
}

/* hook syntax highlighter in here..  */
void mrg_hl_text (Ctx *cr, const char *text)
{
  int i;
  MrgString *word = mrg_string_new ("");
  for (i = 0; i < text[i]; i++)
  {
    switch (text[i])
    {
      case ';':
      case '-':
      case '\'':
      case '>':
      case '<':
      case '=':
      case '+':
      case ' ':
      case ':':
      case '"':
      case '*':
      case '/':
      case '\\':
      case '[':
      case ']':
      case ')':
      case ',':
      case '(':
        if (word->length)
        {
          mrg_hl_token (cr, word->str);
          mrg_string_set (word, "");
        }
        mrg_string_append_byte (word, text[i]);
        mrg_hl_token (cr, word->str);
        mrg_string_set (word, "");
        break;
      default:
        ctx_rgb (cr, 0,0,0);
        mrg_string_append_byte (word, text[i]);
        break;
    }
  }
  if (word->length)
    mrg_hl_token (cr, word->str);

  mrg_string_free (word, 1);
}

void ctx_listen (Ctx     *ctx,
                 CtxEventType  types,
                 CtxCb    cb,
                 void*    data1,
                 void*    data2);



/* x and y in cairo user units ; returns x advance in user units  */
float mrg_draw_string (Mrg *mrg, MrgStyle *style, 
                      float x, float y,
                      const char *string,
                      int utf8_len)
{
  float new_x, old_x;
  char *temp_string = NULL;
  Ctx *cr = mrg_cr (mrg);

  ctx_current_point (cr, &old_x, NULL);

  if (utf8_len < 0)
    utf8_len = ctx_utf8_strlen (string);

  if (ctx_utf8_strlen (string) != utf8_len)
  {
    const char *t;
    int i;

    temp_string = strdup (string);
    for (i = 0, t = temp_string ;i < utf8_len && *t; i++)
    {
      t += ctx_utf8_len (*t);
    }
    *(char *)t = 0;
    string = temp_string;
  }
#if 0
  if (mrg_is_terminal (mrg) && mrg_em (mrg) <= CPX * 4 / mrg->ddpx)
  {
    const char *t;
    int i;

    /* XXX: include transforms */
    int offset;
    double u = x , v = y;
    cairo_matrix_t matrix;
    cairo_get_matrix (mrg_cr (mrg), &matrix);
    cairo_matrix_transform_point (&matrix, &u, &v);

    //u = ctx_floorf(u);
    //v = ctx_floorf(v);
    
    offset = (int)(v/CPX) * ((int)(mrg->width/CPX) * 4) + (int)(u/CPX) * 4;

    old_x = x;
    for (i = 0, t = string; *t; i++)
    {
      if ( v >= 0 && u >= 0 &&
          (int)u/CPX < (int)(mrg->width/CPX) &&
          (int)v/CPX < (int)(mrg->height/CPX))
      {
        int styleno = offset/4;
        memcpy (&mrg->glyphs[offset], t, ctx_utf8_len (*t));
        mrg->styles[styleno] = mrg->state->fg +
                               mrg->state->bg * 8 +
                               (mrg->state->style.text_decoration & 
                                (MRG_BOLD|MRG_DIM|MRG_UNDERLINE|MRG_REVERSE)) * 64;;
      }
      t += ctx_utf8_len (*t);
      offset += 4;
      x += CPX / mrg->ddpx;
    }
    new_x = x;
  }
  else 
#endif
  if (mrg->in_paint)
  {
    ctx_font_size (cr, style->font_size);

    if (PROP(text_stroke_width) > 0.01)
    {
      CtxColor *color = ctx_color_new ();
      ctx_get_color (cr, CTX_text_stroke_color, color);
      mrg_ctx_set_source_color (cr, color);
      ctx_begin_path (cr);
      ctx_move_to   (cr, x, y - _mrg_text_shift (mrg));
      ctx_line_width (cr, PROP(text_stroke_width));
      ctx_line_join (cr, CTX_JOIN_ROUND);
      ctx_text_stroke (cr, string);
      ctx_color_free (color);
    }


    {
    CtxColor *color = ctx_color_new ();
    ctx_get_color (cr, CTX_color, color);
    mrg_ctx_set_source_color (cr, color);
    ctx_color_free (color);
    }
    ctx_move_to   (cr, x, y - _mrg_text_shift (mrg));
    ctx_current_point (cr, &old_x, NULL);

    /* when syntax highlighting,.. should do it as a coloring
     * directly here..
     */

    const char *syntax_highlight = PROPS(syntax_highlight);
    if (!syntax_highlight) syntax_highlight = "";

    if (syntax_highlight[0] == 0)
      ctx_text (cr, string);
    else if (!strcmp (syntax_highlight, "C"))
      mrg_hl_text (cr, string);
    else
      ctx_text (cr, string);

    ctx_current_point (cr, &new_x, NULL);

    if (style->text_decoration & MRG_UNDERLINE)
      {
        ctx_move_to (cr, old_x, y);
        ctx_line_to (cr, new_x, y);
        ctx_stroke (cr);
      }
    if (style->text_decoration & MRG_LINETHROUGH)
      {
        ctx_move_to (cr, old_x, y - style->font_size / 2);
        ctx_line_to (cr, new_x, y - style->font_size / 2);
        ctx_stroke (cr);
      }
    if (style->text_decoration & MRG_OVERLINE)
      {
        ctx_move_to (cr, old_x, y - style->font_size);
        ctx_line_to (cr, new_x, y - style->font_size);
        ctx_stroke (cr);
      }
    ctx_move_to (cr, new_x, y);
  }
  else
  {
    ctx_font_size (cr, style->font_size);
    new_x = old_x + ctx_text_width (cr, string);
  }

  if (mrg->text_listen_active)
  {
    float em = mrg_em (mrg);
    int no = mrg->text_listen_count-1;
    float x, y;

    ctx_current_point (cr, &x, &y);

    //fprintf (stderr, "[%s %f (%f)]\n", string, old_x, new_x-old_x+1);

    ctx_begin_path (cr);
    ctx_rectangle (cr,
        old_x, y - em, new_x - old_x + 1, em * mrg->state->style.line_height);
    ctx_listen (cr,
                mrg->text_listen_types[no],
                mrg->text_listen_cb[no],
                mrg->text_listen_data1[no],
                mrg->text_listen_data2[no]);
    ctx_begin_path (cr);
    ctx_move_to (cr, x, y);
  }

  if (temp_string)
    free (temp_string);

  return new_x - old_x;
}

float mrg_addstr (Mrg *mrg, float x, float y, const char *string, int utf8_length);

float paint_span_bg_final (Mrg   *mrg, float x, float y,
                           float  width)
{
  MrgStyle *style = mrg_style (mrg);
  Ctx *cr = mrg_cr (mrg);
  if (style->display != MRG_DISPLAY_INLINE)
    return 0.0;
  CtxColor *background_color = ctx_color_new ();
  ctx_get_color (cr, CTX_background_color, background_color);

  if (!ctx_color_is_transparent (background_color))
  {
    ctx_save (cr);
    ctx_rectangle (cr, x,
                         y - mrg_em (mrg) * style->line_height +_mrg_text_shift (mrg)
                         ,
                         width + PROP(padding_right),
                         mrg_em (mrg) * style->line_height);
    mrg_ctx_set_source_color (cr, background_color);
    ctx_fill (cr);
    ctx_restore (cr);
  }

  _mrg_border_top_r (mrg, x, y - mrg_em (mrg) , width, mrg_em (mrg));
  _mrg_border_bottom_r (mrg, x, y - mrg_em (mrg), width, mrg_em (mrg));
  _mrg_border_right (mrg, x, y - mrg_em (mrg), width, mrg_em (mrg));

  ctx_color_free (background_color);
  return PROP(padding_right) + PROP(border_right_width);
}

float paint_span_bg (Mrg   *mrg, float x, float y,
                     float  width)
{
  MrgStyle *style = mrg_style (mrg);
  Ctx *cr = mrg_cr (mrg);
  if (!cr)
    return 0.0;
  float left_pad = 0.0;
  float left_border = 0.0;
  if (style->display != MRG_DISPLAY_INLINE)
    return 0.0;

  CtxColor *background_color = ctx_color_new ();
  ctx_get_color (cr, CTX_background_color, background_color);

  if (!mrg->state->span_bg_started)
  {
    left_pad = PROP(padding_left);
    left_border = PROP(border_left_width);
    mrg->state->span_bg_started = 1;
  }

  if (!ctx_color_is_transparent (background_color))
  {
    ctx_save (cr);
    ctx_rectangle (cr, x + left_border,
                         y - mrg_em (mrg) * style->line_height +_mrg_text_shift (mrg)
                         ,
                         width + left_pad,
                         mrg_em (mrg) * style->line_height);
    mrg_ctx_set_source_color (cr, background_color);
    ctx_fill (cr);
    ctx_restore (cr);
  }

  if (left_pad || left_border)
  {
    _mrg_border_left (mrg, x + left_pad + left_border, y - mrg_em (mrg) , width, mrg_em (mrg));
    _mrg_border_top_l (mrg, x + left_pad + left_border, y - mrg_em (mrg) , width , mrg_em (mrg));
    _mrg_border_bottom_l (mrg, x + left_pad + left_border, y - mrg_em (mrg), width , mrg_em (mrg));
  }
  else
  {
    _mrg_border_top_m (mrg, x, y - mrg_em (mrg) , width, mrg_em (mrg));
    _mrg_border_bottom_m (mrg, x, y - mrg_em (mrg), width, mrg_em (mrg));
  }

  ctx_color_free (background_color);

  return left_pad + left_border;
}

float
mrg_addstr (Mrg *mrg, float x, float y, const char *string, int utf8_length)
{
  float wwidth = measure_word_width (mrg, string); //XXX get rid of some computation here
  float left_pad;
  left_pad = paint_span_bg (mrg, x, y, wwidth);

  {
    float tx = x;
    float ty = y;
    ctx_user_to_device (mrg_cr (mrg), &tx, &ty);
    if (ty > ctx_height (mrg->ctx) * 2 ||
        tx > ctx_width (mrg->ctx)* 2 ||
        tx < -ctx_width (mrg->ctx) * 2 ||
        ty < -ctx_height (mrg->ctx) * 2)
    {
      /* bailing early*/
    }
    else
    mrg_draw_string (mrg, &mrg->state->style, x + left_pad, y, string, utf8_length);
  }

  return wwidth + left_pad;
}

/******** end of core text-drawing primitives **********/

#if 0
void mrg_xy (Mrg *mrg, float x, float y)
{
  mrg->x = x * mrg_em (mrg);
  mrg->y = y * mrg_em (mrg);
}
#endif

void mrg_set_xy (Mrg *mrg, float x, float y)
{
  mrg->x = x;
  mrg->y = y;
  mrg->state->overflowed = 0;
}

float mrg_x (Mrg *mrg)
{
  return mrg->x;
}

float mrg_y (Mrg *mrg)
{
  return mrg->y;
}

void mrg_set_wrap_skip_lines (Mrg *mrg, int skip_lines);
void mrg_set_wrap_max_lines  (Mrg *mrg, int max_lines);

void mrg_set_wrap_skip_lines (Mrg *mrg, int skip_lines)
{
    mrg->state->skip_lines = skip_lines;
}

void mrg_set_wrap_max_lines  (Mrg *mrg, int max_lines)
{
    mrg->state->max_lines = max_lines;
}

//#define SNAP

static void _mrg_nl (Mrg *mrg)
{
  mrg->x = _mrg_dynamic_edge_left(mrg);
  mrg->y += mrg->state->style.line_height * mrg_em (mrg);
#ifdef SNAP
  float em = mrg_em (mrg);  /* XXX: a global body-line spacing 
                               snap is better grid design */
  mrg->x = ceil (mrg->x / em) * em;
  mrg->y = ceil (mrg->y / em) * em;
#endif

  if (mrg->y >= 
      mrg->state->edge_bottom - PROP(padding_bottom))
  {
    mrg->state->overflowed=1;
  }

  if (mrg->state->post_nl)
    mrg->state->post_nl (mrg, mrg->state->post_nl_data, 0);
}

static void _mrg_spaces (Mrg *mrg, int count)
{
  while (count--)
    {
     if (mrg->state->style.print_symbols)
        mrg->x+=mrg_addstr (mrg, mrg->x, mrg->y, "␣", -1);
     else
     {
        float diff = mrg_addstr (mrg, mrg->x, mrg->y, " ", 1);

#if 0
        if (mrg_is_terminal (mrg) && mrg_em (mrg) <= CPX * 4 / mrg->ddpx)
        {
        }
        else
#endif
        {
          if (mrg->state->style.text_decoration & MRG_REVERSE)
          {
            Ctx *cr = mrg_cr (mrg);
            ctx_rectangle (cr, mrg->x + diff*0.1, mrg->y + mrg_em(mrg)*0.2, diff*0.8, -mrg_em (mrg)*1.1);
            ctx_rgb (cr, 1,1,1);
            ctx_fill (cr);
          }
        }
        mrg->x += diff;
     }
    }
}

#define EMIT_NL() \
    do {wraps++; \
    if (wraps >= max_lines)\
      return wraps;\
    if (skip_lines-- <=0)\
      {\
         if (print) { if (gotspace)\
             _mrg_spaces (mrg, 1);\
         if (cursor_start == pos -1 && cursor_start>0 && mrg->text_edited)\
           {\
             mrg_start (mrg, ".cursor", NULL);\
             _mrg_spaces (mrg, 1);\
             _mrg_nl (mrg);\
             mrg_end (mrg);\
           }\
         else\
           _mrg_nl (mrg);\
         } else _mrg_nl (mrg);\
      }\
    if (skip_lines<=0)\
      mrg_set_xy (mrg, _mrg_dynamic_edge_left(mrg), mrg_y (mrg));}while(0)

#define EMIT_NL2() \
    do {\
    if (skip_lines-- <=0)\
      {\
         if (print) {if (gotspace)\
             _mrg_spaces (mrg, 1);\
         if (cursor_start == *pos -1 && cursor_start>0 && mrg->text_edited)\
           {\
             mrg_start (mrg, ".cursor", NULL);\
             _mrg_spaces (mrg, 1);\
             _mrg_nl (mrg);\
             mrg_end (mrg);\
           }\
         else\
           _mrg_nl (mrg);\
         } else _mrg_nl (mrg);\
      }\
    if (skip_lines<=0)\
      mrg_set_xy (mrg, _mrg_dynamic_edge_left(mrg), mrg_y (mrg));}while(0)



static void mrg_get_edit_state (Mrg *mrg, 
     float *x, float *y, float *s, float *e,
     float *em_size)
{
  if (x) *x = mrg->e_x;
  if (y) *y = mrg->e_y;
  if (s) *s = mrg->e_ws;
  if (e) *e = mrg->e_we;
  if (em_size) *em_size = mrg->e_em;
}


static void emit_word (Mrg *mrg,
                       int  print,
                       const char *data,
                       const char *word,
                       int         max_lines,
                       int         skip_lines,
                       int         cursor_start,
                       int        *pos,
                       int        *wraps,
                       int        *wl,
                       int         c,
                       int         gotspace)
{
    float len = ctx_utf8_strlen (word);
    float wwidth = measure_word_width (mrg, word);

    if (mrg->x + wwidth >= _mrg_dynamic_edge_right (mrg))
    {
      if (mrg->x > mrg_edge_left(mrg) || *wraps != 0)
      {
        EMIT_NL2();
      }
    }

    if (mrg->x != mrg_edge_left(mrg) && gotspace)
      { 
        if ((skip_lines<=0)) 
          { 
            if (cursor_start == *pos-1 && cursor_start>=0 && mrg->text_edited)
            { 
              if (print) { 
               mrg_start (mrg, ".cursor", NULL);
               _mrg_spaces (mrg, 1); 
               mrg_end (mrg);
              } else { 
               mrg->x += measure_word_width (mrg, " ");
              }
            }
            else 
              {
                if (print){
                  if (mrg->state->style.print_symbols)
                    {
                      mrg_start (mrg, "dim", NULL);
                      mrg->x += mrg_addstr (mrg, mrg->x, mrg->y, "␣", -1);
                      mrg_end (mrg);
                    }
                  else
                    _mrg_spaces (mrg, 1);
                } else {
                  if (mrg->state->style.print_symbols)
                  {
                    mrg->x += measure_word_width (mrg, "␣");
                  }
                  else
                  {
                    mrg->x += measure_word_width (mrg, " ");
                  }
                }
              } 
          }
      } 
    if ((skip_lines<=0)) {
      if (print){if (cursor_start >= *pos && *pos + len > cursor_start && mrg->text_edited)
        { 
#if 0  // XXX: there is a bug in mrg_addstr it doesn't respect the length argument 
          mrg->x += mrg_addstr (mrg, mrg->x, mrg->y, word, cursor_start - *pos);
          mrg_start (mrg, ".cursor", NULL);
          mrg->x += mrg_addstr (mrg, mrg->x, mrg->y, mrg_utf8_skip (word, cursor_start - *pos), 1);
          mrg_end (mrg);
          mrg->x += mrg_addstr (mrg, mrg->x, mrg->y, mrg_utf8_skip (word, cursor_start - *pos + 1), len - (cursor_start - *pos) - 1);
#else

          char *dup, *dup2, *dup3;

          dup = strdup (word);
          dup2 = strdup (ctx_utf8_skip (dup, cursor_start - *pos));
          dup3 = strdup (ctx_utf8_skip (dup, cursor_start - *pos + 1));
          *((char*)ctx_utf8_skip (dup,  cursor_start - *pos)) = 0;
          *((char*)ctx_utf8_skip (dup2, 1)) = 0;

          mrg->x += mrg_addstr (mrg, mrg->x, mrg->y, dup, -1);
          mrg_start (mrg, ".cursor", NULL);
          mrg->x += mrg_addstr (mrg, mrg->x, mrg->y, dup2, -1);
          mrg_end (mrg);
          mrg->x += mrg_addstr (mrg, mrg->x, mrg->y, dup3, -1);

          free (dup);
          free (dup2);
          free (dup3);
#endif
        }
      else
        {
          mrg->x += mrg_addstr (mrg, mrg->x, mrg->y, word, len); 
        }
      } else {
          mrg->x += wwidth;
      }
    }
    *pos += len;
    *wl = 0;

}

static int mrg_print_wrap (Mrg        *mrg,
                           int         print,
                           const char *data, int length,
                           int         max_lines,
                           int         skip_lines,
                           int         cursor_start,
                           float     *retx,
                           float     *rety)
{
  char word[400]="";
  int wl = 0;
  int c;
  int wraps = 0;
  int pos;
  int gotspace = 0;

  if (mrg->state->overflowed)
  {
    return 0;
  }

  pos = 0;

  if (max_lines <= 0)
    max_lines = 4096;
  if (retx)
    *retx = -1;

  if (mrg->text_edited && print)
    {
      mrg->e_x = mrg->x;
      mrg->e_y = mrg->y;
      mrg->e_ws = mrg_edge_left(mrg);
      mrg->e_we = mrg_edge_right(mrg);
      mrg->e_em = mrg_em (mrg);
#if 0
      if (mrg->scaled_font)
        cairo_scaled_font_destroy (mrg->scaled_font);
#endif
      ctx_font_size (mrg_cr (mrg), mrg_style(mrg)->font_size);
      //mrg->scaled_font = cairo_get_scaled_font (mrg_cr (mrg));
      //cairo_scaled_font_reference (mrg->scaled_font);
    }

  for (c = 0 ; c < length && data[c] && ! mrg->state->overflowed; c++)
    switch (data[c])
      {
        case '\n':
          if (wl)
            {
              emit_word (mrg, print, data, word, 
                         max_lines, skip_lines,
                         cursor_start,
                         &pos, &wraps, &wl, c, gotspace);
            }
          pos++;

          if (mrg->state->style.print_symbols && print)
          {
            mrg_start (mrg, "dim", NULL);
            mrg->x+=mrg_addstr (mrg, mrg->x, mrg->y, "¶", -1);\
            mrg_end (mrg);
          }
          EMIT_NL();
          gotspace = 0;
          break;
        case '\t': // XXX: this collapses tabs to a single space
        case ' ':
          if (wl == 0)
            {
              if (cursor_start == pos-1 && cursor_start>=0 && mrg->text_edited)
                {
                  if (print)
                  {
                    mrg_start (mrg, ".cursor", NULL);
                    _mrg_spaces (mrg, 1);
                    mrg_end (mrg);
                  }
                  else
                    mrg->x+=mrg_addstr (mrg, mrg->x, mrg->y, " ", -1);
                }
              else
                {
                  if (mrg->state->style.print_symbols)
                    {
                      mrg_start (mrg, "dim", NULL);
                      mrg->x+=mrg_addstr (mrg, mrg->x, mrg->y, "␣", -1);
                      mrg_end (mrg);
                    }
                  else
                    {
                      mrg->x+=mrg_addstr (mrg, mrg->x, mrg->y, " ", -1);
                    }
                }
            }
          else
            {
              emit_word (mrg, print, data, word,
                         max_lines, skip_lines,
                         cursor_start,
                         &pos, &wraps, &wl, c, gotspace);
            }
          pos++;

          if (retx && *retx < 0 && pos >= cursor_start)
            {
              float tailwidth;
              const char *rest = &word[ctx_utf8_strlen (word) - (pos-cursor_start)];
#if 0
              if (mrg_is_terminal (mrg))
                tailwidth = (pos-cursor_start -1) * CPX / mrg->ddpx;
              else
#endif
                tailwidth = measure_word_width (mrg, rest);
              *retx = mrg->x - tailwidth;
              *rety = mrg->y;
              return pos;
            }
          gotspace = 1;
          break;
        default:
          word[wl++]= data[c];
          word[wl]  = '\0';
          break;
      }
  if (wl) /* orphaned word for last line. */
    {
      emit_word (mrg, print, data, word, 
                 max_lines, skip_lines,
                 cursor_start,
                 &pos, &wraps, &wl, c, gotspace);
    }
   /* cursor at end */
   if (cursor_start == pos && cursor_start>=0 && mrg->text_edited)
    {
      if (print)
      {
        if (c && data[c-1]==' ')
          mrg->x += measure_word_width (mrg, " ");
        mrg_start (mrg, ".cursor", NULL);
        _mrg_spaces (mrg, 1);
        mrg_end (mrg);
      }
      else
        mrg->x += measure_word_width (mrg, " ");
    }
  if (retx && *retx < 0 && pos >= cursor_start)
    {
       *retx = mrg->x; 
       *rety = mrg->y;
      return pos;
    }
  return wraps;
}

int mrg_print_get_xy (Mrg *mrg, const char *string, int no, float *x, float *y)
{
  int ret;
  if (!string)
    return 0;

  if (mrg_edge_left(mrg) != mrg_edge_right(mrg))
    {
      float ox, oy;
      ox = mrg->x;
      oy = mrg->y;
      ret = mrg_print_wrap (mrg, 0, string, strlen (string), mrg->state->max_lines,
                             mrg->state->skip_lines, no, x, y);
      mrg->x = ox;
      mrg->y = oy;
      return ret;
    }
  if (y) *y = mrg->y;
  if (x) *x = mrg->x + no; // XXX: only correct for nct/monospace

  return 0;
}

typedef struct _MrgGlyph MrgGlyph;

struct _MrgGlyph{
  unsigned long index; /*  done this way, the remnants of layout; before feeding
                        *  glyphs positions in cairo, similar to how pango would do
                        *  can be reused for computing the caret nav efficiently.
                        */
  float x;
  float y;
  int   no;
};

static int mrg_print_wrap2 (Mrg        *mrg,
                           int         print,
                           const char *data, int length,
                           int         max_lines,
                           int         skip_lines,
                           CtxList   **list)
{
  char word[400]="";
  int wl = 0;
  int c;
  int wraps = 0;
  int pos;
  int gotspace = 0;
  int cursor_start = -1;

  MrgGlyph *g = calloc (sizeof (MrgGlyph), 1);
  g->x = length;
  g->y = 42;
  g->index = 44;
  g->no = 2;
  ctx_list_append (list, g);

  if (mrg->state->overflowed)
  {
    return 0;
  }

  pos = 0;

  if (max_lines <= 0)
    max_lines = 4096;

  if (mrg->text_edited && print)
    {
      mrg->e_x = mrg->x;
      mrg->e_y = mrg->y;
      mrg->e_ws = mrg_edge_left(mrg);
      mrg->e_we = mrg_edge_right(mrg);
      mrg->e_em = mrg_em (mrg);
#if 0
      if (mrg->scaled_font)
        cairo_scaled_font_destroy (mrg->scaled_font);
#endif
      ctx_font_size (mrg_cr (mrg), mrg_style(mrg)->font_size);
#if 0
      mrg->scaled_font = cairo_get_scaled_font (mrg_cr (mrg));
      cairo_scaled_font_reference (mrg->scaled_font);
#endif
    }

  for (c = 0 ; c < length && data[c] && ! mrg->state->overflowed; c++)
    switch (data[c])
      {
        case '\n':
          if (wl)
            {
              emit_word (mrg, print, data, word, 
                         max_lines, skip_lines,
                         cursor_start,
                         &pos, &wraps, &wl, c, gotspace);
            }
          pos++;

          if (mrg->state->style.print_symbols && print)
          {
            mrg_start (mrg, "dim", NULL);
            mrg->x+=mrg_addstr (mrg, mrg->x, mrg->y, "¶", -1);\
            mrg_end (mrg);
          }
          EMIT_NL();
          gotspace = 0;
          break;
        case ' ':
          if (wl == 0)
            {
              if (cursor_start == pos-1 && cursor_start>=0 && mrg->text_edited)
                {
                  if (print)
                  {
                    mrg_start (mrg, ".cursor", NULL);
                    _mrg_spaces (mrg, 1);
                    mrg_end (mrg);
                  }
                  else
                    mrg->x+=mrg_addstr (mrg, mrg->x, mrg->y, " ", -1);
                }
              else
                {
                  if (mrg->state->style.print_symbols)
                    {
                      mrg_start (mrg, "dim", NULL);
                      mrg->x+=mrg_addstr (mrg, mrg->x, mrg->y, "␣", -1);
                      mrg_end (mrg);
                    }
                  else
                    {
                      mrg->x+=mrg_addstr (mrg, mrg->x, mrg->y, " ", -1);
                    }
                }
            }
          else
            {
              emit_word (mrg, print, data, word, 
                         max_lines, skip_lines,
                         cursor_start,
                         &pos, &wraps, &wl, c, gotspace);
            }
          pos++;
          
#if 0
          if (retx && *retx < 0 && pos >= cursor_start)
            {
              float tailwidth;
              const char *rest = &word[ctx_utf8_strlen (word) - (pos-cursor_start)];
              if (mrg_is_terminal (mrg))
                tailwidth = (pos-cursor_start -1) * CPX / mrg->ddpx;
              else
                tailwidth = measure_word_width (mrg, rest);
              *retx = mrg->x - tailwidth;
              *rety = mrg->y;
              return pos;
            }
#endif
          gotspace = 1;
          break;
        default:
          word[wl++]= data[c];
          word[wl]  = '\0';
          break;
      }
  if (wl) /* orphaned word for last line. */
    {
      emit_word (mrg, print, data, word, 
                 max_lines, skip_lines,
                 cursor_start, 
                 &pos, &wraps, &wl, c, gotspace);
    }
   /* cursor at end */
   if (cursor_start == pos && cursor_start>=0 && mrg->text_edited)
    {
      if (print)
      {
        mrg_start (mrg, ".cursor", NULL);
        _mrg_spaces (mrg, 1);
        mrg_end (mrg);
      }
      else
        mrg->x += measure_word_width (mrg, " ");
    }
#if 0
  if (retx && *retx < 0 && pos >= cursor_start)
    {
       *retx = mrg->x; 
       *rety = mrg->y;
      return pos;
    }
#endif
  return wraps;
}

CtxList *mrg_print_get_coords (Mrg *mrg, const char *string)
{
  CtxList *ret = NULL;
  if (!string)
    return ret;

  if (mrg_edge_left(mrg) != mrg_edge_right(mrg))
    {
      float ox, oy;
      ox = mrg->x;
      oy = mrg->y;
      mrg_print_wrap2 (mrg, 0, string, strlen (string), mrg->state->max_lines,
                       mrg->state->skip_lines, &ret);
      mrg->x = ox;
      mrg->y = oy;
      return ret;
    }

  return ret;
}

#include <math.h>

int mrg_print (Mrg *mrg, const char *string)
{
  float ret;
  MrgStyle *style = mrg_style (mrg);

#ifdef SNAP
  float em = mrg_em (mrg);  /* XXX: a global body-line spacing 
                               snap is better grid design */
  mrg->x = ceil (mrg->x / em) * em;
  mrg->y = ceil (mrg->y / em) * em;
#endif

  if (mrg->text_edited)
    mrg_string_append_str (mrg->edited_str, string);

  if (style->display == MRG_DISPLAY_NONE)
    return 0.0;

  if (!string)
    return 0;

  if (mrg_edge_left(mrg) != mrg_edge_right(mrg))
   return mrg_print_wrap (mrg, 1, string, strlen (string), mrg->state->max_lines, mrg->state->skip_lines, mrg->cursor_pos, NULL, NULL);

  ret  = mrg_addstr (mrg, mrg->x, mrg->y, string, ctx_utf8_strlen (string));
  mrg->x += ret;
  return ret;
}

void _mrg_text_prepare (Mrg *mrg)
{
  hl_state_c = MRG_HL_NEUTRAL;
}

void _mrg_text_init (Mrg *mrg)
{
  // XXX: this should be done in a prepre,.. not an init?
  //
  mrg->state->style.line_height = 1.0;
  mrg->state->style.print_symbols = 0;
}

void  mrg_text_listen_done (Mrg *mrg)
{
  mrg->text_listen_active = 0;
}

void  mrg_text_listen_full (Mrg *mrg, CtxEventType types,
                            CtxCb cb, void *data1, void *data2,
                      void   (*finalize)(void *listen_data, void *listen_data2, void *finalize_data),
                      void    *finalize_data)
{
  int no = mrg->text_listen_count;
  if (cb == NULL)
  {
    mrg_text_listen_done (mrg);
    return;
  }
  if (no + 1 >= MRG_MAX_TEXT_LISTEN)
  {
    fprintf (stderr, "mrg text listen overflow\n");
    return;
  }

  mrg->text_listen_types[no] = types;
  mrg->text_listen_cb[no] = cb;
  mrg->text_listen_data1[no] = data1;
  mrg->text_listen_data2[no] = data2;
  mrg->text_listen_finalize[no] = finalize;
  mrg->text_listen_finalize_data[no] = finalize_data;
  mrg->text_listen_count++;
  mrg->text_listen_active = 1;
}

void  mrg_text_listen (Mrg *mrg, CtxEventType types,
                       CtxCb cb, void *data1, void *data2)
{
  mrg_text_listen_full (mrg, types, cb, data1, data2, NULL, NULL);
}


static void cmd_home (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  mrg->cursor_pos = 0;
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_end (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  mrg->cursor_pos = ctx_utf8_strlen (mrg->edited_str->str);
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_backspace (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  char *new;
  const char *rest = ctx_utf8_skip (mrg->edited_str->str, mrg->cursor_pos);
  const char *mark = ctx_utf8_skip (mrg->edited_str->str, mrg->cursor_pos-1);

  if (mrg->cursor_pos <= 0)
    {
      mrg->cursor_pos = 0;
    }
  else
    {
      new = malloc (strlen (mrg->edited_str->str) + 1);
      memcpy (new, mrg->edited_str->str, ((mark - mrg->edited_str->str)));
      memcpy (new + ((mark - mrg->edited_str->str)), rest, strlen (rest));
      new [strlen (mrg->edited_str->str)-(rest-mark)] = 0;
      mrg->update_string (new, mrg->update_string_user_data);
      mrg_string_set (mrg->edited_str, new);
      free (new);
      mrg->cursor_pos--;
    }
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_delete (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  char *new;
  const char *rest = ctx_utf8_skip (mrg->edited_str->str, mrg->cursor_pos+1);
  const char *mark = ctx_utf8_skip (mrg->edited_str->str, mrg->cursor_pos);

  new = malloc (strlen (mrg->edited_str->str) + 1);
  memcpy (new, mrg->edited_str->str, ((mark - mrg->edited_str->str)));
  memcpy (new + ((mark - mrg->edited_str->str)), rest, strlen (rest));
  new [strlen (mrg->edited_str->str)-(rest-mark)] = 0;

  mrg->update_string (new, mrg->update_string_user_data);
  mrg_string_set (mrg->edited_str, new);
  free (new);
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_down (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  float e_x, e_y, e_s, e_e, e_em;
  float cx, cy;
  cx = cy = 0;
 
  mrg_get_edit_state (mrg, &e_x, &e_y, &e_s, &e_e, &e_em);
  mrg_set_edge_left (mrg, e_s - PROP (padding_left));
  mrg_set_edge_right (mrg, e_e + PROP (padding_right));
  mrg_set_xy (mrg, e_x, e_y);
  mrg_print_get_xy (mrg, mrg->edited_str->str, mrg->cursor_pos, &cx, &cy);

  {
    int no;
    int best = mrg->cursor_pos;
    float best_score = 10000000000.0;
    float best_y = cy;
    int strl = ctx_utf8_strlen (mrg->edited_str->str);
    for (no = mrg->cursor_pos + 1; no < mrg->cursor_pos + 256 && no < strl; no++)
    {
      float x = 0, y = 0;
      float attempt_score = 0.0;
      mrg_set_xy (mrg, e_x, e_y);
      mrg_print_get_xy (mrg, mrg->edited_str->str, no, &x, &y);

      if (y > cy && best_y == cy)
        best_y = y;

      if (y > cy)
        attempt_score = (y - best_y);
      else
        attempt_score = 1000.0;

      attempt_score += fabs(cx-x) / 10000000.0;

      if (attempt_score <= best_score)
      {
        best_score = attempt_score;
        best = no;
      }
    }
    if (best_y == cy)
    {
      mrg->cursor_pos = strl;
#if 0
      ctx_key_press (mrg, 0, "down-nudge", 0);
#endif
      mrg_queue_draw (mrg, NULL);
      return;
    }
    mrg->cursor_pos = best;
  }

  if (mrg->cursor_pos >= ctx_utf8_strlen (mrg->edited_str->str))
    mrg->cursor_pos = ctx_utf8_strlen (mrg->edited_str->str) - 1;
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_up (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  float e_x, e_y, e_s, e_e, e_em;
  float cx = 0.0f, cy = 0.0f;
  mrg_get_edit_state (mrg, &e_x, &e_y, &e_s, &e_e, &e_em);

  mrg_set_edge_left  (mrg, e_s - PROP(padding_left));
  mrg_set_edge_right (mrg, e_e + PROP(padding_right));

  mrg_set_xy (mrg, e_x, e_y);
  mrg_print_get_xy (mrg, mrg->edited_str->str, mrg->cursor_pos, &cx, &cy);

  /* XXX: abstract the finding of best cursor pos for x coord to a function */
  {
    int no;
    int best = mrg->cursor_pos;
    float best_y = cy;
    float best_score = 1000000000000.0;
    for (no = mrg->cursor_pos - 1; no>= mrg->cursor_pos - 256 && no > 0; no--)
    {
      float x = 0, y = 0;
      float attempt_score = 0.0;
      mrg_set_xy (mrg, e_x, e_y);
      mrg_print_get_xy (mrg, mrg->edited_str->str, no, &x, &y);

      if (y < cy && best_y == cy)
        best_y = y;

      if (y < cy)
        attempt_score = (best_y - y);
      else
        attempt_score = 1000.0;

      attempt_score += fabs(cx-x) / 10000000.0;

      if (attempt_score < best_score)
      {
        best_score = attempt_score;
        best = no;
      }
    }
    mrg->cursor_pos = best;
    if (best_y == cy)
    {
      mrg->cursor_pos = 0;
      mrg_queue_draw (mrg, NULL);
      ctx_key_press (event->ctx, 0, "up-nudge", 0);
      return; // without stop propagate this should permit things registered earlier to fire
    }
  }

  if (mrg->cursor_pos < 0)
    mrg->cursor_pos = 0;
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

int mrg_get_cursor_pos (Mrg *mrg)
{
  return mrg->cursor_pos;
}

void mrg_set_cursor_pos (Mrg *mrg, int pos)
{
  mrg->cursor_pos = pos;
  mrg_queue_draw (mrg, NULL);
}

static void cmd_page_down (CtxEvent *event, void *data1, void *data2)
{
  int i;
  for (i = 0; i < 6; i++)
    cmd_down (event, data1, data2);
  ctx_event_stop_propagate (event);
}

static void cmd_page_up (CtxEvent *event, void *data1, void *data2)
{
  int i;
  for (i = 0; i < 6; i++)
    cmd_up (event, data1, data2);
  ctx_event_stop_propagate (event);
}

static void cmd_left (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  mrg->cursor_pos--;
  if (mrg->cursor_pos < 0)
    mrg->cursor_pos = 0;
  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}

static void cmd_right (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  mrg->cursor_pos++;

  /* should mrg have captured the text printed in-between to build its idea
   * of what is being edited, thus being able to do its own internal cursor
   * positioning with that cache?
   */

  if (mrg->cursor_pos > ctx_utf8_strlen (mrg->edited_str->str))
    mrg->cursor_pos = ctx_utf8_strlen (mrg->edited_str->str);

  mrg_queue_draw (mrg, NULL);
  ctx_event_stop_propagate (event);
}


/* the added utf8 bits go to edited_str as well, so that successive edits do work out
 *
 */

static void add_utf8 (Mrg *mrg, const char *string)
{
  char *new;
  const char *rest;
  /* XXX: this is the code the should be turned into a callback/event
   * to digest for the user of the framework, with a reasonable default
   * for using it from C with a string
   */

  rest = ctx_utf8_skip (mrg->edited_str->str, mrg->cursor_pos);

  new = malloc (strlen (mrg->edited_str->str) + strlen (string) + 1);
  memcpy (new, mrg->edited_str->str, (rest-mrg->edited_str->str));
  memcpy (new + (rest-mrg->edited_str->str), string,  strlen (string));
  memcpy (new + (rest-mrg->edited_str->str) + strlen (string),
          rest, strlen (rest));
  new [strlen (string) + strlen (mrg->edited_str->str)] = 0;
  mrg->update_string (new, mrg->update_string_user_data);
  mrg_string_set (mrg->edited_str, new);
  free (new);
  mrg_queue_draw (mrg, NULL);
  mrg->cursor_pos++;
}

static void cmd_unhandled (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  if (!strcmp (event->string, "space"))
  {
    add_utf8 (mrg, " ");
    ctx_event_stop_propagate (event);
  }

  if (ctx_utf8_strlen (event->string) != 1)
    return;

  add_utf8 (mrg, event->string);
  ctx_event_stop_propagate (event);
}

#if 0
static void cmd_space (CtxEvent *event, void *data1, void *data2)
{
  if (!ctx_utf8_strlen (event->key_name) == 1)
    return 0;

  add_utf8 (event->mrg, " ");
  return 1;
}
#endif

static void cmd_return (CtxEvent *event, void *data1, void *data2)
{
  Mrg *mrg = data1;
  // this check excludes terminal from working
  //if (!(ctx_utf8_strlen (event->key_name) == 1))
  //  return;

  add_utf8 (mrg, "\n");
  ctx_event_stop_propagate (event);
}

static void cmd_escape (CtxEvent *event, void *data, void *data2)
{
#if 0
  mrg_edit_string (event->mrg, NULL, NULL, NULL);
#endif
}

void mrg_text_edit_bindings (Mrg *mrg)
{
  ctx_add_key_binding (mrg->ctx, "escape",    NULL, "stop editing",    cmd_escape,      mrg);
  ctx_add_key_binding (mrg->ctx, "return",    NULL, "add newline",     cmd_return,    mrg);
  ctx_add_key_binding (mrg->ctx, "home",      NULL, "cursor to start", cmd_home, mrg);
  ctx_add_key_binding (mrg->ctx, "end",       NULL, "cursor to end",   cmd_end,    mrg);
  ctx_add_key_binding (mrg->ctx, "left",      NULL, "cursor left",     cmd_left,    mrg);
  ctx_add_key_binding (mrg->ctx, "right",     NULL, "cursor right",    cmd_right,  mrg);
  ctx_add_key_binding (mrg->ctx, "up",        NULL, "cursor up",       cmd_up,        mrg);
  ctx_add_key_binding (mrg->ctx, "down",      NULL, "cursor down",     cmd_down,    mrg);
  ctx_add_key_binding (mrg->ctx, "page-up",   NULL, "cursor up",       cmd_page_up,     mrg);
  ctx_add_key_binding (mrg->ctx, "page-down", NULL, "cursor down",     cmd_page_down, mrg);
  ctx_add_key_binding (mrg->ctx, "backspace", NULL, "remove preceding character", cmd_backspace, mrg);
  ctx_add_key_binding (mrg->ctx, "delete",    NULL, "remove character under cursor", cmd_delete, mrg);
  ctx_add_key_binding (mrg->ctx, "unhandled", NULL, "add if key name is 1 char long", cmd_unhandled, mrg);
}

#if 1
void mrg_edit_string (Mrg *mrg, char **string,
                      void (*update_string)(Mrg *mrg,
                        char **string_loc,
                        const char *new_string,
                        void  *user_data),
                      void *user_data)
{
  if (mrg->edited == string)
    return;
  mrg->edited = string;
  mrg->update_string = (void*)update_string;
  mrg->update_string_user_data = user_data;
  if (string)
    mrg->cursor_pos = ctx_utf8_strlen (*string);
  else
    mrg->cursor_pos = 0;
  mrg_queue_draw (mrg, NULL);
}
#endif

void
mrg_printf (Mrg *mrg, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  mrg_print (mrg, buffer);
  free (buffer);
}

void  mrg_set_font_size   (Mrg *mrg, float size)
{
    mrg_set_stylef (mrg, "font-size:%fpx;", size);
}

void _mrg_block_edit (Mrg *mrg)
{
  mrg->text_edit_blocked = 1;
}
void _mrg_unblock_edit (Mrg *mrg)
{
  mrg->text_edit_blocked = 0;
}

void mrg_edit_start_full (Mrg *mrg,
                          MrgNewText  update_string,
                          void *user_data,
                          CtxDestroyNotify destroy,
                          void *destroy_data)
{
  if (mrg->update_string_destroy_notify)
  {
    mrg->update_string_destroy_notify (mrg->update_string_destroy_data);
  }
  mrg->got_edit                     = 1;
  mrg->text_edited                  = 1;
  mrg->update_string                = update_string;
  mrg->update_string_user_data      = user_data;
  mrg->update_string_destroy_notify = destroy;
  mrg->update_string_destroy_data   = destroy_data;
}

void  mrg_edit_start (Mrg *mrg,
                      MrgNewText  update_string,
                      void *user_data)
{
  return mrg_edit_start_full (mrg, update_string, user_data, NULL, NULL);
}

void  mrg_edit_end (Mrg *mrg)
{
  mrg->text_edited = 0;
  mrg_text_edit_bindings (mrg);
}

void _mrg_layout_pre (Mrg *mrg, MrgHtml *html)
{
  MrgStyle *style;
  float dynamic_edge_left, dynamic_edge_right;

  html->state_no++;
  html->state = &html->states[html->state_no];
  *html->state = html->states[html->state_no-1];

  style = mrg_style (mrg);

  html->state->original_x = mrg_x (mrg);
  html->state->original_y = mrg_y (mrg);

  if (html->state_no)
  {
    dynamic_edge_right = _mrg_parent_dynamic_edge_right(html);
    dynamic_edge_left = _mrg_parent_dynamic_edge_left(html);
  }
  else
  {
    dynamic_edge_right = mrg_edge_right(mrg);
    dynamic_edge_left = mrg_edge_left(mrg);
  }

  if (style->clear & MRG_CLEAR_RIGHT)
    clear_right (html);
  if (style->clear & MRG_CLEAR_LEFT)
    clear_left (html);

  if (style->display == MRG_DISPLAY_BLOCK ||
      style->display == MRG_DISPLAY_LIST_ITEM)
  {
    if (PROP(padding_left) + PROP(margin_left) + PROP(border_left_width)
        != 0)
    {
      mrg_set_edge_left (mrg, mrg_edge_left (mrg) +
        PROP(padding_left) + PROP(margin_left) + PROP(border_left_width));
    }
    if (PROP(padding_right) + PROP(margin_right) + PROP(border_right_width)
        != 0)
    {
      mrg_set_edge_right (mrg, mrg_edge_right (mrg) -
        (PROP(padding_right) + PROP(margin_right) + PROP(border_right_width)));
    }

    if (PROP(margin_top) > html->state->vmarg)
    mrg_set_edge_top (mrg, mrg_y (mrg) + PROP(border_top_width) + (PROP(margin_top) - html->state->vmarg));
    else
    {
      /* XXX: just ignoring vmarg when top-margin is negative? */
      mrg_set_edge_top (mrg, mrg_y (mrg) + PROP(border_top_width) + (PROP(margin_top)));
    }

    html->state->block_start_x = mrg_edge_left (mrg);
    html->state->block_start_y = mrg_y (mrg);
  }

  if (style->display == MRG_DISPLAY_LIST_ITEM)
  {
    float x = mrg->x;
    _mrg_draw_background_increment (mrg, html, 0);
    mrg->x -= mrg_em (mrg) * 1;
    mrg_print (mrg, "•"); //⚫"); //●");
    mrg->x = x;
  }

  switch (style->position)
  {
    case MRG_POSITION_RELATIVE:
      /* XXX: deal with style->right and style->bottom */
      ctx_translate (mrg_cr (mrg), PROP(left), PROP(top));
      /* fallthrough */

    case MRG_POSITION_STATIC:

      if (style->float_ == MRG_FLOAT_RIGHT)
      {
        float width = PROP(width);

        if (width == 0.0)
        {
          MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
          if (geo->width)
            width = geo->width;
          else
            width = mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

        width = (width + PROP(padding_right) + PROP(padding_left) + PROP(border_left_width) + PROP(border_right_width));


        if (width + PROP(margin_left) + PROP(margin_right) >
            mrg_edge_right(mrg)-mrg_edge_left(mrg))
        {
          clear_both (html);
          mrg_set_edge_left (mrg, mrg_edge_right (mrg) - width);
          mrg_set_edge_right (mrg, mrg_edge_right (mrg) - (PROP(margin_right) + PROP(padding_right) + PROP(border_right_width)));

        }
        else
        {
        while (dynamic_edge_right - dynamic_edge_left < width + PROP(margin_left) + PROP(margin_right))
        {
          mrg_set_xy (mrg, mrg_x (mrg), mrg_y (mrg) + 1.0);
          dynamic_edge_right = _mrg_parent_dynamic_edge_right(html);
          dynamic_edge_left = _mrg_parent_dynamic_edge_left(html);
        }

        mrg_set_edge_left (mrg, dynamic_edge_right - width);
        mrg_set_edge_right (mrg, dynamic_edge_right - (PROP(margin_right) + PROP(padding_right) + PROP(border_right_width)));

        }

        mrg_set_edge_top (mrg, mrg_y (mrg) + (PROP(margin_top) - html->state->vmarg) - mrg_em(mrg));

        html->state->block_start_x = mrg_x (mrg);
        html->state->block_start_y = mrg_y (mrg);
        html->state->floats = 0;

      } else if (style->float_ == MRG_FLOAT_LEFT)
      {
        float left, y;

        float width = PROP(width);

        if (width == 0.0)
        {
          MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
          if (geo->width)
            width = geo->width;
          else
            width = 4 * mrg_em (mrg);//mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

        width = (width + PROP(padding_right) + PROP(padding_left) + PROP(border_left_width) + PROP(border_right_width));

        if (width + PROP(margin_left) + PROP(margin_right) >
            mrg_edge_right(mrg)-mrg_edge_left(mrg))
        {
          clear_both (html);
          left = mrg_edge_left (mrg) + PROP(padding_left) + PROP(border_left_width) + PROP(margin_left);
        }
        else
        {
        while (dynamic_edge_right - dynamic_edge_left < width + PROP(margin_left) + PROP(margin_right))
        {
          mrg_set_xy (mrg, mrg_x (mrg), mrg_y (mrg) + 1.0);
          dynamic_edge_right = _mrg_parent_dynamic_edge_right(html);
          dynamic_edge_left = _mrg_parent_dynamic_edge_left(html);
        }
          left = dynamic_edge_left + PROP(padding_left) + PROP(border_left_width) + PROP(margin_left);
        }

        y = mrg_y (mrg);

        mrg_set_edge_left (mrg, left);
        mrg_set_edge_right (mrg,  left + width +
            PROP(padding_left) + PROP(border_right_width));
        mrg_set_edge_top (mrg, mrg_y (mrg) + (PROP(margin_top) - html->state->vmarg) - mrg_em(mrg));
        html->state->block_start_x = mrg_x (mrg);
        html->state->block_start_y = y - style->font_size + PROP(padding_top) + PROP(border_top_width);
        html->state->floats = 0;

        /* change cursor point after floating something left; if pushed far
         * down, the new correct
         */
        mrg_set_xy (mrg, html->state->original_x = left + width + PROP(padding_left) + PROP(border_right_width) + PROP(padding_right) + PROP(margin_right) + PROP(margin_left) + PROP(border_left_width),
            y - style->font_size + PROP(padding_top) + PROP(border_top_width));
      } /* XXX: maybe spot for */
      break;
    case MRG_POSITION_ABSOLUTE:
      {
        html->state->floats = 0;
        mrg_set_edge_left (mrg, PROP(left) + PROP(margin_left) + PROP(border_left_width) + PROP(padding_left));
        mrg_set_edge_right (mrg, PROP(left) + PROP(width));
        mrg_set_edge_top (mrg, PROP(top) + PROP(margin_top) + PROP(border_top_width) + PROP(padding_top));
        html->state->block_start_x = mrg_x (mrg);
        html->state->block_start_y = mrg_y (mrg);
      }
      break;
    case MRG_POSITION_FIXED:
      {
        int width = PROP(width);

        if (!width)
        {
          MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
          if (geo->width)
            width = geo->width;
          else
            width = mrg_edge_right (mrg) - mrg_edge_left (mrg);
        }

        ctx_identity (mrg_cr (mrg));
        ctx_scale (mrg_cr(mrg), mrg_ddpx (mrg), mrg_ddpx (mrg));
        html->state->floats = 0;

        mrg_set_edge_left (mrg, PROP(left) + PROP(margin_left) + PROP(border_left_width) + PROP(padding_left));
        mrg_set_edge_right (mrg, PROP(left) + PROP(margin_left) + PROP(border_left_width) + PROP(padding_left) + width);//mrg_width (mrg) - PROP(padding_right) - PROP(border_right_width) - PROP(margin_right)); //PROP(left) + PROP(width)); /* why only padding and not also border?  */
        mrg_set_edge_top (mrg, PROP(top) + PROP(margin_top) + PROP(border_top_width) + PROP(padding_top));
        html->state->block_start_x = mrg_x (mrg);
        html->state->block_start_y = mrg_y (mrg);
      }
      break;
  }

  if (style->display == MRG_DISPLAY_BLOCK ||
      style->display == MRG_DISPLAY_INLINE_BLOCK ||
      style->float_)
  {
     float height = PROP(height);
     float width = PROP(width);

     if (!height)
       {
         MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
         if (geo->height)
           height = geo->height;
         else
           height = mrg_em (mrg) * 4;
       }
     if (!width)
       {
         MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);
         if (geo->width)
           width = geo->width;
         else
           width = mrg_em (mrg) * 4;
       }

    if (height  /* XXX: if we knew height of dynamic elements
                        from previous frame, we could use it here */
       && style->overflow == MRG_OVERFLOW_HIDDEN)
       {
         ctx_rectangle (mrg_cr(mrg),
            html->state->block_start_x - PROP(padding_left) - PROP(border_left_width),
            html->state->block_start_y - mrg_em(mrg) - PROP(padding_top) - PROP(border_top_width),
            width + PROP(border_right_width) + PROP(border_left_width) + PROP(padding_left) + PROP(padding_right), //mrg_edge_right (mrg) - mrg_edge_left (mrg) + PROP(padding_left) + PROP(padding_right) + PROP(border_left_width) + PROP(border_right_width),
            height + PROP(padding_top) + PROP(padding_bottom) + PROP(border_top_width) + PROP(border_bottom_width));
         ctx_clip (mrg_cr(mrg));
       }

    html->state->ptly = 0;
    _mrg_draw_background_increment (mrg, html, 0);
  }
}

#if 0
static void mrg_css_add_class (Mrg *mrg, const char *class_name)
{
  int i;
  MrgStyleNode *node = &mrg->state->style_node;
  for (i = 0; node->classes[i]; i++);
  node->classes[i] = mrg_intern_string (class_name);
}

static void mrg_css_add_pseudo_class (Mrg *mrg, const char *pseudo_class)
{
  int i;
  MrgStyleNode *node = &mrg->state->style_node;
  for (i = 0; node->pseudo[i]; i++);
  node->pseudo[i] = mrg_intern_string (pseudo_class);
}
#endif

void _mrg_set_wrap_edge_vfuncs (Mrg *mrg,
    float (*wrap_edge_left)  (Mrg *mrg, void *wrap_edge_data),
    float (*wrap_edge_right) (Mrg *mrg, void *wrap_edge_data),
    void *wrap_edge_data)
{
  mrg->state->wrap_edge_left = wrap_edge_left;
  mrg->state->wrap_edge_right = wrap_edge_right;
  mrg->state->wrap_edge_data = wrap_edge_data;
}

void _mrg_set_post_nl (Mrg *mrg,
    void (*post_nl)  (Mrg *mrg, void *post_nl_data, int last),
    void *post_nl_data
    )
{
  mrg->state->post_nl = post_nl;
  mrg->state->post_nl_data = post_nl_data;
}


void _mrg_layout_post (Mrg *mrg, MrgHtml *html)
{
  Ctx *ctx = mrg->ctx;
  float vmarg = 0;
  MrgStyle *style = mrg_style (mrg);
  float height = PROP(height);
  float width = PROP(width);
  
  /* adjust cursor back to before display */

  if ((style->display == MRG_DISPLAY_BLOCK || style->float_) &&
       height != 0.0)
  {
    float diff = height - (mrg_y (mrg) - (html->state->block_start_y - mrg_em(mrg)));
    mrg_set_xy (mrg, mrg_x (mrg), mrg_y (mrg) + diff);
    if (diff > 0)
      _mrg_draw_background_increment (mrg, html, 1);
  }

  /* remember data to store about float, XXX: perhaps better to store
   * straight into parent state?
   */
  if (style->float_)
  {
    int was_float = 0;
    float fx,fy,fw,fh; // these tempvars arent really needed.
    was_float = style->float_;
    fx = html->state->block_start_x - PROP(padding_left) - PROP(border_left_width) - PROP(margin_left);
    fy = html->state->block_start_y - mrg_em(mrg) - PROP(padding_top) - PROP(border_top_width)
      - PROP(margin_top);

    fw = mrg_edge_right (mrg) - mrg_edge_left (mrg)
     + PROP(padding_left) + PROP(border_left_width) + PROP(margin_left)
     + PROP(padding_right) + PROP(border_right_width) + PROP(margin_right);

    fh = mrg_y (mrg) - (html->state->block_start_y - mrg_em(mrg))
         + PROP(margin_bottom) + PROP(padding_top) + PROP(padding_bottom) + PROP(border_top_width) + PROP(border_bottom_width) + PROP(margin_top) + mrg_em (mrg);

    html->states[html->state_no-1].float_data[html->states[html->state_no-1].floats].type = was_float;
    html->states[html->state_no-1].float_data[html->states[html->state_no-1].floats].x = fx;
    html->states[html->state_no-1].float_data[html->states[html->state_no-1].floats].y = fy;
    html->states[html->state_no-1].float_data[html->states[html->state_no-1].floats].width = fw;
    html->states[html->state_no-1].float_data[html->states[html->state_no-1].floats].height = fh;
    html->states[html->state_no-1].floats++;
  }

  if (style->display == MRG_DISPLAY_BLOCK || style->float_)
  {
    MrgGeoCache *geo = _mrg_get_cache (html, style->id_ptr);

    if (width == 0)
    {
#if 0
      if (mrg_y (mrg) == (ctx->state->block_start_y))
        geo->width = mrg_x (mrg) - (ctx->state->block_start_x);
      else
        geo->width = mrg->state->edge_right  - (ctx->state->block_start_x);
#endif
      geo->width = mrg_x (mrg) - (html->state->block_start_x);
    }
    else
      geo->width = width;

    //:mrg_edge_right (mrg) - mrg_edge_left (mrg);
    if (height == 0)
      geo->height = mrg_y (mrg) - (html->state->block_start_y - mrg_em(mrg));
    else
      geo->height = height;

    geo->gen++;

    mrg_box (mrg,
        html->state->block_start_x,
        html->state->block_start_y - mrg_em(mrg),
        geo->width,
        geo->height);

    {
      CtxMatrix transform;
      ctx_get_matrix (mrg_cr (mrg), &transform);
      float x = ctx_pointer_x (ctx);
      float y = ctx_pointer_y (ctx);
      ctx_matrix_invert (&transform);
      ctx_matrix_apply_transform (&transform, &x, &y);

      if (x >= html->state->block_start_x &&
          x <  html->state->block_start_x + geo->width &&
          y >= html->state->block_start_y - mrg_em (mrg) &&
          y <  html->state->block_start_y - mrg_em (mrg) + geo->height)
      {
        geo->hover = 1;
      }
      else
      {
        geo->hover = 0;
      }
    }

    //mrg_edge_right (mrg) - mrg_edge_left (mrg), mrg_y (mrg) - (html->state->block_start_y - mrg_em(mrg)));

    if (!style->float_ && style->display == MRG_DISPLAY_BLOCK)
    {
      vmarg = PROP(margin_bottom);

      mrg_set_xy (mrg, 
          mrg_edge_left (mrg),
          mrg_y (mrg) + vmarg + PROP(border_bottom_width));
    }
  }
  else if (style->display == MRG_DISPLAY_INLINE)
  {
    mrg->x += paint_span_bg_final (mrg, mrg->x, mrg->y, 0);
  }

  if (style->position == MRG_POSITION_RELATIVE)
    ctx_translate (mrg_cr (mrg), -PROP(left), -PROP(top));

  if (style->float_ ||
      style->position == MRG_POSITION_ABSOLUTE ||
      style->position == MRG_POSITION_FIXED)
  {
    mrg_set_xy (mrg, html->state->original_x,
                     html->state->original_y);
  }
  if (html->state_no)
    html->states[html->state_no-1].vmarg = vmarg;

  html->state_no--;
  html->state = &html->states[html->state_no];
}

enum {
  HTML_ATT_UNKNOWN = 0,
  HTML_ATT_STYLE,
  HTML_ATT_CLASS,
  HTML_ATT_ID,
  HTML_ATT_HREF,
  HTML_ATT_REL,
  HTML_ATT_SRC
};

typedef struct MrgEntity {
   uint32_t    name;
   const char *value;
} MrgEntity;

static MrgEntity entities[]={
  {CTX_shy,    ""},   // soft hyphen,. should be made use of in wrapping..
  {CTX_nbsp,   " "},  //
  {CTX_lt,     "<"},
  {CTX_gt,     ">"},
  {CTX_trade,  "™"},
  {CTX_copy,   "©"},
  {CTX_middot, "·"},
  {CTX_bull,   "•"},
  {CTX_Oslash, "Ø"},
  {CTX_oslash, "ø"},
  {CTX_hellip, "…"},
  {CTX_aring,  "å"},
  {CTX_Aring,  "Å"},
  {CTX_aelig,  "æ"},
  {CTX_AElig,  "Æ"},
  {CTX_Aelig,  "Æ"},
  {CTX_laquo,  "«"},
  {CTX_raquo,  "»"},

  /*the above were added as encountered, the rest in anticipation  */

  {CTX_reg,    "®"},
  {CTX_deg,    "°"},
  {CTX_plusmn, "±"},
  {CTX_sup2,   "²"},
  {CTX_sup3,   "³"},
  {CTX_sup1,   "¹"},
  {CTX_ordm,   "º"},
  {CTX_para,   "¶"},
  {CTX_cedil,  "¸"},
  {CTX_bull,   "·"},
  {CTX_amp,    "&"},
  {CTX_mdash,  "–"},
  {CTX_apos,   "'"},
  {CTX_quot,   "\""},
  {CTX_iexcl,  "¡"},
  {CTX_cent,   "¢"},
  {CTX_pound,  "£"},
  {CTX_euro,   "€"},
  {CTX_yen,    "¥"},
  {CTX_curren, "¤"},
  {CTX_sect,   "§"},
  {CTX_phi,    "Φ"},
  {CTX_omega,  "Ω"},
  {CTX_alpha,  "α"},

  /* XXX: incomplete */

  {0, NULL}
};

void
ctx_set (Ctx *ctx, uint32_t key_hash, const char *string, int len);

static void
mrg_parse_transform (Mrg *mrg, CtxMatrix *matrix, const char *str)
{
  if (!strncmp (str, "matrix", 5))
  {
    char *s;
    int numbers = 0;
    double number[12]={0.0,};
    ctx_matrix_identity (matrix);
    s = (void*) ctx_strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }
    matrix->m[0][0] = number[0];
    matrix->m[0][1] = number[1];
    matrix->m[1][0] = number[2];
    matrix->m[1][1] = number[3];
    matrix->m[2][0] = number[4];
    matrix->m[2][1] = number[5];
  }
  else if (!strncmp (str, "scale", 5))
  {
    char *s;
    int numbers = 0;
    double number[12]={0,0};
    ctx_matrix_identity (matrix);
    s = (void*) ctx_strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }
    if (numbers <= 1)
      ctx_matrix_scale (matrix, number[0], number[0]);
    else
      ctx_matrix_scale (matrix, number[0], number[1]);
  }
  else if (!strncmp (str, "translate", 5))
  {
    char *s;
    int numbers = 0;
    double number[12];
    ctx_matrix_identity (matrix);
    s = (void*) ctx_strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }
    ctx_matrix_translate (matrix, number[0], number[1]);
  }
  else if (!strncmp (str, "rotate", 5))
  {
    char *s;
    int numbers = 0;
    double number[12];
    ctx_matrix_identity (matrix);
    s = (void*) ctx_strchr (str, '(');
    if (!s)
      return;
    s++;
    for (; *s; s++)
    {
      switch (*s)
      {
        case '+':case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
        number[numbers] = strtod (s, &s);
        s--;
        numbers++;
      }
    }
    ctx_matrix_rotate (matrix, number[0] / 360.0 * 2 * M_PI);
  }
  else
  {
    fprintf (stderr, "unhandled transform: %s\n", str);
    ctx_matrix_identity (matrix);
  }
}

int
mrg_parse_svg_path (Mrg *mrg, const char *str)
{
  /* this function is the seed of the ctx parser */
  Ctx *ctx = mrg_cr (mrg);
  char  command = 'm';
  char *s;
  int numbers = 0;
  double number[12];
  double pcx, pcy, cx, cy;

  if (!str)
    return -1;
  ctx_move_to (ctx, 0, 0);
  cx = 0; cy = 0;
  pcx = cx; pcy = cy;

  s = (void*)str;
again:
  numbers = 0;

  for (; s && *s; s++)
  {
    switch (*s)
    {
      case 'z':
      case 'Z':
        pcx = cx; pcy = cy;
        ctx_close_path (ctx);
        break;
      case 'm':
      case 'a':
      case 's':
      case 'S':
      case 'M':
      case 'c':
      case 'C':
      case 'l':
      case 'L':
      case 'h':
      case 'H':
      case 'v':
      case 'V':
         command = *s;
         break;

      case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
      if (*s == '-')
      {
        number[numbers] = -strtod (s+1, &s);
        s--;
      }
      else
      {
        number[numbers] = strtod (s, &s);
        s--;
      }
      numbers++;

      switch (command)
      {
        case 'a':
          /* fallthrough */
        case 'A':
          if (numbers == 7)
          {
            /// XXX: NYI
            s++;
            goto again;
          }
          /* fallthrough */
        case 'm':
          if (numbers == 2)
          {
            ctx_rel_move_to (ctx, number[0], number[1]);
	    cx += number[0];
	    cy += number[1];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'h':
          if (numbers == 1)
          {
            ctx_rel_line_to (ctx, number[0], 0.0);
	    cx += number[0];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'v':
          if (numbers == 1)
          {
            ctx_rel_line_to (ctx, 0.0, number[0]);
	    cy += number[0];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'l':
          if (numbers == 2)
          {
            ctx_rel_line_to (ctx, number[0], number[1]);
	    cx += number[0];
	    cy += number[1];
            pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'c':
          if (numbers == 6)
          {
            ctx_rel_curve_to (ctx, number[0], number[1],
                                    number[2], number[3],
                                    number[4], number[5]);
            pcx = cx + number[2];
	    pcy = cy + number[3];
	    cx += number[4];
	    cy += number[5];
            s++;
            goto again;
          }
          break;
	case 's':
          if (numbers == 4)
          {
            ctx_curve_to (ctx, 2 * cx - pcx, 2 * cy - pcy,
                                number[0] + cx, number[1] + cy,
                                number[2] + cx, number[3] + cy);
	    pcx = number[0] + cx;
	    pcy = number[1] + cy;
	    cx += number[2];
	    cy += number[3];
            s++;
            goto again;
          }
          break;
        case 'M':
          if (numbers == 2)
          {
            ctx_move_to (ctx, number[0], number[1]);
	    cx = number[0];
	    cy = number[1];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'H':
          if (numbers == 1)
          {
            ctx_line_to (ctx, number[0], cy);
	    cx = number[0];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'V':
          if (numbers == 1)
          {
            ctx_line_to (ctx, cx, number[0]);
	    cy = number[0];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'L':
          if (numbers == 2)
          {
            ctx_line_to (ctx, number[0], number[1]);
	    cx = number[0];
	    cy = number[1];
	    pcx = cx; pcy = cy;
            s++;
            goto again;
          }
          break;
        case 'C':
          if (numbers == 6)
          {
            ctx_curve_to (ctx, number[0], number[1],
                                number[2], number[3],
                                number[4], number[5]);
	    pcx = number[2];
	    pcy = number[3];
	    cx = number[4];
	    cy = number[5];
            s++;
            goto again;
          }
          break;
        case 'S':
          if (numbers == 4)
          {
            float ax = 2 * cx - pcx;
            float ay = 2 * cy - pcy;
            ctx_curve_to (ctx, ax, ay,
                                number[0], number[1],
                                number[2], number[3]);
	    pcx = number[0];
	    pcy = number[1];
	    cx = number[2];
	    cy = number[3];
            s++;
            goto again;
          }
          break;
        default:
          fprintf (stderr, "_%c", *s);
          break;
      }
      break;
      default:
        break;
    }
  }
  return 0;
}

static void
mrg_parse_polygon (Mrg *mrg, const char *str)
{
  Ctx *ctx = mrg_cr (mrg);
  char *s;
  int numbers = 0;
  int started = 0;
  double number[12];

  if (!str)
    return;
  ctx_move_to (ctx, 0, 0);

  s = (void*)str;
again:
  numbers = 0;

  for (; *s; s++)
  {
    switch (*s)
    {
      case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
      number[numbers] = _ctx_parse_float (s, &s);
      s--;
      numbers++;

      if (numbers == 2)
      {
        if (started)
          ctx_line_to (ctx, number[0], number[1]);
        else
        {
          ctx_move_to (ctx, number[0], number[1]);
          started = 1;
        }
        s++;
        goto again;
      }
      default:
        break;
    }
  }
}

void _mrg_set_wrap_edge_vfuncs (Mrg *mrg,
    float (*wrap_edge_left)  (Mrg *mrg, void *wrap_edge_data),
    float (*wrap_edge_right) (Mrg *mrg, void *wrap_edge_data),
    void *wrap_edge_data);

void _mrg_set_post_nl (Mrg *mrg,
    void (*post_nl)  (Mrg *mrg, void *post_nl_data, int last),
    void *post_nl_data);


int mrg_get_contents (Mrg         *mrg,
                      const char  *referer,
                      const char  *input_uri,
                      char       **contents,
                      long        *length);


void  mrg_text_listen (Mrg *mrg, CtxEventType types,
                       CtxCb cb, void *data1, void *data2);

void  mrg_text_listen_full (Mrg *mrg, CtxEventType types,
                            CtxCb cb, void *data1, void *data2,
          void (*finalize)(void *listen_data, void *listen_data2, void *finalize_data),
          void  *finalize_data);
void  mrg_text_listen_done (Mrg *mrg);

char *_mrg_resolve_uri (const char *base_uri, const char *uri);
typedef struct _MrgImage MrgImage;
struct _MrgImage
{
  char *path;
  CtxBuffer *surface;
};


MrgImage *mrg_query_image (Mrg        *mrg,
                           const char *path,
                           int        *width,
                           int        *height)
{
  return NULL;
}

void mrg_image (Mrg *mrg, float x0, float y0, float width, float height, float opacity, const char *path, int *used_width, int *used_height)
{
}

enum
{
  URI_STATE_IN_PROTOCOL = 0,
  URI_STATE_IN_HOST,
  URI_STATE_IN_PORT,
  URI_STATE_E_S1,
  URI_STATE_E_S2,
  URI_STATE_IN_PATH,
  URI_STATE_IN_FRAGMENT,
};

int split_uri (char *uri,
               char **protocol,
               char **host,
               char **port,
               char **path,
               char **fragment)
{
  char *p;
  *protocol =
  *host =
  *port =
  *path =
  *fragment = NULL;

  if (strstr (uri, "//") || strchr(uri, ':'))
  {
    int mr = URI_STATE_IN_PROTOCOL;

    if (protocol)
      *protocol = uri;

    if (uri[0] == '/' &&
        uri[1] == '/')
    {
      mr = URI_STATE_E_S1;
      *protocol = NULL;
    }

    for (p = uri; *p; p++)
    {
      switch (mr)
     {
        case URI_STATE_IN_PROTOCOL:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_E_S1;
              break;
          }
          break;
        case URI_STATE_E_S1:
          switch (*p)
          {
            default:
              mr = URI_STATE_IN_HOST;
              if (path) *path = p;
              break;
            case '/':
              mr = URI_STATE_E_S2;
              break;
          }
          break;
        case URI_STATE_E_S2:
          switch (*p)
          {
            default:
              // XXX ?
              break;
            case '/':
              mr = URI_STATE_IN_HOST;
              if (host) *host = p+1;
              break;
          }
          break;
        case URI_STATE_IN_HOST:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_IN_PORT;
              if (port) *port = p+1;
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
         break;
        case URI_STATE_IN_PORT:
          switch (*p)
          {
            default:
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
          break;
        case URI_STATE_IN_PATH:
          switch (*p)
          {
            default:
              break;
            case '#':
              *p = '\0';
              mr = URI_STATE_IN_FRAGMENT;
              if (fragment) *fragment = p+1;
              break;
          }
          break;
      }
    }
  }
  else
  {

    int mr = URI_STATE_IN_HOST;
    if (protocol)
      *protocol = NULL;

    if (uri[0]=='/')
    {
      if (host)
        *host = NULL;
      if (port)
        *port = NULL;
      *uri = '\0';
      mr = URI_STATE_IN_PATH;
      if (path) *path = uri+1;
    }
    else
    {
      mr = URI_STATE_IN_PATH;
      if (path) *path = uri;
    }

    for (p = uri; *p; p++)
    {
      switch (mr)
      {
        case URI_STATE_IN_PROTOCOL:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_E_S1;
              break;
          }
          break;
        case URI_STATE_E_S1:
          switch (*p)
          {
            default:
              // XXX ?
              break;
            case '/':
              mr = URI_STATE_E_S2;
              break;
          }
          break;
        case URI_STATE_E_S2:
         switch (*p)
          {
            default:
              // XXX ?
              break;
            case '/':
              mr = URI_STATE_IN_HOST;
              if (host) *host = p+1;
              break;
          }
          break;
        case URI_STATE_IN_HOST:
          switch (*p)
          {
            default:
              break;
            case ':':
              *p = '\0';
              mr = URI_STATE_IN_PORT;
              if (port) *port = p+1;
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
          break;
        case URI_STATE_IN_PORT:
          switch (*p)
          {
            default:
              break;
            case '/':
              *p = '\0';
              mr = URI_STATE_IN_PATH;
              if (path) *path = p+1;
              break;
          }
          break;
        case URI_STATE_IN_PATH:
          switch (*p)
          {
            default:
              break;
            case '#':
              *p = '\0';
              mr = URI_STATE_IN_FRAGMENT;
              if (fragment) *fragment = p+1;
              break;
          }
          break;
      }
   }
  }
  if (*protocol && (*protocol)[0] == 0)
    *protocol = NULL;
  return 0;
}

char *_mrg_resolve_uri (const char *base_uri, const char *uri)
{
  char *ret;
  char *uri_dup = strdup (uri);

  if (!base_uri)
    return uri_dup;

  char *base_dup = strdup (base_uri);

  char *protocol = NULL;
  char *host = NULL;
  char *port = NULL;
  char *path = NULL;
  char *fragment = NULL;
  char *base_protocol = NULL;
  char *base_host = NULL;
  char *base_port = NULL;
  char *base_path = NULL;
  char *base_fragment = NULL;

  //int retlen = 0;
  int samehost = 0;

  split_uri (uri_dup, &protocol, &host, &port, &path, &fragment);
  split_uri (base_dup, &base_protocol, &base_host, &base_port, &base_path, &base_fragment);

  if (!protocol)
    protocol = base_protocol;
  if (!host)
  {
    host = base_host;
    port = base_port;
    samehost = 1;
  }
  ret = malloc (
      (path?strlen (path):0)
      + (fragment?strlen (fragment):0) +
      (host?strlen (host):0) + 640);
  if (protocol)
  {
    if (uri[0] == '/' && uri[1] != '/')
      sprintf (ret, "%s://%s%s%s%s", protocol, host, port?":":"", port?port:"", uri);
    else if (uri[0] == '.' && uri[1] == '.' && uri[2] == '/' &&
             uri[3] == '.' && uri[4] == '.')
    {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      if (base_path[strlen (base_path)-1] == '/')
        base_path[strlen (base_path)-1] = 0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else
        base_path[0]=0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      if (base_path[strlen (base_path)-1] == '/')
        base_path[strlen (base_path)-1] = 0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else
        base_path[0]=0;

      sprintf (ret, "c%s://%s%s%s/%s%s%s%s", protocol, host, port?":":"", port?port:"", samehost?base_path:"", &path[6], fragment?"#":"", fragment?fragment:"");
    }
    else if (uri[0] == '.' && uri[1] == '.')
    {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      if (base_path[strlen (base_path)-1] == '/')
        base_path[strlen (base_path)-1] = 0;
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else
        base_path[0]=0;

      sprintf (ret, "%s://%s%s%s/%s%s%s%s", protocol, host, port?":":"", port?port:"", samehost?base_path:"", &path[3], fragment?"#":"", fragment?fragment:"");
    }
    else
   {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      else if (ctx_strchr (base_path, '/'))
        ctx_strchr (base_path, '/')[1] = 0;
      else
        base_path[0] = 0;

      if (host)
      sprintf (ret, "%s://%s%s%s/%s%s%s%s", protocol, host, port?":":"", port?port:"", samehost?base_path:"", path, fragment?"#":"", fragment?fragment:"");
      else
      sprintf (ret, "%s:%s%s%s%s", protocol, samehost?base_path:"", path, fragment?"#":"", fragment?fragment:"");

    }
  }
  else
  {
    if (uri[0] == '/')
      sprintf (ret, "%s", path);
    else
    {
      if (strrchr (base_path, '/'))
        strrchr (base_path, '/')[1] = 0;
      sprintf (ret, "/%s%s", base_path, path);
    }
  }

  free (uri_dup);
  free (base_dup);
  return ret;
}

static float
_ctx_str_get_float (const char *string, int no)
{
  float ret = 0.0f;
  int number_no = 0;
  const char *s = string;

  while (*s == ' ')s++;

  while (*s && number_no < no)
  {
     while ( *s && ((*s >= '0' && *s <= '9') || (*s=='.') || (*s=='-'))) s ++;
     number_no ++;
     while (*s == ' ')s++;
  }
  if (*s)
    return atof (s);

  return ret;
}

void mrg_xml_render (Mrg *mrg,
                     char *uri_base,
                     void (*link_cb) (CtxEvent *event, void *href, void *link_data),
                     void *link_data,
                     void *(finalize)(void *listen_data, void *listen_data2, void *finalize_data),
                     void *finalize_data,
                     char *html_)
{
  char *html;
  MrgXml *xmltok;
  MrgHtml *htmlctx        = &mrg->html;
  uint32_t tag[MRG_MAX_STATE_DEPTH];
  int pos             = 0;
  int type            = t_none;
  static int depth    = 0;
  int in_style        = 0;
  int should_be_empty = 0;
  int tagpos          = 0;
  MrgString *style = mrg_string_new ("");
  int whitespaces = 0;
  uint32_t att = 0;

  html = malloc (strlen (html_) + 3);
  sprintf (html, "%s ", html_);
  xmltok = xmltok_buf_new (html);

  {
    int no = mrg->text_listen_count;
    mrg->text_listen_data1[no] = link_data;
    mrg->text_listen_data2[no] = html_;
    mrg->text_listen_finalize[no] = (void*)finalize;
    mrg->text_listen_finalize_data[no] = finalize_data;
    mrg->text_listen_count++;
  }

  _mrg_set_wrap_edge_vfuncs (mrg, wrap_edge_left, wrap_edge_right, htmlctx);
  _mrg_set_post_nl (mrg, _mrg_draw_background_increment, htmlctx);
  htmlctx->mrg = mrg;
  htmlctx->state = &htmlctx->states[0];

  while (type != t_eof)
  {
    char *data = NULL;
    type = xmltok_get (xmltok, &data, &pos);

    if (type == t_tag ||
        type == t_att ||
        type == t_endtag ||
        type == t_closeemptytag ||
        type == t_closetag)
    {
      int i;
      for (i = 0; data[i]; i++)
        data[i] = tolower (data[i]);
    }

    switch (type)
    {
      case t_entity:
        {
          int i;
          int dealt_with = 0;
          if (data[0]=='#')
          {
            int c = atoi (&data[1]);
            mrg_printf (mrg, "%c", c);
          }
          else
          {
            uint32_t hash = ctx_strhash (data, 0);
          for (i = 0; entities[i].name && !dealt_with; i++)
            if (hash == entities[i].name)
            {
              mrg_print (mrg, entities[i].value);
              dealt_with = 1;
            }
          }

          if (!dealt_with){
            mrg_start (mrg, "dim", (void*)((size_t)pos));
            mrg_print (mrg, data);
            mrg_end (mrg);
          }
        }
        break;
      case t_word:
        if (in_style)
        {
          mrg_stylesheet_add (mrg, data, uri_base, MRG_STYLE_XML, NULL);
        }
        else
        {
          mrg_print (mrg, data);
        }
        whitespaces = 0;
        break;

      case t_whitespace:
        if (in_style)
        {
          mrg_stylesheet_add (mrg, data, uri_base, MRG_STYLE_XML, NULL);
        }
        else
        {
          switch (mrg_style (mrg)->white_space)
          {
            case MRG_WHITE_SPACE_PRE: /* handles as pre-wrap for now */
            case MRG_WHITE_SPACE_PRE_WRAP:
              mrg_print (mrg, data);
              break;
            case MRG_WHITE_SPACE_PRE_LINE:
              switch (*data)
              {
                case ' ':
                  whitespaces ++;
                  if (whitespaces == 1)
                    mrg_print (mrg, " ");
                  break;
                case '\n':
                  whitespaces = 0;
                  break;
              }
              break;
            case MRG_WHITE_SPACE_NOWRAP: /* XXX: handled like normal, this is bad.. */
            case MRG_WHITE_SPACE_NORMAL: 
              whitespaces ++;
              if (whitespaces == 1)
                mrg_print (mrg, " ");
              break;
          }
        }
        break;
      case t_tag:
        //htmlctx->attributes = 0;
        //ctx_save (mrg->ctx);
        tagpos = pos;
        mrg_string_clear (style);
        break;
      case t_att:
        //if (htmlctx->attributes < MRG_XML_MAX_ATTRIBUTES-1)
        //  strncpy (htmlctx->attribute[htmlctx->attributes], data, MRG_XML_MAX_ATTRIBUTE_LEN-1);
        att = ctx_strhash (data, 0);
        //fprintf (stderr, "  %s:%i\n", data, att);
        break;
      case t_val:
        //if (htmlctx->attributes < MRG_XML_MAX_ATTRIBUTES-1)
        //  strncpy (htmlctx->value[htmlctx->attributes++], data, MRG_XML_MAX_VALUE_LEN-1);
        ctx_set_string (mrg->ctx, att, data);
        {
            uint32_t style_attribute[] ={
              CTX_fill_rule,
              CTX_font_size,
              CTX_font_family,
              CTX_fill_color,
              CTX_fill,
              CTX_stroke_width,
              CTX_stroke_color,
              CTX_stroke_linecap,
              CTX_stroke_miterlimit,
              CTX_stroke_linejoin,
              CTX_stroke,
              //CTX_viewBox,
              CTX_color,
              CTX_background_color,
              CTX_background,
              0};
            char *style_attribute_names[] ={
              "fill_rule",
              "font_size",
              "font_family",
              "fill_color",
              "fill",
              "stroke_width",
              "stroke_color",
              "stroke_linecap",
              "stroke_miterlimit",
              "stroke_linejoin",
              "stroke",
              //"viewBox",
              "color",
              "background_color",
              "background",
              0};

              int j;
              for (j = 0; style_attribute[j]; j++)
                if (att == style_attribute[j])
                {
                  mrg_string_append_printf (style, "%s: %s;",
                      style_attribute_names[j], data);
                  break;
                }
          }
        break;
      case t_endtag:
        {
          uint32_t data_hash = ctx_strhash (data, 0);

        if (depth && (data_hash == CTX_tr && tag[depth-1] == CTX_td))
        {
          mrg_end (mrg);
        //ctx_restore (mrg->ctx);
          depth--;
          mrg_end (mrg);
        //ctx_restore (mrg->ctx);
          depth--;
        }
        if (depth && (data_hash == CTX_tr && tag[depth-1] == CTX_td))
        {
          mrg_end (mrg);
        //ctx_restore (mrg->ctx);
          depth--;
          mrg_end (mrg);
        //ctx_restore (mrg->ctx);
          depth--;
        }
        else if (depth && ((data_hash == CTX_dd && tag[depth-1] == CTX_dt) ||
                      (data_hash == CTX_li && tag[depth-1] == CTX_li) ||
                      (data_hash == CTX_dt && tag[depth-1] == CTX_dd) ||
                      (data_hash == CTX_td && tag[depth-1] == CTX_td) ||
                      (data_hash == CTX_tr && tag[depth-1] == CTX_tr) ||
                      (data_hash == CTX_dd && tag[depth-1] == CTX_dd) ||
                      (data_hash == CTX_p &&  tag[depth-1] == CTX_p)))
        {
          mrg_end (mrg);
        //ctx_restore (mrg->ctx);
          depth--;
        }

        tag[depth] = data_hash;
        depth ++;

        {
          char combined[256]="";
          char *klass = (char*)PROPS(class);
          /* XXX: spaces in class should be turned into .s making
           * it possible to use multiple classes
           */
          const char *id = PROPS(id);

          const char *pseudo = "";

          if (klass)
          {
            klass = strdup (klass);
            if(1){
              int i;
              for (i = 0; klass[i]; i++)
                if (klass[i] == ' ')
                  klass[i]='.';
            }
          }

          //if (mrg_style(mrg)->id_ptr)
          { // XXX : perhaps do this a tiny bit differently?
            MrgGeoCache *geo = _mrg_get_cache (htmlctx, (void*)(size_t)(tagpos));
            if (geo && geo->hover)
            {
              if (ctx_pointer_is_down (mrg->ctx, 0))
                pseudo = ":active:hover";
              else
                pseudo = ":hover";
            }
          }
          sprintf (combined, "%s%s%s%s%s%s",
              data,
              klass?".":"",
              klass?klass:"",
              id?"#":"",
              id?id:"", pseudo);

          if (klass)
            free (klass);
            /* collect XML attributes and convert into CSS declarations */
          mrg_string_append_str (style, PROPS(style));
          mrg_start_with_style (mrg, combined, (void*)((size_t)tagpos), style->str);
        }

        if (data_hash == CTX_g)
        {
          const char *transform;
          if ((transform = PROPS(transform)))
            {
              CtxMatrix matrix;
              mrg_parse_transform (mrg, &matrix, transform);
              ctx_apply_matrix (mrg_cr (mrg), &matrix);
            }
        }

        else if (data_hash == CTX_svg)
        {
          const char *vbox = PROPS(viewbox);
          float x = _ctx_str_get_float (vbox, 0);
          float y = _ctx_str_get_float (vbox, 1);
          float width = _ctx_str_get_float (vbox, 2);
          float height = _ctx_str_get_float (vbox, 3);
          fprintf (stderr, "viewBox:%s   %f %f %f %f\n", vbox, x, y, width, height);
          ctx_view_box (mrg->ctx, x, y, width, height);
        }

        else if (data_hash == CTX_polygon)
        {
          mrg_parse_polygon (mrg, PROPS(d));
          mrg_path_fill_stroke (mrg);
        }

        else if (data_hash == CTX_path)
        {
          mrg_parse_svg_path (mrg, PROPS(d));
          mrg_path_fill_stroke (mrg);
        }

        else if (data_hash == CTX_rect)
        {
          float width  = PROP(width);
          float height = PROP(height);
          float x      = PROP(x);
          float y      = PROP(y);

          ctx_rectangle (mrg_cr (mrg), x, y, width, height);
          mrg_path_fill_stroke (mrg);
        }

        else if (data_hash == CTX_text)
        {
          mrg->x = PROP(x);
          mrg->y = PROP(y);
        }

        if (data_hash == CTX_a)
        {
          if (link_cb && ctx_is_set_now (mrg->ctx, CTX_href))
            mrg_text_listen_full (mrg, CTX_CLICK, link_cb, _mrg_resolve_uri (uri_base, ctx_get_string (mrg->ctx, CTX_href)), link_data, (void*)free, NULL); //XXX: free is not invoked according to valgrind
        }

        else if (data_hash == CTX_style)
          in_style = 1;
        else
          in_style = 0;

        should_be_empty = 0;

        if (data_hash == CTX_link)
        {
          const char *rel;
          if ((rel=PROPS(rel)) && !strcmp (rel, "stylesheet") && ctx_is_set_now (mrg->ctx, CTX_href))
          {
            char *contents;
            long length;
            mrg_get_contents (mrg, uri_base, ctx_get_string (mrg->ctx, CTX_href), &contents, &length);
            if (contents)
            {
              mrg_stylesheet_add (mrg, contents, uri_base, MRG_STYLE_XML, NULL);
              free (contents);
            }
          }
        }

        if (data_hash == CTX_img && ctx_is_set_now (mrg->ctx, CTX_src))
        {
          int img_width, img_height;
          const char *src = ctx_get_string (mrg->ctx, CTX_src);

          if (mrg_query_image (mrg, src, &img_width, &img_height))
          {
            float width =  PROP(width);
            float height = PROP(height);

            if (width < 1)
            {
               width = img_width;
            }
            if (height < 1)
            {
               height = img_height *1.0 / img_width * width;
            }

            _mrg_draw_background_increment (mrg, &mrg->html, 0);
            mrg->y += height;

            mrg_image (mrg,
            mrg->x,
            mrg->y - height,
            width,
            height,
            1.0f,
            src, NULL, NULL);

            mrg->x += width;
          }
          else
          {
            mrg_printf (mrg, "![%s]", src);
          }
        }
#if 1
        switch (data_hash)
        {
          case CTX_link:
          case CTX_meta:
          case CTX_input:
          case CTX_img:
          case CTX_br:
          case CTX_hr:
            should_be_empty = 1;
            mrg_end (mrg);
            //ctx_restore (mrg->ctx);
            depth--;
        }
#endif
        }
        break;

      case t_closeemptytag:
      case t_closetag:
        if (!should_be_empty)
        {
          uint32_t data_hash = ctx_strhash (data, 0);
          if (!strcmp (data, "a"))
          {
            mrg_text_listen_done (mrg);
          }
          in_style = 0;
          mrg_end (mrg);
          //ctx_restore (mrg->ctx);
          depth--;

          if (tag[depth] != data_hash)
          {
            if (tag[depth] == CTX_p)
            {
              mrg_end (mrg);
              //ctx_restore (mrg->ctx);
              depth --;
            } else 
            if (depth > 0 && tag[depth-1] == data_hash)
            {
              mrg_end (mrg);
              //ctx_restore (mrg->ctx);
              depth --;
            }
            else if (depth > 1 && tag[depth-2] == data_hash)
            {
              int i;
              for (i = 0; i < 2; i ++)
              {
                depth --;
                mrg_end (mrg);
                //nctx_restore (mrg->ctx);
              }
            }
#if 0
            else if (depth > 2 && tag[depth-3] == data_hash)
            {
              int i;
              fprintf (stderr, "%i: fixing close of %s when %s wass open\n", pos, data, tag[depth]);

              for (i = 0; i < 3; i ++)
              {
                depth --;
                mrg_end (mrg);
               // ctx_restore (mrg->ctx);
              }
            }
            else if (depth > 3 && tag[depth-3] == data_hash)
            {
              int i;
              fprintf (stderr, "%i: fixing close of %s when %s wasss open\n", pos, data, tag[depth]);

              for (i = 0; i < 4; i ++)
              {
                depth --;
                mrg_end (mrg);
                //ctx_restore (mrg->ctx);
              }
            }
            else if (depth > 4 && tag[depth-5] == data_hash)
            {
              int i;
              fprintf (stderr, "%i: fixing close of %s when %s wassss open\n", pos, data, tag[depth]);

              for (i = 0; i < 5; i ++)
              {
                depth --;
                mrg_end (mrg);
                //ctx_restore (mrg->ctx);
              }
            }
#endif
            else
            {
              if (data_hash == CTX_table && tag[depth] == CTX_td)
              {
                depth--;
                mrg_end (mrg);
                //ctx_restore (mrg->ctx);
                depth--;
                mrg_end (mrg);
                //ctx_restore (mrg->ctx);
              }
              else if (data_hash == CTX_table && tag[depth] == CTX_tr)
              {
                depth--;
                mrg_end (mrg);
                //ctx_restore (mrg->ctx);
              }
            }
          }
        }
        break;
    }
  }

  xmltok_free (xmltok);
  if (depth!=0){
    fprintf (stderr, "html parsing unbalanced, %i open tags.. \n", depth);
    while (depth > 0)
    {
      //fprintf (stderr, " %s ", tag[depth-1]);
      mrg_end (mrg);
      depth--;
    }
    fprintf (stderr, "\n");
  }

//  ctx_list_free (&htmlctx->geo_cache); /* XXX: no point in doing that here */
  mrg_string_free (style, 1);
  free (html);
}

void mrg_xml_renderf (Mrg *mrg,
                      char *uri_base,
                      void (*link_cb) (CtxEvent *event, void *href, void *link_data),
                      void *link_data,
                      char *format,
                      ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  mrg_xml_render (mrg, uri_base, link_cb, link_data, NULL, NULL, buffer);
  free (buffer);
}

void mrg_print_xml (Mrg *mrg, char *xml)
{
  mrg_xml_render (mrg, NULL, NULL, NULL, NULL, NULL, xml);
}

void
mrg_printf_xml (Mrg *mrg, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char  *buffer;
  va_start(ap, format);
  needed = vsnprintf(NULL, 0, format, ap) + 1;
  buffer = malloc(needed);
  va_end (ap);
  va_start(ap, format);
  vsnprintf(buffer, needed, format, ap);
  va_end (ap);
  mrg_print_xml (mrg, buffer);
  free (buffer);
}

void mrg_set_size (Mrg *mrg, int width, int height)
{
  if (ctx_width (mrg->ctx) != width || ctx_height (mrg->ctx) != height)
  {
    ctx_set_size (mrg->ctx, width, height);
    mrg_queue_draw (mrg, NULL);
  }
}

int
_mrg_file_get_contents (const char  *path,
                   char       **contents,
                   long        *length)
{
  FILE *file;
  long  size;
  long  remaining;
  char *buffer;

  file = fopen (path,"rb");

  if (!file)
    return -1;

  if (!strncmp (path, "/proc", 4))
  {
    buffer = calloc(2048, 1);
    *contents = buffer;
    *length = fread (buffer, 1, 2047, file);
    buffer[*length] = 0;
    return 0;
  }
  else
  {
    fseek (file, 0, SEEK_END);
    *length = size = remaining = ftell (file);
    rewind (file);
    buffer = malloc(size + 8);
  }

  if (!buffer)
    {
      fclose(file);
      return -1;
    }

  remaining -= fread (buffer, 1, remaining, file);
  if (remaining)
    {
      fclose (file);
      free (buffer);
      return -1;
    }
  fclose (file);
  *contents = buffer;
  buffer[size] = 0;
  return 0;
}


static int
_mr_get_contents (const char  *referer,
                 const char  *uri,
                 char       **contents,
                 long        *length)
{
  char *uri_dup;
  char *protocol = NULL;
  char *host = NULL;
  char *port = NULL;
  char *path = NULL;
  char *fragment = NULL;
  uint32_t protocol_hash;
#if 0
  if (!strncmp (uri, "mrg:", 4))
  {
    return _mrg_internal_get_contents (referer, uri, contents, length);
  }
#endif

  uri_dup = strdup (uri);
  split_uri (uri_dup, &protocol, &host, &port, &path, &fragment);
  protocol_hash = protocol?ctx_strhash (protocol, 0):0;
#if 0
  if (protocol && protocol_hash == CTX_http)
  {
    int len;
    char *pathdup = malloc (strlen (path) + 2);
    pathdup[0] = '/';
    strcpy (&pathdup[1], path);
   // fprintf (stderr, "%s %i\n",host, port?atoi(port):80);
    char *cont = _mrg_http (NULL, host, port?atoi(port):80, pathdup, &len);
    *contents = cont;
    *length = len;
    //fprintf (stderr, "%s\n", cont);
    //
    fprintf (stderr, "%i\n", len);
    free (uri_dup);
    return 0;
  } else
#endif
  if (protocol && protocol_hash == CTX_file)
  {
    char *path2 = malloc (strlen (path) + 2);
    int ret;
    sprintf (path2, "/%s", path);
    ret = _ctx_file_get_contents (path2, (uint8_t**)contents, length);

    free (path2);
    free (uri_dup);
    fprintf (stderr, "a%i\n", (int)*length);
    return ret;
  }
  else
  {
    char *c = NULL;
    long  l = 0;
    int ret;
    free (uri_dup);
    ret = _mrg_file_get_contents (uri, &c, &l);
    if (contents) *contents = c;
    if (length) *length = l;
    return ret;
  }

  return -1;
}


static CtxList *cache = NULL;

typedef struct _CacheEntry {
  char *uri;
  char *contents;
  long  length;
} CacheEntry;

/* caching uri fetcher
 */
int
mrg_get_contents_default (const char  *referer,
                          const char  *input_uri,
                          char       **contents,
                          long        *length,
                          void        *ignored_user_data)
{
  CtxList *i;

  /* should resolve before mrg_get_contents  */
  char *uri = _mrg_resolve_uri (referer, input_uri);

  for (i = cache; i; i = i->next)
  {
    CacheEntry *entry = i->data;
    if (!strcmp (entry->uri, uri))
    {
      *contents = malloc (entry->length + 1);
      memcpy (*contents, entry->contents, entry->length);
      (*contents)[entry->length]=0;
      free (uri);
      if (length) *length = entry->length;
      if (length)
      {
        return 0;
      }
      else
      {
        return -1;
      }
    }
  }

  {
    CacheEntry *entry = calloc (sizeof (CacheEntry), 1);
    char *c = NULL;
    long  l = 0;

    entry->uri = uri;
    _mr_get_contents (referer, uri, &c, &l);
    if (c){
      entry->contents = c;
      entry->length = l;
    } else
    {
      entry->contents = NULL;
      entry->length = 0;
    }
    ctx_list_prepend (&cache, entry);

#if MRG_URI_LOG
    if (c)
      fprintf (stderr, "%li\t%s\n", l, uri);
    else
      fprintf (stderr, "FAIL\t%s\n", uri);
#endif
  }

  return mrg_get_contents_default (referer, input_uri, contents, length, ignored_user_data);
}

void _mrg_init (Mrg *mrg, int width, int height)
{
  _ctx_events_init (mrg->ctx);
  mrg->state = &mrg->states[0];
  /* XXX: is there a better place to set the default text color to black? */
#if 0
  mrg->state->style.color.red =
  mrg->state->style.color.green =
  mrg->state->style.color.blue = 0;
  mrg->state->style.color.alpha = 1;
#endif
  {
    CtxColor *color = ctx_color_new ();
    ctx_color_set_rgba (ctx_get_state (mrg->ctx), color, 0, 0, 0, 1);
    ctx_set_color (mrg->ctx, CTX_color, color);
    ctx_color_free (color);
  }

  mrg->ddpx = 1;
  if (getenv ("MRG_DDPX"))
  {
    mrg->ddpx = strtod (getenv ("MRG_DDPX"), NULL);
  }
  mrg_set_size (mrg, width, height);
  _mrg_text_init (mrg);

  mrg->html.state = &mrg->html.states[0];
  mrg->html.mrg = mrg;
  mrg->style = mrg_string_new ("");

  mrg_set_mrg_get_contents (mrg, mrg_get_contents_default, NULL);
  mrg->style_global = mrg_string_new ("");


  {
    const char *global_css_uri = "mrg:theme.css";

    if (getenv ("MRG_CSS"))
      global_css_uri = getenv ("MRG_CSS");

    char *contents;
    long length;
    mrg_get_contents (mrg, NULL, global_css_uri, &contents, &length);
    if (contents)
    {
      mrg_string_set (mrg->style_global, contents);
      free (contents);
    }
  }
}

void mrg_style_defaults (Mrg *mrg)
{
  Ctx *ctx = mrg->ctx;
  float em = 16;
  mrg_set_em (mrg,  em);
  mrg_set_rem (mrg, em);
  mrg_set_edge_left (mrg, 0);
  mrg_set_edge_right (mrg, mrg_width (mrg));
  mrg_set_edge_bottom (mrg, mrg_height (mrg));
  mrg_set_edge_top (mrg, 0);
  mrg_set_line_height (mrg, 1.2);

  SET_PROP(stroke_width, 1.0f);
  {
    CtxColor *color = ctx_color_new ();
    ctx_color_set_from_string (mrg->ctx, color, "transparent");
    ctx_set_color (ctx, CTX_stroke_color, color);
    ctx_color_free (color);
  }
  {
    CtxColor *color = ctx_color_new ();
    ctx_color_set_from_string (mrg->ctx, color, "black");
    ctx_set_color (ctx, CTX_fill_color, color);
    ctx_color_free (color);
  }

  mrg_stylesheet_clear (mrg);
  _mrg_init_style (mrg);

  if (mrg->style_global->length)
  {
    mrg_stylesheet_add (mrg, mrg->style_global->str, NULL, MRG_STYLE_GLOBAL, NULL);
  }

  if (mrg->style->length)
    mrg_stylesheet_add (mrg, mrg->style->str, NULL, MRG_STYLE_GLOBAL, NULL);
}


Mrg *mrg_new (Ctx *ctx, int width, int height)
{
  Mrg *mrg;

  mrg = calloc (sizeof (Mrg), 1);
  mrg->ctx = ctx;
  mrg->in_paint = 1;
  mrg->do_clip = 1;
  _mrg_init (mrg, width, height);
  mrg_set_size (mrg, width, height);
  mrg_style_defaults (mrg);

#if 0
  printf ("%f %i %i\n", mrg->state->style.font_size, mrg_width(mrg), mrg_height(mrg));
  printf ("sizeof(Mrg) %li (was: 1142496)\n", sizeof(Mrg));
  printf ("sizeof(MrgState) %li\n", sizeof(MrgState));
  printf ("sizeof(MrgStyle) %li\n", sizeof(MrgStyle));
  printf ("sizeof(MrgHtmlState) %li\n", sizeof(MrgHtmlState));
  printf ("sizeof(MrgHtml) %li\n", sizeof(MrgHtml));
  printf ("sizeof(MrgCssParseState) %li\n", sizeof(MrgCssParseState));
#endif

  return mrg;
}

void mrg_destroy (Mrg *mrg)
{
  if (mrg->edited_str)
    mrg_string_free (mrg->edited_str, 1);
  mrg->edited_str = NULL;
  free (mrg);
}
#endif
