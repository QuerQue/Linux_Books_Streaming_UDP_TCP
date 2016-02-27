Streaming of text files. The program is using UDP and TCP protocols to communicate. 

server is running with 2 parameters 
- port number and name of the directory with text files
e.g. ./server 300 books/

the client is running with 7 parameters: 
-i ip number 
-n number of books you want to stream 
-t seconds 
-s nanoseconds 
-j (1-words, 2-letters, 3-lines)
 -p server port 
-o port number the client expects to receive answer 

e.g. ./client -i 127.0.0.1 -n 1 -t 0 -s 500000000 -j 3 -p 3000 -o 4000



compilation:
gcc -Wall client.c -o client lcrypto

gcc -Wall server.c -o server lcrypto



All rights reserved. No part of this repository may be reproduced, published, distributed, displayed, performed for public use. Only for private and non-profit use.