# Semaphores
Orion manages a number of semaphores for protection of delicate data and resources such as

* Mongo Requests
* Transaction ID
* Subscription Cache
* Timing Statistics
* Mongo Connection Pool
* Metrics Manager
* Alarm Manager
* Log File
* Notification Queue
* Notification Queue Statistics

Of these semaphores, the first four use helper functions in `lib/common/sem.[cpp|h]`, while the others are part of their
respective structure/class.


## Mongo Request Semaphore
The *Mongo Request Semaphore* resides in `lib/common/sem.cpp` and its semaphore variable is	`reqSem`.  
The functions to take/give the semaphore are	`reqSemTake` and `reqSemGive`.  
The function `reqSemTake` is somewhat special as there are four different working modes for the Mongo Request Semaphore:

* None
* Only for **Read** operations
* Only for **Write** operations
* Both **Read and Write** operations

None meaning that the semaphore isn't used.  
The operation mode of this semaphore is set using the CLI option `-reqMutexPolicy`. Default value is "Both Read and	Write operations".

This semaphore is used for each and every request to the database - **only** by top level functions of mongoBackend, i.e. those functions
that are external and called by service routines.


## Transaction ID Semaphore
The *Transaction ID Semaphore* resides in `lib/common/sem.cpp` and its semaphore variable is `transSem`.  
The functions to take/give the semaphore are  `transSemTake` and `transSemGive`.  

Each REST request that Orion receives is given a unique **Transaction ID**:  

To ensure a unique identifier of the transaction, the startTime down to milliseconds
of the broker is used as prefix (to almost guarantee its uniqueness among brokers)
Furthermore, a running number is appended for the transaction.
A 32 bit signed number is used, so its max value is 0x7FFFFFFF (2,147,483,647).

If the running number overflows, a millisecond is added to the start time of the broker.
As the running number starts from 1 again after overflow, we need this to distinguish the first transaction
after a running number overflow from the VERY first transaction (as both will have running number 1).
Imagine that the start time of the broker is XXXXXXXXX.123:

* XXXXXXXXX.123.1  # the VERY first transaction
* XXXXXXXXX.124.1  # the first transaction after running number overflow

The whole thing is stored in the thread variable 'transactionId', supported by the logging library 'liblm'.  

Now, the **running number** needs to be protected when incremented and this semaphore is used for that purpose.
See the function `transactionIdSet` in the file `lib/common/globals.cpp`.


## Subscription Cache Semaphore
The *Subscription Cache Semaphore* resides in `lib/common/sem.cpp` and its semaphore variable is `cacheSem`.  
The functions to take/give the semaphore are `cacheSemTake` and `cacheSemGive` and they are used by functions in two different libraries:

* mongoBackend and
* cache

Due to the implementation of the subscription cache, especially how it is refreshed, this semaphore cannot be used in low level functions of the
cache library, as one would suspect, but rather in higher level functions, which makes the implementation a little bit tricky.
Any changes in where this semaphore is taken/given needs careful consideration.  

Details on this semaphore is already present in the [dedicated document of the Subscription Cache](subscriptionCache.md)


## Timing Statistics Semaphore
The *Timing Statistics Semaphore* resides in `lib/common/sem.cpp` and its semaphore variable is called `timeStatSem`.  
The functions to take/give the semaphore are `timeStatSemTake()` and `timeStatSemGive()`.

Timing Statistics were invented as a tool to detect bottlenecks in run-time and as system-calls are used to measure time, this impacts the
performance of Orion so it has been made optional, by default OFF but can be turned ON using the CLI parameter `-statTiming`.  
The statistics measurements are collected by the function `requestCompleted()` in `lib/rest/rest.cpp` and put together in `lib/common/statistics.cpp`,
function `renderTimingStatistics()`. These both functions use the semaphore, obviously. It is not used anywhere else.


## Mongo Connection Pool Semaphores
Orion implements a pool for connections to the database, and this pool needs protection by a semaphore, of course, to obtain/release connections.  
Actually, two semaphores are used.
The variables holding these two semaphores are called:

* connectionPoolSem
* connectionSem

and they are initialized in `mongoConnectionPoolInit()` in `lib/mongoBackend/mongoConnectionPool.cpp`, and taken/given in two functions of the same file:

* mongoPoolConnectionGet()
* mongoPoolConnectionRelease()

The variables holding the semaphores are static and thus cannot be accessed outside this file (`mongoConnectionPool.cpp`).

This is how the Mongo Connection Pool is protected:

* One binary semaphore that protects the pool itself - that is a connection-vector (`connectionPoolSem`).
* One counting semaphore that makes the caller wait until there is at least one free connection (`connectionSem`).

There is a limited number of connections and the first thing to do is to wait for a connection to become
available (any of the N connections in the pool) - this is done waiting on the counting semaphore that is 
initialized with "POOL SIZE" - meaning the semaphore can be taken N times if the pool size is N.   

Once a connection is free, `sem_wait(&connectionSem)` returns and we now have to take the semaphore 
that protects for pool itself (we *are* going to modify the vector of the pool - can only do it in one thread at a time ...)

After taking a connection, the semaphore `connectionPoolSem` is given, as all modifications to the connection pool have finished.
The other semaphore however, `connectionSem`, is kept and it is not given until we finish using the connection.

The function mongoPoolConnectionRelease releases the counting semaphore 'connectionSem'.
Very important to call the function 'mongoPoolConnectionRelease' after finishing using the connection !


## Metrics Manager Semaphore
The Metrics Manager needs a semaphore to protect its list of metrics, of course, and this semaphore is a private member of the class `MetricsManager`, called `sem`. Two, also private methods have been developed to access the semaphore:

* MetricsManager::semTake()
* MetricsManager::semGive()

As the methods are private, they can only be accessed by the instance of MetricsManager, which is a singleton in Orion.
The semaphore is used when ever the Metrics List is read or updated, which is dones by just four of the methods of MetricsManager:

* MetricsManager::add()
* MetricsManager::release()
* MetricsManager::reset()
* MetricsManager::toJson()


## Alarm Manager Semaphore
The Alarm Manager is pretty similar to the Metrics Manager, and its semaphore follows the same pattern.  
The class `AlarmManager` has a private field called `sem` and methods:

* AlarmManager::semTake()
* AlarmManager::semGive()

The semaphore protects the list of alarms and is accessed by the following methods:

* notificationErrorLogAlwaysSet
* badInputLogAlwaysSet
* dbErrorLogAlwaysSet
* dbError
* dbErrorReset
* notificationError
* notificationErrorReset
* badInput
* badInputReset

#### NOTE
The Alarm Manager semaphore is private in the class `AlarmManager`, **but** the methods `semTake()` and `semGive()` are **public**.
This is a mistake, the methods should be private as well. They aro only called from within methods of `AlarmManager`, so, no problem in
making the methods private.


## Log File Semaphore
Orion keeps a log file and a semaphore is needed to protect the log file from two threads writing to it at the same time.
The variable holding this semaphore is called `sem` and it resides in `lib/logMsg/logMsg.cpp`.  It is a static variable so it is not visible outside this file.
The semaphore is initialized in the function `lmSemInit()` and used in the two static functions `semTake` and `semGive`, which in their turn are used in:

* lmOut() and
* lmClear()


## Notification Queue Semaphore
When a thread pool is selected (using the CLI parameter -notificationMode), for sending of notifications, a queue is used to feed the notifications
to the workers in the thread pool. This queue is of course protected by a semaphore. 
The semaphore, of type `boost::mutex` is called `mtx` and is a private member of the class `SyncQOverflow`, found in `src/lib/common/SyncQOverflow.h`:
```
template <typename Data> class SyncQOverflow
{
private:
  std::queue<Data>           queue;
  mutable boost::mutex       mtx;
  boost::condition_variable  addedElement;
  size_t                     max_size;

public:
  SyncQOverflow(size_t sz): max_size(sz) {}
  bool     try_push(Data element);
  Data     pop();
  size_t   size() const;
};
```
The class `QueueWorkers` includes a private member of type SyncQOverflow, while the class `QueueNotifier` includes a private member of type QueueWorkers.
Finally, `contextBrokerInit()` in `src/app/contextBroker/contextBroker.cpp` creates an instance of `QueueNotifier` as a singleton, when requested (when CLI parameter `-notificationMode` equals **threadpool**).
The method `try_push()` of the template class `SyncQOverflow` takes this semaphore before pushing items to the notification queue and likewise the method `pop()` takes it before popping an element.
This semaphore protects pushing and popping to the notification queue. However, there is a need for a mechanism to wait for items in the queue as well and
for this purpose a `boost::condition_variable` called `addedElement` is used. `addedElement` is a private member of the class `SyncQOverflow`.


## Notification Queue Statistics Semaphore
The statistics of the Notification Queue is protected by the semaphore `mtxTimeInQ` in `lib/ngsiNotify/QueueStatistics.cpp`.
This semaphore is of the type `boost::mutex` and it is used whenever the timing statistics of the Notification Queue is modified/queried:

* QueueStatistics::getTimeInQ()
* QueueStatistics::addTimeInQWithSize()
* QueueStatistics::getQSize()
* QueueStatistics::reset()

