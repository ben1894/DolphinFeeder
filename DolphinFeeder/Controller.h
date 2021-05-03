#pragma once
#define NOMINMAX
#include <Windows.h>
#include <iostream>
#include <vector>
#include "Serial.h"
#include "public.h"
#include "vjoyinterface.h"
#include "easyinput.h"
#include <string>

const byte key = 255; //Signals the start and end of data being sent
const int chunkSize = 5; //Number of bytes being sent

/*
	The controller class manages the creation and editing of controllers 
	to feed to the driver
*/
class Controller
{
public:
	std::vector<byte> receivedData; //Received data buffer
	std::string name = ""; //Name of the controller
	int iInterface; //VJoy interface number for controller
	int nButtons; //Number of buttons being controlled
	int comNumber; //Com port number
	CSerial serialPort; //Serial port object

	Controller()
	{
	}

	//Relinquishes the device on the controller's interface and closes the 
	//sertial port associated with the controller
	~Controller()
	{
		RelinquishVJD(iInterface);
		serialPort.Close();
	}

	//Goes through all the steps needed to set up a controller with the dll
	int configureInterface(int newInterface)
	{
		iInterface = newInterface;
		VjdStat status = GetVJDStatus(iInterface);

		//Makes sure the status of the device on the given interface is free
		switch (status)
		{
		case VJD_STAT_OWN:
			printf("vJoy Device %d is already owned by this feeder\n", iInterface);
			break;
		case VJD_STAT_FREE:
			printf("vJoy Device %d is free\n", iInterface);
			break;
		case VJD_STAT_BUSY:
			printf("vJoy Device %d is already owned by another feeder\nCannot continue\n", iInterface);
			return -3;
		case VJD_STAT_MISS:
			printf("vJoy Device %d is not installed or disabled\nCannot continue\n", iInterface);
			return -4;
		default:
			printf("vJoy Device %d general error\nCannot continue\n", iInterface);
		}

		// Acquire the target
		if ((status == VJD_STAT_OWN) || ((status == VJD_STAT_FREE) && (!
			AcquireVJD(iInterface))))
		{
			printf("Failed to acquire vJoy device number %d.\n", iInterface);
			return -1;
		}
		else
		{
			printf("Acquired: vJoy device number %d.\n", iInterface);
		}

		nButtons = GetVJDButtonNumber(iInterface);

		return 1;
	}

	//Opens a serial port with a given comPort
	int configureSerialPort(int comPort)
	{
		if (serialPort.Open(comPort, 57600))
		{
			comNumber = comPort;
			return 1;
		}
		else
		{
			return -1;
		}
	}

	//Updates the values of the device on the interface
	void updateController()
	{
		int size = serialPort.ReadDataWaiting(); //Number of bytes waiting

		//Makes sure there are bytes waiting to be read from the com port
		if (size > 0)
		{
			//store data to array
			byte* lpBuffer = new byte[size];
			int nBytesRead = serialPort.ReadData(lpBuffer, size);

			//add data to full vector
			receivedData.insert(receivedData.end(), &lpBuffer[0], &lpBuffer[size]);

			delete[]lpBuffer;
		}

		//Attempts to find the key in the receivedData vector
		int position = -1;
		bool found = false;
		for (int i = (receivedData.size() - chunkSize) - 1; i >= 0; i--)
		{
			//for (int q = 0; q < receivedData.size(); q++)
			//{
			//	std::cout << (int)receivedData[q] << " ";
			//}
			if (receivedData[i] == key)
			{
				found = true;
				position = i;
				break;
			}
		}

		if (found)
		{
			//For debugging
			/*for(unsigned int i = 0; i < receivedData.size(); i++)
			{
				std::cout << (int)receivedData[i] << ",";
			}
			//std::cout << "\n";

			//Sending order
			/*
				EEBlue.write(255);
				EEBlue.write(buttonStates);
				EEBlue.write(x10);
				EEBlue.write(x100);
				EEBlue.write(y10);
				EEBlue.write(y100);
			*/
			//iReport.lButtons |= (long)receivedData[position + 1]; //bit operations will be needed for more than 8 buttons
			JOYSTICK_POSITION_V2 iReport; //Can store all the data of a device
			iReport.bDevice = (BYTE)iInterface;

			iReport.lButtons = receivedData[position + 1];
			iReport.wAxisX = receivedData[position + 2] + (receivedData[position + 3] * 100);
			iReport.wAxisY = receivedData[position + 4] + (receivedData[position + 5] * 100);

			//erase values that were just read and those before it too
			receivedData.erase(receivedData.begin(), receivedData.begin() + position + chunkSize);

			//update the dll
			UpdateVJD(iInterface, &iReport);
		}
	}
};

