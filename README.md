# Spectrum To Colour
A small program that converts given spectral data to RGB-values. 


## Usage example
You can clone this repository to a suitable location on your hard-drive using 

```sh
git clone https://github.com/sschimper/SpectrumToColour.git
```

Navigate to 

```sh
SectrumToColour/cmake-build-debug
```
Here you can find the executable with the name "spectocol".

Using this command, you will get an overview over the functionality of this program:

```sh
./spectocol --help
```

For example, if you want to calculate the RGB value, based of the luminaire "CIE Illuminant D65 (daylight)" function and the "XRite Colour Checker SG Patch E2 (dark skin)" with 32 random samples, 
you can use the command:

```sh
./spectocol --random 32 -l cied -r e2
```
