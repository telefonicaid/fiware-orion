# Performance tuning

**Warning**: This section of the manual is yet work in progress. For the moment, just a list of topics that
should be covered in the final document:

* Mongo 3.0 suitability
* The different notificationModes and their impact on performance
* Setting connection memory limit
* Using thread pools (both notifications and MHD) and its suitability, taking into account the OS per-process limit.
  Drawbacks (queue/connection saturation)
* Changing the thread limit per process (ulimit). The "thread cannot be created error"
* Identifying bootlenecks looking at sem statistics
* Logging impact on performance
* Mutex policy impact on performance
* Write concern impact on performance
* httpTimeout impact on performance
* subCacheIval impact on performance (tradeoff between "too frequent" and "too infrequent")
* Any other?
