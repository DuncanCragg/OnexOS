#ifndef MATHLIB_H
#define MATHLIB_H

// ---------------------------------

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a < _b ? _a : _b; })

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a > _b ? _a : _b; })

#define abs(x) \
   ({ __typeof__ (x) _x = (x); \
      _x < 0 ? -_x : _x; })

// ---------------------------------

#endif
