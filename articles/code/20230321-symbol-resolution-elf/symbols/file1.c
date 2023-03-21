// Undefined global symbols, prefixed with "u_g_"
extern int	u_g_bar;
extern int	u_g_foo(int);

// Tentative symbols, prefixed with "t_g_"
int		t_g_bar;

// Defined symbols, prefixed with "d_g_"
int		d_g_bar = 1;
int		d_g_bar_zero = 0;
int d_g_foo()
{
	// reference those undefined vars here,
	// otherwise they will be optimized out
	// in the object file
	return (u_g_foo(u_g_bar));
}

// local symbols, prefixed with "l_", decorated with "static"
static int l_bar = 1;
static int l_zero;
static int l_foo()
{
	return l_bar;
}

// weak symbols
#pragma weak w_bar
int w_bar = 1;
// another weak symbol definition style
__attribute__((weak)) int w_foo = 2;
// error: weak declaration of ‘w_local’ must be public
// __attribute__((weak)) static int w_local = 2;
