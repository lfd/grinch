#ifndef _CONST_H
#define _CONST_H

#ifdef __ASSEMBLY__
#define _AC(X,Y)	X
#define _AT(T,X)	X
#else /* !__ASSEMBLY__ */
#define __AC(X,Y)	(X##Y)
#define _AC(X,Y)	__AC(X,Y)
#define _AT(T,X)	((T)(X))
#endif

#define _BITUL(x)	(_UL(1) << (x))
#define _BITULL(x)	(_ULL(1) << (x))

#define _UL(x)		(_AC(x, UL))
#define _ULL(x)		(_AC(x, ULL))

#endif /* _CONST_H */
