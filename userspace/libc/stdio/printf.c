#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include <stdarg.h>

void printf(const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    size_t len = strlen(s);
    for (int i = 0; i < len; i++)
    {
        if (s[i] == '%')
        {
            switch (s[i + 1])
            {
                case 'i':
                    int int_param = va_arg(ap, int);
                    char buffer[11];
                    int j = 0;

                    if (int_param == 0) 
                    {
                        write(1, "0", 1);
                        i++;
                        break;
                    }

                    while (int_param > 0) 
                    {
                        buffer[j++] = '0' + (int_param % 10);
                        int_param /= 10;
                    }

                    while (j > 0) 
                    {
                        write(1, &buffer[--j], 1);
                    }
                    i++;
                    break;
                case 'x':
                    int hex_param_lower = va_arg(ap, int);
                    char hex_digits_lower[] = "0123456789abcdef";

                    write(1, "0x", 2);
                    for (int j = 28; j >= 0; j -= 4)
                    {
                        int digit = (hex_param_lower >> j) & 0xF;
                        write(1, &hex_digits_lower[digit], 1);
                    }
                    i++;
                    break;
                case 'X':
                    int hex_param_upper = va_arg(ap, int);
                    char hex_digits_upper[] = "0123456789ABCDEF";

                    write(1, "0x", 2);
                    for (int j = 28; j >= 0; j -= 4)
                    {
                        int digit = (hex_param_upper >> j) & 0xF;
                        write(1, &hex_digits_upper[digit], 1);
                    }
                    i++;
                    break;
                case 's':
                    char *string_param = va_arg(ap, char*);
                    size_t char_len = strlen(string_param);
                    write(1, string_param, char_len);
                    i++;
                    break;
                case 'c':
                    char c = (char)va_arg(ap, int);
                    write(1, &c, 1);
                    i++;
                    break;
            }
        }
        else
        {
            write(1, &s[i], 1);
        }
    }
}
