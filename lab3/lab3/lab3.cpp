
#include <iostream>
#include <Windows.h>
#include <string>
#include <tchar.h>
#include <optional>
#include <fstream>
#include <functional>
#include <chrono>
#include <timeapi.h>
#include <vector>

#pragma comment(lib, "winmm.lib")

using namespace std;

//DWORD START_TIME = 0;
using namespace std::chrono;
high_resolution_clock::time_point START_TIME;

struct ParamsForThread
{
    int number;
    int cycleCount;
};

void FibonacciNumbersSearch(int N)
{
    int a = 0, b = 1;
    for (int i = 0; i < N; i++)
    {
        cout << a << " ";
        int next = a + b;
        a = b;
        b = next;
    }
}

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
    ParamsForThread* param = reinterpret_cast<ParamsForThread*>(lpParam);
    ofstream output;
    output.open("thread" + to_string(param->number) + ".txt");

    for (size_t i = 0; i < param->cycleCount; i++)
    {
        FibonacciNumbersSearch(39);
        //float ellapsedTime = (float(timeGetTime()) - float(START_TIME));
        float elapsedSeconds = duration_cast<duration<float>>(high_resolution_clock::now() - START_TIME).count();
        output << elapsedSeconds << endl;
    }

    output.close();

    delete param;
    ExitThread(0);
}

int _tmain(int argc, _TCHAR* argv[])
{
    unsigned cycleCount;

    cin >> cycleCount;

    HANDLE* handles = new HANDLE[2];

    START_TIME = high_resolution_clock::now();

    for (int i = 0; i < 2; i++)
    {
        ParamsForThread* params = new ParamsForThread;
        params->number = i + 1;
        params->cycleCount = cycleCount;
        handles[i] = CreateThread(NULL, 0, &ThreadProc, params, CREATE_SUSPENDED, NULL);
    }

    SetThreadPriority(handles[0], 1);
    SetThreadPriority(handles[1], 2);

    for (int i = 0; i < 2; i++)
    {
        ResumeThread(handles[i]);
    }

    WaitForMultipleObjects(2, handles, true, INFINITE);

    delete[] handles;
    return 0;
}