# udp-joystick

Use this to generate UDP packets of joystick state, then pipe/netcat them to another local or remote program.

## Usage

In one terminal, start listening on port 5000

    nc -ulp 5000   

In another,

    netcat -u 127.0.0.1 5001

