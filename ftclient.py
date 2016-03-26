'''
Program: ftclient.py
Author: Andrew M. Calhoun
Email: calhouna@oregonstate.edu
Due Date: 3/6/2016

Resources Used: https://books.google.com/books/about/Foundations_of_Python_Network_Programmin.html?id=9HGUc8AO2xQC
                Project 1 Code
                http://stackoverflow.com/questions/120656/directory-listing-in-python
Extra Credit: Includes the ability to overwrite files or not.
              Client Username and Password Login
'''

from socket import * 
import sys
import os
import signal

''' Function: initSocket
    Description: Opens up the sockets necessary to do the data transfers.
    Parameters: (string)hostname, (int)serverPort
    Pre: User uses client to connect to server.
    Post: Data, user, and control sockets are opened and data can be transfered.
'''

def initSocket(hostname, serverPort):
    clientSocket = socket(AF_INET, SOCK_STREAM)

    if clientSocket.connect((hostname, serverPort)) == -1:
        print "ERROR: Could not connect to ", hostname, ":", serverPort

    return clientSocket

''' function: isError
    Description: Reports if an error occurs.
    Parameters: (string)msg
    Pre:  Something goes wrong in the client, usually an incorrect command or cannot bind to socket.
    Post: Error message is reported to help user figure out what went wrong.
'''

def isError(msg):
    print msg
    sys.exit(0)

''' Function: openConnection
    Description: Established a connection via the client sockets.
    Parameters: (char/string)command, (int)clientSocket
    Pre:  sockets are initiated, and connection needs to be established.
    Post: Connection established. Command send to the server.
'''

def openConnection(command, clientSocket):
    clientSocket.send(command)

''' EXTRA CREDIT FUNCTION: clientLogin
    Description: Login functionality on the client side.
    Parameters: (string)hostname, (int)serverPort
    Pre:  User sends command and starts client.
    Post: User's name and password is checked against a file in the server.
            If successful, carries out command. Otherwise quits to commandline.
'''

def clientLogin(hostname, serverPort):

    loginSocket = socket(AF_INET, SOCK_STREAM)
    if loginSocket.connect((hostname, serverPort)) == -1:
        print "Something dun fucked up."

    print "Enter your username >> "
    username = raw_input()
    loginSocket.send(username)

    print "Enter your password >> "
    password = raw_input()
    loginSocket.send(password)

    res = loginSocket.recv(1024)
    print res

    if len(res) == 0 or res == "ACCEPTED":
        print "Login successful."
        loginSocket.close()

    else:
        print "Login information incorrect. Please try again."
        loginSocket.close()
        sys.exit(1)

''' Function: serverMsg
    Description: Receives messages from the server, getting permission to continue a 
                command request or returning an error.
    Parameters: clientSocket, hostname, serverPort
    Pre:  Client awaits response from server.
    Post: Response fulfilled or a reason is given.
'''

def serverMsg(clientSocket, hostname, serverPort):
    res = clientSocket.recv(1024)
    if len(res) == 0 or res == "SUCCESS":
        return "SUCCESS"
    else:
        print "Server says: ", res
        return res

''' Function: serverMsg
    Description: Receives messages from the server, getting permission to continue a 
                command request or returning an error.
    Parameters: clientSocket, hostname, serverPort
    Pre:  Client awaits response from server.
    Post: Response fulfilled or a reason is given.
'''

''' Function: listDir
    Description: Lists the directory.
    Parameters: dataSock
    Pre:  -l command sent.
    Post: Directory listed.
'''

def listDir(dataSock):
    contents = []
    data = dataSock.recv(500)

    while data:
        data = dataSock.recv(500)
        print data
    
    dataSock.close()
    print "Directory listed successfully."
  

''' Function: recvFile
    Description: Retrieves a text file from 
    Parameters: dataSock
    Pre:  -l command sent.
    Post: Directory listed.
''' 

def recvFile(dataSock, filename):

    overwr = 'null'
    fileExists = 0

    for dirname, dirnames, filenames, in os.walk('.'):
        for currentfile in filenames:
            if currentfile == filename:
                
                print "File '%s' already exists. Do you want to overwrite? Otherwise file will be saved as duplicate." % filename
                fileExists = 1

                while(overwr != "Y" and overwr != "N" and overwr != "C" and fileExists != 0):
                    print "Do you wish to overwrite? (Y/N):"
                    overwr = raw_input()
                    if overwr == "Y":
                        print "Overwriting file."
                        filename = filename
                    elif overwr == "N":
                        print "File write cancelled."
                        dataSock.close()
                        return
                    else: 
                        print "Please input Y/N only."
       
    #if fileExists == 0:
    #    print "File does not exist, please try again."

    file = open(filename, 'w')
    data = dataSock.recv(1024)
    while data:
        with open(filename, 'w') as f:
            f.write(data)
        data = dataSock.recv(1024)
    print "File transferred successfully!!!"

    # sys.exit(0)

# main function, has some validation

def main():

    if len(sys.argv) < 5 or len(sys.argv) > 6:
        print("usage: python ftclient.py <serverPort> <command: -l | -g> <filename if -g> <dataPort> \n SEE README.TXT FOR ASSISTANCE")
        sys.exit(0)

    hostname = sys.argv[1]
    serverPort = int(sys.argv[2])
    commandTag = sys.argv[3]


    if len(sys.argv) == 5:
        dataPort = int(sys.argv[4])

    elif len(sys.argv) == 6:
        filename = sys.argv[4]
        dataPort = int(sys.argv[5])

    if(serverPort < 1024 or serverPort > 65535):
        isError("Error: Ports must be in range [1024..65535]")
        
    if(dataPort < 1024 or dataPort > 65535):
        isError("Error: Ports must be in range [1024..65535]")
        
    if dataPort == serverPort:
        isError("ERROR: Server and data port cannot match.")
        
    if commandTag != '-l' and commandTag != '-g':
        isError("ERROR: Invalid command. Use -l | - g or see README.txt for information.")
        
    elif commandTag == '-l' and len(sys.argv) != 5:
        isError("ERROR: Invalid number of arguments for command -l. Please see README.txt for information.")
        
    elif commandTag == '-g' and len(sys.argv) != 6:
        isError("ERROR: Invalid number of arguments for command -g. Please see README.txt for information.")

    clientSocket = initSocket(hostname, serverPort)

    openConnection(str(sys.argv[1:]), clientSocket)
    clientLogin(hostname, serverPort)


    ctrlSockRes = serverMsg(clientSocket, hostname, serverPort)

    if ctrlSockRes == "SUCCESS":
        if commandTag == "-l":
            dataPort = int(sys.argv[4])
            dataSock = initSocket(hostname, dataPort)
            print("Receiving directory information from host...\n")
            listDir(dataSock)

        elif commandTag == "-g":
            dataPort = int(sys.argv[5])
            dataSock = initSocket(hostname, dataPort)
            ctrlSockRes = serverMsg(clientSocket, hostname, serverPort)

            if ctrlSockRes == "SUCCESS":
                print "Receiving from host..."
                recvFile(dataSock, filename)

        else:
            print "ERROR: Command could not be completed. Terminating program."
            sys.exit()


        if clientSocket.close() == -1:
            print "ERROR: Could not close connection."
            sys.exit(0)


if __name__ == "__main__":
    main()