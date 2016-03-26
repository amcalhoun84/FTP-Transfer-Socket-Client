/* 
** Name:		Andrew M. Calhoun
** Assignment:  FTP Socket - Project 2
** Description: This is the server for an FTP services. Connected
** clients can request or send a file. The FTP control is managed
** over the connection. After closing, a connection with one
** client, it'll listen for another connection until an interrupt
** (Ctrl-C) is sent.
**
** Sources:		http://beej.us/guide/bgnet/
**				http://stackoverflow.com/questions/10837514/creating-a-c-ftp-server
**				http://man7.org/linux/man-pages/man3/getnameinfo.3.html
**				CS344 - Operating Systems I, Lectures 10-19
**
**				EXTRA CREDIT: Server has a log in check. 
*/


#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstdlib>
#include <string>
#include <cstring>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <unistd.h>
#include <fstream>

#define BL 10 // intended for multithreading but did not have time to implement

using namespace std;

void isError(string errorMsg);
int initiateSocket(int serverPort);
int openConnection(int sockfd, string type);
void sendDir(int sockfd);
void sendFile(int dataSockfd, int controlSockfd, string fileName);
bool handleRequest(int newSockfd, int sockfd);
bool serverLogin(int userSockfd);


/* 
EXTRA CREDIT FUNCTION: bool serverLogin()
Description: The login sock handler. It reads inputs when prompted by the
client to login.
Parameters: int userSockfd
Pre: User sends client commands.
Post: User is logged in and command is carried out by the program.
*/

bool serverLogin(int userSockfd)
{
	DIR *srvDir;
	FILE *srvLogin;
	srvDir = opendir(".");
	int userRead, pwRead;
	char userbuffer[256], username[256];
	char passbuffer[256], password[256];
	
	ifstream readFile;	// easier to use for password verification
	string msg;

	userRead = recv(userSockfd, userbuffer, sizeof(userbuffer), 0);
	
	// for debugging
	//cout << userRead << endl;
	//cout << userbuffer << endl;

	if(userRead > 0) userbuffer[userRead] = '\0';
	else if(userRead < 0) isError("ERROR: Could not receive user name from client.");
	else if(userRead == 0)	return false;

	readFile.open(userbuffer);

	if(!readFile)
	{
		msg = "User does not exist";
		if(send(userSockfd, msg.c_str(), strlen(msg.c_str()), 0) < 0)
			cout << "ERROR: Could not send denial message." << endl;
	}

	else {

		pwRead = recv(userSockfd, passbuffer, sizeof(passbuffer), 0);
		readFile >> password;
		
		// for debugging
		//cout << "passbuffer from client: " << passbuffer << "post buffer" << endl;
		//cout << "password from file: " << password << "post buffer " << endl;

		if(strcmp(password, passbuffer) == 0)
		{
			msg = "ACCEPTED";
			if(send(userSockfd, msg.c_str(), strlen(msg.c_str()), 0) < 0)
				cout << "ERROR: Could not send establishment message." << endl;
			return true;
		}

		else 
		{
			msg = "User name and password do not match.";
			if(send(userSockfd, msg.c_str(), strlen(msg.c_str()), 0) < 0)
			cout << "ERROR: Could not send denial message." << endl;
			return false;
		}
	}

}

void isError(string errorMsg)
{
	cout << "Something went wrong! Program Terminated. \n Reason: " << errorMsg << endl;
	exit(1);
}

int initiateSocket(int serverPort) {
	struct sockaddr_in server;
	int sockfd;
	int optVal = 1;

	//create socket file descriptor
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		cout << "Error creating socket." << endl;
	}

	//initialize socket
	bzero((char *) &server, sizeof(server));	
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(serverPort);
	
	//allow reuse of addresses
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal)) == -1) 
		cout << "ERROR: Could not set SO_REUSEADDR" << endl;

	//bind to port
	if(bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
		isError("ERROR: Could not bind to host.");
	}

	//start listening for connections
	if(listen(sockfd, 5) < 0)
		cout << "ERROR: Problem listening for connections." << endl;
	
	return sockfd;
}

/* 
FUNCTION: int openConnection()
Description: Opens a connect with a socket request. Returns what sort of
			connection it is. 
Parameters: int sockfd, string type
Pre: Server is initialized, and client sends connection request.
Post: Connection is established and data can be transmitted.
*/

int openConnection(int sockfd, string type) {
	int newSockfd;
	struct sockaddr_in clientAddress;
	socklen_t addressSize;
	char host[1024], service[20];
	addressSize = sizeof(clientAddress);

	//establish new connection
	newSockfd = accept(sockfd, (struct sockaddr *)&clientAddress, &addressSize);
	
	if(newSockfd < 0)
		cout << "ERROR: Could not accept connection." << endl;
	else {
		getnameinfo((struct sockaddr *)&clientAddress, sizeof(clientAddress), host, sizeof(host), service, sizeof(service), 0);
		cout << type << " // connection from " << host << " established." << endl;
	}
	
	return newSockfd;
}

/* 
** Function: void sendDir()
** Description: Sends the directory contents to the client.
** Parameters: int sockfd
** Pre: User has requested a file.
** Post: If the file exists, it is transmitted. Otherwise an error message
**	is returned.
*/ 

void sendDir(int sockfd) {
	DIR *curDir;
	struct dirent *current;
	curDir = opendir(".");
	string newLine = "\n";
	string start = "start";

	if(!curDir)
		cout << "ERROR: Could not call opendir()" << endl;
	
	if(send(sockfd, start.c_str(), strlen(start.c_str()), 0) < 0)
		cout << "ERROR: Could not send start to client" << endl;	

	while((current = readdir(curDir))) {
		//send all directory contents
		cout << current->d_name << endl;
		if(send(sockfd, current->d_name, strlen(current->d_name), 0) < 0)
			cout << "ERROR: Could not send " << current->d_name << " to client" << endl;	
		usleep(200);	
	}
	
	closedir(curDir);
}

/* 
** Function: void sendFile()
** Description: Sends the file to the client. Finds the file, and if it exists,
**	transmits it to the client.
** Parameters: int dataSockfd, int controlSockfd, string fileName
** Pre: User has requested a file.
** Post: If the file exists, it is transmitted. Otherwise an error message
**	is returned.
*/ 

void sendFile(int dataSockfd, int controlSockfd, string fileName) {
	DIR *curDir;
	struct dirent *current;
	bool fileExists = false;
	curDir = opendir(".");
	FILE *filefd;
	char buffer[2048];
	string msg;

	if(!curDir)
		cout << "ERROR: Could not call opendir()" << endl;

	
	//check if requested file exists in directory
	while((current = readdir(curDir))) {
		if(strcmp(current->d_name, fileName.c_str()) == 0) {
			fileExists = true;
			break;
		}
	}
	closedir(curDir);

	if(fileExists) {
		//send to client
		filefd = fopen(fileName.c_str(), "r");
		if(filefd == NULL)
			cout << "ERROR: Problem opening file" << endl;
			
		msg = "SUCCESS";
		if(send(controlSockfd, msg.c_str(), strlen(msg.c_str()), 0) < 0)
			cout << "ERROR: Could not send establishment message." << endl;

		while(fgets(buffer, sizeof(buffer), filefd) != NULL) {
			if(send(dataSockfd, buffer, strlen(buffer), 0) < 0)
				cout << "ERROR: Problem sending file data" << endl;
			usleep(10);
		}
		fclose(filefd);
	}
	else {
		cout << "File not found. Sending error message to client." << endl;
		msg = "FILE NOT FOUND";
		if(send(controlSockfd, msg.c_str(), strlen(msg.c_str()), 0) < 0)
			cout << "Error sending error message to client" << endl;		
	}
}

/* 
** Function: bool handleRequest()
** Description: Handles the user request based on whether the command is
	-l or -g. Based on the user and their arguments, it will either list
	the directory or send the file to the client, depending on the parameter.
** Parameters: int newSockfd, int sockfd
** Pre: User is logged in and has sent a command.
** Post: User either has a file sent to their client or are given a directory
**		 listing, depending on their command.
*/ 

bool handleRequest(int newSockfd, int sockfd) {
	int bytesRead, i, dataSockfd, welcomeSockfd, dataPort;
	char buffer[504];
	char *tok, *args[504];
	const char *msg;	

	//receive command
	bytesRead = recv(newSockfd, buffer, sizeof(buffer), 0);

	//handle errors receiving command
	if(bytesRead > 0) buffer[bytesRead] = '\0';
	else if(bytesRead < 0) isError("ERROR: Could not receive command from client.");
	else if(bytesRead == 0)	return false;

	
	//parse buffer for command arguments
	tok = strtok(buffer, "[',]\n ");
	for(i = 0; tok != NULL; i++) {
		args[i] = tok;
		tok = strtok(NULL, "[',]\n ");
	}

	//list directory or send requested file
	if(strcmp(args[2], "-l") == 0) {
		dataPort = atoi(args[3]);
		welcomeSockfd = initiateSocket(dataPort);				
		
		cout << "List directory requested on port " << dataPort << endl;		
		
		if(welcomeSockfd < 1)
			cout << "ERROR: Could not create welcome socket" << endl;

		//tell client that server is ready for data connection	
		msg = "SUCCESS";
		if(send(newSockfd, msg, strlen(msg), 0) < 0)
			cout << "ERROR: Cannot send data connection message to client" << endl;
		
		dataSockfd = openConnection(welcomeSockfd, "Data");
		if(dataSockfd < 1)
			cout << "ERROR: Could not create data connection" << endl;

		//send directory to client	
		cout << "Sending directory contents to port " << dataPort << endl; 
		sendDir(dataSockfd);
	}	
	else if(strcmp(args[2], "-g") == 0) {
		dataPort = atoi(args[4]);
		welcomeSockfd = initiateSocket(dataPort);				
		
		cout << "File '" << args[3] << "' requested on port " << args[4] << endl;
		
		if(welcomeSockfd < 1)
			cout << "ERROR: Could not create welcome socket" << endl;

		//tell client that server is ready for data connection	
		msg = "SUCCESS";
		if(send(newSockfd, msg, strlen(msg), 0) < 0)
			cout << "ERROR: Could not send data connection message to client" << endl;
		
		dataSockfd = openConnection(welcomeSockfd, "Data");
		if(dataSockfd < 1)
			cout << "ERROR: Could not create data connection" << endl;
		
		//send requested file to client
		cout << "Sending '" << args[3] << "' to port " << args[4] << endl;
		sendFile(dataSockfd, newSockfd, args[3]);
	}
	else {
		//send error message to client
		msg = "INVALID COMMAND";
		if(send(newSockfd, msg, strlen(msg), 0) < 0)
			cout << "ERROR: Problem sending error message to client" << endl;
	}

	//after request has been handled, close data socket
	if(close(dataSockfd) == -1 || close(welcomeSockfd) == -1)
		cout << "ERROR: Could not close data socket" << endl;

	return true;
}

int main(int argc, char* argv[]) {
	

	if(argc != 2) isError("Usage: ftserver <port>. Please see README.TXT for additional information.");

	int sockfd, controlSockfd, userSockfd;
	int serverPort = atoi(argv[1]);
	bool login = false;

	//set up socket and start listening
	sockfd = initiateSocket(serverPort);

	cout << "Server open on port: " << serverPort << endl;

	//listen on server_port until program is terminated 
	while(1) {	
		

		cout << "Awaiting command from client." << endl;

		//establish control connection 
		controlSockfd = openConnection(sockfd, "Control");

		if(login == false)
		{
			cout << "Awaiting log in from client..." << endl;
			userSockfd = openConnection(sockfd, "USER");
			login = serverLogin(userSockfd);
			
			if(login==true)
			{
				//handle client request
				if(!handleRequest(controlSockfd, sockfd)) 
					break;
			}

			userSockfd = 0;


		} 

			login = false; // reset after each log-in
	}
	
	
	return 0;
}