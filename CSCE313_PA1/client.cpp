/*
	Author of the starter code
	Timothy Tsai
	Department of Computer Science & Engineering
	Texas A&M University
	Date: 9/28/2024

	Please include your Name, UIN, and the date below
	Name: Timothy Tsai
	UIN: 332003717
	Date: 9/28/2024
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <fstream>

using namespace std;

int main(int argc, char *argv[])
{
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	int m = MAX_MESSAGE;
	string filename = "";

	// Add other arguments heres
	while ((opt = getopt(argc, argv, "p:t:e:f:m:")) != -1)
	{
		switch (opt)
		{
		case 'p':
			p = atoi(optarg);
			break;
		case 't':
			t = atof(optarg);
			break;
		case 'e':
			e = atoi(optarg);
			break;
		case 'f':
			filename = optarg;
			break;
		case 'm':
			m = atoi(optarg);
			break;
		}
	}

	// Task 1:
	// Run the server process as a child of the client process
	// server needs './server' , '-m' '<val for -m arg>', 'Null'

	pid_t pid = fork();
	if (pid < 0)
	{
		cerr << "Fork Failed" << endl;
		exit(1);
	}

	char *args[] = {
		(char *)"./server", (char *)"-m", (char *)to_string(m).c_str(), nullptr};

	if (pid == 0)
	{ // child process

		execvp(args[0], args);

		cerr << "Execvp failed" << endl;
		exit(1);
	}
	FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

	// Task 4:
	// Request a new channel

	// Task 2:
	// single datapoint, only run p,t,e != -1
	// Request data points
	char buf[MAX_MESSAGE]; // 256
	if (p != -1 && t != -1 && e != -1)
	{
		datamsg x(p, t, e);

		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg));
		double reply;
		chan.cread(&reply, sizeof(double));
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}

	// Task 3:
	// Request files
	// if p != -1, request 1000 datapoints
	// loop over 1st 1000 lines
	// send request for ecg 1
	// send request for ecg 2
	// write line to recieved/x1.csv
	if (p != -1)
	{

		filemsg fm(0, 0);
		string fname = "./received/x1.csv";
		int len = sizeof(filemsg) + (fname.size() + 1);
		char *buf2 = new char[len];

		ofstream file(fname);
		if (!file.is_open())
		{
			cout << "File failed to open" << endl;
		}
		for (double i = 0; i < 4.0; i += .004)
		{
			// copy ecg 1
			datamsg x(p, i, 1);
			memcpy(buf2, &x, sizeof(datamsg));
			chan.cwrite(buf2, sizeof(datamsg));
			double reply1;
			chan.cread(&reply1, sizeof(double));

			// copy ecg 2
			datamsg y(p, i, 2);
			memcpy(buf2, &y, sizeof(datamsg));
			chan.cwrite(buf2, sizeof(datamsg));
			double reply2;
			chan.cread(&reply2, sizeof(double));

			//print out for testing
			// cout << i << "," << reply1 << "," << reply2 << endl;
			//copy into file
			file << i << "," << reply1 << "," << reply2 << endl;
		}
		cout<< "Closing file" << endl;
		file.close();
		cout << "File closed" << endl;

		delete[] buf2;
		// __int64_t file_length;
		// chan.cread(&file_length, sizeof(__int64_t));
		// cout << "The length of " << fname << " is " << file_length << endl;
	}

	// Task 5:
	//  Closing all the channels
	MESSAGE_TYPE quit = QUIT_MSG;
	chan.cwrite(&quit, sizeof(MESSAGE_TYPE));
}
