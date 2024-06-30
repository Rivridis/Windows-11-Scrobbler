#include "pch.h"]

using namespace std;
using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Media::Control;
using namespace Windows::System::Threading;



int flag = 0;
int tflag = 0;
string timestamp = "";
string api_key();

static void scrobble(wstring artistc, wstring titlec,string timestampc)
{
    CURL* curl;
    CURLcode res;
    string arts = to_string(artistc);
    string titl = to_string(titlec);
    string apikey = api_key();
    if(arts == "")
	{
		arts = "Unknown";
	}


    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://ws.audioscrobbler.com/2.0/");
        string reqformat = std::format("artist={}&track={}&timestamp={}&api_key={}&api_sig={}&sk={}", arts, titl, timestampc,apikey,"test","test");
        cout<<reqformat;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reqformat);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        curl_easy_cleanup(curl);
    }

}

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


        if (percentage >= 60.0 && flag != 1)
        {
            cout << "Scrobbled" << endl;
            scrobble(artist, title,timestamp );
            wcout << title << endl;
            flag = 1;

        }
        if (percentage < 45.0)
        {
            flag = 0;
            if (percentage < 0.1 && tflag != 1)
            {
                const auto p1 = std::chrono::system_clock::now();
                auto time = std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
                timestamp = std::to_string(time);
                cout << timestamp;
                tflag = 1;
                std::this_thread::sleep_for(std::chrono::seconds(3));
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