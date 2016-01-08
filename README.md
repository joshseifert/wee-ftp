# wee-ftp
FTP server/client, written for a networking CS class, demonstrating socket programming.

The server is written in C. It accepts one argument on the command line, the port on which it listens for connections. It may either list the contents of the requested directory, or send a file to the user. Per instructor specifications, this FTP program only transfers files from server->client.
It runs in the background until explicitly killed. When it receives an incoming connection, it creates a new socket, so that it may listen for new connections while concurrently transmitting data.

The client is written in Python. On the command line, the user enters the server host's name, the command port, the data port, and the requested command, either -l (list directory) or -g (get file from directory), the latter also requiring the requested file name.
