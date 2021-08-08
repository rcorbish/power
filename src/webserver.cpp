
#include "mongoose.h"
#include <ostream>
#include <sstream>
#include <fstream>
#include <vector>

#include "History.hpp"

static const char *s_http_port = "https://0.0.0.0:8111";
static const char *CertFileName = "cert.pem" ;
static const char *KeyFileName = "key.pem" ;
extern const char *WebPageSource ;

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
        } else if( mg_http_match_uri(msg, "/home" ) ) {
            mg_http_reply(nc, 200, "Content-Type: text/html\nServer: Sprinklers\r\n", "%s", WebPageSource ) ;
        } else {
            char addr_buf[128] ;
            const char * remote_addr = mg_ntoa( &nc->peer, addr_buf, sizeof(addr_buf) ) ;          
            std::cout << "Bad call from " << remote_addr << "\n" << msg->method.ptr << std::endl ;
            mg_http_reply(nc, 400, nullptr, "" ) ;
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

const char *WebPageSource = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<meta charset='utf-8'/>\n"
"<link rel='shortcut icon' href='data:image/x-icon;base64,AAABAAEAEBAQAAEABAAoAQAAFgAAACgAAAAQAAAAIAAAAAEABAAAAAAAgAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAA/4QAAOvl3wDy7+sAz8a8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABEAARAAEQAAEQABEAARAAAAAAAAAAAAAAEQABEAARAAARAAEQABEAAAAAAAAAAABEQENENEQABEREREREQkQEI0REJENERERERCRENEREQERERDREJEAABCNEQkREAABEREREJEQAAENEBCRENAAABEQERARAAAAAAABEAAAADOcwAAznMAAP//AADnOQAA5zkAAP//AACIBwAAAAEAAAAAAAAAAAAAgAMAAMAHAACABwAAhAcAAMRPAAD+fwAA' type='image/x-icon'>\n"

"<script>\n"

"function reqListener () {\n"
"   var data = JSON.parse(this.responseText);\n"
"   var div = document.getElementById( 'rainfall' );\n"
"   for( var i=0 ; i<data.history.length ; i++ ) {\n"
"       var newNode = document.createElement('div');\n"
"       var ht = (5*data.history[i].rain);\n"
"       ht = ht>199 ? 199 : ht ;"
"       var height = '' + ht + 'px';\n"
"       newNode.style.margin = '10px' ;\n"
"       newNode.style.height = height;\n"
"       newNode.style.width = '25px';\n"
"       newNode.style.position = 'relative';\n"
"       newNode.style.display = 'inline-block';\n"
"       newNode.style.verticalAlign = 'baseline';\n"
"       newNode.style.backgroundColor = 'blue';\n"
"       div.appendChild( newNode );\n"
"   }\n"
"}\n"

"function document_loaded() {\n"
"   var oReq = new XMLHttpRequest();\n"
"   oReq.addEventListener('load', reqListener);\n"
"   oReq.open('GET', 'history');\n"
"   oReq.send();\n"
"}\n"

"window.addEventListener('load', document_loaded);\n"

"</script>\n"

"</head>\n"
"<body>\n"

"<div id='rainfall' style='height:200px;overflow:hidden;'>\n"
"</div>\n"

"</body>\n"
"</html>\n"
;