#include <iostream>
#include <iomanip>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "snap7.h"
#include <stdint.h>

#include <bitset>

#include <regex>

#include <math.h>
#include <vector>
#include <windows.h>
#include <unistd.h>
#include <cstdlib>
#include <valarray>


#ifdef OS_WINDOWS
#define WIN32_LEAN_AND_MEAN

#include <cstdint>

#define NOT_STUPID 1
#define ENDIAN NOT_STUPID

#endif

#define PLC_OFF 4
#define PLC_ON 8
#define DEFAULT_RACK 0
#define DEFAULT_SLOT 1

using namespace std;

//--------------------------------- DOMYSLNE --------------------------------
TS7Client* Client;

byte Buffer[65536]; // 64 K bufor

char* Address;
int Start = 0, End = 5; // koniec i poczatek bloku danych
int DB = 1; // numer bloku danych

int ok = 0; // testy prawidlowe
int ko = 0; // testy nieprawidlowe

bool JobDone = false;
int JobResult = 0;

int Timeout = 1000; // czas miedzy zapytaniami
int onlyOnce = false; // odpytanie serwera tylko raz

void S7API CliCompletion(void* usrPtr, int opCode, int opResult) {
	JobResult = opResult;
	JobDone = true;
}

bool Check(int Result, const char* function) {
	int execTime = Client->ExecTime();
	cout << "| " << function << "\n";
	cout << "+-----------------------------------------------------+\n";
	if (Result == 0) {
		cout << "| Wynik          : OK\n";
		cout << "| Czas wykonania : " << execTime << " ms\n";
		cout << "+-----------------------------------------------------+\n";
		ok++;
	}
	else {
		cout << "| Blad! \n";
		if (Result < 0) {
			cout << "| Blad biblioteki (-1)\n";
		}
		else {
			cout << "| " << CliErrorText(Result) << "\n";
		}
		cout << "+-----------------------------------------------------+\n";
		ko++;
	}
	return Result == 0;
}

bool CliConnect(int Rack, int Slot) {
	int res = Client->ConnectTo(Address, Rack, Slot);
	cout << "+-----------------------------------------------------+\n";
	if (Check(res, "UNIT")) {
		cout << "| Polaczono z: " << Address << "\n";
		cout << "| (Rack = " << Rack << ", Slot = " << Slot << ", DB = " << DB << ", Start = " << Start << ", Ilosc = " << End << ")\n";
		cout << "+-----------------------------------------------------+\n";

	};
	return res == 0;
}

void CliDisconnect() {
	Client->Disconnect();
}

//---------------------------------------------------------------------------

//---------------------------------- S7POLL ---------------------------------

class s7poll {

public:
	void displayMsg(string type);
	bool validateIP(string IP);
	float littleToBigEndian(const float f);
};

void s7poll::displayMsg(string type) {
	if (type == "MENU") {
		system("CLS");

		cout << "+-----------------------------------------------------+\n" <<
			" UZYCIE: s7poll <adres IP> <opcje>\n" <<
			" PRZYKLAD:\n" <<
			" s7poll 127.0.0.1 -db 1 -r 2 -s 3 -p 0 -i 20 -time 1500 -int\n";

		cout << "+-----------------------------------------------------+\n OPCJE:\n" <<
			" adres IP   - adres IP PLC (wymagane jako 1 parametr)\n"
			" -r         - rack (domyslnie 0)\n" <<
			" -s         - slot (domyslnie 1)\n" <<
			" -db [x]    - numer bloku danych (domyslnie 1)\n" <<
			" -p [x]     - poczatkowy adres danych (domyslnie 0)\n" <<
			" -i [x]     - ilosc wyswietlanych danych (domyslnie 5)\n" <<
			" -time [ms] - czas miedzy zapytaniami w ms (domyslnie 1000)\n" <<
			" -1         - odczyta dane z serwera tylko raz\n" <<
			" -h         - wyswietla pomoc\n" <<
			"-------------------------------------------------------\n" <<
			" -hex       - wyswietla dane w systemie HEX\n" <<
			" -int       - wyswietla dane jako liczby calkowite\n" <<
			" -bin       - wyswietla dane w systemie binarnym\n" <<
			" -float     - wyswietlna dane jako liczby zmiennoprzecinkowe\n"
			"+-----------------------------------------------------+\n";
		exit(1);
	}
	else if (type == "REQUEST") {
		cout << "<----------- S7POLL (Ctrl + C - wyjdz) ----------->\n";
	}
	else if (type == "WRONG_IP") {
		cout << "Bledny adres IP\n";
	}
	else if (type == "PLC_OFF") {
		cout << "Polaczanie z PLC zostalo przerwane\n";
		exit(1);
	}
	else if (type == "PLC_ON") {
		cout << "";
	}
}

bool s7poll::validateIP(string IP) {
	regex ipv4("(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])");
	regex ipv6("((([0-9a-fa-f]){1,4})\\:){7}([0-9a-fa-f]){1,4}");

	return regex_match(IP, ipv6) || regex_match(IP, ipv4);
}

float s7poll::littleToBigEndian(const float f) {
	float retVal;
	char* floatToConvert = (char*)&f;
	char* returnFloat = (char*)&retVal;

	returnFloat[0] = floatToConvert[3];
	returnFloat[1] = floatToConvert[2];
	returnFloat[2] = floatToConvert[1];
	returnFloat[3] = floatToConvert[0];

	return retVal;
}

volatile sig_atomic_t stop;

void consoleSignal(int signum) {
	stop = 1;
}

//---------------------------------------------------------------------------

//----------------------------------- MAIN ----------------------------------

int main(int argc, char* argv[]) {

	s7poll S7;
	vector <string> args(argv, argv + argc);

	Client = new TS7Client();
	Client->SetAsCallback(CliCompletion, NULL);

	signal(SIGINT, consoleSignal);

	int arrSize = 0;
	int paramRack = DEFAULT_RACK;
	int paramSlot = DEFAULT_SLOT;
	int plcStatus = PLC_ON;
	bool typeFloat = false;

	for (size_t i = 1; i < args.size(); i++) {
		if (args[i] == "-db") {
			DB = stoi(args[i + 1]);
		}
		else if (args[i] == "-p") {
			Start = stoi(args[i + 1]);
		}
		else if (args[i] == "-i") {
			End = stoi(args[i + 1]);
		}
		else if (args[i] == "-time") {
			Timeout = stoi(args[i + 1]);
		}
		else if (args[i] == "-r") {
			paramRack = stoi(args[i + 1]);
		}
		else if (args[i] == "-s") {
			paramSlot = stoi(args[i + 1]);
		}
		else if (args[i] == "-h") {
			S7.displayMsg("MENU");
		}
		else if (args[i] == "-1") {
			onlyOnce = true;
		}
		else if (args[i] == "-float") {
			typeFloat = true;
		}
	}

	(typeFloat == true) ? arrSize = Start + (End * 4) : arrSize = Start + End;

	char tab[arrSize];

	Address = argv[1];

	if (S7.validateIP(Address)) {
		if (CliConnect(paramRack, paramSlot)) {
			while (!stop) {

				(typeFloat == true) ? Client->DBRead(DB, Start, End * 4, tab) : Client->DBRead(DB, Start, End, tab);

				plcStatus = Client->PlcStatus();

				(plcStatus == PLC_OFF) ? S7.displayMsg("PLC_OFF") : S7.displayMsg("PLC_ON");

				// Typ float
				unsigned char buf[4];
				float f;
				int x = 0, last = Start;
				// Typ float

				S7.displayMsg("REQUEST");
				for (size_t i = 1; i < args.size(); i++) {

					for (int j = 0; j < End; j++) {
						if (args[i] == "-int") {
							cout << "[" << (j + Start) << "]: " << (unsigned int)tab[j] << "\n";
						}
						else if (args[i] == "-hex") {
							cout << "[" << (j + Start) << "]: ";
							printf("%X\n", tab[j] & 0x000000FF); // FFFFFFC3 -> C3
						}
						else if (args[i] == "-float") {
							buf[0] = tab[x++];
							buf[1] = tab[x++];
							buf[2] = tab[x++];
							buf[3] = tab[x++];

							f = *(float*)&buf;
							f = S7.littleToBigEndian(f); // DD CC BB AA -> AA BB CC DD

							cout << "[" << last << " - " << last + 3 << "]: " << f << "\n";
							last = last + 4;
						}
						else if (args[i] == "-bin") {
							cout << "[" << (j + Start) << "]: " << bitset < 16 >(tab[j]) << "\n";
						}
					}
				}

				(onlyOnce == true) ? exit(1) : Sleep(Timeout);

			}
		}
	}
	else {
		S7.displayMsg("WRONG_IP");
	}

	CliDisconnect();

	delete Client;
	return 0;
}
