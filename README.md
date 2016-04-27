# inputconfd

`inputconfd` is a basic daemon that monitors udev events for new keyboard or
mouse devices and automatically runs a command to configure them. For example,
I use it as follows in my `~/.xinitrc`:

```
inputconfd --keyboard="xmodmap ~/.Xmodmap; xset r rate 150 50 -b" &
```

The commands passed to the `--keyboard` and `--mouse` options are passed to the
shell, so they can be arbitrarily complicated. Both commands will be run upon
starting the daemon and again any time an applicable device is added. If a
command fails, a message will be printed but the daemon will continue running.

`inputconfd` has the following dependencies:

- libudev

The installation path and compilation flags can be tweaked by editing
`config.mk`. Then, run the usual

```
make
make install
```
