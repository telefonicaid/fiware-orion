
# -----------------------------------------------------------------------------
#
# style_check
#
function style_check
{
  style_check.sh -d src/lib/logMsg
  if [ "$?" != 0 ]
  then
    echo Lint Errors:
    cat LINT_ERRORS
    echo "================================================================="
    cat LINT | grep -v "Done processing"
    echo 
    exit 1
  fi
}

style_check src/lib/logMsg
