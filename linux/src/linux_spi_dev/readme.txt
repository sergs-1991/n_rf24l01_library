It's a wrapper/backend implementation which uses a standard Linux
device driver - spi_dev.

As GPIO pins are controled from the user space by the GPIO SYS interface,
they has to be properly configured, for example look at .prepare.sh script
for the odroid-u3 board.
