# C++ Socket Programming: Simple blockchain system
Digital transaction is based on the blockchain, a chain of information blocks where we store all the transactions that have taken place since the start of the system. I implemented a simplified version of the blockchain system for digital transactions with C++ socket programming.

[Demo video](https://youtu.be/uVTw2MP3JQw)

## Table of contents
* [Introduction](#introduction)
* [Illustrations](#illustrations)
* [Technologies](#technologies)
* [Setup](#setup)
* [Features](#features)
* [Demo](#demo)
* [Reference](#reference)

## Introduction
There are three nodes(Backend Servers) on the blockchain, and each node store a group of transactions. While transaction protocol is responsible for updating the digital wallet of each user in the blockchain, for this project I have main server in charge of running the calculations and updating the wallets for each user. Each transaction in the blockchain includes, in the following order, the transaction serial number, sender, receiver, and amount being transferred.

## Illustrations
![](https://i.imgur.com/Vi9717L.png)

* **Client (clientA, clientB)**
Two clients issue a request for finding their current amount of coins in their account, transfer coins to another user, get its own statistical report and update a file statement with all transactions in order. These requests will be sent to a main server over TCP connection, and the main server will interact with three backend servers for pulling information and data processing.
* **Main Server (serverM)**
The main server will connect to server A, B and C over UDP. Requesting the transaction information from backend servers, the main server can find out the current balance of the user, transfer money from one user to another, generate statistical report of user's transactions and store all transactions in a file statement on the Main server.
* **Backend Server (serverA, serverB, serverC)**
Server A has access to block1.txt, server B has access to block2.txt and server C has access to block3.txt, and each text file has a group of transactions that are stored without order.

## Technologies
C++11

## Setup
* To run this project, compile all files using ```make```.
* Open 4 different terminals to run serverM, serverA, serverB and serverC using ```./serverM``` ```./serverA``` ```./serverB``` ```./serverC```.
* Open 2 different terminals for clientA and clientB, and run client with argument. ```./clientA <argument>``` ```./clientB <argument>```

## Features
![](https://i.imgur.com/SLOpxQt.png)

## Reference
* Beej's Code: http://www.beej.us/guide/bgnet/
    * Chapter5 System Calls or Bust:
      - create/bind/connect/listen/accept TCP socket
      - create/bind UDP socket
      - send/receive TCP message
      - send/receive UDP message
    * Chapter7.2 poll()—Synchronous I/O Multiplexing:
      - poll TCP socket