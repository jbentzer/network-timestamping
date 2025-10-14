# Timestamping

Proof of concept application for reading PTP time stamp on network packages. Runs on Linux.
The receiver enables timestamps enables hardware and software timestamps on received network packages. The sender is a simple tool to send packages to the receiver.
The capabilities of the network card can be checked with:

```console
  ethtool -T <interface>
```

## Build

Build receiver and sender with:

```console
  make
```

Delete binaries with:

```console
  make clean
```

Build the receiver with:

```console
  make receiver
```

or

```console
  g++ receiver.cpp -o receiver
```

Compile the sender with:

```console
  make sender
```

or

```console
  g++ sender.cpp -o sender
```

## Usage

### Sender

A simple UDP sender that can send messages to a specified destination and port.

Help:

```console
./sender [--help] [-h]
```

Default destination is 127.0.0.1 and default port is 319 (PTP event messages).

Example:

```console
./sender --dest 192.168.1.100 --port 12345 --msg "Hello, World!"
```

### Receiver

A simple UDP receiver that listens on a specified port and prints received messages along with their software and hardware timestamps.

Help:

```console
./receiver [--help] [-h]
```

Default port is 319 (PTP event messages).
Note: Requires root privileges to receive hardware timestamps.


Example:

```console
sudo ./receiver --port 12345
```
