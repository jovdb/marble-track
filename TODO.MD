- At startup always start AP and connect to the network if possible.
At this way it should be possible that user on the network and users not on the network can connect.

- Make it possible to configure network via website (AP)
  The esp could hav a websockects action to return a list of networks (SSID) seen by the  esp with some information ( return sorted by strength) 
The uses can select the SSID he want to connect with and enter a password.
The ESP should have an action so the website can update the network credentials.

- Led: Clear blink state values if led is not blinking (on or off)

- Implement WebMcp for all WS message the website can send or receive

- Wheel: Check state, targetAngle always seems to be  1?

- Wheel: When moving to a location we are already at, wheel does not move and stay in moving state?

- make USB monitor docking in the website

- Wheel, I would expect angle 0 is when zero sensor is triggered. Check if that is the start angle configured.


- notification when not used for all multiples of 5min

- launcher needs attention

- Lift still sometimes keeps going down when zero is reached.

- Ball on back of wheel sometimes got stuck on exit.

- Servo show disabled state in website

- state wheel ( current breakpoint to -1: when moving to target)