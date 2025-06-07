Name: Zachary Venzor
Student ID: 2081422

Project: HTTP Proxy Server

Files:

* myproxy.c: Main source code for the HTTP proxy server.
* Makefile: Compiles the proxy server.
* doc/README.txt: This documentation file.

---

**How to Use**:
Compile with:
make

Run the proxy:
./myproxy -p <port> -a forbidden_path -l log_path
Example:
./myproxy -p 8080 -a forbidden.txt -l access.log

**Design Overview**:

* Multithreaded TCP server that listens on a specified port.
* For each client, spawns a thread that:

  * Parses the HTTP request
  * Rejects unsupported methods (e.g., POST, CONNECT)
  * Filters out forbidden domains using a list
  * Forwards GET/HEAD requests to destination server over HTTP (port 80)
  * Logs client IP, request line, status code, and size

**Shortcomings**:

* Does not support HTTPS tunneling (i.e., no CONNECT proxying)
* Does not cache content
* Only supports HTTP/1.1 (no persistent connections)
* Relies on synchronous I/O per thread

**Test Cases and Results**:

1. **GET Request**

   * Request: GET [http://example.com/](http://example.com/)
   * Response: 200 OK
   * Log: 1521 bytes

2. **HEAD Request**

   * Request: HEAD [http://example.com/](http://example.com/)
   * Response: 200 OK
   * Log: 235 bytes

3. **Forbidden Domain**

   * Request: GET [http://fakenews.com/](http://fakenews.com/)
   * Response: 403 Forbidden
   * Log: 0 bytes

4. **Unsupported Method (POST)**

   * Request: POST [http://example.com/](http://example.com/)
   * Response: 501 Not Implemented
   * Log: 0 bytes

5. **Nonexistent Domain**

   * Request: GET [http://nonexistent12345.ucsc/](http://nonexistent12345.ucsc/)
   * Response: 502 Bad Gateway
   * Log: 0 bytes

6. **Unsupported CONNECT Method**

   * Request: CONNECT [www.example.com:443](http://www.example.com:443)
   * Response: 501 Not Implemented
   * Log: 0 bytes

**Notes**:

* Currently forbidden sites must match exactly or with '[www](http://www).' prefix. Ask prof what to do. 
