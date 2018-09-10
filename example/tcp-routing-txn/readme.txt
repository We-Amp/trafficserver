Example plugin for passthrough routing on normal HTTP ports in ATS, demonstrating TSHttpTxnTunnel

Configure ports with tr-in or tr-full, no ssl

compilation and installation, prefix tsxs with ATS bin directory
tsxs -I../../lib -o tcp-routing-txn.so tcp-routing-txn.cc
tsxs -i -o tcp-routing-txn.so
