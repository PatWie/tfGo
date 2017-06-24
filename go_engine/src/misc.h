// Author: Patrick Wieschollek <mail@patwie.com>

#ifndef ENGINE_MISC_H
#define ENGINE_MISC_H

const int N = 19;

#define map2line(x,y) (((x) * 19 + (y)))
#define valid_pos(x) (((x>=0) && (x < N)))

#endif