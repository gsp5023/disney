# NCP m5 on RPI

Welcome to the guide for building NCP m5 on RaspberryPi. RaspberryPi is our development equivalent of lower end Broadcom devices. 

All builds are done using a turn-key docker image container so this is generally much faster than building on rpi from scratch.

### 1) Install Docker

__Ubuntu (16.04 or 18.04 LTS)__

* `sudo apt update`
* `sudo apt-get install apt-transport-https ca-certificates curl gnupg-agent software-properties-common`
* `curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -`
* `sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) 
   stable"`
* `sudo apt update`
* `sudo apt install docker-ce docker-ce-cli containerd.io`
* `sudo usermod -aG docker $USER`

Exit and login again (note: this might require reboot)

Make sure it works: `docker run --rm -it alpine sh`

__Linux Mint 19.1__

* `sudo apt update`
* `sudo apt-get install apt-transport-https ca-certificates curl gnupg-agent software-properties-common`
* `curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -`

Manually edit `/etc/apt/sources.list.d/additional-repositories.list`

and add `deb https://download.docker.com/linux/ubuntu bionic stable` to it.

* `sudo apt-get update`
* `sudo apt-get install docker-ce docker-compose`
* `sudo usermod -aG docker $USER`

Exit and login again (note: this might require reboot)

Make sure it works: `docker run --rm -it alpine sh`

### 2) Collect/Download the RPI header/lib tars

If you have access to `disneystreaming.com/vpn` then you can skip the steps below and instead pull the image: 
 * `docker pull ncp.docker.artifactory.global.bamgrid.net/ncp/linux/rpi:0.2.0`

Software should be placed in `~/rpirefsw`

RPI2 in `~/rpirefsw/RPI2`
* rpi_usr_include.tar.bz2
* rpi_usr_lib.tar.bz2

Note: RPI4 uses the same image as RPI2 and thus you don't need to build one. If you haven't build one, the files still go in `~/rpirefsw/RPI2`

If you have a disneystreaming.com email rpi2 include/libs can be found at the link below. If you don't you can still ask for share permissions, but there may be a delay.

Pre collected tars: https://s3.console.aws.amazon.com/s3/buckets/cd-ncp-core-build-tools-external/raspberrypi/rpirefsw/rpi/?region=us-east-1

otherwise on your rpi collect the relevant files
* tar and bz2 your `/usr/include` and your `/usr/lib`
	* `sudo tar -cvjf rpi_usr_include.tar.bz2 /usr/include`
	* `sudo tar -cvjf rpi_usr_lib.tar.bz2 /usr/lib`
* copy/move the files over to the applicable folder location on your host.

### 3) Compiling m5

1) Run the docker build script to create a docker image
2) run premake on your host (depending on the target):
	* `./premake5 gmake2 --target=rpi2`
	* `./premake5 gmake2 --target=rpi4`
3) Run the docker_bash script for your rpi target, this will give you a bash prompt inside a docker container. The script will mount your current working directory.
4) `cd build`
5) `make`

### 4) setup sharing, or copy `ncp-m5/build/bin` over to the rpi

linux: 

1) install NFS kernel server on your host machine: `sudo apt install -y nfs-kernel-server`
2) `sudo vim /etc/exports`
3) add line:
	* `/home/YOURUSERNAME/projects/ncp-m5-rpi *(rw,sync,no_root_squash,no_subtree_check)`
4) `sudo exportfs -ra`
5) mount the directories from the linux prompt on your rpi:
	* `mkdir /refsw`
	* `mkdir /ncp-m5`
	* `mount -t nfs YOUR_NFS_SHARE_IP:/home/YOURUSERNAME/projects/ncp-m5-rpi /ncp-m5`

windows:
1) enable sharing on the folder you wish to share.
2) on rpi:
	* install cifs `sudo apt-get install cifs-utils`
	* make a folder for us to later mount to. e.g. `mkdir ~/repos`
	* run `sudo nano /etc/fstab` add:
		* `//<YOUR_IP>/<shared_folder> /home/pi/<desired_folder_name> cifs username=PC_USER_NAME,iocharset=utf8,file_mode=0777,dir_mode=0777,uid=1000,gid=1000 0 0`
		* e.g. `//192.168.1.12/repos /home/pi/repos cifs username=rnichols,iocharset=utf8,file_mode=0777,dir_mode=0777,uid=1000,gid=1000 0 0`
	* save, and reboot `sudo reboot`
	* to mount the shared folder (on each reboot) run `sudo mount -a` and enter your PC's log in password.
	* If your IP is not static (for your windows machine) you may need to edit the fstab file to re-sync it to your host's IP. (supposedly PC names work, but I haven't had luck)
		* if you are getting `unable to find suitable address` you may need to open ports 137-139 and 445 in your firewall

### 5) Run M5 samples
1) `cd /ncp-m5`
2) `./build/bin/emu_stb_gpu_rpi2_arm7-a_32/debug/<app you wish to run>`

* If you get a black screen, black tiles, or in general things appearing to not load:
    * you most likely need to increase the amount of vram set aside for your rpi.
        * on your rpi `sudo nano /boot/config.txt`
        * add: `gpu_mem=200` near the end (for ease of finding if you need to edit)
		* save the file, then reboot the device either hard reboot or via `sudo reboot` in a terminal
