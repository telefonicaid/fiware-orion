set -e

SOURCE="telefonicaiot/fiware-orion"

DOCKER_TARGET="fiware/$(basename $(git rev-parse --show-toplevel))"
QUAY_TARGET="quay.io/fiware/$(basename $(git rev-parse --show-toplevel))"
VERSION=$(git describe --tags $(git rev-list --tags --max-count=1))

docker pull --platform amd64 "$SOURCE":"$VERSION"

for i in "$@" ; do
    if [[ $i == "docker" ]]; then
        echo 'cloning from '"$SOURCE $VERSION"' to '"$DOCKER_TARGET"
        docker tag "$SOURCE":"$VERSION" "$DOCKER_TARGET":"$VERSION"
        docker tag "$SOURCE":"$VERSION" "$DOCKER_TARGET":latest
        docker push -q "$DOCKER_TARGET":"$VERSION"
        docker push -q "$DOCKER_TARGET":latest
    fi
    if [[ $i == "quay" ]]; then
        echo 'cloning from '"$SOURCE" "$VERSION"' to '"$QUAY_TARGET"
        docker tag "$SOURCE":"$VERSION" "$QUAY_TARGET":"$VERSION"
        docker tag "$SOURCE":"$VERSION" "$QUAY_TARGET":latest
        docker push -q "$QUAY_TARGET":"$VERSION"
        docker push -q "$QUAY_TARGET":latest
    fi
done

