Scheduling in xv6
=================

- `getreadcount` and `sigalarm` and `sigreturn` syscalls were added to xv6.
    - The `getreadcount` syscall returns the number of times the process has been scheduled. 
    - The `sigalarm` syscall sets the alarm for the process. 
    - The `sigreturn` syscall returns the process to the state it was in before the signal was raised.

- FCFS and MLFQ scheduling policies have been implemented in xv6.
