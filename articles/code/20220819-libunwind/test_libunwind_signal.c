#include <sys/types.h>
#include <signal.h>

extern void unwind_by_libunwind();

static void signal_handler(int signo) 
{
    unwind_by_libunwind();
}

int main() {
  signal(SIGUSR1, signal_handler);
  raise(SIGUSR1);
  return 0;
}