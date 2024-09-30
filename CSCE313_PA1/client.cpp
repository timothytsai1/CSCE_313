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
#include <chrono>

using namespace std;
using namespace std::chrono;

int main(int argc, char *argv[])
{
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	int m = MAX_MESSAGE;
	bool c = false;
	string filename = "";

	vector<FIFORequestChannel *> channels;

	// Add other arguments heres
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1)
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
		case 'c':
			c = true;
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
	FIFORequestChannel *control_chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(control_chan);
	char buf[MAX_MESSAGE]; // 256
	// Task 4:
	// Request a new channel
	if (c)
	{
		MESSAGE_TYPE newChannel = NEWCHANNEL_MSG;
		control_chan->cwrite(&newChannel, sizeof(MESSAGE_TYPE));

		char newChannelName[256];
		control_chan->cread(newChannelName, sizeof(newChannelName));

		FIFORequestChannel *testChannel = new FIFORequestChannel(newChannelName, FIFORequestChannel::CLIENT_SIDE);

		channels.push_back(testChannel);
	}
	FIFORequestChannel *chan = channels.back();
	// Task 2:
	// single datapoint, only run p,t,e != -1
	// Request data points

	if (p != -1 && t != -1 && e != -1)
	{
		datamsg x(p, t, e);

		memcpy(buf, &x, sizeof(datamsg));
		chan->cwrite(buf, sizeof(datamsg));
		double reply;
		chan->cread(&reply, sizeof(double));
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}

	// if p != -1, request 1000 datapoints
	// loop over 1st 1000 lines
	// send request for ecg 1
	// send request for ecg 2
	// write line to recieved/x1.csv

	if (p != -1)
	{

		ofstream file("./received/x1.csv");
		if (!file.is_open())
		{
			cout << "File failed to open" << endl;
		}
		for (double i = 0; i < 4.0; i += .004)
		{
			// copy ecg 1
			datamsg x(p, i, 1);
			memcpy(buf, &x, sizeof(datamsg));
			chan->cwrite(buf, sizeof(datamsg));
			double reply1;
			chan->cread(&reply1, sizeof(double));

			// copy ecg 2
			datamsg y(p, i, 2);
			memcpy(buf, &y, sizeof(datamsg));
			chan->cwrite(buf, sizeof(datamsg));
			double reply2;
			chan->cread(&reply2, sizeof(double));

			// print out for testing
			//  cout << i << "," << reply1 << "," << reply2 << endl;
			// copy into file
			file << i << "," << reply1 << "," << reply2 << endl;
		}
		// cout<< "Closing file" << endl;
		file.close();
		// cout << "File closed" << endl;
	}

	// Task 3:
	// Request files

	if (!filename.empty())
	{
		filemsg fm(0, 0);
		string fname = filename;

		int len = sizeof(filemsg) + (fname.size() + 1);
		char *buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan->cwrite(buf2, len);

		delete[] buf2;
		__int64_t file_length;
		chan->cread(&file_length, sizeof(__int64_t));

		auto start = high_resolution_clock::now();
		int buffer = MAX_MESSAGE - sizeof(filemsg);
		__int64_t chunk = buffer;
		int numChunks = (file_length + chunk - 1) / chunk;
		cout << "The length of " << fname << " is " << file_length << endl;

		ofstream file("received/" + fname, ios::app | ios::binary);
		if (!file.is_open())
		{
			cout << "File failed to open" << endl;
		}

		for (int i = 0; i < numChunks; i++)
		{
			__int64_t offset = i * chunk;
			int bytes = file_length - offset;
			int chunkLength = (bytes < chunk) ? bytes : chunk;

			filemsg message(offset, chunkLength);

			int messageSize = sizeof(filemsg);
			char *messageBuffer = new char[messageSize + fname.size() + 1];
			memcpy(messageBuffer, &message, messageSize);

			strcpy(messageBuffer + messageSize, fname.c_str());
			chan->cwrite(messageBuffer, messageSize + fname.size() + 1);

			char *data = new char[chunkLength];
			chan->cread(data, chunkLength);
			file.write(data, chunkLength);
			delete[] messageBuffer;
			delete[] data;
		}

		file.close();

		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop - start);

		cout << "Time taken to transfer " << fname << ": " << duration.count() << " ms" << endl;
		string received = "received/" + fname;
		string original = "BIMDC/" + fname;
		string diffCommand = "diff " + received + " " + original;
		int diffCheck = system(diffCommand.c_str());

		if (diffCheck == 0)
		{
			cout << "File matches original." << endl;
		}
		else
		{
			cout << "File does not match original." << endl;
		}
	}

	// Task 5:
	//  Closing all the channels
	MESSAGE_TYPE quit = QUIT_MSG;
	control_chan->cwrite(&quit, sizeof(MESSAGE_TYPE));
	if (c)
	{
		chan->cwrite(&quit, sizeof(MESSAGE_TYPE));
	}

	for (auto chan : channels)
	{
		delete chan;
	}
}
