"""
Name:	ftclient.py
Author:	Josh Seifert
Date Last Modified:	08/07/2015

Description: This is a simple ftp client, written in Python, to demonstrate socket programming.
It communicates with an ftp server, ftserver.c. It has two functions - it may either request a
list of files in the server's directory, or retrieve a file from that server. It may be launched
from the command line with either 4 or 5 arguments - the server host, a command port, a command
(-l to list directory, -g to retrieve a file), a data port, and an optional file name. The
client connects to the server on the command socket and sends the command. If it gets confirmation
from the server that the command is valid, it establishes a data socket to receive the requested
information or file. It then closes sockets, and returns to the command line.

Fundamentals of socket programming in Python, including the statements needed to
make connections, from https://docs.python.org/2/howto/sockets.html. Some of this code is reused
from Project 1 from this same course. References for specific lines of code are made with inline comments.

This program has been tested and verified to work correctly on OSU's Flip servers.
"""

import socket 	#sockets API required for this assignment
import getopt 	#C-style command line arguments
import os
import sys		#allow script to exit
import time		#allows sleep, solves timing issues with creating data socket

"""
createSocket
Receives: serverHost, passed in via the command line
		port, either for the command socket or data socket, again via command line
Returns: sockfd, number indicating the socket on which connection is made
Function: Creates a socket, and connects to the host and port specified on command line.
		Used when creating both the command and data sockets.
"""
def createSocket(serverHost, port):
	sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sockfd.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	
	# The server often rejects the client's data socket, possibly the client tries to connect
	# before the server is listening for it. Pausing for 0.1 seconds seems to solve the issue.
	time.sleep(0.1)
	
	try:
		sockfd.connect((serverHost, int(port)))
	except:
		print "Error connecting"
		sys.exit()
		
	#print "DEBUG: Successfully created a socket\n"
		
	return sockfd

"""
sendCommand
Receives: commandSocket, the number identifying the command socket connection with the server
		serverHost, passed in as a command line arguments
		command, passed in a command line argument, indicating what action the client wants to perform
		dataPort, passed in a command line argument, the port on which to establish a data socket
		fileName, passed in as an optional command line argument, the name of the file to retrieve
			from the server.
Returns: None
Function: Sends the command to the server, and creates a data socket by which to receive the requested
	information. If a valid command was sent, receives a message of "VALID_COMMAND" on the command
	socket and launches the function to receive either the directory listing, or requested file.
	Otherwise, prints an error message (as "response" from server)
"""
def sendCommand(commandSocket, serverHost, command, dataPort, fileName):
	commandSocket.send(command + ' ' + str(dataPort) + ' ' + fileName)
	
	dataSocket = createSocket(serverHost, dataPort)
	
	# If client sent valid command, response should be "VALID_COMMAND", otherwise specifies error.
	response = commandSocket.recv(512)
	if response != 'VALID_COMMAND':
		print response	#error message
	else:
		if command == '-l':
			getList(dataSocket)
		elif command == '-g':
			getFile(dataSocket, fileName)
	dataSocket.close()
	#print "dataSocket closed."

"""
getList
Receives: dataSocket, the data socket on which information is passed between client and server
Returns: None
Function: Gets the current directory from the server, and prints it to screen
"""
def getList(dataSocket):
	dirList = ''
	dirBuffer = '\n'
	#In case directory is very long, can receive information in multiple packets.
	while dirBuffer != '':
		dirBuffer = dataSocket.recv(1024)
		dirList = dirList + dirBuffer
	print dirList

"""
getFile
Receives: dataSocket, the data socket on which information is passed between client and server
		fileName, the name of the file the client wants to retrieve from the server
Returns: None
Function: Gets the requested file from the server as a string of data, and writes it to a file. 
		Checks that the file name does not already exist in the current directory, and if it does,
		appends "-COPY" to the receives file as to not overwrite the previous file.
"""
def getFile(dataSocket, fileName):
	print "Receiving " + fileName + " from server, please wait..."
	# Check current directory to ensure file does not already exist.
	for file in os.listdir('.'):
		# If file already exists, append "-COPY" to received file, as to not overwrite
		if file == fileName:
			fileName = fileName + "-COPY"
			print "File already exists. File saved as " + fileName
	
	fileData = '' #file data will be saved as a string
	dataBuffer = "\n"
	while dataBuffer != '':
		dataBuffer = dataSocket.recv(1024)
		fileData = fileData + dataBuffer
	
	# Resource for creating file using WITH statement from https://docs.python.org/2/tutorial/inputoutput.html
	with open(fileName,'w') as receivedFile:
		receivedFile.write(fileData)
		
	print "File transfer completed.\n"

"""
MAIN
Receives: server host name, command port, data port, command, (optional) file name from command line
Returns: None
Function: Checks for correct number of command line arguments. Gives default name to fileName field if
		none specified. Launches functions to create a command line socket, and send the command to the 
		server. Closes sockets when terminating.
"""	
if __name__ == "__main__":
	if len(sys.argv) < 5 or len(sys.argv) > 6:	#Ensure user entered proper command line information
		print 'Usage: python ftclient.py serverHost commandPort -l|-g dataPort [fileName]\n'
		sys.exit()

	serverHost = sys.argv[1]
	commandPort = int(sys.argv[2])
	command = sys.argv[3]
	dataPort = int(sys.argv[4])
	if len(sys.argv) == 6:
		fileName = sys.argv[5];
	else:
		fileName = "undefinedFile"

#	print "DEBUG: \nserverHost = " + serverHost + "\ncommandPort  = " + str(commandPort) + "\ncommand = " + command + "\ndataPort = " + str(dataPort) + "\nfileName = " + fileName
	commandSocket = createSocket(serverHost, commandPort)

	sendCommand(commandSocket, serverHost, command, dataPort, fileName)
	
	commandSocket.close()