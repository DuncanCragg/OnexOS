
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <g2d.h>

uint16_t del_this_word=0;
char*    add_this_word=0;

uint16_t word_index=1;
bool     in_word=false;
char     edit_word[64];
uint8_t  cursor=0;

uint8_t kbd_page=1;
int16_t kbd_x=0;
int16_t kbd_y=0;

int16_t text_scroll_offset=0;

// ---------------------- keyboard ------------------------ {

static void del_word(){
  if(word_index==1) return;
  del_this_word=word_index-1;
  add_this_word=0;
  word_index--;
}

static void add_word(){
  del_this_word=0;
  add_this_word=edit_word;
  word_index++;
}

static void del_char() {
  if(in_word){
    if(cursor==0) return;
    edit_word[--cursor]=0;
    if(cursor==0) in_word=false;
    return;
  }
  del_word();
}

static void add_char(unsigned char c) {
  if(c!=' '){
    if(cursor==62) return;
    in_word=true;
    edit_word[cursor++]=c;
    edit_word[cursor]=0;
    return;
  }
  if(in_word) add_word();
  in_word=false;
  cursor=0;
}

static unsigned char kbd_pages[7][20]={

  { "     " },
  { "ERTIO"
    "ASDGH"
    "CBNM>"
    "^#  ~" },
  { "QWYUP"
    "FJKL "
    " ZXV<"
    "^#  ~" },
  { "+ ()-"
    "/'#@*"
    "%!;:>"
    "^#,.~" },
  { "^&[]_"
    " \"{}~"
    " ?<><"
    "^#,.~" },
  { "+123-"
    "/456*"
    "%789>"
    "^#0.=" },
  { "^123e"
    " 456 "
    " 789<"
    "^#0.=" }
};


#define SELECT_TYPE 15
#define SELECT_PAGE 14
#define DELETE_LAST 16

static void key_hit(bool down,
                    int16_t dx, int16_t dy,
                    uint16_t cb_control, uint16_t ki){

  if(down) return;

  if(ki==SELECT_TYPE){
    if(kbd_page==1 || kbd_page==2) kbd_page=3;
    else
    if(kbd_page==3 || kbd_page==4) kbd_page=5;
    else
    if(kbd_page==5 || kbd_page==6) kbd_page=1;
  }
  else
  if(ki==SELECT_PAGE){
    if(kbd_page==1) kbd_page=2;
    else
    if(kbd_page==2) kbd_page=1;
    else
    if(kbd_page==3) kbd_page=4;
    else
    if(kbd_page==4) kbd_page=3;
    else
    if(kbd_page==5) kbd_page=6;
    else
    if(kbd_page==6) kbd_page=5;
  }
  else
  if(ki==DELETE_LAST){

    del_char();
  }
  else {

    add_char(kbd_pages[kbd_page][ki]);

    if(kbd_page==2 || kbd_page==6) kbd_page--;
    else
    if(kbd_page==3 || kbd_page==4) kbd_page=1;
  }
}

#define KEY_SIZE  41
#define KEY_H_SPACE 7
#define KEY_V_SPACE 1

static void kbd_drag(bool down, int16_t dx, int16_t dy, uint16_t c, uint16_t d){
  if(!down || dx+dy==0) return;
  if(dx*dx>dy*dy) kbd_x+=dx;
  else            kbd_y+=dy;
}

void show_keyboard(uint8_t g2d_node){

  uint8_t kbd_g2d_node = g2d_node_create(g2d_node,
                                         kbd_x, kbd_y,
                                         g2d_node_width(g2d_node), 215,
                                         kbd_drag,0,0);
  if(!kbd_g2d_node) return;

  g2d_node_rectangle(kbd_g2d_node, 0,0,
                     g2d_node_width(kbd_g2d_node),g2d_node_height(kbd_g2d_node),
                     G2D_GREY_F);

  uint16_t kx=0;
  uint16_t ky=30;

  unsigned char pressed=0;

  for(uint8_t j=0; j<4; j++){
    for(uint8_t i=0; i<5; i++){

      uint16_t ki = i + j*5;

      unsigned char key = kbd_pages[kbd_page][ki];

      uint8_t key_g2d_node = g2d_node_create(kbd_g2d_node,
                                             kx, ky,
                                             KEY_SIZE+KEY_H_SPACE,
                                             KEY_SIZE+KEY_V_SPACE,
                                             key_hit, 0,ki);

      uint16_t key_bg=(pressed==key)? G2D_GREEN: G2D_GREY_1A;
      g2d_node_rectangle(key_g2d_node,
                         KEY_H_SPACE/2,KEY_V_SPACE/2,
                         KEY_SIZE,KEY_SIZE, key_bg);

      g2d_node_text(key_g2d_node, 13,7, G2D_BLACK, 4, "%c", key);

      kx+=KEY_SIZE+KEY_H_SPACE;
    }
    kx=0;
    ky+=KEY_SIZE+KEY_V_SPACE;
  }
}

// }--------------------------------------------------------


