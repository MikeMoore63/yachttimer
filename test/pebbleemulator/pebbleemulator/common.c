/*
 * Pebble Stopwatch - common utilities
 * Copyright (C) 2013 Katharine Berry
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "yachtimermodel.h"
static    PblTm toFormat;

void itoa1(int num, char* buffer) {
    const char digits[10] = "0123456789";
    buffer[0] = digits[num % 10];
}
void itoa2(int num, char* buffer) {
    const char digits[10] = "0123456789";
    if(num > 99) {
        buffer[0] = '9';
        buffer[1] = '9';
        return;
    } else if(num > 9) {
        buffer[0] = digits[num / 10];
    } else {
        buffer[0] = '0';
    }
    buffer[1] = digits[num % 10];
}
void format_lap(time_t lap_time, char* buffer,int bufferlen) {
    yachtimer_setPblTime(&toFormat,lap_time / ASECOND);
    int hundredths = (lap_time / DECISECOND) % 10;
    int hours = lap_time / (60 * 60 * ASECOND);
    
    if(hours < 24)
    {
    	string_format_time(buffer, bufferlen, "%R:%S.",&toFormat);

    	itoa1(hundredths, &buffer[9]);
        *(buffer+10)='\0';
    }
    else
    {
    	string_format_time(buffer, bufferlen, "%w:%H:%M:%S.",&toFormat);
    }
}
