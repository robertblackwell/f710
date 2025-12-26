# F710

This repo provides a small c++ library for reading the state of a Logitech F710 game controller.

Two implementations of a Reader classes are provided.

## reader.h

In the file reader.h is an implementation of a `Reader` class that implements an asynchronous read 
using the select system call. 

The select call is efficient in this application as input is performed against only a single file
descriptor.

## asio_reader.h

In the file asio_reader.h is a second implementation of a `Reader` that uses __boost::asio__ for
asyncronous reading.


The purpose of this code is so that I can control a differential drive robot that I am building.

The way the code works is specific to how I want to drive my robot and is not in any way a generalized
F710 interface. For examples as it stands it does not read any of the buttons on the controller.


