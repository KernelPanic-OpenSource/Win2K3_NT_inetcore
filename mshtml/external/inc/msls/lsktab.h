#ifndef LSKTAB_DEFINED
#define LSKTAB_DEFINED

#include "lsdefs.h"


enum lsktab								/* Kinds of tabs */
{
	lsktLeft,
	lsktCenter,
	lsktRight,
	lsktDecimal,
	lsktChar
};

typedef enum lsktab LSKTAB;


#endif  /* !LSKTAB_DEFINED     */
