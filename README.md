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

