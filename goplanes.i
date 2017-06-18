%module goplanes

%{
    #define SWIG_FILE_WITH_INIT
    #include "goplanes.h"
%}

%include "numpy.i"

%init %{
    import_array();
%}

%apply (int* INPLACE_ARRAY1, int DIM1) {(int* data, int len)}
%include "goplanes.h"