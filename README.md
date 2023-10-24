# Assignment **Simple messanger**

ITMO's network programming assignment. The server uses one thread per connection model and BIO to handle connections. The client uses select() to observe both I/O and stdin events. This is a simple practice of block I/O.

You need to develop the simplest instant messaging system (chat, messenger)

## Server

Must receive messages from clients and send messages to all connected clients. Use threads to work with multiple clients. The name of the binary file being executed must be **server**. The server must have a console interface in the parameters to which is transferred: port

## Client

Each client must have own nickname, set by the user. When you receive a message from another client, the screen should display the time of receiving the message, the user-sender's nickname, and the text of the message. An example: \{05:20\} [John] Hi!


The client must have a console interface. The name of the **client** for binary file. The client accepts the work options through the command line arguments in the following order: server address, port, nickname.

To prevent the received (incoming) messages from interfering with the user's typing, it is suggested that a separate mode of sending a message, for example, when the **m** key is pressed, the user enters his message, new messages from other users are temporarily not displayed, after sending the message (by Enter) the mode is automatically turned off.

## Protocol

Client-\>Server
| Nickname size  | Nick                | Body size       | Body            |
| -------------  |-------------        | -----           |-                |
| 4 bytes        | Nickname size bytes | 4 bytes         |  Data size bytes|
| Network format | -                   |  Network format | -               |

Server-\>Client
| Nickname size  | Nick                | Body size       | Body            | Data size      | Date            |
| -------------  |-------------        | -----           |-                | -              | -               |
| 4 bytes        | Nickname size bytes | 4 bytes         |  Data size bytes| 4 bytes        | Date size bytes |
| Network format | -                   |  Network format | -               | Network format | -               |

## Features that are not required of the server and the client
 - Client registration, authorization, authentication
 - More than one channel of communication
 - Keeping your correspondence history
 - Processing time zones
 - Working with languages other than English

## Requirements
 - The server and the client must be written in the language of the C/C++
 - Make should be used as an build system
 - The code must be successfully compiled and worked for Linux and MacOS
 - Valgrind and Google Thread Sanitizer should not find errors in the code

The directory structure of the project looks like this:
```
├── Dockerfile
├── server.cpp
├── client.cpp
├── Makefile
└── README.md
```

It is forbidden to modify **Makefile/README.md/.clang-format/.gitlab-ci.yml** and other hidden files in this directory. The rest of the files can be changed at your discretion. You can also add new cpp/h files if necessary. If you want to use CMake instead of GMake, you can add **CMakeLists.txt **, but do not forget that ** Makefile** is forbidden to touch.

# Setup Environment
All tests are run under `ubuntu:18.04`. You can choose a different version of ubuntu or a different distribution. But then the tests work on `ubuntu:18.04` is not guaranteed. To use `ubuntu:18.04` there are at least three ways:
 - native OS installation
 - installation using Virtual Box
 - installation using docker

Recommended method using docker. If you want to try another way, then it's under your own setup. Docker usage example:

 1. Building an image
 ```bash
$ sudo docker build -t messanger:latest .
Sending build context to Docker daemon  26.03MB
Step 1/5 : FROM ubuntu:18.04
 ---> 3339fde08fc3
Step 2/5 : LABEL name="messanger"
 ---> Using cache
 ---> 18d4d446880d
Step 3/5 : RUN apt update
 ---> Using cache
 ---> 7b8914da4b6a
Step 4/5 : RUN apt install -y --allow-unauthenticated     clang     g++     clang-tidy     clang-format-7     make
 ---> Using cache
 ---> 73647810ca79
Step 5/5 : RUN ln -s /usr/bin/clang-format-7 /usr/bin/clang-format
 ---> Using cache
 ---> d149e813ff12
Successfully built d149e813ff12
Successfully tagged messanger:latest
 ```
 2. Running an instance
 ```bash
$ sudo docker run -v $(pwd):/home/messanger --rm -it messanger
root@352d2930048c:/#
 ```

Running tidy tests (static analyzer):
```bash
$ make clang_tidy
clang-tidy *.cpp tests/*.cpp -checks=-*,clang-analyzer-*,-clang-analyzer-cplusplus* -- -I tests -I . -std=c++17 -g -O3 -Werror -Wall -Wextra -pthread -pedantic
```

Your code must comply with [google style guide](https://google.github.io/styleguide/cppguide.html ). To check this condition, you need to run:

```bash
$ make clang_format
diff -u <(cat *.cpp *.h tests/*.cpp) <(clang-format *.cpp *.h tests/*.cpp)
```

If clang-format gives an error, then you can fix the code under style guide using the command:
```bash
$ make clang_format_fix
clang-format -i *.cpp *.h tests/*.cpp
```

# Publish Results
 - In this repository, you need to develop in the development branch
 - After you decide that the task is ready, you need to create a merge request in the master branch. You need to add one of the course teachers to reviewers
 - Further communication, code review, testing will take place through this merge request
 - All corrections of comments should go through one merge request. **No need to create separate merge requests after each fix.**
 - After the merge request is approved and merged in the master, the task is considered completed

# Pipeline
After creating a merge request, a pipeline will appear in your branch consisting of 3 stages **build**, **tidy**, **test**.
 - **build** - checking the build of the project under different compilers clang++/g++
 - **tidy** - static analyzer and style guide compliance check
 - **test** - integration tests that check the performance of your project

In addition to pipeline stages, merge request will display tests and their execution results. For dropped tests, you can see the detailed reason for the falls. Detailed information about the gitlab pipeline can be found [here](https://docs.gitlab.com/ee/ci/pipelines/)
