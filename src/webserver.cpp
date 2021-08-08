
#include "mongoose.h"
#include <ostream>
#include <sstream>
#include <fstream>
#include <vector>

#include "History.hpp"

static const char *s_http_port = "https://0.0.0.0:8111";
static const char *CertFileName = "cert.pem" ;
static const char *KeyFileName = "key.pem" ;

struct mg_tls_opts tls_opts ;

void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) ;
std::string parseFile( const char* fileName ) ;

int main(int argc, char *argv[]) {

    const char *historyFileName = (argc>1) ? argv[1] : HistoryLogName ;
    std::cout << "Using history file: " << historyFileName << std::endl ;

    memset( &tls_opts, 0, sizeof(tls_opts) ) ;
    tls_opts.cert = (argc>2) ? argv[2] : (char*)CertFileName ;
    tls_opts.certkey = (argc>3) ? argv[3] : (char*)KeyFileName ;

    struct mg_mgr mgr;
    struct mg_connection *nc;
    const char *err_str;

    mg_mgr_init( &mgr ) ;

    nc = mg_http_listen(&mgr, s_http_port, ev_handler, (void *)historyFileName ) ;
    if (nc == NULL) {
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



void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data ) {
    struct http_message *hm = (struct http_message *)ev_data;

    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *msg = (struct mg_http_message*)ev_data ;  
        if( mg_http_match_uri(msg, "/history" ) ) {
            std::string s = parseFile( (const char *)fn_data) ;
            mg_http_reply(nc, 200, "Content-Type: application/json\nServer: Sprinklers\r\n", "%s", s.c_str() ) ;
        } else if( mg_http_match_uri(msg, "/favicon.ico" ) ) {
            mg_http_reply(nc, 200, "Server: Sprinklers\r\n", "" ) ;
        } else {
            char addr_buf[128] ;
            const char * remote_addr = mg_ntoa( &nc->peer, addr_buf, sizeof(addr_buf) ) ;          
            std::cout << "Bad call from " << remote_addr << " to " << msg->method.ptr << std::endl ;
            mg_http_reply(nc, 400, "Server: Sprinklers\r\n", "" ) ;
        }
    } else if (ev == MG_EV_ACCEPT ) {
        mg_tls_init( nc, &tls_opts ) ;
    }
}


constexpr const char *LogFileName = "sprinkler.log" ;

std::string parseFile( const char *historyFileName ) {
    std::list<HistoryEntry> history = loadHistory( historyFileName ) ;

    std::ostringstream buf ;

    buf << "{\"history\":[" ;

    bool first = true ;
    for( auto &i : history ) {
        if( !first ) buf << "," ;
        buf << "{ \"time\":" << i.eventTimes << ",\"rain\":" <<i.rainFall <<
               ",\"forecast\":" << i.forecastRain << 
               ",\"duration\":" << i.sprinkleTime << "}" ;
        first = false ;
    }
    buf << "]}" ;
    return buf.str() ;
}
