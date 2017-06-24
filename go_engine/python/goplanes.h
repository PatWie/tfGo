#ifndef GOPLANES_H
#define GOPLANES_H

int planes_from_file(char* str, int strlen, int* data, int len, int moves);
int planes_from_bytes(char *bytes, int byteslen, int* data, int len, int moves);

void planes_from_position(int* bwhite, int wm, int wn, 
                          int* bblack, int bm, int bn, 
                          int* data, int len, 
                          int is_white);
#endif