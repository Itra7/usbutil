#ifndef UTILITIES_H
#define UTILITIES_H

#define offsetof(type, member) __builtin_offsetof(type, member)
#define typeof_member(T, m)	typeof(((T*)0)->m)

#define container_of(ptr, type, member) ({				        \
	void *__mptr = (void *)(ptr);					            \
	((type *)(__mptr - offsetof(type, member))); })



#endif