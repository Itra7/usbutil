#include <stdio.h>
#include <stdarg.h>

#include "usbutilerrno.h"

void usbutil_dbg(int ERROR_CODE, const char *format, ...){    
    #ifdef USBDEBUG
        va_list args;
        va_start(args, format);
    
        fprintf(stdout, "%s ", error_msg[ERROR_CODE]);
    
        while(*format != '\0'){
            if(*format == '%' && *(format+1)){
                format++;
                switch(*format){
                    case 'd':{
                        int i = va_arg(args, int);
                        fprintf(stdout, "%d ", i);
                        break;
                    }
                    case 'c':{
                        int c = va_arg(args, int);
                        fprintf(stdout, "%c ", c);
                        break;
                    }
                    case 's':{
                        char* str = va_arg(args, char*);
                        fprintf(stdout, "%s ", str);
                        break;
                    }
                }
            }else{
                fprintf(stdout, "%c", *format);
            }
            ++format;
        }
        fprintf(stdout, "\n");
    
        va_end(args);
    #endif
        return;
}