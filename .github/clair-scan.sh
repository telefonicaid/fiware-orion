#!/bin/bash

failLevel=$1
image=$2

docker run -d --network host -p 5432:5432  --env POSTGRES_HOST_AUTH_METHOD=trust --env POSTGRES_DB=clair --env POSTGRES_USER=clair --env POSTGRES_PASSWORD=clair  docker.io/library/postgres:12
docker run -d --network host -v $(pwd)/.github/config.yaml:/config/config.yaml quay.io/projectquay/clair:4.3.5

timetorun=30
stoptime=$((timetorun + $(date +%s)))

function checkClairStatus() {
    DONE=$(docker logs -n 1000 clair 2>&1 | jq 'select(.message=="starting background updates")' -)

    if [ -z "$DONE" ]; then
	return 0;
    else
        return 1;
    fi
}

echo "Checking clair status..."
while [ true ]
do
    if [[ $(date +%s) > $stoptime ]]; then
        echo "[Error]: Timeout waiting for Clair"
        docker logs -n 1000 clair
        exit 1;
    fi

    checkClairStatus
    STATUS=$?

    curl --max-time 5 http://localhost:6060 &> /dev/null 2>&1

    if [[ "$?" -eq 0 && $STATUS -eq 1 ]]; then
        echo "Clair is ready!"
        break;
    fi

    sleep 5;
    echo "Waiting for Clair to be ready..."
done

wget https://github.com/quay/clair/releases/download/v4.3.5/clairctl-linux-amd64
chmod +x clairctl-linux-amd64
./clairctl-linux-amd64 report --out json $image > clair.report

low=$(cat clair.report | jq  ' .vulnerabilities[].normalized_severity | select(contains("Low"))' | wc -l)
medium=$(cat clair.report | jq  ' .vulnerabilities[].normalized_severity | select(contains("Medium"))' | wc -l)
high=$(cat clair.report | jq  ' .vulnerabilities[].normalized_severity | select(contains("High"))' | wc -l)
critical=$(cat clair.report | jq  ' .vulnerabilities[].normalized_severity | select(contains("Critical"))' | wc -l)

echo "CVE report: "
echo "Critical : $critical"
echo "High : $high"
echo "Medium : $medium"
echo "Low : $low"

if [ "$failLevel" = "low" ]; then
  if [ "$low" -gt "0" ]; then
    exit 1
  fi
elif [ "$failLevel" = "medium" ]; then
  if [ "$medium" -gt "0" ]; then
    exit 1
  fi
elif [ "$failLevel" = "high" ]; then
  if [ "$high" -gt "0" ]; then
    exit 1
  fi
elif [ "$failLevel" = "critical" ]; then
  if [ "$critical" -gt "0" ]; then
    exit 1
  fi
fi

exit 0