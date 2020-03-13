
# RotaryEncoder
A RotaryEncoder to MQTT daemon, designed for a raspberry Pi running shairport-sync, but can easily be modified to support more<br>
It supports Volume up, Volume Down by rotating encoder and Mute by pressing encoder.

<b>This is designed to be small and very efficient daemon, completley self sufficent</b></br>
Utilises the following libraries (included within this repo) :-
* [GPIO_Pi](https://github.com/sfeakes/GPIO_Pi)
* [Mongoose](https://github.com/cesanta/mongoose)

External libraries (Only needed if using / compiling with forked-daapd API)
* curl / libcurl4-openssl-dev (Only needed if using forked-daapd API)

# Notes
```
Usage is as follows.
        -h         (this message)
        -d         (run as daemon)
        -ms        (MQTT Server:Port)
        -mt        (MQTT Topic)
        -ds        (DAAPD Server:Port)
        -dp        (DAAPD Player ID)
        -rc        (Rotaryencoder Clock GPIO pin)
        -rd        (Rotaryencoder Data GPIO pin)
        -rs        (Rotaryencoder Switch GPIO pin)

MQTT example         :- RotaryEncoder -ms 192.168.1.20:1883 -mt "shairport-sync/Living Room/remote" -rc 27 -rd 22 -rc 17
forked-daapd example :- RotaryEncoder -ds 192.168.1.21:80 -dp 11259997263078 -rc 22 -rd 27 -rs 17
```
# Install

clone the git repo and in most cases simply run, no need to compile.
```
cd ~
git clone https://github.com/sfeakes/RotaryEncoder
cd RotaryEncoder
sudo ./release/RotaryEncoder -ms 192.168.1.20:1883 -mt "shairport-sync/Living Room/remote" -rc 27 -rd 22 -rc 17
```

If that works, install it as a service
```
sudo make install
--or--
sudo ./release/install.sh
```

To compile
```
edit Makefile and command out `ADD_CURL := 1` if not using forked-daapd / curl
make clean
make
```

# Configuration

edit /etc/default/rotartencoder

Options should be self explnatory, just pick how you want to set volume, either post to shairport-sync using MQTT or forked-daapd using REST API's.  Posting to forked-daapd is a little quicker at the moment, but this only works if you are actually running forked-daapd as you DAAP server.

For shairport-sync you need to enable MQTT and remote with something similar to the below in `/etc/shairport-sync.conf`
```
mqtt =
{
        enabled = "yes";
        hostname = "trident";           // Hostname of the MQTT Broker
        port = 1883;                    // Port on the MQTT Broker to connect to
        topic = "shairport-sync/study";
        publish_parsed = "yes";
        enable_remote = "yes";
}
```

If using forked-daapd, just get the ID of the player you're setting the volume on, and use that in rotaryencoder's config.  Hit the below URL for a list of players on your system.
```
http://<forked-daapd-server>:<port>/api/outputs
```

## 1.1 Release
* added support to use forked-daapd API's

## 1.0 Initial Release
