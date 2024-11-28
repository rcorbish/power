
#include "mongoose.h"
#include <ostream>
#include <sstream>
#include <fstream>
#include <vector>

#include "History.hpp"
#include "Weather.hpp"

static const char *s_http_port = "https://0.0.0.0:8111";
static const char *CertFileName = "cert.pem";
static const char *KeyFileName = "key.pem";
extern const char *WebPageSource;

struct mg_tls_opts tls_opts;
struct mg_http_serve_opts html_opts;
struct mg_http_serve_opts css_opts;
struct mg_http_serve_opts pdf_opts;
struct mg_http_message home;

void ev_handler(struct mg_connection *nc, int ev, void *ev_data);
std::string parseFile(const char* fileName);
std::string getCurrentWeather();
std::string getTime();

static long lastWeatherRead = 0L;
static char weatherMessage[1024];
Weather *weather;

int main( int argc, char *argv[] ){

    const char *historyFileName = (argc>1) ? argv[1] : HistoryLogName;
    std::cout << "Using history file: " << historyFileName << std::endl;

    memset( &tls_opts, 0, sizeof(tls_opts));

    tls_opts.cert = mg_file_read(&mg_fs_posix, (argc>2) ? argv[2] : CertFileName );
    tls_opts.key = mg_file_read(&mg_fs_posix, (argc>2) ? argv[3] : KeyFileName );

    memset( &html_opts, 0, sizeof(html_opts));
    html_opts.mime_types = "html=text/html";
    html_opts.extra_headers = "Content-Type: text/html\nServer: Sprinklers\r\n";

    memset( &css_opts, 0, sizeof(css_opts));
    css_opts.mime_types = "html=text/css";
    css_opts.extra_headers = "Content-Type: text/css\nServer: Sprinklers\r\n";

    memset( &home, 0, sizeof(home));

    weather = new Weather( "31525", 24, 24);
    weather->init();

    struct mg_mgr mgr;
    struct mg_connection *nc;
    const char *err_str;

    mg_mgr_init( &mgr ) ;

    nc = mg_http_listen(&mgr, s_http_port, ev_handler, (void *)historyFileName ) ;
    if (nc == nullptr) {
        std::cerr << "Error starting server on port " << s_http_port << std::endl ;
        exit( 1 ) ;
    }

    std::cout << "Starting RESTful server on port " << s_http_port << std::endl ;
    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    return 0; 
}



void ev_handler(struct mg_connection *nc, int ev, void *ev_data ) {
    struct http_message *hm = (struct http_message *)ev_data;

    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *msg = (struct mg_http_message*)ev_data;  
        if( mg_match(msg->uri, mg_str("/history"), nullptr ) ) {
            std::string s = parseFile( (const char *)nc->fn_data);
            mg_http_reply(nc, 200, "Content-Type: application/json\nServer: Sprinklers\r\n", "%s", s.c_str() ) ;
        } else if( mg_match(msg->uri, mg_str("/weather"), nullptr ) ) {
            std::string s = getCurrentWeather();
            mg_http_reply(nc, 200, "Content-Type: text/html\nServer: Sprinklers\r\n", "%s", s.c_str());
        } else if( mg_match(msg->uri, mg_str("/home"), nullptr ) ) {
            mg_http_serve_file( nc, &home, "home.html", &html_opts);
        } else if( mg_match(msg->uri, mg_str("/css.css"), nullptr ) ) {
            mg_http_serve_file( nc, &home, "css.css", &css_opts);
        } else if( mg_match(msg->uri, mg_str("/favicon.ico"), nullptr ) ) {
            mg_http_reply(nc, 400, nullptr, "Code:Xenon");
        } else {
            char addr_buf[256];
            const auto len = mg_snprintf( addr_buf, sizeof(addr_buf), "%.*s, Bad call from %M", (int)msg->uri.len, msg->uri.buf,  mg_print_ip, &nc->rem );
            std::cerr << addr_buf << std::endl;
            mg_http_reply(nc, 400, nullptr, "");
        }
    } else if (ev == MG_EV_ACCEPT) {
        mg_tls_init( nc, &tls_opts);
    } else if (ev == MG_EV_ERROR) {
        std::cerr << "Error: " << (char *) ev_data << std::endl ;
    }
}

constexpr const char *LogFileName = "sprinkler.log";

std::string parseFile( const char *historyFileName ) {
    std::list<HistoryEntry> history = loadHistory( historyFileName );

    std::ostringstream buf;

    buf << "{\"history\":[";

    bool first = true;
    for( auto &i : history ) {
        if( !first ) buf << ",";
        buf << "{ \"time\":" << i.eventTimes << ",\"rain\":" <<i.rainFall <<
               ",\"forecast\":" << i.forecastRain << 
               ",\"duration\":" << i.sprinkleTime << "}";
        first = false;
    }
    buf << "]}";
    return buf.str();
}


std::string getCurrentWeather(){
    long now = time(nullptr);
    if( (now - lastWeatherRead) > (60 * 30) ) {   // only read every 30 mins
        lastWeatherRead = now;
        weather->read();
        double totalRain = weather->getRecentRainfall();
        double forecastRain = weather->getForecastRainfall();
        std::string desc = weather->getDescription();
        snprintf( weatherMessage, sizeof(weatherMessage),
            "Current %s<br>Previous 48hrs %4.2f<br>Forceast 24hrs %4.2f<br>As of %s", 
            desc.c_str(), totalRain, forecastRain, getTime().c_str()); 
    }
    return std::string(weatherMessage);
}

std::string getTime(){
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80]; 

    time( &rawtime );
    timeinfo = localtime(&rawtime);

    strftime (buffer,80,"%d-%b-%Y %H:%M:%S ",timeinfo);

    return std::string(buffer);
}