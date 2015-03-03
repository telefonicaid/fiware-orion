for db in $(dbList.sh | awk '{ print $1 }' | grep ftest)
do
  dbReset.sh $db
done
