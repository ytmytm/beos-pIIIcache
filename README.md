pIIIcache (20060104)
====================

http://members.elysium.pl/ytm/html/beos.html
http://ytm.bossstation.dnsalias.org/html/beos.html

(c) by Maciej Witkowiak <ytm@elysium.pl>, <mwitkowiak@gmail.com>

#ABOUT
This is a small device driver that enables L2 cache on PCs with buggy BIOSes,
e.g. IBM ThinkPad 600E with CPU upgraded to Pentium III. This means significant CPU speed increase.

The code is based on pIIIcache Linux Kernel 2.6 module by Damien Challet <challet@maths.ox.ac.uk>
which was based on modified code from LinuxBIOS project.



#INSTALLATION
Just run ./install.sh script. It will copy p3cache binary to $HOME/config/add-ons/kernel/drivers/bin and
make appropriate link in ../drivers/dev/misc.

L2 cache is enabled upon loading the driver so it will work instantly after install and right after each
reboot.

The driver shouldn't make any problems, but if it does - boot into service mode and remove it.



#NOTES
You can check cache status by using cpuid tool, get it from bebits.com.

If something doesn't work, you may try to remove comment from DEBUGLOG line at the top of
driver.c file, then recompile and reinstall. Log will be written to $HOME/p3cache.log file.
However, please don't contact me with problems. I have only ported the driver to BeOS, I really don't
know how it works and I have no idea about various Intel CPU quirks.

L2 cache enable function can be triggered by writing anything to /dev/misc/p3cache, e.g.
```
  echo "foo" >/dev/misc/p3cache
```


#LICENSE
This software is derived from GPL pIIIcache Linux kernel module, so it had to be put under GPL license too.

Have fun! 

