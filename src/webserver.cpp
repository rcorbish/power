
#include "mongoose.h"
#include <ostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>

#include "Connection.hpp"
#include "History.hpp"
#include "Weather.hpp"
#include "Options.hpp"

constexpr const char *s_http_port = "https://0.0.0.0:8111";
constexpr const char *CertFileName = "cert.pem";
constexpr const char *KeyFileName = "key.pem";

constexpr int  SecurityPort = 9111 ;
constexpr int  Buffer_Size = 1024 ;

struct mg_tls_opts tls_opts;
struct mg_http_serve_opts html_opts;
struct mg_http_serve_opts css_opts;
struct mg_http_serve_opts pdf_opts;
struct mg_http_message home;

void ev_handler(struct mg_connection *nc, int ev, void *ev_data);
std::string parseFile(const char* fileName);
std::string getCurrentWeather();
std::string getDeviceState( const std::string &device );
std::string getSecurityInfo( const int port );

static long lastWeatherRead = 0L;
static char weatherMessage[1024];
Weather *weather;
Connection *con = nullptr; 

Args args;

int main( int argc, char *argv[] ){

    Args args = parseOptions( argc, argv );

    // Open connection to the device
    con = new Connection(); 
    for( int i=0 ; i<10 ; i++ ) {
        if( con->found( args.device ) ) break;
        con->discover();
        std::this_thread::sleep_for(std::chrono::seconds(7));
    }
    std::cout << "Using device name: " << args.device << std::endl;
    std::cout << "Using history file: " << args.historyFile << std::endl;

    memset( &tls_opts, 0, sizeof(tls_opts));

    tls_opts.cert = mg_file_read(&mg_fs_posix, args.certificateFile.c_str() );
    tls_opts.key = mg_file_read(&mg_fs_posix, args.keyFile.c_str() );

    memset( &html_opts, 0, sizeof(html_opts));
    html_opts.mime_types = "html=text/html";
    html_opts.extra_headers = "Content-Type: text/html\nServer: Sprinklers\r\n";

    memset( &css_opts, 0, sizeof(css_opts));
    css_opts.mime_types = "html=text/css";
    css_opts.extra_headers = "Content-Type: text/css\nServer: Sprinklers\r\n";

    memset( &home, 0, sizeof(home));

    weather = new Weather( args.zip, args.previousHoursToLookForRain, args.forecastHours);
    weather->init();

    struct mg_mgr mgr;
    struct mg_connection *nc;
    const char *err_str;

    mg_mgr_init( &mgr ) ;

    nc = mg_http_listen(&mgr, s_http_port, ev_handler, (void *)&args ) ;
    if (nc == nullptr) {
        std::cerr << "Error starting server on port " << s_http_port << std::endl ;
        exit( 1 ) ;
    }

    std::cout << "Starting RESTful server on port " << s_http_port << std::endl ;
    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    delete con;
    return 0; 
}



void ev_handler(struct mg_connection *nc, int ev, void *ev_data ) {
    struct http_message *hm = (struct http_message *)ev_data;

    const auto args = (const Args *)nc->fn_data;

    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *msg = (struct mg_http_message*)ev_data;  
        if( mg_match(msg->uri, mg_str("/history"), nullptr ) ) {
            std::string s = parseFile( args->historyFile.c_str() );
            mg_http_reply(nc, 200, "Content-Type: application/json\nServer: Sprinklers\r\n", "%s", s.c_str() ) ;
        } else if( mg_match(msg->uri, mg_str("/weather"), nullptr ) ) {
            std::string s = getCurrentWeather();
            mg_http_reply(nc, 200, "Content-Type: text/html\nServer: Sprinklers\r\n", "%s", s.c_str());
        } else if( mg_match(msg->uri, mg_str("/home"), nullptr ) ) {
            mg_http_serve_file( nc, &home, "home.html", &html_opts);
        } else if( mg_match(msg->uri, mg_str("/state"), nullptr ) ) {
            std::string s = getDeviceState(args->device);
            mg_http_reply(nc, 200, "Content-Type: text/html\nServer: Sprinklers\r\n", "%s", s.c_str());
        } else if( mg_match(msg->uri, mg_str("/security-data"), nullptr ) ) {
            std::string s = getSecurityInfo(SecurityPort);
            mg_http_reply(nc, 200, "Content-Type: application/json\nServer: Sprinklers\r\n", "%s", s.c_str());
        } else if( mg_match(msg->uri, mg_str("/security"), nullptr ) ) {
            mg_http_serve_file( nc, &home, "security.html", &html_opts);
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


std::string getDeviceState( const std::string &device) {
    if( con == nullptr ) {
        return "Sprinklers are not found";
    }

    try {
        bool isOn = con->get(device) ;

        char buffer[128];
        snprintf( buffer, sizeof(buffer), "Sprinklers are %s", (isOn ? "ON" : "OFF") ); 
        return std::string(buffer);
    } catch( const std::string &e ) {
        std::cerr << e << std::endl;
    }
    return "Can't get device state";
}


std::string getSecurityInfo( const int port) {

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[Buffer_Size+1] = {0};
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return "";
    }
    
    // Set up server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return "";
    }
    
    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return "";
    }
        
    // Send 3 bytes to server
    send(sock, "Go\n", 3, 0);
    
    // Receive response from server
    std::stringstream ss;
    int valread = 0;
    do {
        valread = read(sock, buffer, Buffer_Size);
        if( valread > 0 ) {
            buffer[valread] = 0;
            ss << buffer ;
        }
    } while( valread > 0 );
    
    // Close the connection
    close(sock);
    return ss.str();
}
