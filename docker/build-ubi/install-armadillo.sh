curl -O https://centos.pkgs.org/7/springdale-computational-x86_64/armadillo-9.700.2-1.el8.x86_64.rpm -L
if [ -f armadillo-9.700.2-1.el8.x86_64.rpm ]
then
  rpm -i armadillo-9.700.2-1.el8.x86_64.rpm
else
  echo Error downloading armadillo-9.700.2-1.el8.x86_64.rpm
fi
