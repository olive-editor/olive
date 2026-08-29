

#ifndef _ofxKeySyms_h_
#define _ofxKeySyms_h_

// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause


/**
   \addtogroup PropertiesGeneral
*/
/*@{*/

/** @brief Property used to indicate which a key on the keyboard or a button on a button device has been pressed

  - Type - int X 1
  - Property Set - an read only in argument for the actions ::kOfxInteractActionKeyDown, ::kOfxInteractActionKeyUp and ::kOfxInteractActionKeyRepeat.
  - Valid Values - one of any specified by #defines in the file ofxKeySyms.h.

This property represents a raw key press, it does not represent the 'character value' of the key. 

This property is associated with a ::kOfxPropKeyString property, which encodes the UTF8
value for the keypress/button press. Some keys (for example arrow keys) have no UTF8 equivalant.

Some keys, especially on non-english language systems, may have a UTF8 value, but \em not a keysym values, in these
cases, the keysym will have a value of kOfxKey_Unknown, but the ::kOfxPropKeyString property will still be set with
the UTF8 value.

 */
#define kOfxPropKeySym "kOfxPropKeySym"

/** @brief This property encodes a single keypresses that generates a unicode code point. The value is stored as a UTF8 string. 

  - Type - C string X 1, UTF8
  - Property Set - an read only in argument for the actions ::kOfxInteractActionKeyDown, ::kOfxInteractActionKeyUp and ::kOfxInteractActionKeyRepeat.
  - Valid Values - a UTF8 string representing a single character, or the empty string.

This property represents the UTF8 encode value of a single key press by a user in an OFX interact.

This property is associated with a ::kOfxPropKeySym which represents an integer value for the key press. Some keys (for example arrow keys) have no UTF8 equivalant, 
in which case this is set to the empty string "", and the associate ::kOfxPropKeySym is set to the equivilant raw key press.

Some keys, especially on non-english language systems, may have a UTF8 value, but \em not a keysym values, in these
cases, the keysym will have a value of kOfxKey_Unknown, but the ::kOfxPropKeyString property will still be set with
the UTF8 value.
*/
#define kOfxPropKeyString "kOfxPropKeyString"

/*
  The keysyms below have been lifted wholesale out of the X-Windows key symbol header file. Only
  the names have been changed to protect the innocent.
*/

/*@}*/

/*
Copyright (c) 1987, 1994  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

**/

/**
   \defgroup KeySyms OFX Key symbols

   These keysymbols are used as values by the ::kOfxPropKeySym property to indicate the value
of a key that has been pressed. A corresponding ::kOfxPropKeyString property is also set to
contain the unicode value of the key (if it has one).

   The special keysym ::kOfxKey_Unknown is used to set the ::kOfxPropKeySym property in cases
where the key has a UTF8 value which is not supported by the symbols below.

*/
/*@{*/

#define kOfxKey_Unknown		0x0

/*
 * TTY Functions, cleverly chosen to map to ascii, for convenience of
 * programming, but could have been arbitrary (at the cost of lookup
 * tables in client code.
 */
#define kOfxKey_BackSpace		0xFF08	/* back space, back char */
#define kOfxKey_Tab			0xFF09
#define kOfxKey_Linefeed		0xFF0A	/* Linefeed, LF */
#define kOfxKey_Clear		0xFF0B
#define kOfxKey_Return		0xFF0D	/* Return, enter */
#define kOfxKey_Pause		0xFF13	/* Pause, hold */
#define kOfxKey_Scroll_Lock		0xFF14
#define kOfxKey_Sys_Req		0xFF15
#define kOfxKey_Escape		0xFF1B
#define kOfxKey_Delete		0xFFFF	/* Delete, rubout */



/* International & multi-key character composition */

#define kOfxKey_Multi_key		0xFF20  /* Multi-key character compose */
#define kOfxKey_SingleCandidate	0xFF3C
#define kOfxKey_MultipleCandidate	0xFF3D
#define kOfxKey_PreviousCandidate	0xFF3E

/* Japanese keyboard support */

#define kOfxKey_Kanji		0xFF21	/* Kanji, Kanji convert */
#define kOfxKey_Muhenkan		0xFF22  /* Cancel Conversion */
#define kOfxKey_Henkan_Mode		0xFF23  /* Start/Stop Conversion */
#define kOfxKey_Henkan		0xFF23  /* Alias for Henkan_Mode */
#define kOfxKey_Romaji		0xFF24  /* to Romaji */
#define kOfxKey_Hiragana		0xFF25  /* to Hiragana */
#define kOfxKey_Katakana		0xFF26  /* to Katakana */
#define kOfxKey_Hiragana_Katakana	0xFF27  /* Hiragana/Katakana toggle */
#define kOfxKey_Zenkaku		0xFF28  /* to Zenkaku */
#define kOfxKey_Hankaku		0xFF29  /* to Hankaku */
#define kOfxKey_Zenkaku_Hankaku	0xFF2A  /* Zenkaku/Hankaku toggle */
#define kOfxKey_Touroku		0xFF2B  /* Add to Dictionary */
#define kOfxKey_Massyo		0xFF2C  /* Delete from Dictionary */
#define kOfxKey_Kana_Lock		0xFF2D  /* Kana Lock */
#define kOfxKey_Kana_Shift		0xFF2E  /* Kana Shift */
#define kOfxKey_Eisu_Shift		0xFF2F  /* Alphanumeric Shift */
#define kOfxKey_Eisu_toggle		0xFF30  /* Alphanumeric toggle */
#define kOfxKey_Zen_Koho		0xFF3D	/* Multiple/All Candidate(s) */
#define kOfxKey_Mae_Koho		0xFF3E	/* Previous Candidate */

/* Cursor control & motion */

#define kOfxKey_Home			0xFF50
#define kOfxKey_Left			0xFF51	/* Move left, left arrow */
#define kOfxKey_Up			0xFF52	/* Move up, up arrow */
#define kOfxKey_Right		0xFF53	/* Move right, right arrow */
#define kOfxKey_Down			0xFF54	/* Move down, down arrow */
#define kOfxKey_Prior		0xFF55	/* Prior, previous */
#define kOfxKey_Page_Up		0xFF55
#define kOfxKey_Next			0xFF56	/* Next */
#define kOfxKey_Page_Down		0xFF56
#define kOfxKey_End			0xFF57	/* EOL */
#define kOfxKey_Begin		0xFF58	/* BOL */


/* Misc Functions */

#define kOfxKey_Select		0xFF60	/* Select, mark */
#define kOfxKey_Print		0xFF61
#define kOfxKey_Execute		0xFF62	/* Execute, run, do */
#define kOfxKey_Insert		0xFF63	/* Insert, insert here */
#define kOfxKey_Undo			0xFF65	/* Undo, oops */
#define kOfxKey_Redo			0xFF66	/* redo, again */
#define kOfxKey_Menu			0xFF67
#define kOfxKey_Find			0xFF68	/* Find, search */
#define kOfxKey_Cancel		0xFF69	/* Cancel, stop, abort, exit */
#define kOfxKey_Help			0xFF6A	/* Help */
#define kOfxKey_Break		0xFF6B
#define kOfxKey_Mode_switch		0xFF7E	/* Character set switch */
#define kOfxKey_script_switch        0xFF7E  /* Alias for mode_switch */
#define kOfxKey_Num_Lock		0xFF7F

/* Keypad Functions, keypad numbers cleverly chosen to map to ascii */

#define kOfxKey_KP_Space		0xFF80	/* space */
#define kOfxKey_KP_Tab		0xFF89
#define kOfxKey_KP_Enter		0xFF8D	/* enter */
#define kOfxKey_KP_F1		0xFF91	/* PF1, KP_A, ... */
#define kOfxKey_KP_F2		0xFF92
#define kOfxKey_KP_F3		0xFF93
#define kOfxKey_KP_F4		0xFF94
#define kOfxKey_KP_Home		0xFF95
#define kOfxKey_KP_Left		0xFF96
#define kOfxKey_KP_Up		0xFF97
#define kOfxKey_KP_Right		0xFF98
#define kOfxKey_KP_Down		0xFF99
#define kOfxKey_KP_Prior		0xFF9A
#define kOfxKey_KP_Page_Up		0xFF9A
#define kOfxKey_KP_Next		0xFF9B
#define kOfxKey_KP_Page_Down		0xFF9B
#define kOfxKey_KP_End		0xFF9C
#define kOfxKey_KP_Begin		0xFF9D
#define kOfxKey_KP_Insert		0xFF9E
#define kOfxKey_KP_Delete		0xFF9F
#define kOfxKey_KP_Equal		0xFFBD	/* equals */
#define kOfxKey_KP_Multiply		0xFFAA
#define kOfxKey_KP_Add		0xFFAB
#define kOfxKey_KP_Separator		0xFFAC	/* separator, often comma */
#define kOfxKey_KP_Subtract		0xFFAD
#define kOfxKey_KP_Decimal		0xFFAE
#define kOfxKey_KP_Divide		0xFFAF

#define kOfxKey_KP_0			0xFFB0
#define kOfxKey_KP_1			0xFFB1
#define kOfxKey_KP_2			0xFFB2
#define kOfxKey_KP_3			0xFFB3
#define kOfxKey_KP_4			0xFFB4
#define kOfxKey_KP_5			0xFFB5
#define kOfxKey_KP_6			0xFFB6
#define kOfxKey_KP_7			0xFFB7
#define kOfxKey_KP_8			0xFFB8
#define kOfxKey_KP_9			0xFFB9



/*
 * Auxilliary Functions; note the duplicate definitions for left and right
 * function keys;  Sun keyboards and a few other manufactures have such
 * function key groups on the left and/or right sides of the keyboard.
 * We've not found a keyboard with more than 35 function keys total.
 */

#define kOfxKey_F1			0xFFBE
#define kOfxKey_F2			0xFFBF
#define kOfxKey_F3			0xFFC0
#define kOfxKey_F4			0xFFC1
#define kOfxKey_F5			0xFFC2
#define kOfxKey_F6			0xFFC3
#define kOfxKey_F7			0xFFC4
#define kOfxKey_F8			0xFFC5
#define kOfxKey_F9			0xFFC6
#define kOfxKey_F10			0xFFC7
#define kOfxKey_F11			0xFFC8
#define kOfxKey_L1			0xFFC8
#define kOfxKey_F12			0xFFC9
#define kOfxKey_L2			0xFFC9
#define kOfxKey_F13			0xFFCA
#define kOfxKey_L3			0xFFCA
#define kOfxKey_F14			0xFFCB
#define kOfxKey_L4			0xFFCB
#define kOfxKey_F15			0xFFCC
#define kOfxKey_L5			0xFFCC
#define kOfxKey_F16			0xFFCD
#define kOfxKey_L6			0xFFCD
#define kOfxKey_F17			0xFFCE
#define kOfxKey_L7			0xFFCE
#define kOfxKey_F18			0xFFCF
#define kOfxKey_L8			0xFFCF
#define kOfxKey_F19			0xFFD0
#define kOfxKey_L9			0xFFD0
#define kOfxKey_F20			0xFFD1
#define kOfxKey_L10			0xFFD1
#define kOfxKey_F21			0xFFD2
#define kOfxKey_R1			0xFFD2
#define kOfxKey_F22			0xFFD3
#define kOfxKey_R2			0xFFD3
#define kOfxKey_F23			0xFFD4
#define kOfxKey_R3			0xFFD4
#define kOfxKey_F24			0xFFD5
#define kOfxKey_R4			0xFFD5
#define kOfxKey_F25			0xFFD6
#define kOfxKey_R5			0xFFD6
#define kOfxKey_F26			0xFFD7
#define kOfxKey_R6			0xFFD7
#define kOfxKey_F27			0xFFD8
#define kOfxKey_R7			0xFFD8
#define kOfxKey_F28			0xFFD9
#define kOfxKey_R8			0xFFD9
#define kOfxKey_F29			0xFFDA
#define kOfxKey_R9			0xFFDA
#define kOfxKey_F30			0xFFDB
#define kOfxKey_R10			0xFFDB
#define kOfxKey_F31			0xFFDC
#define kOfxKey_R11			0xFFDC
#define kOfxKey_F32			0xFFDD
#define kOfxKey_R12			0xFFDD
#define kOfxKey_F33			0xFFDE
#define kOfxKey_R13			0xFFDE
#define kOfxKey_F34			0xFFDF
#define kOfxKey_R14			0xFFDF
#define kOfxKey_F35			0xFFE0
#define kOfxKey_R15			0xFFE0

/* Modifiers */

#define kOfxKey_Shift_L		0xFFE1	/* Left shift */
#define kOfxKey_Shift_R		0xFFE2	/* Right shift */
#define kOfxKey_Control_L		0xFFE3	/* Left control */
#define kOfxKey_Control_R		0xFFE4	/* Right control */
#define kOfxKey_Caps_Lock		0xFFE5	/* Caps lock */
#define kOfxKey_Shift_Lock		0xFFE6	/* Shift lock */

#define kOfxKey_Meta_L		0xFFE7	/* Left meta */
#define kOfxKey_Meta_R		0xFFE8	/* Right meta */
#define kOfxKey_Alt_L		0xFFE9	/* Left alt */
#define kOfxKey_Alt_R		0xFFEA	/* Right alt */
#define kOfxKey_Super_L		0xFFEB	/* Left super */
#define kOfxKey_Super_R		0xFFEC	/* Right super */
#define kOfxKey_Hyper_L		0xFFED	/* Left hyper */
#define kOfxKey_Hyper_R		0xFFEE	/* Right hyper */

#define kOfxKey_space               0x020
#define kOfxKey_exclam              0x021
#define kOfxKey_quotedbl            0x022
#define kOfxKey_numbersign          0x023
#define kOfxKey_dollar              0x024
#define kOfxKey_percent             0x025
#define kOfxKey_ampersand           0x026
#define kOfxKey_apostrophe          0x027
#define kOfxKey_quoteright          0x027	/* deprecated */
#define kOfxKey_parenleft           0x028
#define kOfxKey_parenright          0x029
#define kOfxKey_asterisk            0x02a
#define kOfxKey_plus                0x02b
#define kOfxKey_comma               0x02c
#define kOfxKey_minus               0x02d
#define kOfxKey_period              0x02e
#define kOfxKey_slash               0x02f
#define kOfxKey_0                   0x030
#define kOfxKey_1                   0x031
#define kOfxKey_2                   0x032
#define kOfxKey_3                   0x033
#define kOfxKey_4                   0x034
#define kOfxKey_5                   0x035
#define kOfxKey_6                   0x036
#define kOfxKey_7                   0x037
#define kOfxKey_8                   0x038
#define kOfxKey_9                   0x039
#define kOfxKey_colon               0x03a
#define kOfxKey_semicolon           0x03b
#define kOfxKey_less                0x03c
#define kOfxKey_equal               0x03d
#define kOfxKey_greater             0x03e
#define kOfxKey_question            0x03f
#define kOfxKey_at                  0x040
#define kOfxKey_A                   0x041
#define kOfxKey_B                   0x042
#define kOfxKey_C                   0x043
#define kOfxKey_D                   0x044
#define kOfxKey_E                   0x045
#define kOfxKey_F                   0x046
#define kOfxKey_G                   0x047
#define kOfxKey_H                   0x048
#define kOfxKey_I                   0x049
#define kOfxKey_J                   0x04a
#define kOfxKey_K                   0x04b
#define kOfxKey_L                   0x04c
#define kOfxKey_M                   0x04d
#define kOfxKey_N                   0x04e
#define kOfxKey_O                   0x04f
#define kOfxKey_P                   0x050
#define kOfxKey_Q                   0x051
#define kOfxKey_R                   0x052
#define kOfxKey_S                   0x053
#define kOfxKey_T                   0x054
#define kOfxKey_U                   0x055
#define kOfxKey_V                   0x056
#define kOfxKey_W                   0x057
#define kOfxKey_X                   0x058
#define kOfxKey_Y                   0x059
#define kOfxKey_Z                   0x05a
#define kOfxKey_bracketleft         0x05b
#define kOfxKey_backslash           0x05c
#define kOfxKey_bracketright        0x05d
#define kOfxKey_asciicircum         0x05e
#define kOfxKey_underscore          0x05f
#define kOfxKey_grave               0x060
#define kOfxKey_quoteleft           0x060	/* deprecated */
#define kOfxKey_a                   0x061
#define kOfxKey_b                   0x062
#define kOfxKey_c                   0x063
#define kOfxKey_d                   0x064
#define kOfxKey_e                   0x065
#define kOfxKey_f                   0x066
#define kOfxKey_g                   0x067
#define kOfxKey_h                   0x068
#define kOfxKey_i                   0x069
#define kOfxKey_j                   0x06a
#define kOfxKey_k                   0x06b
#define kOfxKey_l                   0x06c
#define kOfxKey_m                   0x06d
#define kOfxKey_n                   0x06e
#define kOfxKey_o                   0x06f
#define kOfxKey_p                   0x070
#define kOfxKey_q                   0x071
#define kOfxKey_r                   0x072
#define kOfxKey_s                   0x073
#define kOfxKey_t                   0x074
#define kOfxKey_u                   0x075
#define kOfxKey_v                   0x076
#define kOfxKey_w                   0x077
#define kOfxKey_x                   0x078
#define kOfxKey_y                   0x079
#define kOfxKey_z                   0x07a
#define kOfxKey_braceleft           0x07b
#define kOfxKey_bar                 0x07c
#define kOfxKey_braceright          0x07d
#define kOfxKey_asciitilde          0x07e

#define kOfxKey_nobreakspace        0x0a0
#define kOfxKey_exclamdown          0x0a1
#define kOfxKey_cent        	       0x0a2
#define kOfxKey_sterling            0x0a3
#define kOfxKey_currency            0x0a4
#define kOfxKey_yen                 0x0a5
#define kOfxKey_brokenbar           0x0a6
#define kOfxKey_section             0x0a7
#define kOfxKey_diaeresis           0x0a8
#define kOfxKey_copyright           0x0a9
#define kOfxKey_ordfeminine         0x0aa
#define kOfxKey_guillemotleft       0x0ab	/* left angle quotation mark */
#define kOfxKey_notsign             0x0ac
#define kOfxKey_hyphen              0x0ad
#define kOfxKey_registered          0x0ae
#define kOfxKey_macron              0x0af
#define kOfxKey_degree              0x0b0
#define kOfxKey_plusminus           0x0b1
#define kOfxKey_twosuperior         0x0b2
#define kOfxKey_threesuperior       0x0b3
#define kOfxKey_acute               0x0b4
#define kOfxKey_mu                  0x0b5
#define kOfxKey_paragraph           0x0b6
#define kOfxKey_periodcentered      0x0b7
#define kOfxKey_cedilla             0x0b8
#define kOfxKey_onesuperior         0x0b9
#define kOfxKey_masculine           0x0ba
#define kOfxKey_guillemotright      0x0bb	/* right angle quotation mark */
#define kOfxKey_onequarter          0x0bc
#define kOfxKey_onehalf             0x0bd
#define kOfxKey_threequarters       0x0be
#define kOfxKey_questiondown        0x0bf
#define kOfxKey_Agrave              0x0c0
#define kOfxKey_Aacute              0x0c1
#define kOfxKey_Acircumflex         0x0c2
#define kOfxKey_Atilde              0x0c3
#define kOfxKey_Adiaeresis          0x0c4
#define kOfxKey_Aring               0x0c5
#define kOfxKey_AE                  0x0c6
#define kOfxKey_Ccedilla            0x0c7
#define kOfxKey_Egrave              0x0c8
#define kOfxKey_Eacute              0x0c9
#define kOfxKey_Ecircumflex         0x0ca
#define kOfxKey_Ediaeresis          0x0cb
#define kOfxKey_Igrave              0x0cc
#define kOfxKey_Iacute              0x0cd
#define kOfxKey_Icircumflex         0x0ce
#define kOfxKey_Idiaeresis          0x0cf
#define kOfxKey_ETH                 0x0d0
#define kOfxKey_Eth                 0x0d0	/* deprecated */
#define kOfxKey_Ntilde              0x0d1
#define kOfxKey_Ograve              0x0d2
#define kOfxKey_Oacute              0x0d3
#define kOfxKey_Ocircumflex         0x0d4
#define kOfxKey_Otilde              0x0d5
#define kOfxKey_Odiaeresis          0x0d6
#define kOfxKey_multiply            0x0d7
#define kOfxKey_Ooblique            0x0d8
#define kOfxKey_Ugrave              0x0d9
#define kOfxKey_Uacute              0x0da
#define kOfxKey_Ucircumflex         0x0db
#define kOfxKey_Udiaeresis          0x0dc
#define kOfxKey_Yacute              0x0dd
#define kOfxKey_THORN               0x0de
#define kOfxKey_ssharp              0x0df
#define kOfxKey_agrave              0x0e0
#define kOfxKey_aacute              0x0e1
#define kOfxKey_acircumflex         0x0e2
#define kOfxKey_atilde              0x0e3
#define kOfxKey_adiaeresis          0x0e4
#define kOfxKey_aring               0x0e5
#define kOfxKey_ae                  0x0e6
#define kOfxKey_ccedilla            0x0e7
#define kOfxKey_egrave              0x0e8
#define kOfxKey_eacute              0x0e9
#define kOfxKey_ecircumflex         0x0ea
#define kOfxKey_ediaeresis          0x0eb
#define kOfxKey_igrave              0x0ec
#define kOfxKey_iacute              0x0ed
#define kOfxKey_icircumflex         0x0ee
#define kOfxKey_idiaeresis          0x0ef
#define kOfxKey_eth                 0x0f0
#define kOfxKey_ntilde              0x0f1
#define kOfxKey_ograve              0x0f2
#define kOfxKey_oacute              0x0f3
#define kOfxKey_ocircumflex         0x0f4
#define kOfxKey_otilde              0x0f5
#define kOfxKey_odiaeresis          0x0f6
#define kOfxKey_division            0x0f7
#define kOfxKey_oslash              0x0f8
#define kOfxKey_ugrave              0x0f9
#define kOfxKey_uacute              0x0fa
#define kOfxKey_ucircumflex         0x0fb
#define kOfxKey_udiaeresis          0x0fc
#define kOfxKey_yacute              0x0fd
#define kOfxKey_thorn               0x0fe
#define kOfxKey_ydiaeresis          0x0ff

/*@}*/

#endif
