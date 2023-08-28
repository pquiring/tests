/*

Test : Catching all hardware exceptions.

Windows : cl main.cpp

Linux : gcc -std=c++11 main.cpp -lstdc++ -lm

*/

#include <cstdio>
#include <cstdlib>
#include <setjmp.h>

#ifdef _WIN32
#include <windows.h>
#include <errhandlingapi.h>
#include <malloc.h>
#include <float.h>
#else
#include <signal.h>
#include <math.h>
#include <fenv.h>
#endif

#define count (16)
#define stack (4 * 1024)

extern "C" {

jmp_buf buf;  //NOTE : this should be thread local in a multi-threaded app (and stored in a queue if nested try / catch blocks are used)

#ifdef _WIN32
bool was_stack_overflow = false;  //NOTE : this should be thread local in a multi-threaded app
LONG exception_filter(_EXCEPTION_POINTERS *ExceptionInfo) {
  was_stack_overflow = ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW;
  longjmp(buf, 1);
  return 0;
}
#else
void handler(int sig,siginfo_t *info, void *context) {
  longjmp(buf, 1);
}
#endif

void func1(int z) {
  printf("func1:%d\n", z);
}

void func1f(float z) {
  printf("func1f:%f\n", z);
}

void func2(const char* msg, int z) {
  printf("%s:%d\n", msg, z);
}

void func3() {
  printf("done!\n");
}

int recursive(int val) {
  int data[1024];
  return recursive(val + 1);
}

//after a catch some cleanup is required
void after_catch() {
#ifdef _WIN32
  if (was_stack_overflow) {
    _resetstkoflw();
    //see this website for explaination of _resetstkoflw() https://jeffpar.github.io/kbarchive/kb/315/Q315937/
  }
#else
  //on Linux need to re-enable FPEs after a longjmp()
  feenableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW);
#endif
}

}

int main(int argc, const char **argv) {
  int z = 0;
  float fz = 0.0f;
#ifdef _WIN32
  printf("Win32 setup\n");
  SetUnhandledExceptionFilter(exception_filter);
  //enable floating point exceptions
  unsigned int control_word;
  _controlfp_s(&control_word, _MCW_DN, 0xffff);
#else
  printf("Linux setup\n");
  stack_t altstack;
  altstack.ss_sp =  malloc(stack);
  altstack.ss_flags = 0;
  altstack.ss_size = stack;

  sigaltstack(&altstack, NULL);
  //NOTE : sigaltstack with SA_ONSTACK is required to catch stack overflows

  struct sigaction act = { 0 };
  act.sa_flags = SA_SIGINFO | SA_NODEFER | SA_ONSTACK;
  act.sa_sigaction = &handler;

  sigaction(SIGSEGV, &act,NULL);
  sigaction(SIGFPE, &act,NULL);
//  sigaction(SIGILL, &act,NULL);
  //enable float point exceptions
  feenableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW);
#endif
  int jmp;
  int cntr;

  cntr=0;
  for(int a=0;a<count;a++) {
    jmp = setjmp(buf);
    if (jmp == 0) {
      // try : div by zero
      int x = 123;
      int y = 0;
      z = x / y;
      func1(z);
    } else {
      //catch block
      after_catch();
      cntr++;
    }
  }
  func2("/0", cntr);

  cntr=0;
  for(int a=0;a<count;a++) {
    jmp = setjmp(buf);
    if (jmp == 0) {
      //try : NPE
      int *p = NULL;
      *p = 0;
      func1(z);
    } else {
      //catch block
      after_catch();
      cntr++;
    }
  }
  func2("NPE", cntr);

  cntr=0;
  for(int a=0;a<count;a++) {
    jmp = setjmp(buf);
    if (jmp == 0) {
      //try : FPE
      float fx = 123.0f;
      float fy = 0.0f;
      fz = fx / fy;
      func1f(fz);
    } else {
      //catch block
      after_catch();
      cntr++;
    }
  }
  func2("FPE", cntr);

  cntr=0;
  for(int a=0;a<count;a++) {
    jmp = setjmp(buf);
    if (jmp == 0) {
      //try : stack overflow
      recursive(0);
      func1(z);
    } else {
      //catch block
      after_catch();
      cntr++;
    }
  }
  func2("StackOverflow", cntr);

  func3();

  return 0;
}
