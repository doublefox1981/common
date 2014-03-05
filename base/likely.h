/*
 * steal from facebook's folly
 */

#ifndef FOLLY_BASE_LIKELY_H_
#define FOLLY_BASE_LIKELY_H_

#undef LIKELY
#undef UNLIKELY

#if defined(__GNUC__) && __GNUC__ >= 4
#define LIKELY(x)   (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

#endif /* FOLLY_BASE_LIKELY_H_ */

