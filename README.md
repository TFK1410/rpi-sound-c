# rpi-sound-c
## What it is

This is a project for visualizing sound data on Raspberry Pi 3 using LED screens with HUB75 connections and a soundcard.

The required software components are FFTW3 , rpi-rgb-led-matrix and portaudio libraries.

For now the repository is in a state where all code is written to be as optimal as it can sacrificing some of the readability of the code. The configuration for the display is also all left in the source files itself. One of the next TODOs here is adding some external loading of the configuration in runtime.

Software includes:
* Display of FFT bins in logarithmic scale with color gradient starting from green at the bottom through yellow to red
* White dot scale similar to those seen in Winamp spectrum display which will hold the temporary max value and after some time it will start to fall
* Beat detection based on value spikes in subbands above the average value from history
* Ability to add more ways of displaying the data and for it to change dynamically

## Before running

Before compiling the code make sure to apply the diff inside the build directory to the rpi-rgb-led-matrix submodule repository. This is because this library is written with C++ in mind and the C bindings are missing the options to add Your own pixel mapper. More details about that can be found in the README's of the rpi-rgb-led-matrix repository.
```
rpi-sound-c/rpi-rgb-led-matrix $ git apply ../build/rpi-rgb-led-matrix.diff
rpi-sound-c/rpi-rgb-led-matrix $ make
```

To be able to run the software next thing should be to also compile the FFTW library on Rasbperry Pi. I refer to the official website for more info on how to compile FFTW http://www.fftw.org/.

Next thing is setting up the default recording interface on the Rasberry Pi. Command like `arecord -l` should be helpful here as well as looking for the topic of blacklisting unneeded sound cards on the RPI.

One thing that also may recommend is isolating two cores from the four available on the RPI. This can improve the performance a little bit especially for the rpi-rgb-led-matrix library functions. This can be accomplished by adding `isolcpus=2,3 rcu_nocbs=2,3` at the end of /boot/cmdline.txt of the raspberry and then rebooting it. The isolation should be seen when looking at the content of the /proc/cmdline file:
```
$ cat /proc/cmdline
8250.nr_uarts=1 bcm2708_fb.fbwidth=656 bcm2708_fb.fbheight=416 bcm2708_fb.fbswap=1 vc_mem.mem_base=0x3ec00000 vc_mem.mem_size=0x40000000  dwc_otg.lpm_enable=0 console=ttyAMA0,115200 console=tty1 root=PARTUUID=e968d391-02 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait isolcpus=2,3 rcu_nocbs=2,3
```

## Various defines

Currently the code instead of loading the configuration on runtime it is littered with defines across the code. Below is an explanation for them.

| Define                   | Source file | Original value | Purpose
| ------------------------ | ------------- | ------------- | ------------- |
| LED_MATRIX_ROWS          | main.c  | 32            | Height of a single LED display module |
| LED_MATRIX_COLS          | main.c  | 64            | Width of a single LED display module |
| LED_MATRIX_CHAINS        | main.c  | 4             | Number of connected LED displays in a single chain |
| LED_MATRIX_PIXEL_MAPPER  | main.c  | "UInv-Mapper" | Name of the pixel mapper for the rpi-rgb-led-matrix library. This is a new pixel mapper added via the diff file. It allows us to see the display in code as a single 128x64 array rather than 4 separate modules. |
| SAMPLE_RATE              | main.c  | 44100         | Sample rate for PortAudio |
| FFT_CORE                 | main.c  | 1             | The core number to which the FFT thread will be pinned to |
| MAIN_CORE                | main.c  | 2             | The core number to which the main loop will be pinned to *NOTE* rpi-rgb-led-matrix library will automatically pin itself to core 3 when it finds that it is isolated. |
| CHUNK_POWER              | main.c  | 13            | 2^x number of FFT bins to calculate. This example will calculate 8192 bins every time |
| FFT_UPDATE_RATE          | main.c  | 100           | Approximate update rate for the FFT  and beat detection thread |
| MIN_HZ                   | main.c  | 36            | The lowest frequency that will be reflected on the display |
| MAX_HZ                   | main.c  | 20000         | Maximum frequency that will be reflected on the display |
| FFT_CURVE                | main.c  | 0.7           | Smoothing level of the output data. This describes how will the new FFT data apply to the output matrix. The higher number up to 1 will make the display more dynamic. Lower values mean a smoother transitions between old and new values. These values also apply to the beat detection. |
| DISPLAY_CURVE            | main.c  | 0.7           | This is the second value to be applied in a simillar fashion to the FFT_CURVE. This time however it will transform the transition at the display to an exponential(?) curve. Both FFT_CURVE and DISPLAY_CURVE will have to be soon combined into one. |
| LED_MATRIX_BRIGHTNESS    | main.c  | 50            | Maximum brightness of the LED panels. Can be set to values from 0 to 100. |
| WAVE_SPEED               | main.c  | 500           | Arbitrary value which will define the speed of the beat detection wave. |
| SUBBANDS_COUNT           | main.c  | 64            | Number of frequency subbands for the beat detection algorithm |
| SUBBANDS_HISTORY         | main.c  | 43            | Number of previous values of each subband to be stored for the average count |
| SUBBANDS_CONSTANT        | main.c  | 1.1           | It defines the coefficient for detecting the beat. Example describes that a new value that is 1.1 times bigger than the recent average value means that a beat has been detected. |
| WHITE_DOT_HANG           | main.c  | 20            | This describes for how many FFT calculation iterations will the white dots keep their positions. In this example if the max value for a bin hasn't been reached for 20 iterations then the dot will start to fall down. |
| DISP_CHANGE_SEC          | main.c  | 60            | Time in seconds for each change of the display type. |
| DARKER_MULT              | loops.h | 0.4           | Coefficient which will apply to the RIPPLE_WAVE mode. It will make the lower part darker |
| MIN_VAL                  | loops.h | 110           | Minimum value of the FFT bin for it to be displayed at the bottom |
| MAX_VAL                  | loops.h | 60            | Maximum value of the FFT bin for it to be displayed at the top |
| WHITE_DOT_FALL           | loops.h | 0.5           | Pixel height for the white dot to fall on each FFT iteration |
| WAVE_TYPES               | loops.h | 60            | Number of wave types implemented in the code. |

Additionally for color balancing loops.c file containes two LedColor variables:
* `fft_color_base` defines the base color for the fft gradient of the bins.
* `bass_color` defines the color for each of the subbands beat detection color. The blue wave will be triggered by low frequencies, green by mid frequencies and red by high frequencies. Those colors will overlap if the beat is detected across multiple bands.

**Remember to change those values before compiling the software**

## Compile and run

Compile the software using the included makefile
```
$ pwd
/home/pi/share/rpi-sound-c/build
$ make
cc -c -Wall -O3 -I. -I../rpi-rgb-led-matrix/include ../src/main.c -o ../src/main.o
cc -c -Wall -O3 -I. -I../rpi-rgb-led-matrix/include ../src/loops.c -o ../src/loops.o
cc -c -Wall -O3 -I. -I../rpi-rgb-led-matrix/include ../src/utils.c -o ../src/utils.o
cc ../src/main.o ../src/loops.o ../src/utils.o -lportaudio -lpthread -lm -lfftw3 -lstdc++ -L../rpi-rgb-led-matrix/lib -lrgbmatrix -lrt -o fft-c
$ sudo ./fft-c 3 1
ALSA lib pcm.c:2495:(snd_pcm_open_noupdate) Unknown PCM cards.pcm.rear
ALSA lib pcm.c:2495:(snd_pcm_open_noupdate) Unknown PCM cards.pcm.center_lfe
ALSA lib pcm.c:2495:(snd_pcm_open_noupdate) Unknown PCM cards.pcm.side
ALSA lib pcm.c:2495:(snd_pcm_open_noupdate) Unknown PCM cards.pcm.hdmi
ALSA lib pcm.c:2495:(snd_pcm_open_noupdate) Unknown PCM cards.pcm.hdmi
ALSA lib pcm.c:2495:(snd_pcm_open_noupdate) Unknown PCM cards.pcm.modem
ALSA lib pcm.c:2495:(snd_pcm_open_noupdate) Unknown PCM cards.pcm.modem
ALSA lib pcm.c:2495:(snd_pcm_open_noupdate) Unknown PCM cards.pcm.phoneline
ALSA lib pcm.c:2495:(snd_pcm_open_noupdate) Unknown PCM cards.pcm.phoneline
FFT samples: 8192. Samples per callback: 441. Size: 128x64. Hardware gpio mapping: regular
^CClosing fft thread
Stopping
Exiting
```

There are two runtime parameters. The first one describes which wave type will be displayed first. The second one is choosing whether to use the white dots.

The project also has a service file included. It only needs an edit to the ExecStart path with the binary. After that copy it to the systemd directory, reload the services and start / enable it for it to run on boot:
```
$ pwd
/home/pi/share/rpi-sound-c
$ sudo cp fft_display.service /lib/systemd/system/
$ sudo systemctl daemon-reload
$ sudo systemctl start fft_display
$ sudo systemctl enable fft_display
```
