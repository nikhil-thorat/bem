# Networking Exercise

A system is consists of three processes
- **Ear** (Listens for incoming messages)
- **Brain** (Processes the message)
- **Mouth** (Sends message to another process)

## How the System works

*Since ear, brain and mouth are considered as a single atomic system*, We can assume that this system listens for incoming messages, processes them and sends them to another similar system.

-> *Incoming Message* -> **System** -> *Sends Message*

### System Components

1. **Ear**
The Ear process is responsible for listening for incoming messages on specified **port**. Once it receives a message it writes that message in the *ear_to_brain* file.

2. **Brain**
The Brain process is responsible for creating *ear* and *mouth* process and reading/writing to files that are used for communicating between these processes. We use *semaphores* for file synchronization as it is required to know whether if we can read/write to a file.

2. **Mouth**
The Mouth process is responsible for reading message from the *brain_to_mouth* file and sending that message to specified port in the arguments.

## How to Run

Once you compile the code using `make compile` command, it will create 3 files
- ear
- brain
- mouth

You just need to run the `brain` with `listen_on_port` and `send_to_port` arguments, or you can use `make run` command to run program with pre-specified arguments.
