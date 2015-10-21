base=$(basename $1 .test)

out=$(find . -name ${base}.out)
exp=$(find . -name ${base}.regexpect)

$ORION_DIFF $out $exp
