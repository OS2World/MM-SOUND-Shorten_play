#include "shorten.hpp"

#if defined(DEBUG)

int debug(const char* str, ...) {
    const int bufsize=1024;
    static char* buf=new char[bufsize+1];
    static FILE* fp=NULL;
    if (fp==NULL) {
        fp=fopen("shorten.dbg","wt");
        if (fp) {
            }
        }
    if (fp) {
    va_list arg_ptr;
    va_start(arg_ptr, str);
    vsnprintf(buf,bufsize,str,arg_ptr);
    va_end(arg_ptr);
    fprintf(fp,"%s",buf);
    fflush(fp);
    }
    return -1;

    }
#endif
