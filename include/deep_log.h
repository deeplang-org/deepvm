/*
Copyright © 2020 <copyright holders>

Permission is hereby granted, free of charge, to any person obtaining 
a copy of this software and associated documentation files (the “Software”), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software 
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

Author: chinesebear
Email: swubear@163.com
Website: http://chinesebear.github.io
Date: 2020/8/10
Description: head file of dump/debug funtions for logs
*/

#ifndef _DEEP_LOG_H
#define _DEEP_LOG_H


#ifdef __cplusplus
extern "C" {
#endif


void log_printf (const char* pFileName, unsigned int uiLine, const char* pFuncName, const char *pFlag, char *LogFmtBuf, ...);
void log_data(const char *pFileName, unsigned int uiLine, const char* pFuncName, const char *pcStr,unsigned char *pucBuf,unsigned int usLen);
#define deep_error(...)                             log_printf(__FILE__, __LINE__,__FUNCTION__,"<error>",__VA_ARGS__)
#define deep_warn(...)                              log_printf(__FILE__, __LINE__,__FUNCTION__,"<warn>",__VA_ARGS__)
#define deep_debug(...)                             log_printf(__FILE__, __LINE__,__FUNCTION__,"<debug>",__VA_ARGS__)
#define deep_info(...)                              log_printf(__FILE__, __LINE__,__FUNCTION__,"<info>",__VA_ARGS__)
#define deep_dump(pcStr,pucBuf,usLen)               log_data(__FILE__, __LINE__,__FUNCTION__,pcStr,pucBuf,usLen)



#ifdef __cplusplus
}
#endif

#endif  /* _DEEP_LOG_H */
