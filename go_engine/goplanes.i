%module goplanes

%{
    #define SWIG_FILE_WITH_INIT
    #include "goplanes.h"
%}

%include "numpy.i"

%init %{
    import_array();
%}

%apply (char *STRING, int LENGTH) {(char *str, int strlen)}
%apply (char *STRING, int LENGTH) {(char* bytes, int byteslen)}
%apply (int* INPLACE_ARRAY1, int DIM1) {(int* data, int len)}
%include "goplanes.h"