# jack_patch - manage connections between JACK clients

This external is used to patch connections between clients. Also,
it can look up available clients and ports. Regular expressions
are supported to match a subset of available input or output ports.

Supported methods include:
  * `connect`
  * `disconnect`
  * `query`
  * `get_outputs`
  * `get_inputs`
  * `get_connections`
  * `get_clients`

Roman Haefeli, 2022
Based on jackx by Gerard van Dongen, 2003
