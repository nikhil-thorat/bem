# UDP Message Processing System

## Assignment overview

This project demonstrates how multiple program processes can work together
and communicate. Each system has an **ear**, a **brain**, and a **mouth**.
The ear receives a UDP message, the brain processes it, and the mouth sends
the result to another UDP port. The ear, brain, and mouth processes inside a
system communicate through files and POSIX semaphores.

The processing flow is:

```text
UI or another system
        |
        | UDP message
        v
      Ear -- ear_to_brain file --> Brain -- brain_to_mouth file --> Mouth
                                                                    |
                                                                    | UDP result
                                                                    v
                                                          Another system's ear
```

## Source files

- **`src/ui.c`** — Interactive UDP client. It repeatedly prompts for a message
  and a destination port, then sends the message to that port on localhost.
  Enter `quit` or `exit` to send that command and close the UI.
- **`src/ear.c`** — Listens on the system's UDP port. It writes each incoming
  message to `ear_to_brain` and signals the brain that the file is ready.
- **`src/brain.c`** — Starts the ear and mouth processes, reads messages from
  `ear_to_brain`, and writes results to `brain_to_mouth`. It supports `add`,
  `sub`, `mul`, and `div` with two integer operands. A single number is passed
  through as-is. Division produces a decimal result. `quit` and `exit` stop
  the system without forwarding the command.
- **`src/mouth.c`** — Waits for the brain to write a result, prints it, and
  sends it to the UDP port configured for the system. It does not send the
  internal quit marker.
- **`Makefile`** — Provides commands to compile the programs, run one system
  with example ports, and remove generated executables and communication
  directories.

Each brain creates a directory named `system_<listen_port>`. The files
`ear_to_brain` and `brain_to_mouth` hold messages between processes. POSIX
semaphores coordinate access so a process reads a file only after its writer
has finished.

## Requirements

The project uses a C compiler such as Clang, POSIX sockets, and POSIX
semaphores. On Linux, these are generally available in a standard development
environment.

## Compile

From the project directory, run:

```sh
make compile
```

This creates four executables in the project directory: `brain`, `ear`,
`mouth`, and `ui`.

## Run one system

Start a system with a listening port and a port where its mouth should send
messages:

```sh
./brain 8080 8081
```

The ear listens on UDP port `8080`. The mouth sends processed messages to UDP
port `8081`. The brain starts the ear and mouth automatically. Keep this
terminal open while the system runs.

In another terminal, start the UI:

```sh
./ui
```

For example, enter:

```text
Message (add/sub/mul/div or a number; quit to stop UI): add 2 3
Destination port: 8080
```

The brain calculates `2 + 3`, writes `5` to `brain_to_mouth`, and the mouth
prints `5` and sends it to port `8081`. To stop this system, run `./ui` again,
enter `quit`, and enter `8080`.

## Run two connected systems

Use separate terminals. Start the first system:

```sh
./brain 8080 8081
```

Start the second system with its ear listening on the first system's output
port:

```sh
./brain 8081 8082
```

Now run `./ui` in a third terminal and send `mul 4 5` to port `8080`. The first
brain calculates `20` and its mouth sends it to port `8081`. The second ear
receives `20`; the second brain passes that number through, and its mouth
prints `20` and sends it to port `8082`. Since no system is listening on
`8082` in this example, the message does not continue further.

## Supported messages

| Message | Result written by the brain |
| --- | --- |
| `add 2 3` | `5` |
| `sub 9 4` | `5` |
| `mul 3 4` | `12` |
| `div 7 2` | `3.5` |
| `5` | `5` (passed through) |
| `quit` | Stops the receiving system; does not get sent onward |

`exit` is also accepted as an alias for `quit`. The UI accepts one message per
prompt and continues running until `quit`, `exit`, or end-of-input.

## Useful commands

Run the Makefile's example system (`8080` listening, `8081` output):

```sh
make run
```

Remove the generated executables and `system_*` communication directories
after stopping the systems:

```sh
make clean
```
