
# MX5 Hybrid Dash

Just for fun I wanted to build a 'Hybrid Dash' or 'Hybrid Instrument Cluster' for my 2000' MX-5 NB. Hybrid means, that I want to upgrade the OEM instrument cluster to contain some displays and be overall smarter WITHOUT destroying the charme of it. 
This branch contains version 1 of the project. I did not test its functionality in my car yet and I overall do NOT RECOMMEND to build this version for different reasons:
- Like I said this version is not fully tested yet
- Its overall pretty expensive due to using an RaspberryPi4, an Arduino Uno AND a custom made PCB
- After finishing v1 I learned that I designed the power supply in a way, that it probably influences the data lanes and therefore the accuracity of the data (RPM, Speed etc). Because I want version 2 to drop the RPI and the Arduino I did not/will not improve this version. 
This repo contains everything I gathered, used (and saved...). It contains a KiCad project with the PCB and the Arduino Firmware. Because I did not yet finish the RaspberryPi4 software it is NOT contained in this repo. I dont know yet if I will ever finish it and upload it here because its not needed in version 2 anymore. For your information, I used C++ and Qt 6 (Cross Compiled for the RPI4, [see here](https://wiki.qt.io/Cross-Compile_Qt_6_for_Raspberry_Pi)). The applications uses the linux framebuffers. I wrote a custom 'dtoverlay' driver for the used displays, which creates three framebuffers via SPI1 [see here](https://github.com/VogelPapaFinn/gc9a01-overlay-spi1_cs3) for more information.
## Section List

- [MX5 Hybrid Dash](https://github.com/VogelPapaFinn/MX5-HybridDash/tree/v1?tab=readme-ov-file#mx5-hybrid-dash)
- [Section List](https://github.com/VogelPapaFinn/MX5-HybridDash/tree/v1?tab=readme-ov-file#mx5-hybrid-dash)
- [Demo](https://github.com/VogelPapaFinn/MX5-HybridDash/tree/v1?tab=readme-ov-file#section-list)
- [Information about the folders and their content](https://github.com/VogelPapaFinn/MX5-HybridDash/tree/v1?tab=readme-ov-file#information-about-the-folders-and-their-content)
- [Additional Information](https://github.com/VogelPapaFinn/MX5-HybridDash/tree/v1?tab=readme-ov-file#3d-models--additional-information)
- [Contact](https://github.com/VogelPapaFinn/MX5-HybridDash/tree/v1?tab=readme-ov-file#contact)
## Demo Pictures

This section contains some demo pictures so you get an idea of what I actually did. Note that these images are work-in-progress and do NOT represent the finished version (which will probably never exist)!

1. Image of a partially assembled cluster. The GUI is duplicated for testing purposes and not yet finished!
![WhatsApp Image 2024-08-14 at 17 25 46](https://github.com/user-attachments/assets/457070f4-0e5a-4a35-8446-8fc6535c3956)

2. One of the displays in a mount (note, the display in the image is broken!)
![WhatsApp Image 2024-08-14 at 17 25 48](https://github.com/user-attachments/assets/1e8a5361-81c4-43ee-b5b7-3c6052f3ffd8)

3. The same display from the back
![WhatsApp Image 2024-08-14 at 17 25 48(1)](https://github.com/user-attachments/assets/ea8a5bd3-ca7e-4ec9-83f3-a75db4f2d5da)

4. The PCB
![WhatsApp Image 2024-08-14 at 18 19 28](https://github.com/user-attachments/assets/1e02e998-0ce4-4019-8ed2-e4d7b681dec9)

5. The RPI4 and the Arduino mounted on the PCB
![WhatsApp Image 2024-08-14 at 18 19 28(1)](https://github.com/user-attachments/assets/9cef9a0c-9fee-4f15-b8d9-ba80476fc030)

## Information about the folders and their content

Below I will list them and give some brief information about them and their content:
1. **Arduino Sketches** -- This folder contains sketches (source code) for the Arduino IDE to use on the Arduino
    - **ArduinoFirmware** -- Contains the source code of the final 'firmware'
    - **PulseCounter** -- A simple script to measure the frequency/pulses per second on a pin
    - **VSSSimulator** -- A simple script to simulate a Vehicle Speed Sensor -> a Sinus curve
    - **.pdf** -- 'Datasheet' of the used Arduino Uno
2. **Gathered Documents** -- Contains files I found, saved and/or created myself
    - **2000_miata_mx5.pdf** -- Is a manual for a 2000 mx5 which contains all kinds of wiring datagrams etc. It has many useful information (about the instrument cluster. See page 28 & 30)
    - **ArduinoRaspberryCommunication.txt** -- A file which briefly describes how the Arduino and RPI communication 'protocol' should look like
    - **BackOfInstrumentCluster.png** -- An image of the back of my instrument cluster
    - **ClusterConnectorsLayout-From2000Manual.png** -- A screenshot of the layout of the connectors which sit at the back of the cluster. Its copied from the file '2000_miata_mx5.pdf'
    - **displayRequirements.txt** -- A brief description of what the project features should be and possible improvements & ideas
    - **ImageOfClusterConnectors.jpg** -- An image of the clusters which sit at the back of the clusters
    - **NumberedBackOfInstrumentCluster.jpeg** -- The image just with some numbers so you can identify which connector has which number/index
    - **InstrumentCluster_2005-MX-5.pdf** -- Contains information about the cluster, the fuel level sensor and more
    - **PossibleTempSensorValues.txt** -- Possible temperature sensor values and the corresponding temperature in degree celcius
    - **RPMTestData.xlsx** -- Roughly measured VSS output values, the corresponding RPM and a way to calculate the RPM from the measured frequency.
    - **SPI1-CS3_PinoutRPI4.png** -- A pinout for the SPI1 GPIO layout of the RPI4
3. **PCB** -- Contains the KiCad project of the PCB
4. **SpeeduinoConverter (not used)** -- Contains a PCB layout which would be an alternative to the 'VR Conditioner Board' linked in the additional information section

## 3D Models & Additional Information

I designed the display mounts myself with the free-to-use websoftware "OnShape". You can find the 3D model [here](https://cad.onshape.com/documents/e7f03f8bc2dd589c93d10406/w/4d8f17b09202eb8b20daf352/e/a5ddfcce33940dbfa6af1e26?renderMode=0&uiState=66dc995cdcc2976b5f7d5d22). In this OnShape project you can find the bracket and a "cover" which is glued to the top of it. Note: Im not satisfied with it being glued but at the moment I do not have an alternative solution.

Below I will "dump" some links to blogs, threads etc. which contain useful information I used:
- [A thread](https://forum.miata.net/vb/showthread.php?t=780170) I created to gather information around this project.
- [The used displays](https://www.waveshare.com/1.28inch-LCD-Module.htm) which I bought from amazon. They are manufactured by 'WaveShare' and have a screen size of 1.28"
- [Information about the oil gauge](https://www.waveshare.com/1.28inch-LCD-Module.htm)
- [Information about the water temperature relative to the resistance measured](https://www.miataturbo.net/megasquirt-18/water-temo-gauge-thermistor-values-74383/)
- [JLCPCB - A PCB manufacturar](https://jlcpcb.com/)
- [Circuit designer](http://falstad.com/circuit/circuitjs.html) to test small,simple circuits and their behavior
- [3D Model of bracket for a display](https://makerworld.com/en/models/437158#profileId-342358) which inspired my design. It made creating the "claw" mechanism which holds the display into place way easier!
- [Link to the VR Conditioner board](https://bossgarage.eu/en-eu/products/max9926-dual-vr-conditioner) which converts the VSS output to a nice 5V square wave


## Contact

If you have any questions or anything else you are welcome to contact me. There are three possible ways (ranked by my preference):
1. Contact me via the Miata.net forum. Either via my linked thread or via a [private message](https://forum.miata.net/vb/member.php?u=307577)
2. Create an issue here in this GitHub repository
3. Contact me on discord: 'vogelpapafinn'
