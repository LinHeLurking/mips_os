#ifndef INCLUDE_STDIO_H_
#define INCLUDE_STDIO_H_

/* kernel printf */
int printk(const char *fmt, ...);

/* user printk */
int printf(const char *fmt, ...);

/* kernel print whose contents would be printed into screen */
int printks(const char *fmt, ...);

#endif
