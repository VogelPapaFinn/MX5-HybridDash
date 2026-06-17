# MX5-HybridDash (v2)

A hybrid instrument cluster for a *Mazda MX-5 NB*, which replaces the three small analog gauges and replaces them with 1.5" IPS displays. The project aims to look like an *OEM+* look and feel without destroying the old charm of the car.

<p align="center">
  <img src="https://github.com/VogelPapaFinn/MX5-HybridDash/blob/v2/images/FirstTimeInTheCar.jpeg?raw=true" alt="MX5 HybridDash Hardware" width="650">
</p>

## Hardware

The project consists of four custom made PCBs of two types:
- A big Sensor PCB which sits on the back of the cluster, connects to the sensors & ECU and controls the displays
- Three small display PCBs which connect to the displays and receive instructions & data from the Sensor PCB

All four PCBs utilize a powerful ESP32-S3 microcontroller and the CAN bus to communicate with each other. This allows a robust communication without faulty data and provides the possiblity to expand the cluster with new hardware & features in the future-
The displays have a resolution of 360x360 on a 1.5" IPS panel, giving everything a high detailed look.

## Basic Features

Currently the displays can display:
* **RPM**
* **Speed**
* **Indicator (right & left)**
* **Oil Pressure (switch)**
* **Fuel Level**
* **Water Temperature**

This allows for example to calibrate the fuel level sensor so it is more accurate. In my car it shows that the fuel tank is empty when there are 10-15L left. Furthermore due to my size the indicators & the upper scale of the speedometer hide behind the steering wheel. By displaying them in the lower left and right display this isnt an issue anymore.

## Main Feature

My car is a pre-Facelift model and therefore does not have an OBDII connector (at least thats normal here in europe). After a lot of researching, fumbling around, reverse-engineering and testing I managed to establish a **connection** with the **OEM ECU** **without a OBDII connector**. 

At the moment I knowThis of 33 different values that are readable: *(Valus NOT Sensors, as some Sensors output multiple units)*
* **Alternator** - Load (%)
* **Alternator** - Desired Voltage (°V)
* **Cabin Fan** - Toggle
* **Brake** - Toggle
* **Rear Window Defroster** - Toggle
* **Daytime Lights** - Toggle
* **Coolant** - Sensor Voltage (°V)
* **Alternator** - Temperature (°C)
* **EGR** - Toggle
* **Cooling Fan Speed** - On, Slow Speed
* **Cooling Fan Speed** - On, Medium Speed
* **Cooling Fan Speed** - On, Fast Speed
* **Fuel Pump** - Toggle
* **Injector** - Injection Time (ms)
* **Battery** - Low Voltage (Light Indicator)
* **Headlight Left** - Toggle
* **Headlight Right** - Toggle
* **Idle Bypass** - Time (ms)
* **Idle Switch** - Toggle
* **Engine** - Load (%)
* **Airmass** (°V)
* **Airmass** (g/s)
* **Main Relais** - Toggle
* **Engine** - Check Engine Light 
* **Airmass Pre-Cat** - Voltage (°V)
* **Power Steering** - Pressure Switch Toggle
* **RPM**
* **EGR** - Valve Position
* **Ignition Timing** - Advancing (°)
* **Immobilizer** - Toggle
* **Throttle** - Voltage (°V)
* **ECU** - Input Voltage (°V)
* **Speed** (KMH)

## Future Plans
I plan a lot for the project even if its (software wise) stillt at the early beginning. But here is a list of features I aim to implement:
- [ ] Calibration of the sensors
- [ ] Calculation of new Values by combining the hardware & ECU sensors (e.g. current fuel consumption)
- [ ]  Webserver to control the dash
	- [ ] Reading & displaying data from the ECU
- [ ] Customizable display GUI
- [ ] Connection to the smartphone
	- [ ] Google Maps integration / direction notifications in the display GUI
	- [ ] Spotify integration
- [ ] Sensor data tracking & visualization for analyzation (e.g. on track-day use)

Please note that this are just some ideas of mine. This is not a roadmap. This is not organized by priority.

## About me

Hello! Im Finn a 21 year old computer science student from Bonn (Germany). I work as an embedded software engineer (besides my studies of course) and started tinkering around with electronics around two years ago. This project is developed exclusively in my free time for my own fun. I actually started just because I wanted to learn about electronics... Now here I am :)

## Contact

If you have any questions or anything else you are welcome to contact me. There are three possible ways (ranked by my preference):
1. send me a mail: finn.ganser@gmx.de
2. Create an issue here in this GitHub repository
3. Contact me via the Miata.net forum. Either via my linked thread or via a [private message](https://forum.miata.net/vb/member.php?u=307577)

## Lizenz

The whole project is published under the **GPLv3** license. Basically this means everybody can use, alter and provide the project, free and commercial as long as the provided code and hardware stays open source for the user. Note, that this is my conclusion of the license, before acting please read the license and its terms yourself!!
