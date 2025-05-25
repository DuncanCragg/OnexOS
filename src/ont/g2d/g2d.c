
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include <onex-kernel/log.h>

#include <mathlib.h>
#include <g2d.h>
#include <g2d-internal.h>

// ------------

typedef struct g2d_node {

  int16_t xtl;
  int16_t ytl;
  int16_t xbr;
  int16_t ybr;

  uint16_t clip_xtl;
  uint16_t clip_ytl;
  uint16_t clip_xbr;
  uint16_t clip_ybr;

  g2d_node_ev cb;
  uint16_t    cb_control;
  uint16_t    cb_data;

  uint8_t parent;

} g2d_node;

static g2d_node scenegraph[256]; // 1..255

static volatile uint8_t next_node=1; // index is same as node id, 0th not used

uint8_t g2d_node_create(uint8_t parent_id,
                        int16_t x, int16_t y,
                        uint16_t w, uint16_t h,
                        g2d_node_ev cb,
                        uint16_t cb_control, uint16_t cb_data){

  if(!parent_id) next_node=1;

  if(!next_node) return 0; // wrapped over 255 to 0

  int16_t  parent_xtl=0;
  int16_t  parent_ytl=0;
  uint16_t parent_clip_xtl=0;
  uint16_t parent_clip_ytl=0;
  uint16_t parent_clip_xbr=32000;
  uint16_t parent_clip_ybr=32000;
  if(parent_id){
    parent_xtl=scenegraph[parent_id].xtl;
    parent_ytl=scenegraph[parent_id].ytl;
    parent_clip_xtl=scenegraph[parent_id].clip_xtl;
    parent_clip_ytl=scenegraph[parent_id].clip_ytl;
    parent_clip_xbr=scenegraph[parent_id].clip_xbr;
    parent_clip_ybr=scenegraph[parent_id].clip_ybr;
  }
  int16_t xtl=parent_xtl+x;
  int16_t ytl=parent_ytl+y;
  int16_t xbr=parent_xtl+x+w;
  int16_t ybr=parent_ytl+y+h;

  if(xtl > parent_clip_xbr) return 0;
  if(ytl > parent_clip_ybr) return 0;
  if(xbr < parent_clip_xtl) return 0;
  if(ybr < parent_clip_ytl) return 0;

  scenegraph[next_node].xtl=xtl;
  scenegraph[next_node].ytl=ytl;
  scenegraph[next_node].xbr=xbr;
  scenegraph[next_node].ybr=ybr;

  scenegraph[next_node].clip_xtl=max(parent_clip_xtl, max(xtl,0));
  scenegraph[next_node].clip_ytl=max(parent_clip_ytl, max(ytl,0));
  scenegraph[next_node].clip_xbr=min(parent_clip_xbr, max(xbr,0));
  scenegraph[next_node].clip_ybr=min(parent_clip_ybr, max(ybr,0));

  scenegraph[next_node].cb=cb;
  scenegraph[next_node].cb_control=cb_control;
  scenegraph[next_node].cb_data=cb_data;
  scenegraph[next_node].parent=parent_id;

  return next_node++;
}

// ------------- geometry

void g2d_node_rectangle(uint8_t node_id,
                        int16_t x, int16_t y,
                        uint16_t w, uint16_t h,
                        uint16_t colour) {
  if(!node_id) return;

  uint16_t xtlx=max(scenegraph[node_id].xtl+x,0);
  uint16_t ytly=max(scenegraph[node_id].ytl+y,0);
  uint16_t xbrw=max(xtlx+w,0);
  uint16_t ybrh=max(ytly+h,0);
  uint16_t cxtl=max(xtlx, scenegraph[node_id].clip_xtl);
  uint16_t cytl=max(ytly, scenegraph[node_id].clip_ytl);
  uint16_t cxbr=min(xbrw, scenegraph[node_id].clip_xbr);
  uint16_t cybr=min(ybrh, scenegraph[node_id].clip_ybr);

  g2d_internal_rectangle(cxtl, cytl, cxbr, cybr, colour);
}

// ------------- text

void g2d_node_text(uint8_t node_id, int16_t x, int16_t y,
                   uint16_t colour, uint16_t bg, uint8_t size, char* fmt, ...){

  char text[128]; // XXX
  va_list args;
  va_start(args, fmt);
  vsnprintf(text, 128, fmt, args);
  va_end(args);

  if(!node_id) return;

  int16_t ox=scenegraph[node_id].xtl+x;
  int16_t oy=scenegraph[node_id].ytl+y;

  uint16_t cxtl=scenegraph[node_id].clip_xtl;
  uint16_t cytl=scenegraph[node_id].clip_ytl;
  uint16_t cxbr=scenegraph[node_id].clip_xbr;
  uint16_t cybr=scenegraph[node_id].clip_ybr;

  g2d_internal_text(ox, oy, cxtl, cytl, cxbr, cybr, text, colour, bg, size);
}

// ---------------------------------------------------

uint16_t g2d_node_width(uint8_t node_id){
  if(!node_id) return 0;
  return scenegraph[node_id].xbr - scenegraph[node_id].xtl;
}

uint16_t g2d_node_height(uint8_t node_id){
  if(!node_id) return 0;
  return scenegraph[node_id].ybr - scenegraph[node_id].ytl;
}

// ---------------------------------------------------

static bool is_inside(uint8_t n, int16_t x, int16_t y){
  if(x<scenegraph[n].clip_xtl || x>scenegraph[n].clip_xbr) return false;
  if(y<scenegraph[n].clip_ytl || y>scenegraph[n].clip_ybr) return false;
  return true;
}

static volatile bool event_pending=false;

void g2d_touch_event(bool down, uint16_t tx, uint16_t ty){

  static uint16_t last_tx=0;
  static uint16_t last_ty=0;

  int16_t dx=0;
  int16_t dy=0;

  if(down){
    if(last_tx + last_ty){
      dx = (tx - last_tx);
      dy = (ty - last_ty);
    }
    last_tx=tx;
    last_ty=ty;
  }
  else{
    last_tx=0;
    last_ty=0;
  }

  static uint8_t     cb_node=0;
  static g2d_node_ev drag_cb=0;
  static uint16_t    drag_cb_control=0;
  static uint16_t    drag_cb_data=0;

  for(uint8_t n=next_node-1; n && !cb_node; n--){

    if(!is_inside(n, tx, ty)) continue;

    cb_node=n;
    while(cb_node && !scenegraph[cb_node].cb) cb_node=scenegraph[cb_node].parent;
  }

  if(cb_node){
    if(!drag_cb){
      drag_cb=scenegraph[cb_node].cb;
      drag_cb_control=scenegraph[cb_node].cb_control;
      drag_cb_data=scenegraph[cb_node].cb_data;
    }
    drag_cb(down, dx, dy, drag_cb_control, drag_cb_data);
    event_pending=true;
  }

  if(!down){
    cb_node=0;
    drag_cb=0;
    drag_cb_control=0;
    drag_cb_data=0;
  }
}

bool g2d_pending(){
  if(!event_pending) return false;
  event_pending=false;
  return true;
}

// ------------










