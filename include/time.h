/* time.h - xenix86-devsys */
/* In 16-bit C an undeclared function returns int, which would cut the
   32-bit time in half. Declare it before use. */
long	time();
