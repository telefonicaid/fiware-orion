This directory includes some scripts to help to control context subscriptions collection (csubs). All of them share
the same CLI arguments:

* `-db <database>` (mandatory): to specify the DB to process
* `--dry-run` (optional): it avoids actual modification in the DB, just reporting
* `-v` (optional): enables verbose mode

The scripts are:

* csub_dups_checker.py: removes csubs duplicates. Have a look at the script implementation in order to know which
  csub fields are considered part of the 'signature' of the subscription that is used to detect duplicates.
  * csub_dups_test.sh is a helper script that can be used to test this program.
* csub_localhost_reference_checker.py: it checks subscriptions using "localhost" in the callback, removing them
  if they are not allowed (based on the result of the valid_localhost_url() function; you would need to modify it
  to implement your own checks).
* csub_reference_limit_per_ip_checker.py: removes ONTIMEINTERVAL subscriptions exceeding the allowed threshold
  (set in the MAX_PER_IP variable in the code)

