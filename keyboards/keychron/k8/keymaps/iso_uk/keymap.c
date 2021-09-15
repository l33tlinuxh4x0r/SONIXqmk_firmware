/*
Copyright 2020 Dimitris Mantzouranis

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include QMK_KEYBOARD_H
#include "keymap_uk.h"

// Each layer gets a name for readability, which is then used in the keymap matrix below.
// The underscores don't mean anything - you can have a layer called STUFF or any other name.
// Layer names don't all need to be of the same length, obviously, and you can also skip them
// entirely and just use numbers.
enum layer_names {
    _BASE = 0,
    _FL,
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {


/*				+--------------------------------------------------------------------------+----------------+
				| ESC |  | F1 | F2 | F3 | F4 | | F5 | F6 | F7 | F8 | | F9| F10| F11| F12|  | |PSCR|????|PAUS|
				+--------------------------------------------------------------------------+------|----|----|
 				|  ~  |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 |  0 |  - |  = | BACKSP | |INS |SCRL|PGUP|
 				+--------------------------------------------------------------------------+------|----|----|
 				|  TAB  |  Q |  W |  E |  R |  T |  Y |  U |  I |  O |  P |  [ |  ] |   \  | |DEL |END |PGDN|
 				+--------------------------------------------------------------------------+------|----|----|
 				| CAPSLCK  |  A |  S |  D |  F |  G |  H |  J |  K |  L | ; | ' |  RETURN  |                |
 				+--------------------------------------------------------------------------+      |----|    |
 				| LSHIFT     |  Z |  X |  C |  V |  B |  N |  M | , | . |  / |   RSHIFT    |      | UP |    |
 				+--------------------------------------------------------------------------+------|----|----|
 				|LCTRL| LGUI| LALT |            SPACE            | RALT| RGUI | FN | RCTRL | |LFT |DWN |RGT |
 				+--------------------------------------------------------------------------+----------------+
*/
    /*  Row:        0           1          2          3        4         5        6         7        8        9          10         11         12         13         14         15         16    */
    [_BASE] = { {   KC_ESC,     KC_F1,     KC_F2,     KC_F3,   KC_F4,    KC_F5,   KC_F6,    KC_F7,   KC_F8,   KC_F9,     KC_F10,    KC_F11,    KC_F12,    KC_NO,     KC_PSCR,   TG(_FL),   RGB_MODE_FORWARD},
                {   UK_GRV,     UK_1,      UK_2,      UK_3,    UK_4,     UK_5,    UK_6,     UK_7,    UK_8,    UK_9,      UK_0,      UK_MINS,   UK_EQL,    KC_BSPC,   KC_INS,    KC_HOME,   KC_PGUP},
                {   KC_TAB,     KC_Q,      KC_W,      KC_E,    KC_R,     KC_T,    KC_Y,     KC_U,    KC_I,    KC_O,      KC_P,      UK_LBRC,   UK_RBRC,   KC_NO,     KC_DEL,    KC_END,    KC_PGDN},
                {   KC_CAPS,    KC_A,      KC_S,      KC_D,    KC_F,     KC_G,    KC_H,     KC_J,    KC_K,    KC_L,      UK_SCLN,   UK_QUOT,   UK_HASH,   KC_ENT,    KC_NO,     KC_NO,     KC_NO},
                {   KC_LSFT,    UK_BSLS,   KC_Z,      KC_X,    KC_C,     KC_V,    KC_B,     KC_N,    KC_M,    UK_COMM,   UK_DOT,    UK_SLSH,   KC_NO,     KC_RSFT,   KC_NO,     KC_UP,     KC_NO},
                {   KC_LCTL,    KC_LGUI,   KC_LALT,   KC_NO,   KC_NO,    KC_NO,   KC_SPC,   KC_NO,   KC_NO,   KC_NO,     KC_RALT,   KC_MS_BTN2, MO(_FL),  KC_RCTL,   KC_LEFT,   KC_DOWN,   KC_RGHT}
            },
    /*             Esc     F1        F2        F3       F4       F5       F6       F7       F8               F9       F10       F11           F12 */
    /*  Row:       0       1         2         3        4        5        6        7        8                9        10        11            12         13            14                15       16        */
    [_FL] =   { {  RESET,  KC_SLCK,  KC_PAUS, KC_APP,  _______, RGB_VAD, RGB_VAI, KC_MPRV, KC_MPLY,         KC_MNXT, KC_MUTE,  KC_VOLD,      KC_VOLU,    _______,     _______,          _______, _______ },		// F Key Row
                {  _______, _______, _______, _______, _______, _______, _______, _______, KC_KP_ASTERISK,  _______, _______,  KC_KP_MINUS,  KC_KP_PLUS, _______,     _______,          _______, _______ },
                {  _______, _______, _______, _______, _______, _______, _______, _______, _______,         _______, _______,  _______,      _______,    _______,     _______,          _______, _______ },
                {  _______, _______, _______, _______, _______, _______, _______, _______, _______,         _______, _______,  _______,      _______,    KC_KP_ENTER, _______,          _______, _______ },
                {  _______, _______, _______, _______, _______, _______, _______, _______, _______,         _______, _______,  KC_KP_SLASH,  _______,    _______,     _______,          KC_VOLU, _______ },
                {  _______, _______, _______, _______, _______, _______, _______, _______, _______,         _______, _______,  _______,      _______,    _______,     RGB_MODE_REVERSE, KC_VOLD, RGB_MODE_FORWARD }
              }
};

void dip_switch_update_user(uint8_t index, bool active){
  switch(index){
    case 0:
      if(active){ //BT mode
// do stuff
      }
      else{ //Cable mode
// do stuff
      }
      break;
    case 1:
      if(active){ // Win/Android mode
// do stuff
      }
      else{ // Mac/iOS mode
// do stuff
      }
      break;
  }
}

void keyboard_post_init_user(void) {
  // Customise these values to desired behaviour
//   debug_enable=true;
//   debug_matrix=true;
  //debug_keyboard=true;
  //debug_mouse=true;
}
