# Operative Systems H4

<p align="center"> Concurrent search for prime numbers</p>

The goal of the homework is to write a multithreaded C program for Linux that provides searching over a directory structure, and finding prime numbers in files within that structure.

This task does not require work in the xv6 system, nor is it setup of branches of the xv6 repository.

The task involves independently reading man page documents and other materials in order to find the appropriate functions and system calls for directory manipulation and modification time.

It is necessary to form a test example on which all the requirements related to the homework can be illustrated, and submit it with the code. This test example should consist of a directory structure and files that can be used to test the application.

## Task organization

It is necessary to implement two types of threads within the homework that are created and stopped as needed during the application. These two threads are: watcher and worker. Each watcher thread will bind to a directory, and monitor it for changes. When a new file is found, or a change in a file is detected, a new worker thread will be employed for that file. Each worker thread will bind to a file, and will do the job of counting prime numbers within that file. The results of this count are stored in a forest that is the same shape as the directory structure - tree roots are the initial directories, leaves contain prime numbers in individual files, and nodes contain directory sums.

## Watcher threads

A Watcher thread is bound to a directory, and should do an infinite loop to scan that directory as well as lock it for 10 seconds. The following checks are performed in one scan:

● Whether a new file has appeared. If so, you need to employ a worker thread that will find how many prime numbers there are in that file, and report the result.

● Has any file for which we have the result changed since the last count. For each file for which we have a result, the time of the last modification of that file should be   stored. If that time has changed, then you need to reprocess the file using some worker thread.

● Whether a new directory has appeared. If so, you need to create a new watcher thread that will monitor that directory. If a watcher thread already exists for a directory, you do not need to create a new watcher thread.

● Whether a file has been removed. If so, you should remove its result and refresh all nodes above the removed sheet.

● Whether a subdirectory has been removed. If so, you need to refresh all the nodes above that node.

● Whether our directory has been removed. If so, the watcher thread should be turned off.

## Worker threads

The application should have a fixed number of worker threads that can be set when starting the application by asking the user to enter a value. Each time a watcher thread finds a new or modified file, it will employ one of these thread workers to perform a count over the file. If no worker thread is available, the watch thread must not be blocked. A job that was not started because there were no free workers must not be lost, but should be done as soon as a worker becomes free. Make sure that some jobs are not duplicated due to re-scanning the directory by the watcher thread. The worker thread should be blocked if a count does not work.

The files being processed will always be ASCII encoded files, and will contain arbitrary text. The job of a worker is not to read the whole file and find all the prime numbers in it. As a result of the work, the number of prime numbers within the file should be obtained. We will assume that these files can be very large, so it is necessary that these threads also report partial results. For every 1KB of processed data, the file should report the result as partial. When the worker thread completes the count, it should enter the result as final. The sheet that represents the file should contain the number of prime numbers within that file, while the nodes above it should contain the sum of the prime numbers for the directory they represent.

## User interaction

The user should be able to interact with the system completely freely, regardless of whether some jobs are performed in watcher and worker threads. The user can issue the following commands to the system:

add\_dir \<dir\>;

The argument to this command is a directory, given as a relative or absolute path. This directory is the new root for our system, and this command should create a watcher thread for it. If this directory is already present in the system, an error should be reported. There is no need to go deeper into this directory at this time, because its watcher thread will create a watcher thread for subdirectories.

remove\_dir \<dir\>;

The argument to this command is the directory that is rooted in one of the trees being viewed. If a path is specified for which this is not the case, an error should be reported. The specified tree is removed from the results, and all watcher threads for the directories in that tree are turned off.

result [path]

The argument to this command is the path within the result forest - it can be a directory or a file. If the path is for a file, then the result should be printed for that file, and if it is a directory, the number of prime numbers in that directory should be printed in a structured way, as well as for all its files and subdirectories. If the DirA directory contains FileA1, FileA2 and DirB subdirectories with FileB1 and FileB2 files, the printout could be:

-DirA: 30

--DirB: 20

---FileB1: 10

---FileB2: 10

--FileA1: 5

--FileA2: 5

 Partial results (ie results where a worker thread is still processing a file) are printed with an asterisk (\*) next to the number. This star will typically be located next to all the directories above the file, as their results will also be partial at that point. For example, if a count is currently being performed for FileB1, and only 8 prime numbers have been found so far, we will get a printout:

-DirA: 28\*

--DirB: 18\*

---FileB1: 8\*

---FileB2: 10

--FileA1: 5

--FileA2: 5

The argument for the result command is optional, and if it is omitted, then all results should be written, ie. the command behaves as if summoned for all roots.

## Concurrency

Worker threads should be created when the application starts and should typically be in a locked state. They are activated as needed, from the fit of the watcher thread.

Watcher threads are created when the add\_dir command is executed, or when a new directory is found by another watcher thread. These threads are extinguished when deleting a directory (outside the application), or when executing the remove\_dir command for the root, when all watcher threads in that tree are extinguished.

The result tree should always be consistent - it should not happen that phallic results are written in it at any time, or that the tree is not validly formed and that a thread can access an invalid pointer. It is not allowed to use one mutex to control the entire tree, but it is necessary to lock only those elements that are really necessary to lock at a given time.
