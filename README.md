# power
WiFi control for ECO Plugs - C++

## imports
- libssl-dev
- libcurl4-openssl-dev

### build
```
cmake .
make
```

### Web Server certificates

#### Install certbot
```sudo apt-get install certbot```
#### Create certificates
```sudo certbot certonly --standalone```
Then follow instructions: define your web server domain &lt;web-domain&gt;

### Run webserver

&lt;path-to&gt;webserver &lt;log-file&gt; /etc/letsencrypt/live/&lt;web-domain&gt;fullchain.pem /etc/letsencrypt/live/&lt;web-domain&gt;privkey.pem

