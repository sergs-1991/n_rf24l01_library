It's a wrapper/backend implementation for Linux (a *.so library).
A wrapper provides a fd-based interface to the n_rf24l01 transceiver,
so to work with this library you can use a traditional Linux approach
(poll, read, write and so on).

In a src directory you may find several backends implementations.
