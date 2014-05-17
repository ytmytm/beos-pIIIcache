#!/bin/sh

rm -f /boot/home/config/add-ons/kernel/drivers/bin/pIIIcache
rm -f /boot/home/config/add-ons/kernel/drivers/dev/misc/pIIIcache

cp pIIIcache /boot/home/config/add-ons/kernel/drivers/bin/
ln -s /boot/home/config/add-ons/kernel/drivers/bin/pIIIcache /boot/home/config/add-ons/kernel/drivers/dev/misc/pIIIcache
