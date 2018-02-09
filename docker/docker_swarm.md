# How to deploy Orion Context Broker in a Docker Swarm cluster

[Docker Swarm](https://docs.docker.com/engine/swarm/) is the official docker 
cluster management solution and it is natively integrated with Docker Engine.

You can find detailed instructions and consideration on running Orion Context
Broker on Docker Swarm at this [link](https://smartsdk.github.io/smartsdk-recipes/data-management/context-broker/ha/readme/).

Here we provide only a quick overview.

## Installing a Docker Swarm test cluster

To deploy on your linux based system a Docker Swarm test cluster, you can used
[miniswarm](https://github.com/aelsabbahy/miniswarm).

The only pre-requisite are:
* [Docker Machine](https://docs.docker.com/machine/install-machine/)
* [Virtual Box](http://virtualbox.org/)

### Install miniswarm

```bash
# As root
$ curl -sSL https://raw.githubusercontent.com/aelsabbahy/miniswarm/master/miniswarm -o /usr/local/bin/miniswarm
$ chmod +rx /usr/local/bin/miniswarm
```

### Start a cluster

```bash
# 1 manager 2 workers
$ miniswarm start 3
```

### Connect to the cluster

```bash
$ eval $(docker-machine env ms-manager0)
```

## Deploy Orion Context Broker in HA

To deploy Orion in High Availability, you need first to deploy a MongoDB
ReplicaSet, and then deploy on top of it your Orion Context Broker.

### Deploy a MongoDB ReplicaSet cluster

Details on how to deploy a MongoDB ReplicaSet in Docker Swarm are available
[here](https://github.com/smartsdk/mongo-rs-controller-swarm).

1. Create an overlay network

    ```bash
    $ docker network create --opt encrypted -d overlay backend
    ```

1. Create a docker-compose-mongo.yml file (or reuse the scripts in the
   [repository](https://github.com/smartsdk/mongo-rs-controller-swarm)
   that offers additional functionalities such as healthchecks):
    ```yaml
    version: '3.3'

    services:

      mongo:
        image: mongo:3.2
        # the bind_ip option is required from MONGO Version 3.6, alternatively you can use --bind_ip_all
        # However, note that Orion doesn't work yet with MongoDB 3.6. Have a look to issue https://github.com/telefonicaid/fiware-orion/issues/3070
        entrypoint: [ "/usr/bin/mongod", "--replSet", "rs", "--journal", "--smallfiles", "--bind_ip", "0.0.0.0"]
        volumes:
          - mongodata:/data/db
        networks:
          - backend
        deploy:
          mode: global
          restart_policy:
            condition: on-failure
          update_config:
            parallelism: 1
            delay: 1m30s

      controller:
        image: martel/mongo-replica-ctrl:latest
        volumes:
          - /var/run/docker.sock:/var/run/docker.sock
        environment:
          - OVERLAY_NETWORK_NAME=backend
          - MONGO_SERVICE_NAME=mongo_mongo
          - REPLICASET_NAME=rs
          - MONGO_PORT=27017
              # - DEBUG=1 #uncomment to debug the script
        entrypoint: python /src/replica_ctrl.py
        networks:
          - backend
        depends_on:
          - "mongo"
        deploy:
          mode: replicated
          replicas: 1
          placement:
            constraints: [node.role==manager]
          restart_policy:
            condition: on-failure

    volumes:
      # External true ensures that the volume is not re-created if already present
      mongodata:
        external: true

    networks:
      backend:
        external:
          name: backend
    ```

1. Deploy the MongoDB ReplicaSet

    ```bash
    $ docker stack deploy -c docker-compose-mongo.yml mongo
    ```

1. Check that the deployment worked out correctly:

    ```bash
    $ docker service logs -f mongo_controller
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Waiting mongo service (and tasks) (mongo_mongo) to start
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Mongo service is up and running
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:No previous valid configuration, starting replicaset from scratch
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:replSetInitiate: {'ok': 1.0}
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    mongo_controller.1.jx664h87o3ft@ms-manager0    | INFO:__main__:Primary is: 10.0.0.8
    ```

### Deploy a Orion Context Broker cluster

1. Create a docker-compose-orion.yml file (or reuse the scripts in the
   [repository](https://github.com/smartsdk/smartsdk-recipes/tree/master/recipes/data-management/context-broker/ha)
   that offers additional functionalities such as healthchecks and use separate
   networks for frontend and backend):

    ```yaml
    version: '3'

    services:

      orion:
        image: fiware/orion:latest
        ports:
          - "1026:1026"
        command: -logLevel DEBUG -dbhost mongo_mongo -rplSet rs -dbTimeout 10000
        deploy:
          replicas: 2
        networks:
          - backend

    networks:
      backend:
        driver: overlay
        external: true
    ```

1. Deploy the MongoDB ReplicaSet

    ```bash
    $ docker stack deploy -c docker-compose-orion.yml orion
    ```

1. Check that the deployment worked out correctly:

    ```bash
    $ curl $(docker-machine ip ms-manager0):1026/v2/entities -s -S --header 'Accept: application/json' --header 'fiware-service: default' --header 'fiware-ServicePath: /'
    ```

1. Scale up orion:

    ```bash
    $ docker service scale orion_orion=3
    ```
