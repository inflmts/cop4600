# COP4600 Operating Systems

Daniel Li, Fall 2025, University of Florida

## Reptilian

Reptilian is a pairing of the lightweight Android kernel with the Ubuntu
userspace. This course uses it for assignments and projects.

This repository comes with a Reptilian launcher script that automatically
downloads the Reptilian VM image, converts it to qcow2, and runs it in QEMU.
It might even work on Windows with Git Bash, although I haven't tested it.

More info:

- <https://cise.ufl.edu/research/reptilian/wiki/doku.php>
- <https://www.qemu.org>
- <https://www.qemu.org/docs/master/system/invocation.html>
- <https://www.qemu.org/docs/master/system/images.html>

### Getting Started

Requirements:

- An x86-64 machine
- qemu-system-x86\_64
- qemu-img
- curl
- tar

Basic execution:

```
./reptilian
```

The internal SSH server is forwarded to port 4600. Access the VM using:

```
ssh -p 4600 reptilian@localhost
```

The default password is `reptilian`.

To transfer files to and from the emulator, use `scp`:

```
scp -P 4600 <localpath> reptilian@localhost:<vmpath>
scp -P 4600 reptilian@localhost:<vmpath> <localpath>
```

To quit the emulator, either use `quit` (`q`) from the monitor,
or simply close the window.

### Advanced Usage

Initialize the image without starting Reptilian:

```
./reptilian --init
```

Delete and redownload the image:

```
./reptilian --reset
```

Load a VM snapshot on startup:

```
./reptilian --load=NAME
```

To create a snapshot, switch to the QEMU monitor (Ctrl-Alt-2) and use the
`savevm` command. To load a snapshot while the emulator is running, use the
`loadvm` command. To list stored snapshots, use `info snapshots`.
More information is available at:
<https://www.qemu.org/docs/master/system/images.html#vm-snapshots>

See `./reptilian --help` for a full list of options.
