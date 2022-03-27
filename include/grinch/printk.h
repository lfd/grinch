#ifndef _PRINTK_H
#define _PRINTK_H

void puts(const char *msg);
void _puts(const char *msg);
void __attribute__((format(printf, 1, 2))) printk(const char *fmt, ...);

#ifndef dbg_fmt
#define dbg_fmt(__x) (__x)
#endif

#define pr(fmt, ...) printk(dbg_fmt(fmt), ##__VA_ARGS__)
#define ps(str) puts(dbg_fmt(str))

#define trace_error(code) ({						  \
	printk("%s:%d: returning error %s\n", __FILE__, __LINE__, #code); \
	code;								  \
})

#endif /* _PRINTK_H */
