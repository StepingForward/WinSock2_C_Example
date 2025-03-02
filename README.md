# Winsock2.h
This is a small example made for Windows using Winsock2.h in C.
It is capable of creating a local server, creating clients with some extra data(name) and allowing them to chat with each other.

### Client message -> Local Server -> All other clients

## Using
If on windows, no external libraries needed, I used MSVC as the compiler.

When running make sure to have [multiple instances](https://stackoverflow.com/questions/5498911/run-multiple-instances-with-one-click-in-visual-studio) allowing you to test easily.

Enter "s" to create a local server.

Enter "c" to create a client which will automatically connect to the server(or another ip, can be specified in code).

When entering as a client write your name and you can chat with other clients connected to the same local server.

In the server window you can see some extra debug info(joining, leaving, messaging).

#### Hopefully this code can help you some way, have a good day! 
