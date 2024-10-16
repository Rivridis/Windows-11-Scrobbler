#include "pch.h"
#include "md5.h"
using json = nlohmann::json;
#include <boost/locale.hpp> 

using namespace std;
using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Media::Control;
using namespace Windows::System::Threading;



int flag = 0;
int tflag = 0;
string timestamp = "";
string hashed = "";
string sessionKey = "";
string api_key();
string shasec();
string username();
string password();

string to_utf8(const string& input) {
    return boost::locale::conv::to_utf<char>(input, "UTF-8");
}

string signature(const string username, const string password, const string api_key, const string shaseck)
{
    string utf8_username = to_utf8(username);
    string utf8_password = to_utf8(password);
    string utf8_api_key = to_utf8(api_key);
    string utf8_shaseck = to_utf8(shaseck);
	string val = "api_key" + utf8_api_key + "methodauth.getMobileSessionpassword" + utf8_password + "username" + utf8_username + shaseck;
    MD5 md5;
    hashed = md5(val);
    return hashed;

}

static void scrobble(wstring artistc, wstring titlec)
{
    CURL* curl;
    CURLcode res;
    string arts = boost::locale::conv::from_utf(artistc, "UTF-8");
    string titl = boost::locale::conv::from_utf(titlec, "UTF-8");
    string apikey = api_key();
    string shase = shasec();
    if(arts == "")
	{
		arts = "Unknown";
	}


    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {

        curl_easy_setopt(curl, CURLOPT_URL, "https://ws.audioscrobbler.com/2.0/");

        string utf8_artist = to_utf8(arts);
        string utf8_title = to_utf8(titl);
        string utf8_timestamp = to_utf8(timestamp);
        string utf8_apikey = to_utf8(apikey);
        string utf8_hash = to_utf8(hashed);
        string utf8_sessionkey = to_utf8(sessionKey);

        char* encoded_artist = curl_easy_escape(curl, utf8_artist.c_str(), utf8_artist.length());
        char* encoded_title = curl_easy_escape(curl, utf8_title.c_str(), utf8_title.length());
        char* encoded_method = curl_easy_escape(curl, "track.scrobble", 14);  // Length of "track.scrobble"
        char* encoded_timestamp = curl_easy_escape(curl, utf8_timestamp.c_str(), utf8_timestamp.length());
        char* encoded_apikey = curl_easy_escape(curl, utf8_apikey.c_str(), utf8_apikey.length());
        char* encoded_hash = curl_easy_escape(curl, utf8_hash.c_str(), utf8_hash.length());
        char* encoded_sessionkey = curl_easy_escape(curl, utf8_sessionkey.c_str(), utf8_sessionkey.length());

        // Format the URL-encoded data into a request string
        string reqformat = "artist=" + string(encoded_artist) + "&track=" + string(encoded_title) +
            "&method=" + string(encoded_method) +
            "&timestamp=" + string(encoded_timestamp) +
            "&api_key=" + string(encoded_apikey) +
            "&api_sig=" + string(encoded_hash) +
            "&sk=" + string(encoded_sessionkey);

        cout<<reqformat;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reqformat.c_str());
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

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
            scrobble(artist, title);
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

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main()
{
   hashed = signature(username(), password(), api_key(), shasec());
   cout << hashed;
   CURL* curl;
   CURLcode res;
  
   string url = "https://ws.audioscrobbler.com/2.0/";
   string post_fields = "method=auth.getMobileSession&username=" + username() +
       "&password=" + password() + "&api_key=" + api_key() +
       "&api_sig=" + hashed + "&format=json";

   curl_global_init(CURL_GLOBAL_DEFAULT);
   curl = curl_easy_init();

   if (curl) {
       std::string readBuffer;

       // Set the URL for the POST request
       curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

       // Specify that this is a POST request
       curl_easy_setopt(curl, CURLOPT_POST, 1L);

       // Set the POST fields
       curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

       // Set up a callback function to store the result of the request
       curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
       curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

       // Perform the request, res will get the return code
       res = curl_easy_perform(curl);

       // Check for errors
       if (res != CURLE_OK)
           fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
       else {
           // Parse the JSON response to extract the session key
           try {
               json data = json::parse(readBuffer);
               sessionKey = data["session"]["key"];
               cout << "Session Key: " << sessionKey << std::endl;
           }
           catch (const json::exception& e) {
               std::cerr << "Error parsing JSON: " << e.what() << std::endl;
           }
       }

       // Print the response (JSON)
       std::cout << "Response: " << readBuffer << std::endl;

       // Clean up
       curl_easy_cleanup(curl);
   }

   curl_global_cleanup();

   init_apartment();
   StartPeriodicMediaInfoCheck();
   getchar();
   return 0;
}