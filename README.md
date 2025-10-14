# webserv
A simple C++ Web Server

## Description

Webserv is part of a the 42 core curriculum, and is meant to teach students networking at a low level. As such this server is meant to mimic nginx in its behavior, though not structure. To make it single-threaded, non-blocking, as well as prevent crashes it is structured as a finite state machine with a scheduler. It also includes two CGI methods, one in Python and one in Go.

![alt text] (https://github.com/OneGameDad/webserv/blob/feature/readme/images/Webserv%20-%20FSM.jpg "Webserv Finite State Machine Diagram")

## Authors
This was a group project at Hive Helsinki, and was created by
[Xin (Daniel) Feng] (https://github.com/danielxfeng)
[Mohammad Khlouf] (https://github.com/Mohdkhlouf)
[Gregory Pellechi] (https://github.com/OneGameDad)

## Installation
Down the repository
Set up your own config file. You can copy the structure of the one provided "/tests/conf-d.json"
```bash
make
./webserv [config file path]
```

## Status
The project is complete and no further work will be done on it.
