#/usr/bin/env bash

echo 1024 |sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages 
