# Minipass Door Controller Firmware

The device is based on a 16F877A PIC microcontroller. Its purpose is to control access through two doors allowing the entry of only authorized personnel identified by an access code. It can be connected in a bus-type network with other [Minipass Door Controllers](https://github.com/marcelpedreira/minipass-door-controller-firmware), [Minipass Input/Output Controllers](https://github.com/marcelpedreira/minipass-input-output-controller-firmware) and the [Minipass Supervision System](https://github.com/marcelpedreira/minipass-desktop-application). The firmware is developed using C language through the Custom Computer Services PCW compiler for mid-range Microchip microcontrollers.

## Components

- MAX485 interface for communication using Modbus RTU protocol.
- Dip switches to set the device address on the network.
- Real Time Clock for reports generation.
- 2 External EEPROM memories to store access codes and reports respectively.
- 2 relay outputs for connecting the door actuators.
- 2 digital inputs for connecting the REX sensors.
- 2 interfaces for the connection of identification devices using Wiegand protocol for communication.

<!-- ## General Diagram

![Door Controller Diagram](/assets/ControladorPuerta2.JPG) -->
