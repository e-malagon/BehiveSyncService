# SyncService

This repository contains the source code of an SQLite Sync Service fully developed by me. This service was created as part of a personal project to create a distributed multiplatform management application. Each client was designed to use a modified version of SQLite that could collect the transactions made by each client and synchronize them in the background. The synchronization process was completely transparent for the application layer of each client.

Unfortunately, due to time constraints the development of this project was stopped a few years ago. I kept this repository because it contains some C++ useful code that could be reused. I published it as opensource because it could be useful for someone else, also there are a few components that could be candidates to be converted into standalone libraries for another projects.

The SyncService folder contains the original code that is already done a fully functional. The Beehive folder contains a new version. This version contains some improvements regarding the organization of the project, also a change of the backend storage from MySQL to RocksDB, this migration was not concluded.

The components that could be candidates to be a standalone library are:

* HTTP backend to create fully C++ REST services using Nginx, Lighttpd, or Apache as web layer. This component contains code to handle the authentication and cookies.
* TCP backend to handle concurrent binary connections.
* MySQL C++ connector with a connection pool.

There are also some examples of how to connect with the Google API for android developers and the push services in C++, using a JWT token validation. Integration with Lua scripts and some C++ iterators to serialize/deserialize binary or text data.
