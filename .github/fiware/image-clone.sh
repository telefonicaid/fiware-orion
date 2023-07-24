set -e

SOURCE="telefonicaiot/fiware-orion"

DOCKER_TARGET="fiware/$(basename $(git rev-parse --show-toplevel))"
QUAY_TARGET="quay.io/fiware/$(basename $(git rev-parse --show-toplevel))"
VERSION=$(git describe --exclude 'FIWARE*' --tags $(git rev-list --tags --max-count=1))

function clone {
   echo 'cloning from '"$1 $2"' to '"$3"
   docker pull -q "$1":"$2"
   docker tag "$1":"$2" "$3":"$2"
   
   if ! [ -z "$4" ]; then
        echo 'pushing '"$1 $2"' to latest'
        docker push -q "$3":latest
   fi
}

for i in "$@" ; do
    if [[ $i == "docker" ]]; then
        clone "$SOURCE" "$VERSION" "$DOCKER_TARGET" true
    fi
    if [[ $i == "quay" ]]; then
        clone "$SOURCE" "$VERSION" "$QUAY_TARGET" true
    fi
    echo ""
done

