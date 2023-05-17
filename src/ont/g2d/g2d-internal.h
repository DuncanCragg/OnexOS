#ifndef G2D_INTERNAL_H
#define G2D_INTERNAL_H

void g2d_node_rectangle_clipped(uint16_t cxtl, uint16_t cytl,
                                uint16_t cxbr, uint16_t cybr,
                                uint16_t colour);
void g2d_node_text_clipped(int16_t ox, int16_t oy,
                           uint16_t cxtl, uint16_t cytl,
                           uint16_t cxbr, uint16_t cybr,
                           char* text, uint16_t colour, uint16_t bg,
                           uint8_t size);

#endif
