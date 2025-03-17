Introduction
################################

SRS-control is an asynchronous data IO program for SRS system. It communicates with the SRS system through the UDP socket with different commands to, for example, start or stop the data transferring from the system. Data sent from the SRS system is then analyzed in the program in different stages, such as the deserialization, monitoring, data conversion and writing to different files or sockets. There are 4 types of output formats supported: binary file, root file (require ROOT), JSON file and UDP socket. Binary file can store either **as-is** data sent from the SRS system or the Google's Protobuf binary. UDP socket can only send the Google's Protobuf binary due to its easy deserialization.
