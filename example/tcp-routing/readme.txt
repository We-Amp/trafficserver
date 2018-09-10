This plugin will handle all traffic as passthrough
For this to work the listening port has to be set up as tr-in, or tr-full and ssl,
even plain http ports

compile and install by:
tsxs -I../../lib -o tcp-routing-txn.so tcp-routing-txn.cc
tsxs -i -o tcp-routing-txn.so
