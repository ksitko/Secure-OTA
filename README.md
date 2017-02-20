# Secure OTA ESP32

ESP-IDF provides a example doing a firmware updates over the air (OTA). It is unsuitable for production use. Secure-OTA builds upon the OTA example by allowing for secure firmware updates from a AWS S3 bucket. The connection is secured by TLS using Open_SSL. The program compares the current boot partition to that of the remote through a MD5 Checksums. If the hashes are identical then the local firmware is the same as the remote and the program will not attempt to update the firmware. However if the hashes are different then Secure-OTA will erase the partition it has selected to be the new partion, and start the firmware update and download the remote binary. Upon competion it will recalculate the local binary MD5sum, this time of the new parition and compare it to the one in from the remote server. If it finds that they match then it will set the new parition as the new boot partition and restart.

# TODO:
Move from Open_SSL to mbed_TLS

Cleanup wifi thread
