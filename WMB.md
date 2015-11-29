# WMB Protocol #

This page links to WMB protocol web pages, which were referenced in the development of the WMB packet module of Wmb Asm Module. This page also contains information found by Filb and yellowstar6. Filb found the GameID information, yellowstar6 found the WMB Beacon sequence number 9 information. Some info on how the clients request a gameID was found by yellowstar6. WMB means Wireless Multi-Boot. It is also know as DS Download Play, which is Nintendo's official name for it.


# Links #

> WMB Protocol links:
  * [NDS Tech Wiki](http://www.bottledlight.com/ds/index.php/Wifi/WMBProtocol)
  * [gbatek](http://nocash.emubase.de/gbatek.htm#dswifimultiboot)

# Additional information #

### GameID ###

This is the byte in the beacons before the non-advert field. It is what it is called - an ID of a WMB Game/Demo/Binary. With multiple gameIDs, multiple binaries can be broadcasted at once. In the past, this was used for a method of serving demos over WMB in Japan. With official software at least - homebrew  WMB host/client software can't use gameIDs. The client seems to request the gameID, with the last name data packet beginning with bytes 04 81 07 04. Following those bytes is the last UTF-16 character. The bytes following that seem to be fixed: 02 00. When no gameID is specified, 01 00 follows the UTF-16 character? Following that seems to be the gameID, followed by a zero byte. It is unknown where the gameID is, in the RSA packet. However, it is known where the gameID is, in the data packets. The gameID in the data packets are stored in the byte before the data sequence number, and before the always-null byte, and the byte after the command byte in most WMB packets. Wmb Asm can assemble multi-game captures/WMB transfers with multiple gameIDs. Due to the unknown position of the gameID in the RSA packet, some trickery is needed to get the correct RSA signature. See the Wmb Asm source code in SVN for how this is done in Wmb Asm.

### WMB Beacon sequence number 9 ###
When there is no data contained in this payload, the data\_size is 48, otherwise, it is 3 bytes long. It is unknown whether the data\_size value is always 3 bytes long when data is contained in the payload, or if it has some meaning when the value is smaller than 48. The next two bytes are 02 00. This appears to be a "magic number". Following this, is the entries for each of the connected clients' data. The number of connected clients is already specified in the connected\_clients field earlier in this beacon. The first byte in each entry is the length of the characters in the client's UTF-16 user name. After this byte, is a byte signifying the client's favorite color. This value is the same as the one specified in the user settings. Following this, is the client's UTF-16 user name.