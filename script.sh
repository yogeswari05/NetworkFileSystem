#!/usr/bin/bash

gcc NS/namingServer.c NS/helper.c NS/ns_functions.c NS/cache2.c -o namingServer
gcc SS/storageServer.c SS/helper.c SS/ss_functions.c -o storageServer
gcc -o client CS/client.c CS/helper.c CS/cs_functions.c
