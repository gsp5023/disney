# NCP-M5 rpi On Device Debugging

These are brief notes on how to debug on-device. You will need linux.

### Install gdb-multiarch

`sudo apt install gdb-multiarch`

2) on the host machine run gdb-server with the cwd at `ncp-m5/` `gdb-server :<PORT> <excutable to run>` e.g. `gdb-server :9999 ./build/bin/emu_stb_gpu_rpi2_arm7-a_32/debug/tests`
3) on the machine you will be debugging on..
	* `gdb-multiarch`
	* `target remote <DEVICE_IP>:<PORT>`
	* gdb may not be able to find the source files. use `dir` to specify the ncp-m5 directory, if that does not work use `set sysroot` to the applicable folder e.g. `set sysroot ./` -- if the cwd is `ncp-m5/`
		* to verify that gdb has symbols enter `list` when attached, you should have source code dumped to screen.