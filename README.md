# MIDI Controller

## Cloning the project
```bash
# Clone the project, important to note that this project requires submodules
git clone --recurse-submodules git@github.com:jeremycote/MidiPedalboard.git

# OR

# Clone submodules in exisiting project
git clone git@github.com:jeremycote/MidiPedalboard.git
cd MidiPedalboard
git submodule update --init --recursive
```

## Compiling the project
There are two environment variables necessary to compile this project. 
`WIFI_SSID` and `WIFI_PASS` define the credentials necessary to connect the microcontroller to 
the network.

```bash
# Compile project from command line on MacOS or Linux
cd MidiPedalboard
mkdir build
cd build
export WIFI_SSID="ENTER SSID"
export WIFI_PASS="ENTER PASS"
cmake ..
cmake --build . -j8

# Compile project from command line on Windows
cd MidiPedalboard
mkdir build
cd build
$env:WIFI_SSID="ENTER SSID"
$env:WIFI_PASS="ENTER PASS"
cmake ..
cmake --build . -- -m 8
```