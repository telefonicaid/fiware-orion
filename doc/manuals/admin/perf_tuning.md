# Performance tunning

**Warning**: This section of the manual is yet work on progress. By the moment, just a list of topic that
should be covered in the final document:

* Mongo 3.0 suitability
* The different notificationModes and its impact in performance
* Setting connection memory limit
* Using thread pools (both notifications and MHD) and its suitability taking into accoun the OS per-process limit.
  Drawbacks (queue/connection saturation)
* Changing the thread limit per process (ulimit). The "thread cannot be created error"
* Identifying bootleneck looking to sem statistics
* Logging impact on performance
* Mutext policy impact on performance
* Write concern impact on performance
* httpTimeout impact on performance
* subCacheIval impact on performance (trade off between "too frequent" and "too infrequent")
* Any other?
