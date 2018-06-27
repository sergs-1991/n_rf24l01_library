This project is about a code which controls an n_rf24l01 transceiver.
There's a library's core and several wrapper/backends.

Common code is placed in the library's core. Codes related to OS and hardware
are placed in specific directories like linux,... The new backend can be added,
to do this you have to use core's API n_rf24l01_core.h and provide all callbacks.

Also there's some wrappers (with conjunction to backend) which implements more
appropriate API for the n_rf24l01 transceiver.
