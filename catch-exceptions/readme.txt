This test catches hardware exceptions in C++ code.
 - divide by zero
 - NPE / Access Violation
 - FPE
 - stack overflow
All exceptions are catch and execution continues.

Note:longjmp() is used to catch exceptions which could lead to memory leaks.
