1. Checkout POSIX FreeRTOS simulator from megakilo:
```
git clone https://github.com/megakilo/FreeRTOS-Sim.git
```
(tested with latest master branch, hash: f762ddcf89f8ff9b368728b0dcf222589d473c1f)

2. Update "FREERTOS_ROOT" variable in "setvars.sh" based on your location.
```
export FREERTOS_ROOT="/home/user/projects/FreeRTOS-Sim"
```

3. (optional) Edit "FreeRTOSConfig.h" according to your requirements.

4. Set the environment variables:
```
. setvars.sh
```

5. Build the library running:
```
make lib
```

6. "freertos-target" directory now should contain "FreeRTOS_Sim.a" library.

7. Proceed as described in the [build/readme](../readme.txt)