README.txt
----------

Student Name: Zachary Venzor  
Student ID: [YOUR STUDENT ID]  
CruzID: zvenzor  

Project: CSE 156 - HTTP-to-HTTPS Proxy Server  
Filename: myproxy.c  
Executable: bin/myproxy  
Source Directory: src/  
Documentation Directory: doc/  

---

## Overview

This project implements a multi-threaded HTTP proxy server that:
- Accepts HTTP/1.1 requests from web clients (like curl or a browser).
- Filters requests using a list of forbidden domains or IP addresses.
- Forwards valid requests to the corresponding remote web server (over HTTP).
- Forwards the response back to the original client.
- Logs each request, including error responses, to an access log file.

The server supports **concurrent clients** using threads and handles only `GET` and `HEAD` requests. Unsupported methods or access violations return appropriate HTTP error codes.

---

## Usage

Compile using:

```bash
make
