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
Then follow instructions: define your web server domain <web-domain>

### Run webserver

<path-to>/webserver <log-file> /etc/letsencrypt/live/<web-domain>/fullchain.pem /etc/letsencrypt/live/<web-domain>/privkey.pem

