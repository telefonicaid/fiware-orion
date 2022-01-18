
# How to use Orion Context Broker with Docker

You can run Orion Context Broker very easily using docker. There are several ways to accomplish this. These are (in order of complexity):

- _"Have everything automatically done for me"_. See Section **1. The Fastest Way** (recommended).
- _"Only run a Context Broker, I'll take care of where I put my database"_. See Section **2. Run one container**.
- _"Let me see how this docker thing works from the inside"_ or _"I want to customize my Orion Docker file"_ : See Section **3. Build a docker image**.

These are alternative ways to do the same thing, you do not need to do all three of them.

You do need to have docker in your machine. See the [documentation](https://docs.docker.com/installation/) on how to do this.

----
## 1. The Fastest Way

Docker Compose allows you to link an Orion Context Broker container to a MongoDB container in a few minutes. This method requires that you install [Docker Compose](https://docs.docker.com/compose/install/).

Consider this method if you want to try Orion Context Broker and do not want to bother about databases or you don't care about losing data.

Follow these steps:

1. Create a directory on your system on which to work (for example, `~/fiware`).
2. Create a new file called `docker-compose.yml` inside your directory with the following contents (or use the provided [docker-compose.yml](docker-compose.yml)):
	
		mongo:
		  image: mongo:4.4
		  command: --nojournal
		orion:
		  image: fiware/orion-ld
		  links:
		    - mongo
		  ports:
		    - "1026:1026"
		  command: -dbhost mongo

3. Using the command-line and within the directory you created type: `sudo docker-compose up`.

> Regarding --nojournal it is not recommened for production, but it speeds up mongo container start up and avoids some race conditions problems if Orion container is faster and doesn't find the DB up and ready.

After a few seconds you should have your Context Broker running and listening on port `1026`.

Check that everything works with

	curl localhost:1026/version

What you have done with this method is download images for [Orion Context Broker](https://hub.docker.com/r/fiware/orion-ld/) and [MongoDB](https://hub.docker.com/_/mongo/) from the public repository of images called [Docker Hub](https://hub.docker.com/). Then you have created two containers based on both images.

If you want to stop the scenario you have to press Control+C on the terminal where docker-compose is running. Note that you will lose any data that was being used in Orion using this method.

----
## 2. Run one container

This method will launch a container running Orion Context Broker, but it is up to you to provide a MongoDB instance. This MongoDB instance may be running on localhost, other host on your network, another container, or anywhere you have network access to.

> TIP: If you are trying these methods or run them more than once and come across an error saying that the container already exists you can delete it with `docker rm orion1`. If you have to stop it first do `docker stop orion1`.

Keep in mind that if you use these commands you get access to the tags and specific versions of Orion-LD. For example, you may use `fiware/orion-ld:1.0.0` instead of `fiware/orion-ld` in the following commands if you need that particular version. If you do not specify a version you are pulling from `latest` by default.

### 2A. MongoDB is on localhost

To do this run this command

	sudo docker run -d --name orion1 -p 1026:1026 fiware/orion-ld

Check that everything works with

	curl localhost:1026/version

### 2B. MongoDB runs on another docker container
In case you want to run MongoDB on another container you can launch it like this

	sudo docker run --name mongodb -d mongo:4.4

And then run Orion with this command

	sudo docker run -d --name orion1 --link mongodb:mongodb -p 1026:1026 fiware/orion-ld -dbhost mongodb

Check that everything works with

	curl localhost:1026/version

This method is functionally equivalent as the one described in section 1, but doing the steps manually instead of with a docker-compose file. You equally lose your data as soon as you turn off your MongoDB container.

### 2C. MongoDB runs on a different host

If you want to connect to a different MongoDB instance do the following command **instead of** the previous one

	sudo docker run -d --name orion1 -p 1026:1026 fiware/orion-ld -dbhost <MongoDB Host>

Check that everything works with

	curl localhost:1026/version
----
## 3. Build a docker image

Building an image gives more control on what is happening within the Orion Context Broker container. Only use this method if you are familiar with building docker images or really need to change how this image is built. For most purposes you probably don't need to build an image, only deploy a container based on one already built for you (which is covered in sections 1 and 2).

Steps:

1. Download [Orion-LD's source code](https://github.com/FIWARE/context.Orion-LD) from Github (`git clone https://github.com/FIWARE/context.Orion-LD/`)
2. Switch to the repo base-directory
3. (optional) Modify the [Base-Image Dockerfile](Dockerfile-ubi-base) to your liking 
4. (optional) Build the Base-Image `docker build -f docker/Dockerfile-ubi-base -t <YOUR_BASE_IMAGE_TAG>` 
5. Modify the [Orion-LD Dockerfile](Dockerfile-ubi) to your liking
6. Build the Orion-LD image `docker build -f docker/Dockerfile-ubi -t <YOUR_CUSTOM_TAG>`

If you want to know more about images and the building process you can find it in [Docker's documentation](https://docs.docker.com/userguide/dockerimages/).

## 4. Configuration

Environment variables are the recommended way to configure Orion-LD containers. The following options are available:

| Var                               | Default                  | Description                                                                                                |
|-----------------------------------|--------------------------|------------------------------------------------------------------------------------------------------------|
| ORIONLD_PORT                      | 1026                     | Port to receive new connections.                                                                           |
| ORIONLD_FOREGROUND                | `false`                  | Don't start as daemon.                                                                                     |
| ORIONLD_LOCALIP                   | 0.0.0.0                  | IP to receive new connections.                                                                             |
| ORIONLD_PID_PATH                  | `/tmp/contextBroker.pid` | Pid file path.                                                                                             |
| ORIONLD_MONGO_HOST                | `localhost`              | Mongo-DB database host.                                                                                    |
| ORIONLD_MONGO_REPLICA_SET         |                          | Name of the replica-set, if usd.                                                                           |
| ORIONLD_MONGO_USER                |                          | Username for connecting mongo-db.                                                                          |
| ORIONLD_MONGO_PASSWORD            |                          | Password for connecting mongo-db.                                                                          |
| ORIONLD_MONGO_DB                  | `orion`                  | Name of the database to be used inside mongo-db.                                                           |
| ORIONLD_MONGO_TIMEOUT             | 10000                    | Timeout in milliseconds for connections to the replica set (ignored in the case of not using replica set). |
| ORIONLD_MONGO_POOL_SIZE           | 10                       | Size of the mongo-db connection pool.                                                                      |
| ORIONLD_MONGO_WRITE_CONCERN       | 1                        | Write concern (0:unacknowledged, 1:acknowledged) to be used with mongo-db.                                 |
| ORIONLD_MONGO_ID_INDEX            | `false`                  | Should an index on `_id.id` be automatically set.                                                          |
| ORIONLD_USEIPV4                   | `false`                  | Use IP-V4 only.                                                                                            |
| ORIONLD_USEIPV6                   | `false`                  | Use IP-V6 only.                                                                                            |
| ORIONLD_HARAKIRI                  | `false`                  | Commits harakiri on request.                                                                               |
| ORIONLD_HTTPS                     | `false`                  | Use the https 'protocol'.                                                                                  |
| ORIONLD_HTTPS_KEYFILE             |                          | Private server key file (for https).                                                                       |
| ORIONLD_HTTPS_CERTFILE            |                          | Certificate key file (for https).                                                                          |
| ORIONLD_MULTI_SERVICE             | `false`                  | Enable multi-tenancy.                                                                                      |
| ORIONLD_HTTP_TIMEOUT              | -1                       | Timeout in milliseconds for forwards and notifications.                                                    |
| ORIONLD_REQ_TIMEOUT               | 0                        | Connection timeout for REST requests (in seconds).                                                         |
| ORIONLD_MUTEX_POLICY              | `transient`              | Mutex policy (none/read/write/all).                                                                        |
| ORIONLD_CORS_ALLOWED_ORIGIN       |                          | Enable Cross-Origin Resource Sharing with allowed origin. Use '__ALL' for any.                             |
| ORIONLD_CORS_MAX_AGE              | 86400                    | Maximum time in seconds preflight requests are allowed to be cached.                                       |
| ORIONLD_CPR_FORWARD_LIMIT         | 1000                     | Maximum number of forwarded requests to Context Providers for a single client request.                     |
| ORIONLD_SUBCACHE_IVAL             | 0                        | Interval in seconds between calls to Subscription Cache refresh (0: no refresh).                           |
| ORIONLD_NOCACHE                   | `false`                  | Disable subscription cache for lookups.                                                                    |
| ORIONLD_CONN_MEMORY               | 64                       | Maximum memory size per connection (in kilobytes).                                                         |
| ORIONLD_MAX_CONN                  | 1020                     | Maximum number of simultaneous connections.                                                                |
| ORIONLD_TRQ_POOL_SIZE             | 0                        | Size of thread pool for incoming connections.                                                              |
| ORIONLD_NOTIF_MODE                | `transient`              | Notification mode (persistent                                                                              |transient|threadpool:q:n).                                                                                                         |
| ORIONLD_DROP_NOTIF                | `false`                  | Simulate notifications instead of actual sending them (only for testing).                                  |
| ORIONLD_STAT_COUNTERS             | `false`                  | Enable request/notification counters statistics.                                                           |
| ORIONLD_STAT_SEM_WAIT             | `false`                  | Enable semaphore waiting time statistics.                                                                  |
| ORIONLD_STAT_TIMING               | `false`                  | Enable request-time-measuring statistics.                                                                  |
| ORIONLD_STAT_NOTIF_QUEUE          | `false`                  | Enable thread pool notifications queue statistics.                                                         |
| ORIONLD_LOG_SUMMARY_PERIOD        | 0                        | Log summary period in seconds (defaults to 0, meaning 'off').                                              |
| ORIONLD_RELOG_ALARMS              | `false`                  | Log messages for existing alarms beyond the raising alarm log message itself                               |
| ORIONLD_CHECK_ID_V1               | `false`                  | Additional checks for id fields in the NGSIv1 API.                                                         |
| ORIONLD_DISABLE_CUSTOM_NOTIF      | `false`                  | Disable NGSIv2 custom notifications.                                                                       |
| ORIONLD_LOG_FOR_HUMANS            | `false`                  | Human readable logs.                                                                                       |
| ORIONLD_DISABLE_FILE_LOG          | `false`                  | Disable logging into a file                                                                                |
| ORIONLD_DISABLE_METRICS           | `false`                  | Turn off the 'metrics' feature.                                                                            |
| ORIONLD_INSECURE_NOTIF            | `false`                  | Allow HTTPS notifications to peers which certificate cannot be authenticated with known CA certificates.   |
| ORIONLD_NGSIV1_AUTOCAST           | `false`                  | Automatic cast for number, booleans and dates in NGSIv1 update/create attribute operations.                |
| ORIONLD_CONTEXT_DOWNLOAD_TIMEOUT  | 5000                     | Timeout in milliseconds for downloading of contexts.                                                       |
| ORIONLD_CONTEXT_DOWNLOAD_ATTEMPTS | 3                        | Number of attempts for downloading of contexts.                                                            |
| ORIONLD_TROE                      | `false`                  | Enable TRoE - temporal representation of entities.                                                         |
| ORIONLD_TROE_HOST                 | `localhost`              | Host for troe database db server.                                                                          |
| ORIONLD_TROE_PORT                 | 5432                     | Port for troe database db server.                                                                          |
| ORIONLD_TROE_USER                 | `postgres`               | Username for troe database db server.                                                                      |
| ORIONLD_TROE_PWD                  | `password`               | Password for troe database db server.                                                                      |
| ORIONLD_TROE_POOL_SIZE            | 10                       | Port to receive new connections.                                                                           |
| ORIONLD_TMP_TRACES                | `true                    | Port to receive new connections.                                                                           |
| ORIONLD_SOCKET_SERVICE            | `false`                  | Enable the socket service - accept connections via a normal TCP socket.                                    |
| ORIONLD_SOCKET_SERVICE_PORT       | 1027                     | Size of the connection pool for TRoE Postgres database connections.                                        |
| ORIONLD_FORWARDING                | `false`                  | Turn on forwarding.                                                                                        |
| ORIONLD_NO_NOTIFY_FALSE_UPDATE    | `false`                  | Turn off notifications on non-updates.                                                                     |
| ORIONLD_NOSWAP                    | `false`                  | Disable swapping.                                                                                          |

## 5. Other info

Things to keep in mind while working with docker containers and Orion Context Broker.

### 5.1 Data persistence
Everything you do with Orion Context Broker when dockerized is non-persistent. *You will lose all your data* if you turn off the MongoDB container. This will happen with either method presented in this README.

If you want to prevent this from happening take a look at [this link](https://registry.hub.docker.com/_/mongo/) in section *Where to Store Data* of the MongoDB docker documentation. In it you will find instructions and ideas on how to make your MongoDB data persistent.

### 5.2 Using `sudo`

If you do not want to have to use `sudo` follow [these instructions](http://askubuntu.com/questions/477551/how-can-i-use-docker-without-sudo).

### 5.3 Listen on different ports

In `-p 1026:1026` the first value represents the port to listen on localhost. If you want to run a second context broker
on your machine you should change this value to something else, for example `-p 1027:1026`
   
