# XCept

This is XCept V 2.1 BETA from 1993. Can we get it to compile on today's systems?

The following does not work yet. Any help appreciated.

```
sudo apt install -y make gcc
sudo mkdir -p /usr/lib/cept 
cd ./ceptd
make -f makefile.linux
sudo make -f makefile.linux install
sudo cp ../etc/users.cept /usr/lib/cept/users.cept 
sudo cp ../etc/init.cept file /usr/lib/cept/init.cept 
cd ../xcept
make -f makefile.linux
sudo make -f makefile.linux
cd ../lib
sudo make -f makefile.linux
```
