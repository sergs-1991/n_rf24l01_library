This directory contains sources of the library's core. API for this core
is placed up one directory (n_rf24l01_core.h). Generally, this core isn't
used directly, some backend should be used instead. Here's no any build
system 'cause core's sources have to be incorporated in the backend build
system. However if there's no backends suitable for you, you have to
implement such one to make this core work.
