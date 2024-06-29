#include "pch.h"

using namespace std;
using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Media::Control;
using namespace Windows::System::Threading;

int flag = 0;
int tflag = 0;

IAsyncAction GetMediaInfo()
{
    auto sessions = co_await GlobalSystemMediaTransportControlsSessionManager::RequestAsync();

    auto currentSession = sessions.GetCurrentSession();

    if (currentSession)
    {
        auto info = co_await currentSession.TryGetMediaPropertiesAsync();
        wstring title = info.Title().c_str();
        wstring artist = info.Artist().c_str();
        auto timeline = currentSession.GetTimelineProperties();
        auto position = timeline.Position().count();
        auto endtime = timeline.EndTime().count();
        double percentage = 0.0;
        if (endtime > 0)
        {
            percentage = (static_cast<double>(position) / endtime) * 100;
        }


        if (percentage >= 50.0 && flag != 1)
        {
            cout << "Scrobbled" << endl;
            wcout << title << endl;
            flag = 1;
        }
        if (percentage < 45.0)
        {
            flag = 0;
            if (percentage < 1.0 && tflag != 1)
            {
                const auto p1 = std::chrono::system_clock::now();
                cout << std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count() << '\n';

                tflag = 1;
            }
            if (percentage > 1.0)
            {
                tflag = 0;
            }
        }
    }
}

void StartPeriodicMediaInfoCheck()
{
    TimeSpan period{ std::chrono::seconds(1) };
    ThreadPoolTimer::CreatePeriodicTimer([](ThreadPoolTimer const&)
        {
            GetMediaInfo();
        }, period);
}

int main()
{
   init_apartment();
   StartPeriodicMediaInfoCheck();
   getchar();
   return 0;
}