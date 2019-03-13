Subsystem
######################

Subsystem in our framework provides globally-accessible features, like user API or logger etc.
Most of them can be accessible by global free functions within certain namespaces.
The implementation of the subsystem is also extensible,
by implementing component interface corresponding to each subsystem.
All subsystems can be initialized with ``init`` function and shutdown with ``shutdown`` function.


Logger
======================

Progress reporting
======================

Parallelization
======================

Exception handling
======================

Debug IO
======================


