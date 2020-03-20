#ifndef DEFER_H
#define DEFER_H

#ifndef DEFER_MAX_DEFERRED_STATEMENTS
# define DEFER_MAX_DEFERRED_STATEMENTS 32
#endif

#ifdef defined(__GNUC__) || defined(__TINYC__) || (defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86)))

#if defined(__GNUC__) || defined(__TINYC__)
#define TakeLabelAddress(dest, n) (dest) = && _defer_tokpaste(_defer_ini, n)
#define GotoLabelAddress(a) goto *(a)
#else
#define TakeLabelAddress(dest, n) \
	__asm{ mov [dest],offset _defer_tokpaste(_defer_ini, n) }
#define GotoLabelAddress(a) do { _label = (a); __asm{ jmp _label } } while (0)

#endif

#define Deferral \
unsigned char _num_deferrals = 0; \
void *_defer_return_loc = 0, *_deferrals[DEFER_MAX_DEFERRED_STATEMENTS] = {0};

#ifdef __PCC__
# define Defer(block) _Defer(block, __LINE__)
# define Return _Return(__LINE__)
#else
# define Defer(block) _Defer(block, __COUNTER__)
# define Return _Return(__COUNTER__)
#endif

#define _defer_tokpaste(a, b) a ## b

#define _Defer(block, n) do { \
	TakeLabelAddress(_deferrals[_num_deferrals++], n); \
	if (0) { \
		_defer_tokpaste(_defer_ini, n): \
		block; \
		if (_num_deferrals) { \
			GotoLabelAddress(_deferrals[--_num_deferrals]); \
		} else { \
			GotoLabelAddress(_defer_return_loc); \
		} \
	} \
} while (0)

#define _Return(n) \
	if (_num_deferrals) { \
		_defer_return_loc = && _defer_fini_ ## n; \
		GotoLabelAddress(_deferrals[--_num_deferrals]); \
	} \
\
	_defer_fini_ ## n: \
	return

#else /* !__GNUC__ && !__TINYCC__ */

#include <setjmp.h>

#ifdef _MSC_VER
# pragma message("You are using the unsafe longjmp()-based defer implementation.  Expect bugs if you don't know what you're doing.")
#else
# warning You are using the unsafe longjmp()-based defer implementation.  Expect bugs if you don't know what you're doing.
#endif

#define Deferral \
volatile unsigned char _num_deferrals = 0; \
jmp_buf _defer_return_loc = {0}, _deferrals[DEFER_MAX_DEFERRED_STATEMENTS] = {0};

#define Defer(block) do { \
	if (setjmp(_deferrals[_num_deferrals++])) { \
		block; \
		if (_num_deferrals) { \
			longjmp(_deferrals[--_num_deferrals], 1); \
		} else { \
			longjmp(_defer_return_loc, 1); \
		} \
	} \
} while (0)

/* TODO: better to have that break or not?  Need to check asm output */
#define Return do { \
	if (setjmp(_defer_return_loc)) break; \
\
	if (_num_deferrals) { \
		longjmp(_deferrals[--_num_deferrals], 1); \
	} \
} while (0); return

#endif /* __GNUC__ */

#endif /*DEFER_H*/
