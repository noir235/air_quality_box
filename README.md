# MQTT - Air Quality Box

The MQTT air quality box can measure different value of the air around the box. This values can help you to evaluate the air quality in your rooms.In my case, I have in each room one box to understand the environment in my appartment.

How do you get the values from the box: The box itself has no display to show the different values. The box is designed to send each value with the MQTT protocol to a server. The server should able to recieve them as MQTT broker and show you the value for example with an web user interface. I use the famous home assistant and it works great for me.

The following value are measured by the box:
- temperature
- humidity
- CO2
- CO
- NH3
- NO2
- PM 2.5 (dust)
- PM 10 (dust)


Components for MQTT - air quality box:
- ESP8266 nodemcu V3
- nodemcu base
- Grove multichannel gas sensor (CO, NH3, NO2 sensor)
- DHT22 (temperature und humidity sensor)
- MH-Z19 (CO2 sensor)
- SDS011 (PM2.5, PM10 dust sensor)
- power adapter 12V DC, 2A
- Small box or case

![wiring picture](https://github.com/noir235/air_quality_box/blob/master/Wiring.png)
