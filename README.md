# ARP_ASSIGNMENT-3 (S5065491 NAGALAKUNTA SUMANTH)

#Dining Philosophers
===================

Simple iteration of the dining philosophers problem, using only IPC via named pipe to schedule the access
to the shared resources.

Installing and Running
----------------------
Compile everything invoking the makefile already present in the folder
```bash
$ make
```

Then run eveything by calling the master process
```bash
$ ./dining_philosophers
```

It will automatically launch the other process as new windows.

## Requirements
The program requires the software "konsole" to be installed in order to open up new shell windows to
execute the subprocesses in.

## Programs

### dining_philosophers.c ###

The master process, first deletes FIFOs generated in previous iterations, then creates new ones whose name
is also known by waiter and philosophers. It spawns _waiter_ and _philosophers_ as new processes in new 
shell windows. The number of philosophers to be spawned can be modified by changing the value of
``` PH_NUM``` parameter.

### clean_tmp_fifo ###

A bash script to remove FIFO generated in previous iteration of _dining_philosophers_.

### waiter.c ###

The process is what coordinates the access to the shared resources, here represented by the 
```chop[]``` array _(yes, I considered the formulation with chopstick and rice, why should people
eat spaghetti with two forks!?)_.   
It manages to do so in an unending loop, by first checking if some philosophers released the two chopsticks they were holding,
then assessing requests for chopsticks pairs by other philosophers. These are written inside a FIFO 
first opened in read only mode. Among all requests (checked with a _select_) only one is granted access 
to the chopsticks, chosen at random. This is communicated to the lucky philosopher by closing that same FIFO 
than reopening it in write mode, sending a placeholder character, which will be read unblocking that process.   
The FIFO is then closed and reopened again in readonly mode.

### philosopher.c ###

The process opens up the FIFOs in write only mode.
In the endless cycle it first _thinks_, sleeping for a random number of microseconds, than it gets 
naturally hungry, requesting the two closest chopsticks by writing a placeholder character in a 
*request FIFO*. It then closes the FIFO and reopens it in readonly mode, waiting for a positive 
answer from the _waiter_ (which writes on that same pipe as mentioned).  
**NOTE:** the read is probably not mandatory, since closing and reopening the named pipe in read mode would make 
the process hang (obtaining the same effect of a blocking read) until the other end gets open as 
well. It's just placed here as an extra layer of security.  
Eating is modeled by another sleep of a random number of microseconds, followed by another placeholder 
character written on a *release FIFO*, in order to notify the _waiter_ the chopsticks are now free.



