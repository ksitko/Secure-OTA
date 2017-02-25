# Secure OTA

 Secure-OTA allows the ESP32 to securely update firmware from a AWS S3 bucket. The connection is secured by TLS using mbedtls. The program compares the current boot partition to that of the remote through a MD5 Checksums. If the hashes are identical then the program will not attempt to update the firmware. However if the hashes are different then Secure-OTA will erase the partition it has selected to be the new partition and download the remote binary. Upon completion it will recalculate the local checksum, this time of the new partition, and compare it to the one in from the remote server. If it finds that they match then it will set the new partition as the new boot partition and restart.

# TODO:
Cleanup TLS

# Known Issues:
Task watchdog gets triggered during MD5 Check sum. Consider either optimizing or feeding watchdog.

Leaking memory: 128 Bytes per cycle. Need to plug leak.

Fails to verify peer certificate: "The certificate validity starts in the future"
