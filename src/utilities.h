#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdatomic.h>

#define offsetof(type, member) __builtin_offsetof(type, member)
#define typeof_member(T, m)	typeof(((T*)0)->m)

#define container_of(ptr, type, member) ({				        \
	void *__mptr = (void *)(ptr);					            \
	((type *)(__mptr - offsetof(type, member))); })

struct kref {
	atomic_int refcount;
};

static inline void kref_init(struct kref *kref) {
	atomic_store(&kref->refcount, 1);
}

static inline void kref_get(struct kref *kref) {
	atomic_fetch_add_explicit(&kref->refcount, 1, memory_order_relaxed);
}

static inline int kref_put(struct kref *kref, void (*release)(struct kref *kref)) {
	if (atomic_fetch_sub_explicit(&kref->refcount, 1, memory_order_acq_rel) == 1) {
		release(kref);
		return 1;
	}
	return 0;
}

static inline int kref_read(struct kref *kref) {
	return atomic_load(&kref->refcount);
}

#endif