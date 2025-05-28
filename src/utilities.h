#ifndef UTILITIES_H
#define UTILITIES_H

#define offsetof(type, member) __builtin_offsetof(type, member)
#define typeof_member(T, m)	typeof(((T*)0)->m)

#define container_of(ptr, type, member) ({				        \
	void *__mptr = (void *)(ptr);					            \
	static_assert(__same_type(*(ptr), ((type *)0)->member) ||	\
		      __same_type(*(ptr), void),			            \
		      "pointer type mismatch in container_of()");	    \
	((type *)(__mptr - offsetof(type, member))); })



#endif