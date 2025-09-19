#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <numeric>
#include <functional>
#include <chrono>
#include <iomanip>
#include <locale>

#include <tchar.h>
#include <Windows.h>

#include "BitmapPlusPlus.h"

using namespace std;
using namespace bmp;

using Threads = unique_ptr<HANDLE[]>;
using GetEllapsedTimeFn = function<chrono::duration<double>()>;


struct ProcessBitmapInfo
{
    Bitmap* image;
    unsigned lineNumber;
    size_t lineHeight;
    GetEllapsedTimeFn& getTime;
};

Pixel Average(vector<optional<Pixel>> const& v)
{
    auto const count = v.size();
    int sumR = 0;
    int sumG = 0;
    int sumB = 0;
    int pixelsCount = 0;

    for (auto const& pixel : v)
    {
        if (!pixel)
        {
            continue;
        }
        pixelsCount++;
        sumR += pixel->r;
        sumG += pixel->g;
        sumB += pixel->b;
    }

    return Pixel(sumR / pixelsCount, sumG / pixelsCount, sumB / pixelsCount);
}

Pixel GetAverageColor(Bitmap const& img, int x, int y)
{
    vector<optional<Pixel>> pixels = {
        img.get(x, y),
        img.get(x, y - 1),
        img.get(x, y + 1),
        img.get(x - 1, y),
        img.get(x - 1, y - 1),
        img.get(x - 1, y + 1),
        img.get(x + 1, y),
        img.get(x + 1, y - 1),
        img.get(x + 1, y + 1),
    };

    return Average(pixels);
}

DWORD WINAPI BlurBitmap(CONST LPVOID lpParam)
{
    auto data = reinterpret_cast<ProcessBitmapInfo*>(lpParam);

    ofstream output;
    output.open("thread" + to_string(data->lineNumber) + ".txt");

    unsigned startY = data->lineNumber * data->lineHeight;

    auto image = data->image;
    unsigned imageWidth = image->width();

    for (unsigned i = 0; i < 15; i++)
    {
        for (unsigned y = startY; y < startY + data->lineHeight; ++y)
        {
            for (unsigned x = 0; x < imageWidth; ++x)
            {
                image->set(x, y, GetAverageColor(*image, x, y));
            }
        }
        output << data->getTime().count() << endl;
    }

    delete data;
    ExitThread(0);
}


Threads CreateThreads(size_t count, function<ProcessBitmapInfo* (int)> dataCreatorFn)
{
    auto threads = make_unique<HANDLE[]>(count);

    for (unsigned i = 0; i < count; i++)
    {
        threads[i] = CreateThread(
            NULL, 0, &BlurBitmap, dataCreatorFn(i), CREATE_SUSPENDED, NULL
        );
    }

    return threads;
}

void SetCoresLimit(size_t limit)
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    size_t maxCoresCount = sysinfo.dwNumberOfProcessors;

    if (limit > maxCoresCount)
    {
        cout << "Max cores count is " << maxCoresCount << endl;
        limit = maxCoresCount;
    }

    auto procHandle = GetCurrentProcess();
    DWORD_PTR mask = static_cast<DWORD_PTR>((pow(2, maxCoresCount) - 1) / pow(2, maxCoresCount - limit));

    SetProcessAffinityMask(procHandle, mask);
}

GetEllapsedTimeFn StartTimer()
{
    chrono::steady_clock::time_point start = chrono::steady_clock::now();

    return [start]()
    {
        return chrono::steady_clock::now() - start;
    };
}

int main(int argc, char* argv[])
{

    string inputFileName = argv[1];
    string outputFileName = argv[2];
    int threadsCount = atoi(argv[3]);
    int coresCount = atoi(argv[4]);
    int getPriorityVariant = atoi(argv[5]);

    try {
        SetCoresLimit(coresCount);
        auto getEllapsedTime = StartTimer();
        Bitmap* image = new Bitmap(inputFileName);
        unsigned lineHeight = image->height() / threadsCount;

        auto threads = CreateThreads(threadsCount, [lineHeight, image, &getEllapsedTime](unsigned threadNumber) {
            return new ProcessBitmapInfo{
                image,
                threadNumber,
                lineHeight,
                getEllapsedTime
            };
            });

        if (getPriorityVariant == 0)
        {
            SetThreadPriority(threads[0], THREAD_PRIORITY_NORMAL);
            SetThreadPriority(threads[1], THREAD_PRIORITY_NORMAL);
            SetThreadPriority(threads[2], THREAD_PRIORITY_NORMAL);

        }
        else if (getPriorityVariant == 1)
        {
            SetThreadPriority(threads[0], THREAD_PRIORITY_ABOVE_NORMAL);
            SetThreadPriority(threads[1], THREAD_PRIORITY_NORMAL);
            SetThreadPriority(threads[2], THREAD_PRIORITY_NORMAL);
        }
        else if (getPriorityVariant == 2)
        {
            SetThreadPriority(threads[0], THREAD_PRIORITY_ABOVE_NORMAL);
            SetThreadPriority(threads[1], THREAD_PRIORITY_NORMAL);
            SetThreadPriority(threads[2], THREAD_PRIORITY_BELOW_NORMAL);
        }

        for (int i = 0; i < threadsCount; i++)
        {
            ResumeThread(threads[i]);
        }

        WaitForMultipleObjects(threadsCount, threads.get(), true, INFINITE);

        image->save(outputFileName);

        cout << getEllapsedTime().count() << endl;
        //cout << "Processing time: " << duration.count() << " seconds" << std::endl;
    }
    catch (const bmp::Exception& e)
    {
        cout << "[BMP ERROR]: " << e.what() << endl;
        return -1;
    }

    return 0;
}


