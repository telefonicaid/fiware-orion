# Run Orion-LD in a Docker Container

Docker images for Ubuntu and CentOS are produced for each Pull Request merged into the `develop` branch.
To create and run the Orion-LD Docker image, it is necessary to have [Docker](https://www.docker.com/).
and [Docker Compose](https://docs.docker.com/compose) installed. A sample `docker-compose.yml` can be found below:

```yaml
version: "3.5"
services:
  # Orion is the context broker
  orion:
    image: fiware/orion-ld
    hostname: orion
    container_name: fiware-orion
    depends_on:
      - mongo-db
    expose:
      - "1026"
    ports:
      - "1026:1026" 
    command: -dbhost mongo-db -logLevel DEBUG
    healthcheck:
      test: curl --fail -s http://orion:1026/version || exit 1

  # Databases
  mongo-db:
    image: mongo:3.6
    hostname: mongo-db
    container_name: db-mongo
    expose:
      - "27017"
    ports:
      - "27017:27017" 
    command: --nojournal
    volumes:
      - mongo-db:/data
      
volumes:
  mongo-db: ~
```

We also provide Centos and Debian based Dockerfiles to facilitate the building of your own images. 
They can be found in the following [location](https://github.com/FIWARE/context.Orion-LD/tree/develop/docker) 
along  with the documentation describing how to use it.

A public image is also available on [Docker Hub](https://hub.docker.com/r/fiware/orion-ld/) 
